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

#include "BaseWebHostUI.hpp"

#include <iostream>

#include "Path.hpp"
#include "extra/Base64.hpp"

USE_NAMESPACE_DISTRHO

// DPF implementation of VST3 on macOS needs the DISTRHO::UI constructor to be
// called with already scaled dimensions because the later setSize() request in
// setWebView() is ignored; see https://github.com/DISTRHO/DPF/issues/359. Since
// the parent native window only becomes available later on during UI lifecycle,
// scale factor for secondary displays cannot be determined on UI construction.
// VST3/Mac plugins on secondary displays might open with wrong dimensions.
#define MAIN_DISPLAY_SCALE_FACTOR() getDisplayScaleFactor(0)

BaseWebHostUI::BaseWebHostUI(uint widthCssPx, uint heightCssPx,
        uint32_t backgroundColor, bool /*startLoading*/)
    : UIEx(MAIN_DISPLAY_SCALE_FACTOR() * widthCssPx, MAIN_DISPLAY_SCALE_FACTOR() * heightCssPx)
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

BaseWebHostUI::~BaseWebHostUI()
{
    if (fWebView != nullptr) {
        delete fWebView;
    }
}

void BaseWebHostUI::queue(const UiBlock& block)
{
    fUiBlock = block;
    fUiBlockQueued = true;
}

bool BaseWebHostUI::shouldCreateWebView()
{
    // When running as a plugin the UI ctor/dtor can be repeatedly called with
    // no parent window available, do not create the web view in such cases.
    return isStandalone() || (getParentWindowHandle() != 0);
}

