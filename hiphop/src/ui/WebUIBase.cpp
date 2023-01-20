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

uintptr_t kDestinationAll = 0;
uintptr_t kExcludeNone = 0;

WebUIBase::WebUIBase(uint widthCssPx, uint heightCssPx, float initPixelRatio,
                        FunctionArgumentSerializer funcArgSerializer)
    : UIEx(initPixelRatio * widthCssPx, initPixelRatio * heightCssPx)
    , fInitWidthCssPx(widthCssPx)
    , fInitHeightCssPx(heightCssPx)
    , fFuncArgSerializer(funcArgSerializer != nullptr ? funcArgSerializer
                            : [](const char* f) { return f; })
{
    setBuiltInFunctionHandlers();
}

void WebUIBase::queue(const UiBlock& block)
{
    fUiQueueMutex.lock();
    fUiQueue.push(block);
    fUiQueueMutex.unlock();
}

const WebUIBase::FunctionHandler& WebUIBase::getFunctionHandler(const char* name)
{
    return fHandler[serializeFunctionArgument(name).asString()].second;
}

void WebUIBase::setFunctionHandler(const char* name, int argCount, const FunctionHandler& handler)
{
    fHandler[serializeFunctionArgument(name).asString()] = std::make_pair(argCount, handler);
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
    callback("parameterChanged", { index, value });
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
void WebUIBase::programLoaded(uint32_t index)
{
    callback("programLoaded", { index });
}
#endif

#if DISTRHO_PLUGIN_WANT_STATE
void WebUIBase::stateChanged(const char* key, const char* value)
{
    UIEx::stateChanged(key, value);
    callback("stateChanged", { key, value });
}
#endif

#if defined(HIPHOP_SHARED_MEMORY_SIZE)
void WebUIBase::sharedMemoryReady()
{
    callback("sharedMemoryReady");
}

void WebUIBase::sharedMemoryChanged(const uint8_t* data, size_t size, uint32_t hints)
{
    BinaryData binData(data, data + size);
# if defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
    callback("_bsonSharedMemoryChanged", { binData, hints });
# elif defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
    callback("_jsonSharedMemoryChanged", { binData, hints });
# endif
}
#endif

void WebUIBase::onMessageReceived(const Variant& payload, uintptr_t origin)
{
    (void)payload;
    (void)origin;
}

void WebUIBase::handleMessage(const Variant& payload, uintptr_t origin)
{
    if (! payload.isArray() || (payload.getArraySize() == 0)) {
        d_stderr2("Message must be a non-empty array");
        return;
    }

    String function = payload[0].asString();

    if (fHandler.find(function) == fHandler.end()) {
        d_stderr2("Unknown WebUI function");
        return;
    }

    const Variant handlerArgs = payload.sliceArray(1);
    
    ArgumentCountAndFunctionHandler handler = fHandler[function];
    const int argsCount = handlerArgs.getArraySize();

    if (argsCount < handler.first) {
        d_stderr2("Missing WebUI function arguments (%d < %d)", argsCount, handler.first);
        return;
    }

    handler.second(handlerArgs, origin);
}

void WebUIBase::callback(const char* function, Variant args, uintptr_t destination, uintptr_t exclude)
{
    args.insertArrayItem(0, serializeFunctionArgument(function));
    postMessage(args, destination, exclude);
}

Variant WebUIBase::serializeFunctionArgument(const char* function)
{
    return fFuncArgSerializer(function);
}

void WebUIBase::setBuiltInFunctionHandlers()
{
    setFunctionHandler("getInitWidthCSS", 0, [this](const Variant&, uintptr_t origin) {
        callback("getInitWidthCSS", { static_cast<double>(getInitWidthCSS()) }, origin);
    });

    setFunctionHandler("getInitHeightCSS", 0, [this](const Variant&, uintptr_t origin) {
        callback("getInitHeightCSS", { static_cast<double>(getInitHeightCSS()) }, origin);
    });

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    setFunctionHandler("sendNote", 3, [this](const Variant& args, uintptr_t /*origin*/) {
        sendNote(
            static_cast<uint8_t>(args[0].getNumber()),  // channel
            static_cast<uint8_t>(args[1].getNumber()),  // note
            static_cast<uint8_t>(args[2].getNumber())   // velocity
        );
    });
#endif

    setFunctionHandler("editParameter", 2, [this](const Variant& args, uintptr_t /*origin*/) {
        editParameter(
            static_cast<uint32_t>(args[0].getNumber()), // index
            static_cast<bool>(args[1].getBoolean())     // started
        );
    });

    setFunctionHandler("setParameterValue", 2, [this](const Variant& args, uintptr_t /*origin*/) {
        setParameterValue(
            static_cast<uint32_t>(args[0].getNumber()), // index
            static_cast<float>(args[1].getNumber())     // value
        );
    });

#if DISTRHO_PLUGIN_WANT_STATE
    setFunctionHandler("setState", 2, [this](const Variant& args, uintptr_t /*origin*/) {
        setState(
            args[0].getString(), // key
            args[1].getString()  // value
        );
    });
#endif

#if DISTRHO_PLUGIN_WANT_STATE && defined(HIPHOP_SHARED_MEMORY_SIZE)
    setFunctionHandler("writeSharedMemory", 2, [this](const Variant& args, uintptr_t /*origin*/) {
# if defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
        BinaryData data = args[0].getBinaryData();
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
    setFunctionHandler("sideloadWasmBinary", 1, [this](const Variant& args, uintptr_t /*origin*/) {
# if defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
        Variant::BinaryData data = args[0].getBinaryData();
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

    setFunctionHandler("isStandalone", 0, [this](const Variant&, uintptr_t origin) {
        callback("isStandalone", { isStandalone() }, origin);
    });
}
