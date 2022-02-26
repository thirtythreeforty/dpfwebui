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

#include "src/DistrhoDefines.h"
#include "distrho/extra/LeakDetector.hpp"

#include <libwebsockets.h>

START_NAMESPACE_DISTRHO

class WebServer
{
public:
    WebServer();
    virtual ~WebServer();

    // TODO

private:
    static int lwsCallback(struct lws* wsi, enum lws_callback_reasons reason,
                           void* user, void* in, size_t len);

    char                      fMountOrigin[PATH_MAX];
    lws_http_mount            fMount;
    lws_protocols             fProtocol[2];
    lws_context_creation_info fContextInfo;
    lws_context*              fContext;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebServer)

};

END_NAMESPACE_DISTRHO

#endif  // WEB_SERVER_HPP
