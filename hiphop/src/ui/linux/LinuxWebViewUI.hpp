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

#ifndef LINUX_WEBVIEW_UI_HPP
#define LINUX_WEBVIEW_UI_HPP

// Avoid X11 Window and DGL::Window clashing during build by
// including Xlib before any other header that includes DPF.
#include <X11/Xlib.h>

#include "../WebViewUI.hpp"
#include "ChildProcessWebView.hpp"

START_NAMESPACE_DISTRHO

class LinuxWebViewUI : public WebViewUI
{
public:
    LinuxWebViewUI(uint widthCssPx = 0, uint heightCssPx = 0,
        const char* backgroundCssColor = "#fff", bool startLoading = true);
    virtual ~LinuxWebViewUI();

    void openSystemWebBrowser(String& url) override;

protected:
    uintptr_t createStandaloneWindow() override;
    void      processStandaloneEvents() override;

private:
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LinuxWebViewUI)

};

END_NAMESPACE_DISTRHO

#endif  // LINUX_WEBVIEW_UI_HPP
