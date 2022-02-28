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

#include <cstring>

#include "extra/Path.hpp"
#include "WebServer.hpp"

USE_NAMESPACE_DISTRHO

WebServer::WebServer()
{
    lws_set_log_level(LLL_ERR|LLL_WARN|LLL_DEBUG, 0);

    std::memset(fProtocol, 0, sizeof(fProtocol));
    fProtocol[0].name = "lws-dpf";
    fProtocol[0].callback = WebServer::lwsCallback;

    std::strcpy(fMountOrigin, Path::getPluginLibrary() + "/ui/");

    std::memset(&fMount, 0, sizeof(fMount));
    fMount.mountpoint       = "/";
    fMount.mountpoint_len   = std::strlen(fMount.mountpoint);
    fMount.origin           = fMountOrigin;
    fMount.origin_protocol  = LWSMPRO_FILE;
    fMount.def              = "index.html";
#ifndef NDEBUG
    // Caching headers
    fMount.cache_max_age    = 3600;
    fMount.cache_reusable   = 1;
    fMount.cache_revalidate = 1;
#endif

    std::memset(&fContextInfo, 0, sizeof(fContextInfo));
    fContextInfo.port      = 9090; // FIXME - pick available port from a range
    fContextInfo.protocols = fProtocol;
    fContextInfo.mounts    = &fMount;
    fContextInfo.uid       = -1;
    fContextInfo.gid       = -1;
    fContextInfo.user      = this;

    // SSL (WIP)
    // https://github.com/warmcat/libwebsockets/blob/main/READMEs/README.test-apps.md
    // cp -rp ./scripts/client-ca /tmp
    // cd /tmp/client-ca
    // ./create-ca.sh
    // ./create-server-cert.sh server
    // ./create-client-cert.sh client
    //fContextInfo.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    //fContextInfo.ssl_cert_filepath        = "/tmp/client-ca/server.pem";
    //fContextInfo.ssl_private_key_filepath = "/tmp/client-ca/server.key";
    //fContextInfo.ssl_ca_filepath          = "/tmp/client-ca/ca.pem";

    fContext = lws_create_context(&fContextInfo);
}

WebServer::~WebServer()
{
    if (fContext != nullptr) {
        lws_context_destroy(fContext);
        fContext = nullptr;
    }
}

void WebServer::process()
{
    // Avoid blocking - https://github.com/warmcat/libwebsockets/issues/1735
    lws_service(fContext, -1);
}

int WebServer::lwsCallback(struct lws* wsi, enum lws_callback_reasons reason,
                           void* user, void* in, size_t len)
{
    void* userdata = lws_context_user(lws_get_context(wsi));
    WebServer* server = static_cast<WebServer*>(userdata);
    int rc;

    switch (reason) {
        /*case LWS_CALLBACK_ESTABLISHED:
            rc = server->add_client(wsi);
            break;
        case LWS_CALLBACK_CLOSED:
            rc = server->del_client(wsi);
            break;
        case LWS_CALLBACK_RECEIVE:
            rc = server->recv_client(wsi, in, len);
            break;
        case LWS_CALLBACK_SERVER_WRITEABLE:
            rc = server->write_client(wsi);
            break;*/
        default:
            rc = lws_callback_http_dummy(wsi, reason, user, in, len);
            break;
    }

    return rc;
}
