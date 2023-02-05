/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2023 Luciano Iam <oss@lucianoiam.com>
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

#include "LinuxWebViewUI.hpp"
#include "scaling.h"

USE_NAMESPACE_DISTRHO

LinuxWebViewUI::LinuxWebViewUI(uint widthCssPx, uint heightCssPx,
        const char* backgroundCssColor, bool startLoading)
    : WebViewUI(widthCssPx, heightCssPx, backgroundCssColor, device_pixel_ratio())
#if defined(HIPHOP_NETWORK_UI) && defined(HIPHOP_LINUX_WEBVIEW_CEF)
    , fFirstClient(false)
#endif
{
    if (isDryRun()) {
        return;
    }

    setWebView(new ChildProcessWebView(String(kWebViewUserAgent)));

    if (startLoading) {
        load();
    }
}

LinuxWebViewUI::~LinuxWebViewUI()
{
    // TODO - standalone support
}

void LinuxWebViewUI::openSystemWebBrowser(String& url)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "xdg-open %s", url.buffer());

    if (system(buf) != 0) {
        d_stderr("Could not open system web browser - %s", strerror(errno));
    }
}

uintptr_t LinuxWebViewUI::createStandaloneWindow()
{
    // TODO - standalone support
    return 0;
}

void LinuxWebViewUI::processStandaloneEvents()
{
    // TODO - standalone support
}

// No way for setting WebSocket request headers for the CEF web view. Force user
// agent for the first connected client instead. While this depends on timing,
// in practice the first client will be always the plugin embedded web view.
#if defined(HIPHOP_NETWORK_UI) && defined(HIPHOP_LINUX_WEBVIEW_CEF)
void LinuxWebViewUI::onClientConnected(Client client)
{
    if (fFirstClient) {
        return;
    }

    fFirstClient = true;

    String userAgent(kWebViewUserAgent);
    getServer().setClientUserAgent(client, userAgent);

}
#endif
