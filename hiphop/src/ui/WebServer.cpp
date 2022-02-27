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
    lws_service(fContext, 0);
}

int WebServer::lwsCallback(struct lws* wsi, enum lws_callback_reasons reason,
                           void* user, void* in, size_t len)
{
    // TODO

    (void)wsi;
    (void)reason;
    (void)user;
    (void)in;
    (void)len;

    return 0;
}
