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

#ifndef WEBVIEW_BASE_HPP
#define WEBVIEW_BASE_HPP

#include <cstdint>

#include "distrho/extra/String.hpp"
#include "Window.hpp"

#include "extra/JSValue.hpp"

START_NAMESPACE_DISTRHO

class WebViewEventHandler
{
public:
    virtual void handleWebViewLoadFinished() = 0;
    virtual void handleWebViewScriptMessage(const JSValue& args) = 0;
    virtual void handleWebViewConsole(const String& tag, const String& text) = 0;

};

class WebViewBase
{
public:
    WebViewBase();
    virtual ~WebViewBase() {}

    uint getWidth();
    uint getHeight();
    void setSize(uint width, uint height);
    
    uint32_t getBackgroundColor();
    void     setBackgroundColor(uint32_t rgba);
    
    uintptr_t getParent();
    void      setParent(uintptr_t parent);
    
    bool getKeyboardFocus();
    void setKeyboardFocus(bool focus);

    void setPrintTraffic(bool printTraffic);
    void setEventHandler(WebViewEventHandler* handler);
    
    void postMessage(const JSValue& args);

    virtual void realize() = 0;
    virtual void navigate(String& url) = 0;
    virtual void runScript(String& source) = 0;
    virtual void injectScript(String& source) = 0;

protected:
    virtual void onSize(uint width, uint height) = 0;
    virtual void onKeyboardFocus(bool focus) { (void)focus; };

    void injectHostObjectScripts();
    
    void handleLoadFinished();
    void handleScriptMessage(const JSValue& args);

private:
    void addStylesheet(String& source);

    uint      fWidth;
    uint      fHeight;
    uint32_t  fBackgroundColor;
    uintptr_t fParent;
    bool      fKeyboardFocus;
    bool      fPrintTraffic;

    WebViewEventHandler* fHandler;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebViewBase)

};

END_NAMESPACE_DISTRHO

#endif // WEBVIEW_BASE_HPP
