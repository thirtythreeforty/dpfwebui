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

#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include <list>
#include <unordered_map>

#include <limits.h>
#include <libwebsockets.h>

#include "distrho/extra/LeakDetector.hpp"
#include "distrho/extra/String.hpp"

START_NAMESPACE_DISTRHO

typedef struct lws* Client;

struct ClientContext
{
    typedef std::list<unsigned char*> WriteBuffer;
    WriteBuffer writeBuffer;
};

struct WebServerHandler
{
    virtual void handleWebServerConnect(Client) {};
    virtual void handleWebServerDisconnect(Client) {};
    virtual int  handleWebServerRead(Client client, const char* data) = 0;
};

class WebServer
{
public:
    WebServer();
    virtual ~WebServer();

    void init(int port, WebServerHandler* handler, const char* jsInjectTarget = nullptr,
                const char* jsInjectToken = nullptr);
    void injectScript(const String& script);
    void send(const char* data, Client client);
    void broadcast(const char* data, Client exclude = nullptr);
    void serve();

private:
    static int lwsCallback(struct lws* wsi, enum lws_callback_reasons reason,
                           void* user, void* in, size_t len);
    static const char* lwsReplaceFunc(void* data, int index);

    int injectScripts(lws_process_html_args* args);
    int handleRead(Client client, void* in, size_t len);
    int handleWrite(Client client);

    char                       fMountOrigin[PATH_MAX];
    lws_http_mount             fMount;
    lws_protocol_vhost_options fMountOptions;
    lws_protocols              fProtocol[2];
    lws_context_creation_info  fContextInfo;
    lws_context*               fContext;

    typedef std::unordered_map<Client, ClientContext> ClientContextMap;
    ClientContextMap fClients;

    typedef std::list<String> StringList;
    StringList fInjectedScripts;
    String     fInjectToken;

    WebServerHandler *fHandler;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebServer)

};

END_NAMESPACE_DISTRHO

#endif  // WEB_SERVER_HPP
