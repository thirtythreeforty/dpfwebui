/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021 Luciano Iam <oss@lucianoiam.com>
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

#ifndef CEF_HELPER_HPP
#define CEF_HELPER_HPP

#include <string>
#include <vector>

#include <X11/Xlib.h>

#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_client.h"

#include "IpcChannel.hpp"

// Main process
class CefHelper : public CefApp, public CefClient, 
                  public CefBrowserProcessHandler, public CefLoadHandler,
                  public CefRequestHandler, public CefResourceRequestHandler,
                  public CefResponseFilter, public CefDialogHandler
{
public:
    CefHelper();
    virtual ~CefHelper();

    int run(const CefMainArgs& args);

    // CefClient

    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override
    {
        return this;
    }
    
    CefRefPtr<CefLoadHandler> GetLoadHandler() override
    {
        return this;
    }
    
    CefRefPtr<CefDialogHandler> GetDialogHandler() override
    {
        return this;
    }

    CefRefPtr<CefRequestHandler> GetRequestHandler() override
    {
        return this;
    }

    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 CefProcessId sourceProcess,
                                 CefRefPtr<CefProcessMessage> message) override;

    // CefBrowserProcessHandler

    void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> commandLine) override;

    // CefLoadHandler
    
    void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                    TransitionType transitionType) override;

    void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                   int httpStatusCode) override;

    // CefRequestHandler

    CefRefPtr<CefResourceRequestHandler> 
    GetResourceRequestHandler(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              CefRefPtr<CefRequest> request,
                              bool is_navigation,
                              bool is_download,
                              const CefString& request_initiator,
                              bool& disable_default_handling) override;

    // CefResourceRequestHandler

    CefRefPtr<CefResponseFilter>
    GetResourceResponseFilter(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              CefRefPtr<CefRequest> request,
                              CefRefPtr<CefResponse> response) override
    {
        return this;
    }

    CefResourceRequestHandler::ReturnValue
    OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         CefRefPtr<CefRequest> request,
                         CefRefPtr<CefCallback> callback) override;

    // CefResponseFilter

    bool InitFilter() override
    {
        return true;
    }

    CefResponseFilter::FilterStatus
    Filter(void* data_in,
           size_t data_in_size,
           size_t& data_in_read,
           void* data_out,
           size_t data_out_size,
           size_t& data_out_written) override;

    // CefDialogHandler

    bool OnFileDialog(CefRefPtr<CefBrowser> browser,                      
                      FileDialogMode mode,                                
                      const CefString& title,                             
                      const CefString& defaultFilePath,                 
                      const std::vector<CefString>& acceptFilters,
                      CefRefPtr<CefFileDialogCallback> callback) override;
private:
    void runMainLoop();
    void dispatch(const tlv_t& packet);
    void realize(const msg_view_cfg_t* config);
    void navigate(const char* url);
    void runScript(const char* js);
    void injectScript(const char* js);
    void setSize(const msg_view_size_t* size);
    void setKeyboardFocus(bool keyboardFocus);

    bool        fRunMainLoop;
    IpcChannel* fIpc;
    ::Display*  fDisplay;
    ::Window    fContainer;
    std::string fUserAgent;
    std::string fIndexHtml;
    
    CefRefPtr<CefBrowser>            fBrowser;
    CefRefPtr<CefListValue>          fScripts;
    CefRefPtr<CefFileDialogCallback> fDialogCallback;

    // Include the CEF default reference counting implementation
    IMPLEMENT_REFCOUNTING(CefHelper);
};

class CefSubprocess : public CefApp, public CefClient,
                      public CefRenderProcessHandler, public CefV8Handler
{
public:
    CefSubprocess() {}
    virtual ~CefSubprocess() {}

    // CefApp
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override
    {
        return this;
    }

    // CefRenderProcessHandler
    void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                          CefRefPtr<CefV8Context> context) override;
    
    // CefV8Handler
    bool Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval, CefString& exception) override;

private:
    CefRefPtr<CefBrowser> fBrowser;

    IMPLEMENT_REFCOUNTING(CefSubprocess);
};

#endif // CEF_HELPER_HPP
