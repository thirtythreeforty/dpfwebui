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

#include "WebUIBase.hpp"

#include "distrho/DistrhoPluginUtils.hpp"
#include "distrho/extra/Base64.hpp"

USE_NAMESPACE_DISTRHO

WebUIBase::WebUIBase(uint widthCssPx, uint heightCssPx, float initPixelRatio)
    : UIEx(initPixelRatio * widthCssPx, initPixelRatio * heightCssPx)
    , fInitWidthCssPx(widthCssPx)
    , fInitHeightCssPx(heightCssPx)
{
    initHandlers();
}

void WebUIBase::queue(const UiBlock& block)
{
    fUiQueueMutex.lock();
    fUiQueue.push(block);
    fUiQueueMutex.unlock();
}

const WebUIBase::MessageHandler& WebUIBase::getMessageHandler(const char* name)
{
    return fHandler[String(name)].second;
}

void WebUIBase::setMessageHandler(const char* name, int argCount, const MessageHandler& handler)
{
    fHandler[String(name)] = std::make_pair(argCount, handler);
}

bool WebUIBase::isDryRun()
{
    // When running as a plugin the UI ctor/dtor can be repeatedly called with
    // no parent window available, avoid allocating resources in such cases.
    return ! isStandalone() && (getParentWindowHandle() == 0);
}

void WebUIBase::uiIdle()
{
    UIEx::uiIdle();
    fUiQueueMutex.lock();

    while (! fUiQueue.empty()) {
        const UiBlock block = fUiQueue.front();
        fUiQueue.pop();
        block();
    }

    fUiQueueMutex.unlock();
}

void WebUIBase::parameterChanged(uint32_t index, float value)
{
    postMessage({"UI", "parameterChanged", index, value}, DESTINATION_ALL);
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
void WebUIBase::programLoaded(uint32_t index)
{
    postMessage({"UI", "programLoaded", index}, DESTINATION_ALL);
}
#endif

#if DISTRHO_PLUGIN_WANT_STATE
void WebUIBase::stateChanged(const char* key, const char* value)
{
    UIEx::stateChanged(key, value);
    postMessage({"UI", "stateChanged", key, value}, DESTINATION_ALL);
}
#endif

#if defined(HIPHOP_SHARED_MEMORY_SIZE)
void WebUIBase::sharedMemoryReady()
{
    postMessage({"UI", "sharedMemoryReady"}, DESTINATION_ALL);
}

void WebUIBase::sharedMemoryChanged(const uint8_t* data, size_t size, uint32_t hints)
{
    JSValue::BinaryData binData(data, data + size);
# if defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
    postMessage({"UI", "sharedMemoryChanged", binData, hints}, DESTINATION_ALL);
# elif defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
    postMessage({"UI", "_b64SharedMemoryChanged", binData, hints}, DESTINATION_ALL);
# endif
}
#endif

void WebUIBase::onMessageReceived(const JSValue& args, uintptr_t origin)
{
    (void)args;
    (void)origin;
}

void WebUIBase::handleMessage(const JSValue& args, uintptr_t origin)
{
    if (! args.isArray()) {
        d_stderr2("Message must be an array");
        return;
    }

    if ((args.getArraySize() < 2) || (args[0].getString() != "UI")) {
        onMessageReceived(args, origin); // passthrough
        return;
    }

    String key = args[1].getString();

    if (fHandler.find(key) == fHandler.end()) {
        d_stderr2("Unknown WebUI method");
        return;
    }

    const JSValue handlerArgs = args.sliceArray(2);
    
    ArgumentCountAndMessageHandler handler = fHandler[key];
    const int argsCount = handlerArgs.getArraySize();

    if (argsCount < handler.first) {
        d_stderr2("Missing WebUI method arguments (%d < %d)", argsCount, handler.first);
        return;
    }

    handler.second(handlerArgs, origin);
}

void WebUIBase::initHandlers()
{
    setMessageHandler("getInitWidthCSS", 0, [this](const JSValue&, uintptr_t origin) {
        postMessage({"UI", "getInitWidthCSS", static_cast<double>(getInitWidthCSS())}, origin);
    });

    setMessageHandler("getInitHeightCSS", 0, [this](const JSValue&, uintptr_t origin) {
        postMessage({"UI", "getInitHeightCSS", static_cast<double>(getInitHeightCSS())}, origin);
    });

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    setMessageHandler("sendNote", 3, [this](const JSValue& args, uintptr_t /*origin*/) {
        sendNote(
            static_cast<uint8_t>(args[0].getNumber()),  // channel
            static_cast<uint8_t>(args[1].getNumber()),  // note
            static_cast<uint8_t>(args[2].getNumber())   // velocity
        );
    });
#endif

    setMessageHandler("editParameter", 2, [this](const JSValue& args, uintptr_t /*origin*/) {
        editParameter(
            static_cast<uint32_t>(args[0].getNumber()), // index
            static_cast<bool>(args[1].getBoolean())     // started
        );
    });

    setMessageHandler("setParameterValue", 2, [this](const JSValue& args, uintptr_t /*origin*/) {
        setParameterValue(
            static_cast<uint32_t>(args[0].getNumber()), // index
            static_cast<float>(args[1].getNumber())     // value
        );
    });

#if DISTRHO_PLUGIN_WANT_STATE
    setMessageHandler("setState", 2, [this](const JSValue& args, uintptr_t /*origin*/) {
        setState(
            args[0].getString(), // key
            args[1].getString()  // value
        );
    });
#endif

#if DISTRHO_PLUGIN_WANT_STATE && defined(HIPHOP_SHARED_MEMORY_SIZE)
    setMessageHandler("writeSharedMemory", 2, [this](const JSValue& args, uintptr_t /*origin*/) {
# if defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
        JSValue::BinaryData data = args[0].getBinaryData();
# elif defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
        std::vector<uint8_t> data = d_getChunkFromBase64String(args[0].getString());
# endif
        writeSharedMemory(
            data.data(),
            static_cast<size_t>(data.size()),
            static_cast<size_t>(args[1].getNumber()),  // offset
            static_cast<uint32_t>(args[2].getNumber()) // hints
        );
    });

# if defined(HIPHOP_WASM_SUPPORT)
    setMessageHandler("sideloadWasmBinary", 1, [this](const JSValue& args, uintptr_t /*origin*/) {
# if defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
        JSValue::BinaryData data = args[0].getBinaryData();
# elif defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
        std::vector<uint8_t> data = d_getChunkFromBase64String(args[0].getString());
# endif
        sideloadWasmBinary(
            data.data(),
            static_cast<size_t>(data.size())
        );
    });
# endif
#endif // DISTRHO_PLUGIN_WANT_STATE && HIPHOP_SHARED_MEMORY_SIZE

    // It is not possible to implement JS synchronous calls that return values
    // without resorting to dirty hacks. Use JS async functions instead, and
    // fulfill their promises here.

    setMessageHandler("isStandalone", 0, [this](const JSValue&, uintptr_t origin) {
        postMessage({"UI", "isStandalone", isStandalone()}, origin);
    });
}
