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

#ifndef EDGE_WEBVIEW_HPP
#define EDGE_WEBVIEW_HPP

#define UNICODE
#define CINTERFACE
#define COBJMACROS

#include <functional>
#include <string>
#include <vector>

#include "WebView2.h"

#include "../WebViewBase.hpp"
#include "WebView2EventHandler.hpp"

/*
  The easy way to work with Edge WebView2 requires WIL. According to MS:
  "The Windows Implementation Libraries (WIL) is a header-only C++ library
  created to make life easier for developers on Windows through readable
  type-safe C++ interfaces for common Windows coding patterns."
  Unfortunately WIL is not compatible with the MinGW GCC. But because Edge
  WebView2 is a COM component, it can still be integrated using its C interface.

  https://github.com/microsoft/wil/issues/117
  https://github.com/jchv/webview2-in-mingw
  https://www.codeproject.com/Articles/13601/COM-in-plain-C
*/

START_NAMESPACE_DISTRHO

class WeakWebView2EventHandler;

class EdgeWebView : public WebViewBase, public edge::WebView2EventHandler
{
public:
    EdgeWebView();
    virtual ~EdgeWebView();

    void realize() override;
    void navigate(String& url) override;
    void runScript(String& source) override;
    void injectScript(String& source) override;

    // WebView2EventHandler

    HRESULT handleWebView2EnvironmentCompleted(HRESULT result,
                                    ICoreWebView2Environment* environment) override;
    HRESULT handleWebView2ControllerCompleted(HRESULT result,
                                    ICoreWebView2Controller* controller) override;
    HRESULT handleWebView2NavigationCompleted(ICoreWebView2 *sender,
                                    ICoreWebView2NavigationCompletedEventArgs *eventArgs) override;
    HRESULT handleWebView2WebMessageReceived(ICoreWebView2 *sender,
                                    ICoreWebView2WebMessageReceivedEventArgs *eventArgs) override;

    typedef std::function<void(UINT, KBDLLHOOKSTRUCT*, bool)> LowLevelKeyboardHookCallback;

    LowLevelKeyboardHookCallback lowLevelKeyboardHookCallback;

protected:
    void onSize(uint width, uint height) override;

private:
    void errorMessageBox(std::wstring message);
    void webViewLoaderErrorMessageBox(HRESULT result);

    LPWSTR  fHelperClassName;
    HWND    fHelperHwnd;
    HHOOK   fKeyboardHook;
    String  fUrl;
    
    std::vector<String> fInjectedScripts;

    WeakWebView2EventHandler* fHandler;
    ICoreWebView2Controller*  fController;
    ICoreWebView2*            fView;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EdgeWebView)

};


// The event handler lifetime cannot be bound to its owner lifetime, otherwise
// Edge WebView2 could callback a deleted object. That would happen for example
// example if the plugin UI is created and suddenly destroyed before web content
// finishes loading, or before WebView2 has fully initialized itself. This
// behavior can be reproduced by opening the plugin window on Carla and
// immediately closing it before the web UI displays. This handler class must be
// init'd using the new operator and disposed of by calling its release() method.

class WeakWebView2EventHandler : public edge::WebView2EventHandler {
public:
    WeakWebView2EventHandler(edge::WebView2EventHandler* ownerRef)
        : fOwnerWeakRef(ownerRef)
    {
        incRefCount();
    }

    void release()
    {
        fOwnerWeakRef = nullptr;
        if (decRefCount() == 0) {
            delete this;
        }
    }

    HRESULT handleWebView2EnvironmentCompleted(HRESULT result,
                                    ICoreWebView2Environment* environment) override
    {
        if (fOwnerWeakRef != nullptr) {
            return fOwnerWeakRef->handleWebView2EnvironmentCompleted(result, environment);
        } else {
            return E_ABORT;
        }
    }

    HRESULT handleWebView2ControllerCompleted(HRESULT result,
                                    ICoreWebView2Controller* controller) override
    {
        if (fOwnerWeakRef != nullptr) {
            return fOwnerWeakRef->handleWebView2ControllerCompleted(result, controller);
        } else {
            return E_ABORT;
        }
    }

    HRESULT handleWebView2NavigationCompleted(ICoreWebView2 *sender,
                                    ICoreWebView2NavigationCompletedEventArgs *eventArgs) override
    {
        if (fOwnerWeakRef != nullptr) {
            return fOwnerWeakRef->handleWebView2NavigationCompleted(sender, eventArgs);
        } else {
            return E_ABORT;
        }
    }

    HRESULT handleWebView2WebMessageReceived(ICoreWebView2 *sender,
                                    ICoreWebView2WebMessageReceivedEventArgs *eventArgs) override
    {
        if (fOwnerWeakRef != nullptr) {
            return fOwnerWeakRef->handleWebView2WebMessageReceived(sender, eventArgs);
        } else {
            return E_ABORT;
        }
    }

private:
    edge::WebView2EventHandler* fOwnerWeakRef;

};

END_NAMESPACE_DISTRHO

#endif  // EDGE_WEBVIEW_HPP
