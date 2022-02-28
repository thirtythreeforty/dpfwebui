/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2022 Luciano Iam <oss@lucianoiam.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ChildProcessWebView.hpp"

#include <cstdio>
#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <spawn.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/wait.h>

#include "extra/macro.h"
#include "extra/Path.hpp"

extern char **environ;

/*
    On Linux a child process hosts the web view to workaround these issues:
  
  - WebKitGTK needs GTK and linking plugins to UI toolkits is a bad idea
    http://lists.lv2plug.in/pipermail/devel-lv2plug.in/2016-March/001593.html
    https://www.mail-archive.com/gtk-list%40gnome.org/msg34952.html

  - CEF lifecycle and usage of globals is not compatible with plugins
    https://bitbucket.org/chromiumembedded/cef/issues/421

    There is a lengthy relevant discussion on the topic here
    https://gist.github.com/abique/4c1b9b40f3413f0df1591d2a7c760db4
*/

USE_NAMESPACE_DISTRHO

ChildProcessWebView::ChildProcessWebView()
    : fDisplay(0)
    , fBackground(0)
    , fPid(-1)
    , fIpc(nullptr)
    , fIpcThread(nullptr)
    , fChildInit(false)
    , fDisplayScaleFactor(1.f)
{
    fDisplay = XOpenDisplay(0);

    fPipeFd[0][0] = -1;
    fPipeFd[0][1] = -1;

    if (pipe(fPipeFd[0]) == -1) {
        d_stderr("Could not create host->helper pipe - %s", strerror(errno));
        return;
    }

    fPipeFd[1][0] = -1;
    fPipeFd[1][1] = -1;

    if (pipe(fPipeFd[1]) == -1) {
        d_stderr("Could not create helper->host pipe - %s", strerror(errno));
        return;
    }

    fIpc = new IpcChannel(fPipeFd[1][0], fPipeFd[0][1]);
    fIpcThread = new IpcReadThread(fIpc,
        std::bind(&ChildProcessWebView::ipcReadCallback, this, std::placeholders::_1));
    fIpcThread->startThread();

    char rfd[10];
    sprintf(rfd, "%d", fPipeFd[0][0]);
    char wfd[10];
    sprintf(wfd, "%d", fPipeFd[1][1]);
    
    String libPath = Path::getPluginLibrary();
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_addchdir_np(&fa, libPath);

    String helperPath = libPath + "/ui-helper";
    const char *argv[] = { helperPath, rfd, wfd, 0 };

    const int status = posix_spawnp(&fPid, helperPath, &fa, 0, (char* const*)argv, environ);

    if (status != 0) {
        d_stderr("Could not spawn helper child process - %s", strerror(errno));
        return;
    }

    // Busy-wait for init up to 5s
    for (int i = 0; (i < 500) && !fChildInit; i++) {
        usleep(10000L); // 10ms
    }

    if (!fChildInit) {
        d_stderr("Timeout waiting for UI helper init - %s", strerror(errno));
    }

    injectDefaultScripts(); // non-virtual, safe to call

    fIpc->write(OP_INJECT_SHIMS);
}

ChildProcessWebView::~ChildProcessWebView()
{
    if (fPid != -1) {
        fIpc->write(OP_TERMINATE);
#if defined(HIPHOP_LINUX_WEBVIEW_CEF)
        kill(fPid, SIGTERM); // terminate as soon as possible
#endif
        int stat;
        waitpid(fPid, &stat, 0);

        fPid = -1;
    }

    if (fIpcThread != 0) {
        fIpcThread->stopThread(-1);
        fIpcThread = 0;
    }

    if (fIpc != 0) {
        delete fIpc;
        fIpc = 0;
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            if ((fPipeFd[i][j] != -1) && (close(fPipeFd[i][j]) == -1)) {
                d_stderr("Could not close pipe - %s", strerror(errno));
            }

            fPipeFd[i][j] = -1;
        }
    }

    if (fBackground != 0) {
        XDestroyWindow(fDisplay, fBackground);
    }

    XCloseDisplay(fDisplay);
}

