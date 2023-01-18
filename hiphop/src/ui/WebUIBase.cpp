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
    setBuiltInMethodHandlers();
}

void WebUIBase::queue(const UiBlock& block)
{
    fUiQueueMutex.lock();
    fUiQueue.push(block);
    fUiQueueMutex.unlock();
}

const WebUIBase::MethodHandler& WebUIBase::getMethodHandler(const char* name)
{
    return fHandler[String(name)].second;
}

void WebUIBase::setMethodHandler(const char* name, int argCount, const MethodHandler& handler)
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
    notify(DESTINATION_ALL, "parameterChanged", { index, value });
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
void WebUIBase::programLoaded(uint32_t index)
{
    notify(DESTINATION_ALL, "programLoaded", { index });
}
#endif

#if DISTRHO_PLUGIN_WANT_STATE
void WebUIBase::stateChanged(const char* key, const char* value)
{
    UIEx::stateChanged(key, value);
    notify(DESTINATION_ALL, "stateChanged", { key, value });
}
#endif

#if defined(HIPHOP_SHARED_MEMORY_SIZE)
void WebUIBase::sharedMemoryReady()
{
    notify(DESTINATION_ALL, "sharedMemoryReady");
}

void WebUIBase::sharedMemoryChanged(const uint8_t* data, size_t size, uint32_t hints)
{
    BinaryData binData(data, data + size);
# if defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
    notify(DESTINATION_ALL, "sharedMemoryChanged", { binData, hints });
# elif defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
    notify(DESTINATION_ALL, "_b64SharedMemoryChanged", { binData, hints });
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

    String method = getMethodSignature(args);

    if (fHandler.find(method) == fHandler.end()) {
        d_stderr2("Unknown WebUI method");
        return;
    }

    const Variant handlerArgs = args.sliceArray(2);
    
    ArgumentCountAndMethodHandler handler = fHandler[method];
    const int argsCount = handlerArgs.getArraySize();

    if (argsCount < handler.first) {
        d_stderr2("Missing WebUI method arguments (%d < %d)", argsCount, handler.first);
        return;
    }

    handler.second(handlerArgs, origin);
}

String WebUIBase::getMethodSignature(const Variant& args)
{
    return args[1].getString();
}

void WebUIBase::setMethodSignature(Variant& args, String method)
{
    args.insertArrayItem(1, method);
}

void WebUIBase::notify(uintptr_t destination, const char* method, Variant args)
{
    args.insertArrayItem(0, "UI");
    setMethodSignature(args, String(method));
    postMessage(args, destination);
}

void WebUIBase::setBuiltInMethodHandlers()
{
    setMethodHandler("getInitWidthCSS", 0, [this](const Variant&, uintptr_t origin) {
        notify(origin, "getInitWidthCSS", { static_cast<double>(getInitWidthCSS()) });
    });

    setMethodHandler("getInitHeightCSS", 0, [this](const Variant&, uintptr_t origin) {
        notify(origin, "getInitHeightCSS", { static_cast<double>(getInitHeightCSS()) });
    });

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    setMethodHandler("sendNote", 3, [this](const Variant& args, uintptr_t /*origin*/) {
        sendNote(
            static_cast<uint8_t>(args[0].getNumber()),  // channel
            static_cast<uint8_t>(args[1].getNumber()),  // note
            static_cast<uint8_t>(args[2].getNumber())   // velocity
        );
    });
#endif

    setMethodHandler("editParameter", 2, [this](const Variant& args, uintptr_t /*origin*/) {
        editParameter(
            static_cast<uint32_t>(args[0].getNumber()), // index
            static_cast<bool>(args[1].getBoolean())     // started
        );
    });

    setMethodHandler("setParameterValue", 2, [this](const Variant& args, uintptr_t /*origin*/) {
        setParameterValue(
            static_cast<uint32_t>(args[0].getNumber()), // index
            static_cast<float>(args[1].getNumber())     // value
        );
    });

#if DISTRHO_PLUGIN_WANT_STATE
    setMethodHandler("setState", 2, [this](const Variant& args, uintptr_t /*origin*/) {
        setState(
            args[0].getString(), // key
            args[1].getString()  // value
        );
    });
#endif

#if DISTRHO_PLUGIN_WANT_STATE && defined(HIPHOP_SHARED_MEMORY_SIZE)
    setMethodHandler("writeSharedMemory", 2, [this](const Variant& args, uintptr_t /*origin*/) {
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
    setMethodHandler("sideloadWasmBinary", 1, [this](const Variant& args, uintptr_t /*origin*/) {
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

    setMethodHandler("isStandalone", 0, [this](const Variant&, uintptr_t origin) {
        notify(origin, "isStandalone", { isStandalone() });
    });
}
