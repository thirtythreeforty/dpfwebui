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

#ifndef NETWORK_UI_HPP
#define NETWORK_UI_HPP

#include "WebUIBase.hpp"
#include "WebServer.hpp"

START_NAMESPACE_DISTRHO

class NetworkUI : public WebUIBase
{
public:
    NetworkUI(uint width = 0, uint height = 0);
    virtual ~NetworkUI();

    String getLocalUrl();
    String getPublicUrl();

protected:
    void uiIdle() override;

    void postMessage(const JsValueVector& args) override;

private:
    void initHandlers();
    int  findAvailablePort();

    int       fPort;
    WebServer fServer;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkUI)

};

END_NAMESPACE_DISTRHO

#endif  // NETWORK_UI_HPP
