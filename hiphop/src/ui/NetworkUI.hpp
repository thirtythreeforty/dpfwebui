/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2023 Luciano Iam <oss@lucianoiam.com>
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

#include <string>
#include <unordered_map>

#include "distrho/extra/Thread.hpp"

#include "WebUIBase.hpp"
#include "WebServer.hpp"
#if HIPHOP_UI_ZEROCONF
# include "Zeroconf.hpp"
#endif
#include "extra/Variant.hpp"

START_NAMESPACE_DISTRHO

class WebServerThread;

class NetworkUI : public WebUIBase, public WebServerHandler
{
public:
    NetworkUI(uint widthCssPx, uint heightCssPx, float initPixelRatio);
    virtual ~NetworkUI();

    String getLocalUrl();
    String getPublicUrl();

    void setState(const char* key, const char* value);

protected:
    void broadcastMessage(const Variant& args, Client exclude = nullptr);
    void postMessage(const Variant& args, uintptr_t destination) override;

    void parameterChanged(uint32_t index, float value) override;
#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* key, const char* value) override;
#endif

    virtual void onClientConnected(Client client);

private:
    void initHandlers();
    void initServer();
    int  findAvailablePort();
#if HIPHOP_UI_ZEROCONF
    void zeroconfStateUpdated();
#endif

    void handleWebServerConnect(Client client) override;
    int  handleWebServerRead(Client client, const ByteVector& data) override;
    int  handleWebServerRead(Client client, const char* data) override;

    int              fPort;
    WebServer        fServer;
    WebServerThread* fThread;
#if HIPHOP_UI_ZEROCONF
    Zeroconf  fZeroconf;
    bool      fZeroconfPublish;
    String    fZeroconfId;
    String    fZeroconfName;
#endif
    typedef std::unordered_map<uint32_t, float> ParameterMap;
    ParameterMap fParameters;
    bool         fParameterLock;
    typedef std::unordered_map<std::string, std::string> StateMap;
    StateMap     fStates;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkUI)

};

class WebServerThread : public Thread
{
public:
    WebServerThread(WebServer* server) noexcept;
    virtual ~WebServerThread() noexcept;

    void run() noexcept override;

private:
    WebServer* fServer;
    bool       fRun;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebServerThread)
};

END_NAMESPACE_DISTRHO

#endif  // NETWORK_UI_HPP
