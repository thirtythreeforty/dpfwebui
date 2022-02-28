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

#include "WebViewUI.hpp"

#include <iostream>

#include "extra/Path.hpp"

#define HTML_INDEX_PATH "/ui/index.html"

USE_NAMESPACE_DISTRHO

// DPF implementation of VST3 on macOS needs the DISTRHO::UI constructor to be
// called with already scaled dimensions because the later setSize() request in
// setWebView() is ignored; see https://github.com/DISTRHO/DPF/issues/359. Since
// the parent native window only becomes available later on UI lifecycle, scale
// factor for secondary displays cannot be determined on UI construction.
// VST3/Mac plugins on secondary displays might open with wrong dimensions.
#define MAIN_DISPLAY_SCALE_FACTOR() getDisplayScaleFactor(0)

WebViewUI::WebViewUI(uint widthCssPx, uint heightCssPx, uint32_t backgroundColor,
                     bool /*startLoading*/)
    : WebViewUIBase(MAIN_DISPLAY_SCALE_FACTOR() * widthCssPx,
                    MAIN_DISPLAY_SCALE_FACTOR() * heightCssPx)
    , fInitialWidth(widthCssPx)
    , fInitialHeight(heightCssPx)
    , fBackgroundColor(backgroundColor)
    , fMessageQueueReady(false)
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
#ifdef HIPHOP_PRINT_TRAFFIC
    fWebView->setPrintTraffic(true);
#endif
    
#if defined(HIPHOP_INJECT_FRAMEWORK_JS) && !defined(HIPHOP_NETWORK_UI) 
    String js = String(
#include "ui/dpf.js.inc"
    );
    fWebView->injectScript(js);
#endif

    // Cannot call virtual method createStandaloneWindow() from constructor.
    fPlatformWindow = isStandalone() ? createStandaloneWindow() : getParentWindowHandle();

    // Convert CSS pixels to native pixels following the system display scale
    // factor. Then adjust window size so it correctly wraps web content on
    // high density displays, known as Retina or HiDPI.
    const float k = getDisplayScaleFactor(this);
    const uint width = static_cast<uint>(k * static_cast<float>(fInitialWidth));
    const uint height = static_cast<uint>(k * static_cast<float>(fInitialHeight));

    fWebView->setParent(fPlatformWindow);
    fWebView->setBackgroundColor(fBackgroundColor);
    fWebView->setSize(width, height);
    fWebView->realize();

    setSize(width, height);
}

void WebViewUI::load()
{
    if (fWebView != nullptr) {
#ifdef HIPHOP_NETWORK_UI
        // TODO : https://< 127.0.0.1 | lan addr >:port
        String url = String("file:///");
#else
        String url = "file://" + Path::getPluginLibrary() + HTML_INDEX_PATH;
#endif
        fWebView->navigate(url);
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

void WebViewUI::flushInitMessageQueue()
{
    if (fMessageQueueReady) {
        return;
    }

    fMessageQueueReady = true;

    for (InitMessageQueue::iterator it = fInitMessageQueue.begin(); it != fInitMessageQueue.end(); ++it) {
        fWebView->postMessage(*it);
    }
    
    fInitMessageQueue.clear();
}

void WebViewUI::setKeyboardFocus(bool focus)
{
    fWebView->setKeyboardFocus(focus);
}

void WebViewUI::postMessage(const JsValueVector& args)
{
    if (fMessageQueueReady) {
        fWebView->postMessage(args);
    } else {
        fInitMessageQueue.push_back(args);
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

    fHandler["getWidth"] = std::make_pair(0, [this](const JsValueVector&) {
        postMessage({"UI", "getWidth", static_cast<double>(getWidth())});
    });

    fHandler["getHeight"] = std::make_pair(0, [this](const JsValueVector&) {
        postMessage({"UI", "getHeight", static_cast<double>(getHeight())});
    });

    fHandler["isResizable"] = std::make_pair(0, [this](const JsValueVector&) {
        postMessage({"UI", "isResizable", isResizable()});
    });

    fHandler["setWidth"] = std::make_pair(1, [this](const JsValueVector& args) {
        sizeRequest([this, args]() {
            setWidth(static_cast<uint>(args[0].getDouble()));
        });
    });

    fHandler["setHeight"] = std::make_pair(1, [this](const JsValueVector& args) {
        sizeRequest([this, args]() {
            setHeight(static_cast<uint>(args[0].getDouble()));
        });
    });

    fHandler["setSize"] = std::make_pair(2, [this](const JsValueVector& args) {
        sizeRequest([this, args]() {
            setSize(
                static_cast<uint>(args[0].getDouble()), // width
                static_cast<uint>(args[1].getDouble())  // height
            );
        });
    });

    fHandler["getInitialWidth"] = std::make_pair(0, [this](const JsValueVector&) {
        postMessage({"UI", "getInitialWidth", static_cast<double>(getInitialWidth())});
    });

    fHandler["getInitialHeight"] = std::make_pair(0, [this](const JsValueVector&) {
        postMessage({"UI", "getInitialHeight", static_cast<double>(getInitialHeight())});
    });

    fHandler["setKeyboardFocus"] = std::make_pair(1, [this](const JsValueVector& args) {
        setKeyboardFocus(static_cast<bool>(args[0].getBool()));
    });

    fHandler["flushInitMessageQueue"] = std::make_pair(0, [this](const JsValueVector&) {
        flushInitMessageQueue();
    });

    fHandler["openSystemWebBrowser"] = std::make_pair(1, [this](const JsValueVector& args) {
        String url = args[0].getString();
        openSystemWebBrowser(url);
    });
}

void WebViewUI::handleWebViewLoadFinished()
{
    onDocumentReady();
}

void WebViewUI::handleWebViewScriptMessage(const JsValueVector& args)
{
    handleMessage(args);
}

void WebViewUI::handleWebViewConsole(const String& tag, const String& text)
{
    if (tag == "log") {
        std::cout << text.buffer() << std::endl;
    } else if (tag == "info") {
        std::cout << "INFO : " << text.buffer() << std::endl;
    } else if (tag == "warn") {
        std::cout << "WARN : " << text.buffer() << std::endl;
    } else if (tag == "error") {
        std::cerr << "ERROR : " << text.buffer() << std::endl;
    }
}
