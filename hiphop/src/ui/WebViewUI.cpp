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

#include <cstring>

#include "WebViewUI.hpp"

#include "extra/CSSColor.hpp"
#include "extra/Path.hpp"

#define HTML_INDEX_PATH "/ui/index.html"

USE_NAMESPACE_DISTRHO

WebViewUI::WebViewUI(uint widthCssPx, uint heightCssPx, const char* backgroundCssColor,
                     float initPixelRatio)
    : WebViewUIBase(widthCssPx, heightCssPx, initPixelRatio)
    , fBackgroundColor(CSSColor::fromHex(backgroundCssColor))
    , fJsUiReady(false)
    , fPlatformWindow(0)
    , fWebView(nullptr)
{
    setBuiltInFunctionHandlers();
}

WebViewUI::~WebViewUI()
{
    if (fWebView != nullptr) {
        fWebView->setEventHandler(nullptr);
        delete fWebView;
    }
}

void WebViewUI::setWebView(WebViewBase* webView)
{
    fWebView = webView;

    fWebView->setEventHandler(this);
#if defined(HIPHOP_PRINT_TRAFFIC)
    fWebView->setPrintTraffic(true);
#endif
    
#if defined(HIPHOP_INJECT_FRAMEWORK_JS) && !defined(HIPHOP_NETWORK_UI) 
    String js = String(
#include "ui/dpf.js.inc"
    );
    fWebView->injectScript(js);
#endif

    fPlatformWindow = isStandalone() ? createStandaloneWindow() : getParentWindowHandle();
    fWebView->setParent(fPlatformWindow);
    fWebView->setBackgroundColor(fBackgroundColor);

    // Convert CSS pixels to native following the final web view pixel ratio.
    // Then adjust window size so it correctly wraps web content on high density
    // displays, known as Retina or HiDPI. WebViewBase::getDevicePixelRatio()
    // needs a parent window to be set because scaling can vary across displays.
    const float k = fWebView->getDevicePixelRatio();
    const uint width = static_cast<uint>(k * static_cast<float>(getInitWidthCSS()));
    const uint height = static_cast<uint>(k * static_cast<float>(getInitHeightCSS()));
    fWebView->setSize(width, height);
    fWebView->realize();

    setSize(width, height);
}

void WebViewUI::load()
{
    if (fWebView != nullptr) {
#if defined(HIPHOP_NETWORK_UI) 
        if ((! DISTRHO_PLUGIN_WANT_STATE) || isStandalone()) {
            // State is needed for reusing web server port
            String url = getLocalUrl();
            fWebView->navigate(url);
        }
#else
        String url = "file://" + Path::getPluginLibrary() + HTML_INDEX_PATH;
        fWebView->navigate(url);
#endif
    }
}

void WebViewUI::runScript(String& source)
{
    if (fWebView != nullptr) {
        fWebView->runScript(source);
    }
}

void WebViewUI::injectScript(String& source)
{
    // Cannot inject scripts after navigation has started
    if (fWebView != nullptr) {
        fWebView->injectScript(source);
    }
}

void WebViewUI::ready()
{
    fJsUiReady = true;

    for (MessageBuffer::iterator it = fMessageBuffer.begin(); it != fMessageBuffer.end(); ++it) {
        fWebView->postMessage(*it);
    }
    
    fMessageBuffer.clear();
}

void WebViewUI::setKeyboardFocus(bool focus)
{
    fWebView->setKeyboardFocus(focus);
}

#if ! defined(HIPHOP_NETWORK_UI)
void WebViewUI::postMessage(const Variant& args, uintptr_t /*destination*/, uintptr_t /*exclude*/)
{
    if (fJsUiReady) {
        fWebView->postMessage(args);
    } else {
        fMessageBuffer.push_back(args);
    }
}
#endif

void WebViewUI::uiIdle()
{
    WebViewUIBase::uiIdle();

    if (isStandalone()) {
        processStandaloneEvents();
    }
}

#if DISTRHO_PLUGIN_WANT_STATE
void WebViewUI::stateChanged(const char* key, const char* value)
{
    WebViewUIBase::stateChanged(key, value);

# if defined(HIPHOP_NETWORK_UI)
    if ((std::strcmp(key, "_ws_port") == 0) && (fWebView != nullptr)) {
        String url = getLocalUrl();
        fWebView->navigate(url);
    }
# endif
}
#endif

void WebViewUI::sizeChanged(uint width, uint height)
{
    WebViewUIBase::sizeChanged(width, height);
    fWebView->setSize(width, height);
    callback("sizeChanged", { width, height });
}

void WebViewUI::setBuiltInFunctionHandlers()
{
    // These handlers only make sense for the plugin embedded web view

    setFunctionHandler("getWidth", 0, [this](const Variant&, uintptr_t origin) {
        callback("getWidth", { static_cast<double>(getWidth()) }, origin);
    });

    setFunctionHandler("getHeight", 0, [this](const Variant&, uintptr_t origin) {
        callback("getHeight", { static_cast<double>(getHeight()) }, origin);
    });

    setFunctionHandler("isResizable", 0, [this](const Variant&, uintptr_t origin) {
        callback("isResizable", { isResizable() }, origin);
    });

    setFunctionHandler("setWidth", 1, [this](const Variant& args, uintptr_t /*origin*/) {
        setWidth(static_cast<uint>(args[0].getNumber()));
    });

    setFunctionHandler("setHeight", 1, [this](const Variant& args, uintptr_t /*origin*/) {
        setHeight(static_cast<uint>(args[0].getNumber()));
    });

    setFunctionHandler("setSize", 2, [this](const Variant& args, uintptr_t /*origin*/) {
        setSize(
            static_cast<uint>(args[0].getNumber()), // width
            static_cast<uint>(args[1].getNumber())  // height
        );
    });

    setFunctionHandler("setKeyboardFocus", 1, [this](const Variant& args, uintptr_t /*origin*/) {
        setKeyboardFocus(static_cast<bool>(args[0].getBoolean()));
    });

    setFunctionHandler("ready", 0, [this](const Variant&, uintptr_t /*origin*/) {
        ready();
    });

    setFunctionHandler("openSystemWebBrowser", 1, [this](const Variant& args, uintptr_t /*origin*/) {
        String url = args[0].getString();
        openSystemWebBrowser(url);
    });
}

void WebViewUI::handleWebViewLoadFinished()
{
    onDocumentReady();
}

void WebViewUI::handleWebViewScriptMessage(const Variant& args)
{
    handleMessage(args, ORIGIN_EMBEDDED_WEB_VIEW);
}

void WebViewUI::handleWebViewConsole(const String& tag, const String& text)
{
    if (tag == "log") {
        d_stderr("%s", text.buffer());
    } else if (tag == "info") {
        d_stderr("INFO : %s", text.buffer());
    } else if (tag == "warn") {
        d_stderr("WARN : %s", text.buffer());
    } else if (tag == "error") {
        d_stderr("ERROR : %s", text.buffer());
    }
}
