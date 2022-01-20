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

#include <errno.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include "LinuxWebHostUI.hpp"

USE_NAMESPACE_DISTRHO

float DISTRHO::getDisplayScaleFactor(AbstractWebHostUI* ui)
{
    if (ui == nullptr) {
        return 1.f;
    }

    ChildProcessWebView* webview = static_cast<ChildProcessWebView*>(ui->getWebView());

    return webview->getDisplayScaleFactor();
}

LinuxWebHostUI::LinuxWebHostUI(uint baseWidth, uint baseHeight,
        uint32_t backgroundColor, bool startLoading)
    : AbstractWebHostUI(baseWidth, baseHeight, backgroundColor)
{
    if (!shouldCreateWebView()) {
        return;
    }

    ChildProcessWebView* webview = new ChildProcessWebView();
    setWebView(webview); // base class owns web view

#ifdef LXWEBVIEW_GTK
    // Allow JavaScript code to detect the GTK webview and enable some
    // workarounds to compensate for the broken vw/vh/vmin/vmax CSS units and
    // non-working touch events for <input type="range"> elements.
    String js = String(
        "window.DISTRHO.quirks.brokenCSSViewportUnits = true;"
        "window.DISTRHO.quirks.brokenRangeInputTouch = true;"
    );
    webview->injectScript(js);
#endif // LXWEBVIEW_GTK

    if (startLoading) {
        load();
    }
}

LinuxWebHostUI::~LinuxWebHostUI()
{
    // TODO - standalone support
}

void LinuxWebHostUI::openSystemWebBrowser(String& url)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "xdg-open %s", url.buffer());

    if (system(buf) != 0) {
        d_stderr("Could not open system web browser - %s", strerror(errno));
    }
}

uintptr_t LinuxWebHostUI::createStandaloneWindow()
{
    // TODO - standalone support
    return 0;
}

void LinuxWebHostUI::processStandaloneEvents()
{
    // TODO - standalone support
}