void ChildProcessWebView::realize()
{
    const ::Window parent = (::Window)getParent();
    const unsigned long color = getBackgroundColor() >> 8;

    // The only reliable way to keep background color while window manager open
    // and close animations are performed is to paint the provided window. This
    // is needed for hosts that show floating windows like Carla and Bitwig and
    // at least true for the Gnome Shell window manager.
    XSetWindowBackground(fDisplay, parent, color);
    XClearWindow(fDisplay, parent);

    // A colored top view is also needed to avoid initial flicker on REAPER
    // because the child process takes non-zero time to start
    fBackground = XCreateSimpleWindow(fDisplay, parent, 0, 0, getWidth(), getHeight(), 0, 0, 0);
    XMapWindow(fDisplay, fBackground);
    XSetWindowBackground(fDisplay, fBackground, color);
    XClearWindow(fDisplay, fBackground);
    XSync(fDisplay, False);

    msg_win_cfg_t config;
    config.parent = static_cast<uintptr_t>(fBackground);
    config.color = color;
    config.size = { getWidth(), getHeight() };
    fIpc->write(OP_REALIZE, &config, sizeof(config));
}

void ChildProcessWebView::navigate(String& url)
{
    fIpc->write(OP_NAVIGATE, url);
}

void ChildProcessWebView::runScript(String& source)
{
    fIpc->write(OP_RUN_SCRIPT, source);
}

void ChildProcessWebView::injectScript(String& source)
{
    fIpc->write(OP_INJECT_SCRIPT, source);
}

void ChildProcessWebView::onSize(uint width, uint height)
{
    const msg_win_size_t sizePkt = { width, height };
    fIpc->write(OP_SET_SIZE, &sizePkt, sizeof(sizePkt));

    if (fBackground == 0) {
        return;
    }

    XResizeWindow(fDisplay, fBackground, width, height);
    XSync(fDisplay, False);
}

void ChildProcessWebView::onKeyboardFocus(bool focus)
{
    const char val = focus ? 1 : 0;
    fIpc->write(OP_SET_KEYBOARD_FOCUS, &val, sizeof(val));
}

void ChildProcessWebView::ipcReadCallback(const tlv_t& packet)
{
    switch (static_cast<msg_opcode_t>(packet.t)) {
        case OP_HANDLE_INIT:
            handleInit(*static_cast<const float*>(packet.v));
            break;
        case OP_HANDLE_LOAD_FINISHED:
            handleLoadFinished();
            break;
        case OP_HANDLE_SCRIPT_MESSAGE:
            handleHelperScriptMessage(static_cast<const char*>(packet.v), packet.l);
            break;
        default:
            break;
    }
}

void ChildProcessWebView::handleInit(float displayScaleFactor)
{
    fDisplayScaleFactor = displayScaleFactor;
    fChildInit = true;
}

void ChildProcessWebView::handleHelperScriptMessage(const char *payload, int payloadSize)
{
    // Should validate payload is never read past payloadSize 
    JsValueVector args;
    int offset = 0;

    while (offset < payloadSize) {
        const char *type = payload + offset;
        const char *value = type + 1;

        switch (*type) {
            case ARG_TYPE_FALSE:
                offset += 1;
                args.push_back(JsValue(false));
                break;
            case ARG_TYPE_TRUE:
                offset += 1;
                args.push_back(JsValue(true));
                break;
            case ARG_TYPE_DOUBLE:
                offset += 1 + sizeof(double);
                args.push_back(JsValue(*reinterpret_cast<const double *>(value)));
                break;
            case ARG_TYPE_STRING:
                offset += 1 /*type*/ + strlen(value) + 1 /*\0*/;
                args.push_back(JsValue(String(value)));
                break;
            default:
                offset += 1;
                args.push_back(JsValue()); // null
                break;
        }
    }

    handleScriptMessage(args);
}

IpcReadThread::IpcReadThread(IpcChannel* ipc, IpcReadCallback callback)
    : Thread("ipc_read_" XSTR(HIPHOP_PROJECT_ID_HASH))
    , fIpc(ipc)
    , fCallback(callback)
{}

void IpcReadThread::run()
{
    tlv_t packet;

    while (!shouldThreadExit()) {
        if (fIpc->read(&packet, 100/* ms */) == 0) {
            fCallback(packet);
        }
    }
}
