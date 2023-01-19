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
    callback(DESTINATION_ALL, "parameterChanged", { index, value });
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
void WebUIBase::programLoaded(uint32_t index)
{
    callback(DESTINATION_ALL, "programLoaded", { index });
}
#endif

#if DISTRHO_PLUGIN_WANT_STATE
void WebUIBase::stateChanged(const char* key, const char* value)
{
    UIEx::stateChanged(key, value);
    callback(DESTINATION_ALL, "stateChanged", { key, value });
}
#endif

#if defined(HIPHOP_SHARED_MEMORY_SIZE)
void WebUIBase::sharedMemoryReady()
{
    callback(DESTINATION_ALL, "sharedMemoryReady");
}

void WebUIBase::sharedMemoryChanged(const uint8_t* data, size_t size, uint32_t hints)
{
    BinaryData binData(data, data + size);
# if defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
    callback(DESTINATION_ALL, "sharedMemoryChanged", { binData, hints });
# elif defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
    callback(DESTINATION_ALL, "_b64SharedMemoryChanged", { binData, hints });
# endif
}
#endif

void WebUIBase::onMessageReceived(const Variant& args, uintptr_t origin)
{
    (void)args;
    (void)origin;
}

void WebUIBase::handleMessage(const Variant& args, uintptr_t origin)
{
    if (! args.isArray()) {
        d_stderr2("Message must be an array");
        return;
    }

    if ((args.getArraySize() < 2) || (args[0].getString() != "UI")) {
        onMessageReceived(args, origin); // passthrough
        return;
    }

    String function = args[1].asString();

    if (fHandler.find(function) == fHandler.end()) {
        d_stderr2("Unknown WebUI function");
        return;
    }

    const Variant handlerArgs = args.sliceArray(2);
    
    ArgumentCountAndFunctionHandler handler = fHandler[function];
    const int argsCount = handlerArgs.getArraySize();

    if (argsCount < handler.first) {
        d_stderr2("Missing WebUI function arguments (%d < %d)", argsCount, handler.first);
        return;
    }

    handler.second(handlerArgs, origin);
}

void WebUIBase::callback(uintptr_t destination, const char* function, Variant args)
{
    args.insertArrayItem(0, serializeFunctionArgument(function));
    args.insertArrayItem(0, "UI");

    postMessage(args, destination);
}

Variant WebUIBase::serializeFunctionArgument(const char* function)
{
    return fFuncArgSerializer(function);
}

void WebUIBase::setBuiltInFunctionHandlers()
{
    setFunctionHandler("getInitWidthCSS", 0, [this](const Variant&, uintptr_t origin) {
        callback(origin, "getInitWidthCSS", { static_cast<double>(getInitWidthCSS()) });
    });

    setFunctionHandler("getInitHeightCSS", 0, [this](const Variant&, uintptr_t origin) {
        callback(origin, "getInitHeightCSS", { static_cast<double>(getInitHeightCSS()) });
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

    // It is not possible to implement JS synchronous calls that return values
    // without resorting to dirty hacks. Use JS async functions instead, and
    // fulfill their promises here.

    setFunctionHandler("isStandalone", 0, [this](const Variant&, uintptr_t origin) {
        callback(origin, "isStandalone", { isStandalone() });
    });
}
