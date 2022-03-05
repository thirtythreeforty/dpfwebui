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

#ifndef WEB_UI_BASE_HPP
#define WEB_UI_BASE_HPP

#include <functional>
#include <unordered_map>
#include <utility>

#include "extra/UIEx.hpp"
#include "JsValue.hpp"

START_NAMESPACE_DISTRHO

class WebUIBase : public UIEx
{
public:
    WebUIBase(uint width = 0, uint height = 0);
    virtual ~WebUIBase() {}

protected:
    void uiIdle() override {}

    void sizeChanged(uint width, uint height) override;
    void parameterChanged(uint32_t index, float value) override;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void programLoaded(uint32_t index) override;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* key, const char* value) override;
#endif
#if defined(HIPHOP_SHARED_MEMORY_SIZE)
    void sharedMemoryReady() override;
    void sharedMemoryChanged(const unsigned char* data, size_t size, const char* token) override;
#endif

    virtual void postMessage(const JsValueVector& args) = 0;
    virtual void onMessageReceived(const JsValueVector& args);

    void handleMessage(const JsValueVector& args);

    typedef std::function<void(const JsValueVector& args)> MessageHandler;
    typedef std::pair<int, MessageHandler> ArgumentCountAndMessageHandler;
    typedef std::unordered_map<std::string, ArgumentCountAndMessageHandler> MessageHandlerMap;

    MessageHandlerMap fHandler;

private:
    void initHandlers();

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebUIBase)

};

END_NAMESPACE_DISTRHO

#endif  // WEB_UI_BASE_HPP
