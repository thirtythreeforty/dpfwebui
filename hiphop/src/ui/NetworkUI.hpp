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
#if HIPHOP_UI_PUBLISH_DNSSD
# include "Zeroconf.hpp"
#endif
#include "extra/JSValue.hpp"

START_NAMESPACE_DISTRHO

class NetworkUI : public WebUIBase, public WebServerHandler
{
public:
    NetworkUI(uint widthCssPx, uint heightCssPx, float initScaleFactorForVST3);
    virtual ~NetworkUI();

    String getLocalUrl();
    String getPublicUrl();

protected:
    void uiIdle() override;

    void postMessage(const JSValue& args, uintptr_t context) override;

#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* key, const char* value) override;
#endif

private:
    void initHandlers();
    void initServer();
    int  findAvailablePort();

    void handleWebServerConnect(Client client) override;
    int  handleWebServerRead(Client client, const char* data) override;

    int       fPort;
    WebServer fServer;
#if HIPHOP_UI_PUBLISH_DNSSD
    Zeroconf  fZeroconf;
#endif

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkUI)

};

END_NAMESPACE_DISTRHO

#endif  // NETWORK_UI_HPP
