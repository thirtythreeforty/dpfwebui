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

#ifndef NETWORK_WEB_UI_HPP
#define NETWORK_WEB_UI_HPP

#include "extra/UIEx.hpp"
#include "WebServer.hpp"

START_NAMESPACE_DISTRHO

class NetworkWebUI : public UIEx
{
public:
    NetworkWebUI(uint width = 0, uint height = 0, bool automaticallyScaleAndSetAsMinimumSize = false);
    virtual ~NetworkWebUI();

    // TODO

private:
    WebServer fServer;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkWebUI)

};

END_NAMESPACE_DISTRHO

#endif  // NETWORK_WEB_UI_HPP
