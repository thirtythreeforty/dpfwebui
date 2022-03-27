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

#include "WebUIBase.hpp"

#include "distrho/extra/Base64.hpp"

USE_NAMESPACE_DISTRHO

WebUIBase::WebUIBase(uint widthCssPx, uint heightCssPx, float initScaleFactorForVST3)
    : UIEx(initScaleFactorForVST3 * widthCssPx, initScaleFactorForVST3 * heightCssPx)
    , fUnscaledInitWidth(widthCssPx)
    , fUnscaledInitHeight(heightCssPx)
{
    initHandlers();
}

bool WebUIBase::isDryRun()
{
    // When running as a plugin the UI ctor/dtor can be repeatedly called with
    // no parent window available, avoid allocating resources in such cases.
    return ! isStandalone() && (getParentWindowHandle() == 0);
}

void WebUIBase::sizeChanged(uint width, uint height)
{
    UIEx::sizeChanged(width, height);
    postMessage({"UI", "sizeChanged", width, height});
}

void WebUIBase::parameterChanged(uint32_t index, float value)
{
    postMessage({"UI", "parameterChanged", index, value});
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
void WebUIBase::programLoaded(uint32_t index)
{
    postMessage({"UI", "programLoaded", index});
}
#endif

#if DISTRHO_PLUGIN_WANT_STATE
void WebUIBase::stateChanged(const char* key, const char* value)
{
    UIEx::stateChanged(key, value);
    postMessage({"UI", "stateChanged", key, value});
}
#endif

#if HIPHOP_SHARED_MEMORY_SIZE
void WebUIBase::sharedMemoryReady()
{
    postMessage({"UI", "sharedMemoryReady"});
}

void WebUIBase::sharedMemoryChanged(const unsigned char* data, size_t size, uint32_t hints)
{
    (void)size;
    String b64Data = String::asBase64(data, size);
    postMessage({"UI", "_sharedMemoryChanged", b64Data, hints});
}
#endif

void WebUIBase::onMessageReceived(const JSValue& args)
{
    (void)args;
}

void WebUIBase::handleMessage(const JSValue& args)
{
    if ((args.getArraySize() < 2) || (args[0].getString() != "UI")) {
        onMessageReceived(args); // passthrough
        return;
    }

    String key = args[1].getString();

    if (fHandler.find(key.buffer()) == fHandler.end()) {
        d_stderr2("Unknown WebUI method");
        return;
    }

    const JSValue handlerArgs = args.sliceArray(2);
    
    ArgumentCountAndMessageHandler handler = fHandler[key.buffer()];

    if (handler.first != handlerArgs.getArraySize()) {
        d_stderr2("Incorrect WebUI method argument count");
        return;
    }

    handler.second(handlerArgs);
}

void WebUIBase::initHandlers()
{
    fHandler["getInitWidth"] = std::make_pair(0, [this](const JSValue&) {
        postMessage({"UI", "getInitWidth", static_cast<double>(getUnscaledInitWidth())});
    });

    fHandler["getInitHeight"] = std::make_pair(0, [this](const JSValue&) {
        postMessage({"UI", "getInitHeight", static_cast<double>(getUnscaledInitHeight())});
    });

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    fHandler["sendNote"] = std::make_pair(3, [this](const JSValue& args) {
        sendNote(
            static_cast<uint8_t>(args[0].getNumber()),  // channel
            static_cast<uint8_t>(args[1].getNumber()),  // note
            static_cast<uint8_t>(args[2].getNumber())   // velocity
        );
    });
#endif

    fHandler["editParameter"] = std::make_pair(2, [this](const JSValue& args) {
        editParameter(
            static_cast<uint32_t>(args[0].getNumber()), // index
            static_cast<bool>(args[1].getBoolean())     // started
        );
    });

    fHandler["setParameterValue"] = std::make_pair(2, [this](const JSValue& args) {
        setParameterValue(
            static_cast<uint32_t>(args[0].getNumber()), // index
            static_cast<float>(args[1].getNumber())     // value
        );
    });

#if DISTRHO_PLUGIN_WANT_STATE
    fHandler["setState"] = std::make_pair(2, [this](const JSValue& args) {
        setState(
            args[0].getString(), // key
            args[1].getString()  // value
        );
    });
#endif

#if DISTRHO_PLUGIN_WANT_STATE && HIPHOP_SHARED_MEMORY_SIZE
    fHandler["writeSharedMemory"] = std::make_pair(2, [this](const JSValue& args) {
        std::vector<uint8_t> data = d_getChunkFromBase64String(args[0].getString());
        writeSharedMemory(
            static_cast<const unsigned char*>(data.data()),
            static_cast<size_t>(data.size()),
            static_cast<size_t>(args[1].getNumber()),  // offset
            static_cast<uint32_t>(args[2].getNumber()) // hints
        );
    });

#if defined(HIPHOP_WASM_SUPPORT)
    fHandler["sideloadWasmBinary"] = std::make_pair(1, [this](const JSValue& args) {
        std::vector<uint8_t> data = d_getChunkFromBase64String(args[0].getString());
        sideloadWasmBinary(
            static_cast<const unsigned char*>(data.data()),
            static_cast<size_t>(data.size())
        );
    });
#endif
#endif // DISTRHO_PLUGIN_WANT_STATE && HIPHOP_SHARED_MEMORY_SIZE

    // It is not possible to implement JS synchronous calls that return values
    // without resorting to dirty hacks. Use JS async functions instead, and
    // fulfill their promises here. See for example getWidth() and getHeight().

    fHandler["isStandalone"] = std::make_pair(0, [this](const JSValue&) {
        postMessage({"UI", "isStandalone", isStandalone()});
    });
}
