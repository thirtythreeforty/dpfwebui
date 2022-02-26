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

#include "NetworkWebUI.hpp"

USE_NAMESPACE_DISTRHO

NetworkWebUI::NetworkWebUI(uint width, uint height, bool automaticallyScaleAndSetAsMinimumSize)
    : UIEx(width, height, automaticallyScaleAndSetAsMinimumSize)
{
    lws_set_log_level(LLL_ERR|LLL_WARN|LLL_DEBUG, 0);

    std::memset(fLwsProto, 0, sizeof(fLwsProto));
    fLwsProto[0].name = "lws-dpf";
    fLwsProto[0].callback = NetworkWebUI::lwsCallback;

    std::strcpy(fLwsMountOrigin, Path::getPluginLibrary() + "/ui/");

    std::memset(&fLwsMount, 0, sizeof(fLwsMount));
    fLwsMount.mountpoint       = "/";
    fLwsMount.mountpoint_len   = std::strlen(fLwsMount.mountpoint);
    fLwsMount.origin           = fLwsMountOrigin;
    fLwsMount.origin_protocol  = LWSMPRO_FILE;
    fLwsMount.def              = "index.html";
#ifndef NDEBUG
    // Caching headers
    fLwsMount.cache_max_age    = 3600;
    fLwsMount.cache_reusable   = 1;
    fLwsMount.cache_revalidate = 1;
#endif

    std::memset(&fLwsInfo, 0, sizeof(fLwsInfo));
    fLwsInfo.port      = 9090; // FIXME - pick available port from a range
    fLwsInfo.protocols = fLwsProto;
    fLwsInfo.mounts    = &fLwsMount;
    fLwsInfo.uid       = -1;
    fLwsInfo.gid       = -1;
    fLwsInfo.user      = this;

    fLwsContext = lws_create_context(&fLwsInfo);
}

NetworkWebUI::~NetworkWebUI()
{
    if (fLwsContext != nullptr) {
        lws_context_destroy(fLwsContext);
        fLwsContext = nullptr;
    }
}

int NetworkWebUI::lwsCallback(struct lws* wsi, enum lws_callback_reasons reason,
                              void* user, void* in, size_t len)
{
    // TODO

    return 0;
}
