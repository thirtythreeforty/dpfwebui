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
#include <vector>

#include "extra/UIEx.hpp"
#include "WebViewBase.hpp"

#ifdef HIPHOP_NETWORK_UI
# include "NetworkUI.hpp"
#endif

START_NAMESPACE_DISTRHO

class WebUIBase;
float getDisplayScaleFactor(WebUIBase* ui); // implemented for each platform

#ifdef HIPHOP_NETWORK_UI
typedef NetworkUI WebUIBaseParent;
#else
typedef UIEx WebUIBaseParent;
#endif

class WebUIBase : public WebUIBaseParent, private WebViewEventHandler
{
public:
    WebUIBase(uint widthCssPx, uint heightCssPx, uint32_t backgroundColor, 
        bool startLoading = true);
    virtual ~WebUIBase();

    typedef std::function<void()> UiBlock;

    void queue(const UiBlock& block);
    
    uint getInitialWidth() const { return fInitialWidth; }
    uint getInitialHeight() const { return fInitialHeight; }

    uintptr_t getPlatformWindow() const { return fPlatformWindow; }

    WebViewBase* getWebView() { return fWebView; }

    virtual void openSystemWebBrowser(String& url) = 0;

protected:
    bool shouldCreateWebView();
    void setWebView(WebViewBase* webView);

    void load();
    
    void runScript(String& source);
    void injectScript(String& source);

    void webViewPostMessage(const JsValueVector& args);

    void flushInitMessageQueue();
    void setKeyboardFocus(bool focus);

    void uiIdle() override;

#if HIPHOP_ENABLE_SHARED_MEMORY
    void sharedMemoryChanged(const char* metadata, const unsigned char* data, size_t size) override;
#endif

    void sizeChanged(uint width, uint height) override;

    void parameterChanged(uint32_t index, float value) override;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void programLoaded(uint32_t index) override;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* key, const char* value) override;
#endif

    virtual void onWebContentReady() {}
    virtual void onWebMessageReceived(const JsValueVector& args) { (void)args; }

    virtual void sizeRequest(const UiBlock& block);

    virtual uintptr_t createStandaloneWindow() = 0;
    virtual void      processStandaloneEvents() = 0;

private:
    void initHandlers();

    // WebViewEventHandler

    virtual void handleWebViewLoadFinished() override;
    virtual void handleWebViewScriptMessage(const JsValueVector& args) override;
    virtual void handleWebViewConsole(const String& tag, const String& text) override;

    typedef std::vector<JsValueVector> InitMessageQueue;

    typedef std::function<void(const JsValueVector& args)> MessageHandler;
    typedef std::pair<int, MessageHandler> ArgumentCountAndMessageHandler;
    typedef std::unordered_map<std::string, ArgumentCountAndMessageHandler> MessageHandlerMap;

    uint              fInitialWidth;
    uint              fInitialHeight;
    uint32_t          fBackgroundColor;
    bool              fMessageQueueReady;
    bool              fUiBlockQueued;
    uintptr_t         fPlatformWindow;
    WebViewBase*      fWebView;
    UiBlock           fUiBlock;
    InitMessageQueue  fInitMessageQueue;
    MessageHandlerMap fHandler;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebUIBase)

};

END_NAMESPACE_DISTRHO

#endif  // WEB_UI_BASE_HPP
