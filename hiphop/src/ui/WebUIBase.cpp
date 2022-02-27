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

USE_NAMESPACE_DISTRHO

WebUIBase::WebUIBase(uint width, uint height)
    : UIEx(width, height)
{
    // TODO
}

WebUIBase::~WebUIBase()
{
    // TODO
}

void WebUIBase::postMessage(const JsValueVector& args)
{
    // TODO - broadcast to all clients
}

void WebUIBase::onMessageReceived(const JsValueVector& args)
{
    (void)args;
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
    postMessage({"UI", "stateChanged", key, value});
}
#endif

#if HIPHOP_ENABLE_SHARED_MEMORY
void WebUIBase::sharedMemoryChanged(const char* metadata, const unsigned char* data, size_t size)
{
    (void)size;
    String b64Data = String::asBase64(data, size);
    postMessage({"UI", "_sharedMemoryChanged", metadata, b64Data});
}
#endif
