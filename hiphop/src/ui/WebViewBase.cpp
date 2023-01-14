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

#include "WebViewBase.hpp"

// This could be moved into dpf.js but then JavaScript code should be checking
// for the platform type in order to insert JS_POST_MESSAGE_SHIM. Leaving
// platform-dependent code in a single place (C++) for a cleaner approach.
#define JS_CREATE_HOST_OBJECT  "window.host = new EventTarget;" \
                               "window.host.addMessageListener = (lr) => {" \
                               "  window.host.addEventListener('message', (ev) => lr(ev.detail))" \
                               "};" \
                               "window.host.env = {};"
#define JS_CREATE_CONSOLE  "window.console = {" \
                           "  log  : (s) => window.host.postMessage(['console', 'log'  , String(s)])," \
                           "  info : (s) => window.host.postMessage(['console', 'info' , String(s)])," \
                           "  warn : (s) => window.host.postMessage(['console', 'warn' , String(s)])," \
                           "  error: (s) => window.host.postMessage(['console', 'error', String(s)])" \
                           "};"

/**
 * Keep this class generic, plugin specific features belong to WebViewUI.
 */

USE_NAMESPACE_DISTRHO

WebViewBase::WebViewBase()
    : fWidth(0)
    , fHeight(0)
    , fBackgroundColor(0)
    , fParent(0)
    , fKeyboardFocus(false)
    , fPrintTraffic(false)
    , fHandler(nullptr)
{}

uint WebViewBase::getWidth()
{
    return fWidth;
}

uint WebViewBase::getHeight()
{
    return fHeight;
}

void WebViewBase::setSize(uint width, uint height)
{
    fWidth = width;
    fHeight = height;
    onSize(width, height);
}

uint32_t WebViewBase::getBackgroundColor()
{
    return fBackgroundColor;
}

void WebViewBase::setBackgroundColor(uint32_t color)
{
    fBackgroundColor = color;
}

uintptr_t WebViewBase::getParent()
{
    return fParent;
}

void WebViewBase::setParent(uintptr_t parent)
{
    fParent = parent;
    onSetParent(parent);
}

bool WebViewBase::getKeyboardFocus()
{
    return fKeyboardFocus;
}    

void WebViewBase::setKeyboardFocus(bool focus)
{
    fKeyboardFocus = focus;
    onKeyboardFocus(focus);
}

void WebViewBase::setPrintTraffic(bool printTraffic)
{
    fPrintTraffic = printTraffic;
}

void WebViewBase::setEnvironmentBool(const char* key, bool value)
{
    // Mostly used for allowing JavaScript code to detect unavailable features
    // in some web view implementations. window.host.env is merged into
    // DISTRHO.env by dpf.js to keep all environment info in a single place.
    String js = String("window.host.env.") + key + "=" + (value ? "true" : "false") + ";";
    injectScript(js);
}

void WebViewBase::setEventHandler(WebViewEventHandler* handler)
{
    fHandler = handler;
}

void WebViewBase::postMessage(const JSValue& args)
{
#if defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
    // This method implements something like a "reverse postMessage()" aiming to
    // keep the bridge symmetrical. Global window.host is an EventTarget that
    // can be listened for messages.
    String payload = args.toJSON();

    if (fPrintTraffic) {
        d_stderr("cpp->js : %s", payload.buffer());
    }
    
    String js = "window.host.dispatchEvent(new CustomEvent('message',"
                    "{detail:" + payload + "}"
                "));";
    runScript(js);
#else
    (void)args;
#endif
}

void WebViewBase::injectHostObjectScripts()
{
    String js = String(JS_CREATE_HOST_OBJECT) + String(JS_CREATE_CONSOLE);
    injectScript(js);
}

void WebViewBase::handleLoadFinished()
{
    if (fHandler != nullptr) {
        fHandler->handleWebViewLoadFinished();
    }
}

void WebViewBase::handleScriptMessage(const JSValue& args)
{
    if ((args.getArraySize() == 3) && (args[0].getString() == "console")) {
        if (fHandler != nullptr) {
            fHandler->handleWebViewConsole(args[1].getString(), args[2].getString());
        }
    } else {
#if defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
        if (fPrintTraffic) {
            d_stderr("cpp<-js : %s", args.toJSON().buffer());
        }
#endif
        if (fHandler != nullptr) {
            fHandler->handleWebViewScriptMessage(args);
        }
    }
}

void WebViewBase::addStylesheet(String& source)
{
    String js = "document.head.insertAdjacentHTML('beforeend',"
                    "'<style>" + source + "</style>');";
    runScript(js);
}
