/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021 Luciano Iam <oss@lucianoiam.com>
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

#ifndef EXTERNAL_GTK_WEBVIEW_HPP
#define EXTERNAL_GTK_WEBVIEW_HPP

#include <cstdint>
#include <sys/types.h>

#include "extra/Thread.hpp"

#include "AbstractWebView.hpp"

#include "ipc.h"
#include "helper.h"

START_NAMESPACE_DISTRHO

class ExternalGtkWebView : public AbstractWebView
{
friend class IpcReadThread;

public:
    ExternalGtkWebView();
    virtual ~ExternalGtkWebView();

    void setBackgroundColor(uint32_t rgba) override;
    void setSize(uint width, uint height) override;
    void navigate(String& url) override;
    void runScript(String& source) override;
    void injectScript(String& source) override;
    void setKeyboardFocus(bool focus) override;
    void setParent(uintptr_t parent) override;

private:
    ipc_t* ipc() const { return fIpc; }
    int    ipcWriteString(helper_opcode_t opcode, String str) const;
    int    ipcWrite(helper_opcode_t opcode, const void *payload, int payloadSize) const; 
    void   ipcReadCallback(const tlv_t& message);

    void   handleHelperScriptMessage(const char *payload, int payloadSize);

    int     fPipeFd[2][2];
    pid_t   fPid;
    ipc_t*  fIpc;
    Thread* fIpcThread;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExternalGtkWebView)

};

class IpcReadThread : public Thread
{
public:
    IpcReadThread(ExternalGtkWebView& view);
    
    void run() override;

private:
    ExternalGtkWebView& fView;

};

END_NAMESPACE_DISTRHO

#endif  // EXTERNAL_GTK_WEBVIEW_HPP