void BaseWebHostUI::setWebView(BaseWebView* webView)
{
    fWebView = webView;

    fWebView->setEventHandler(this);
#ifdef HIPHOP_PRINT_TRAFFIC
    fWebView->setPrintTraffic(true);
#endif // HIPHOP_PRINT_TRAFFIC
    
    String js = String(
#include "ui/client/distrho-ui.js.inc"
    );
    js += "window.DISTRHO = {UI: UI, quirks: {}};"
          "UI = null;";
    fWebView->injectScript(js);

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

void BaseWebHostUI::load()
{
    if (fWebView != nullptr) {
        String url = "file://" + path::getLibraryPath() + "/ui/index.html";
        fWebView->navigate(url);
    }
}

void BaseWebHostUI::runScript(String& source)
{
    if (fWebView != nullptr) {
        fWebView->runScript(source);
    }
}

void BaseWebHostUI::injectScript(String& source)
{
    // Cannot inject scripts after navigation has started
    if (fWebView != nullptr) {
        fWebView->injectScript(source);
    }
}

void BaseWebHostUI::webViewPostMessage(const JsValueVector& args)
{
    if (fMessageQueueReady) {
        fWebView->postMessage(args);
    } else {
        fInitMessageQueue.push_back(args);
    }
}

void BaseWebHostUI::flushInitMessageQueue()
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

void BaseWebHostUI::setKeyboardFocus(bool focus)
{
    fWebView->setKeyboardFocus(focus);
}

void BaseWebHostUI::uiIdle()
{
    UIEx::uiIdle();
    
    if (fUiBlockQueued) {
        fUiBlockQueued = false;
        fUiBlock();
    }

    if (isStandalone()) {
        processStandaloneEvents();
    }
}

#if HIPHOP_ENABLE_SHARED_MEMORY
void BaseWebHostUI::sharedMemoryChanged(const char* metadata, const unsigned char* data, size_t size)
{
    (void)size;
    String b64Data = String::asBase64(data, size);
    webViewPostMessage({"UI", "_sharedMemoryChanged", metadata, b64Data});
}
#endif // HIPHOP_ENABLE_SHARED_MEMORY

void BaseWebHostUI::sizeChanged(uint width, uint height)
{
    UI::sizeChanged(width, height);
    
    fWebView->setSize(width, height);
    webViewPostMessage({"UI", "sizeChanged", width, height});
}

void BaseWebHostUI::parameterChanged(uint32_t index, float value)
{
    webViewPostMessage({"UI", "parameterChanged", index, value});
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
void BaseWebHostUI::programLoaded(uint32_t index)
{
    webViewPostMessage({"UI", "programLoaded", index});
}
#endif // DISTRHO_PLUGIN_WANT_PROGRAMS

#if DISTRHO_PLUGIN_WANT_STATE
void BaseWebHostUI::stateChanged(const char* key, const char* value)
{
    webViewPostMessage({"UI", "stateChanged", key, value});
}
#endif // DISTRHO_PLUGIN_WANT_STATE

void BaseWebHostUI::sizeRequest(const UiBlock& block)
{
    block();    // on Linux block execution is queued
}

void BaseWebHostUI::initHandlers()
{
    // It is not possible to implement JS synchronous calls that return values
    // without resorting to dirty hacks. Use JS async functions instead, and
    // fulfill their promises here. See for example getWidth() and getHeight().

    fHandler["getWidth"] = std::make_pair(0, [this](const JsValueVector&) {
        webViewPostMessage({"UI", "getWidth", static_cast<double>(getWidth())});
    });

    fHandler["getHeight"] = std::make_pair(0, [this](const JsValueVector&) {
        webViewPostMessage({"UI", "getHeight", static_cast<double>(getHeight())});
    });

    fHandler["isResizable"] = std::make_pair(0, [this](const JsValueVector&) {
        webViewPostMessage({"UI", "isResizable", isResizable()});
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

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    fHandler["sendNote"] = std::make_pair(3, [this](const JsValueVector& args) {
        sendNote(
            static_cast<uint8_t>(args[0].getDouble()),  // channel
            static_cast<uint8_t>(args[1].getDouble()),  // note
            static_cast<uint8_t>(args[2].getDouble())   // velocity
        );
    });
#endif // DISTRHO_PLUGIN_WANT_MIDI_INPUT

    fHandler["editParameter"] = std::make_pair(2, [this](const JsValueVector& args) {
        editParameter(
            static_cast<uint32_t>(args[0].getDouble()), // index
            static_cast<bool>(args[1].getBool())        // started
        );
    });

    fHandler["setParameterValue"] = std::make_pair(2, [this](const JsValueVector& args) {
        setParameterValue(
            static_cast<uint32_t>(args[0].getDouble()), // index
            static_cast<float>(args[1].getDouble())     // value
        );
    });

#if DISTRHO_PLUGIN_WANT_STATE
    fHandler["setState"] = std::make_pair(2, [this](const JsValueVector& args) {
        setState(
            args[0].getString(), // key
            args[1].getString()  // value
        );
    });
#endif // DISTRHO_PLUGIN_WANT_STATE

#if DISTRHO_PLUGIN_WANT_STATEFILES
    fHandler["requestStateFile"] = std::make_pair(1, [this](const JsValueVector& args) {
        requestStateFile(args[0].getString() /*key*/);
    });
#endif // DISTRHO_PLUGIN_WANT_STATEFILES

    fHandler["isStandalone"] = std::make_pair(0, [this](const JsValueVector&) {
        webViewPostMessage({"UI", "isStandalone", isStandalone()});
    });

    fHandler["setKeyboardFocus"] = std::make_pair(1, [this](const JsValueVector& args) {
        setKeyboardFocus(static_cast<bool>(args[0].getBool()));
    });

    fHandler["openSystemWebBrowser"] = std::make_pair(1, [this](const JsValueVector& args) {
        String url = args[0].getString();
        openSystemWebBrowser(url);
    });

    fHandler["getInitialWidth"] = std::make_pair(0, [this](const JsValueVector&) {
        webViewPostMessage({"UI", "getInitialWidth", static_cast<double>(getInitialWidth())});
    });

    fHandler["getInitialHeight"] = std::make_pair(0, [this](const JsValueVector&) {
        webViewPostMessage({"UI", "getInitialHeight", static_cast<double>(getInitialHeight())});
    });

    fHandler["flushInitMessageQueue"] = std::make_pair(0, [this](const JsValueVector&) {
        flushInitMessageQueue();
    });

#if DISTRHO_PLUGIN_WANT_STATE && HIPHOP_ENABLE_SHARED_MEMORY
    fHandler["writeSharedMemory"] = std::make_pair(2, [this](const JsValueVector& args) {
        std::vector<uint8_t> data = d_getChunkFromBase64String(args[1].getString());
        writeSharedMemory(
            args[0].getString(), // metadata
            static_cast<const unsigned char*>(data.data()),
            static_cast<size_t>(data.size())
        );
    });

#if HIPHOP_ENABLE_WASM_PLUGIN
    fHandler["sideloadWasmBinary"] = std::make_pair(1, [this](const JsValueVector& args) {
        std::vector<uint8_t> data = d_getChunkFromBase64String(args[0].getString());
        sideloadWasmBinary(
            static_cast<const unsigned char*>(data.data()),
            static_cast<size_t>(data.size())
        );
    });
#endif // HIPHOP_ENABLE_WASM_PLUGIN
#endif // DISTRHO_PLUGIN_WANT_STATE && HIPHOP_ENABLE_SHARED_MEMORY
}

void BaseWebHostUI::handleWebViewLoadFinished()
{
    onWebContentReady();
}

void BaseWebHostUI::handleWebViewScriptMessage(const JsValueVector& args)
{
    if ((args.size() < 2) || (args[0].getString() != "UI")) {
        onWebMessageReceived(args); // passthrough
        return;
    }

    String key = args[1].getString();

    if (fHandler.find(key.buffer()) == fHandler.end()) {
        d_stderr2("Unknown WebHostUI method");
        return;
    }

    const JsValueVector handlerArgs(args.cbegin() + 2, args.cend());
    
    ArgumentCountAndMessageHandler handler = fHandler[key.buffer()];

    if (handler.first != static_cast<int>(handlerArgs.size())) {
        d_stderr2("Incorrect WebHostUI method argument count");
        return;
    }

    handler.second(handlerArgs);
}

void BaseWebHostUI::handleWebViewConsole(const String& tag, const String& text)
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
