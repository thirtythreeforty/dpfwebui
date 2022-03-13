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

#include <cstring>

#include "WebViewUI.hpp"

#include "extra/Path.hpp"

#define HTML_INDEX_PATH "/ui/index.html"

USE_NAMESPACE_DISTRHO

WebViewUI::WebViewUI(uint widthCssPx, uint heightCssPx, uint32_t backgroundColor,
                     bool /*startLoading*/, float initScaleFactor)
    : WebViewUIBase(widthCssPx, heightCssPx, initScaleFactor)
    , fBackgroundColor(backgroundColor)
    , fJsUiReady(false)
    , fUiBlockQueued(false)
    , fPlatformWindow(0)
    , fWebView(nullptr)
{
    initHandlers();
}

WebViewUI::~WebViewUI()
{
    if (fWebView != nullptr) {
        delete fWebView;
    }
}

void WebViewUI::queue(const UiBlock& block)
{
    fUiBlock = block;
    fUiBlockQueued = true;
}

bool WebViewUI::shouldCreateWebView()
{
    // When running as a plugin the UI ctor/dtor can be repeatedly called with
    // no parent window available, do not create the web view in such cases.
    return isStandalone() || (getParentWindowHandle() != 0);
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

    // Convert CSS pixels to native pixels following the web view scale factor.
    // Then adjust window size so it correctly wraps web content on high density
    // displays, known as Retina or HiDPI.
    const float k = fWebView->getScaleFactor();
    const uint width = static_cast<uint>(k * static_cast<float>(getUnscaledInitWidth()));
    const uint height = static_cast<uint>(k * static_cast<float>(getUnscaledInitHeight()));
    fWebView->setSize(width, height);
    fWebView->realize();

    setSize(width, height);
}

void WebViewUI::load()
{
    if (fWebView != nullptr) {
#if defined(HIPHOP_NETWORK_UI) 
        if ((! DISTRHO_PLUGIN_WANT_STATE) || isStandalone()) {
            // State is needed for reusing the web server port during the plugin
            // lifetime. Note that the local webview still uses the native bridge
            // for messaging instead of WebSockets when loading document via HTTP.
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

void WebViewUI::postMessage(const JSValue& args)
{
    if (fJsUiReady) {
        fWebView->postMessage(args);
    } else {
        fMessageBuffer.push_back(args);
    }
}

void WebViewUI::uiIdle()
{
    WebViewUIBase::uiIdle();
    
    if (fUiBlockQueued) {
        fUiBlockQueued = false;
        fUiBlock();
    }

    if (isStandalone()) {
        processStandaloneEvents();
    }
}

#if DISTRHO_PLUGIN_WANT_STATE
void WebViewUI::stateChanged(const char* key, const char* value)
{
    WebViewUIBase::stateChanged(key, value);

#if defined(HIPHOP_NETWORK_UI)
    if ((std::strcmp(key, "_ws_port") == 0) && (fWebView != nullptr)) {
        String url = getLocalUrl();
        fWebView->navigate(url);
    }
#endif
}
#endif

void WebViewUI::sizeChanged(uint width, uint height)
{
    WebViewUIBase::sizeChanged(width, height);
    fWebView->setSize(width, height);
}

void WebViewUI::sizeRequest(const UiBlock& block)
{
    block();    // on Linux block execution is queued
}

void WebViewUI::initHandlers()
{
    // These handlers only make sense for the local web view

    fHandler["getWidth"] = std::make_pair(0, [this](const JSValue&) {
        postMessage({"UI", "getWidth", static_cast<double>(getWidth())});
    });

    fHandler["getHeight"] = std::make_pair(0, [this](const JSValue&) {
        postMessage({"UI", "getHeight", static_cast<double>(getHeight())});
    });

    fHandler["isResizable"] = std::make_pair(0, [this](const JSValue&) {
        postMessage({"UI", "isResizable", isResizable()});
    });

    fHandler["setWidth"] = std::make_pair(1, [this](const JSValue& args) {
        sizeRequest([this, args]() {
            setWidth(static_cast<uint>(args[0].getNumber()));
        });
    });

    fHandler["setHeight"] = std::make_pair(1, [this](const JSValue& args) {
        sizeRequest([this, args]() {
            setHeight(static_cast<uint>(args[0].getNumber()));
        });
    });

    fHandler["setSize"] = std::make_pair(2, [this](const JSValue& args) {
        sizeRequest([this, args]() {
            setSize(
                static_cast<uint>(args[0].getNumber()), // width
                static_cast<uint>(args[1].getNumber())  // height
            );
        });
    });

    fHandler["setKeyboardFocus"] = std::make_pair(1, [this](const JSValue& args) {
        setKeyboardFocus(static_cast<bool>(args[0].getBoolean()));
    });

    fHandler["ready"] = std::make_pair(0, [this](const JSValue&) {
        ready();
    });

    fHandler["openSystemWebBrowser"] = std::make_pair(1, [this](const JSValue& args) {
        String url = args[0].getString();
        openSystemWebBrowser(url);
    });
}

void WebViewUI::handleWebViewLoadFinished()
{
    onDocumentReady();
}

void WebViewUI::handleWebViewScriptMessage(const JSValue& args)
{
    handleMessage(args);
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
