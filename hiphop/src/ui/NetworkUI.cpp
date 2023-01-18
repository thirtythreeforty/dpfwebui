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

#include <cerrno>
#include <cstring>
#include <utility>
#include <unistd.h>

#include "src/DistrhoDefines.h"

#if defined(DISTRHO_OS_WINDOWS)
# include <Winsock2.h>
# define IS_EADDRINUSE() (WSAGetLastError() == WSAEADDRINUSE) // errno==0
# define CLOSE_SOCKET(s) closesocket(s)
#else
# include <arpa/inet.h>
# include <sys/socket.h>
# define IS_EADDRINUSE() (errno == EADDRINUSE)
# define CLOSE_SOCKET(s) ::close(s)
#endif

#include "NetworkUI.hpp"

#define LOG_TAG "NetworkUI"
#if defined(HIPHOP_NETWORK_SSL)
# define TRANSFER_PROTOCOL "https"
#else
# define TRANSFER_PROTOCOL "http"
#endif
#define FIRST_PORT 49152 // first in dynamic/private range

USE_NAMESPACE_DISTRHO

NetworkUI::NetworkUI(uint widthCssPx, uint heightCssPx, float initPixelRatio)
    : WebUIBase(widthCssPx, heightCssPx, initPixelRatio)
    , fPort(-1)
    , fThread(nullptr)
#if HIPHOP_UI_ZEROCONF
    , fZeroconfPublish(false)
#endif
    , fParameterLock(false)
{
    if (isDryRun()) {
        return;
    }

#if defined(DISTRHO_OS_WINDOWS)
    WSADATA wsaData;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc != 0) {
        d_stderr2(LOG_TAG " : failed WSAStartup()");
        return;
    }
#endif

    setBuiltInMethodHandlers();

    if ((! DISTRHO_PLUGIN_WANT_STATE) || isStandalone()) {
        // Port is not remembered when state support is disabled
        fPort = findAvailablePort();
        if (fPort != -1) {
            initServer();
        }
    }
}

NetworkUI::~NetworkUI()
{
    if (fThread != nullptr) {
        delete fThread;
        fThread = nullptr;
    }
#if defined(DISTRHO_OS_WINDOWS)
    //WSACleanup();
#endif
}

String NetworkUI::getLocalUrl()
{
    return String(TRANSFER_PROTOCOL "://127.0.0.1:") + String(fPort);
}

String NetworkUI::getPublicUrl()
{
    String url = getLocalUrl();

    const int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        d_stderr(LOG_TAG " : failed socket(), errno %d", errno);
        return url;
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("8.8.8.8"); // Google DNS
    addr.sin_port = htons(53);

    if (connect(sockfd, (const sockaddr*)&addr, sizeof(addr)) == 0) {
        socklen_t addrlen = sizeof(addr);

        if (getsockname(sockfd, (sockaddr*)&addr, &addrlen) == 0) {
            const char* ip = inet_ntoa(addr.sin_addr);
            url = String(TRANSFER_PROTOCOL "://") + ip + ":" + String(fPort);
        } else {
            d_stderr(LOG_TAG " : failed getsockname(), errno %d", errno);
        }
    } else {
        //d_stderr(LOG_TAG " : failed connect(), errno %d", errno);
    }

    if (CLOSE_SOCKET(sockfd) == -1) {
        d_stderr(LOG_TAG " : failed close(), errno %d", errno);
    }

    return url;
}

void NetworkUI::setState(const char* key, const char* value)
{
    // Warning : UI::setState() is non-virtual !
    WebUIBase::setState(key, value);
    fStates[key] = value;
}

void NetworkUI::broadcastMessage(const Variant& args, Client exclude)
{
#if defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
    BinaryData data = args.toBSON();
    fServer.broadcast(data.data(), data.size(), exclude);
#elif defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
    fServer.broadcast(args.toJSON(), exclude);
#endif
}

void NetworkUI::postMessage(const Variant& args, uintptr_t destination)
{
#if defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
    BinaryData data = args.toBSON();
    if (destination == DESTINATION_ALL) {
        fServer.broadcast(data.data(), data.size());
    } else {
        fServer.send(data.data(), data.size(), reinterpret_cast<Client>(destination));
    }
#elif defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
    if (destination == DESTINATION_ALL) {
        fServer.broadcast(args.toJSON());
    } else {
        fServer.send(args.toJSON(), reinterpret_cast<Client>(destination));
    }
#endif
}

void NetworkUI::parameterChanged(uint32_t index, float value)
{
    fParameters[index] = value;

    if (fParameterLock) {
        fParameterLock = false;
    } else {
        WebUIBase::parameterChanged(index, value);
    }
}

#if DISTRHO_PLUGIN_WANT_STATE
void NetworkUI::stateChanged(const char* key, const char* value)
{
    if (isDryRun()) {
        return;
    }

    if (std::strcmp(key, "_ws_port") == 0) {
        fPort = std::atoi(value);
        if (fPort == -1) {
            fPort = findAvailablePort();
            setState("_ws_port", std::to_string(fPort).c_str());
        } else {
            //d_stderr(LOG_TAG " : reusing port %d", fPort);
        }
        if (fPort != -1) {
            initServer();
        }
        return;
    }

# if HIPHOP_UI_ZEROCONF
    if (std::strcmp(key, "_zc_publish") == 0) {
        fZeroconfPublish = std::strcmp(value, "true") == 0;
        zeroconfStateUpdated();
        return;
    } else if (std::strcmp(key, "_zc_id") == 0) {
        fZeroconfId = value;
        zeroconfStateUpdated();
        return;
    } else if (std::strcmp(key, "_zc_name") == 0) {
        fZeroconfName = value;
        zeroconfStateUpdated();
        return;
    }
# endif

    fStates[key] = value;

    WebUIBase::stateChanged(key, value);
}
#endif

void NetworkUI::onClientConnected(Client client)
{
    (void)client;
}

void NetworkUI::setBuiltInMethodHandlers()
{
    // Broadcast parameter updates to all clients except the originating one
    const MethodHandler& parameterHandlerSuper = getMethodHandler("setParameterValue");
    setMethodHandler("setParameterValue", 2, [this, parameterHandlerSuper](const Variant& args, uintptr_t origin) {
        queue([this, parameterHandlerSuper, args, origin] {
            const uint32_t index = static_cast<uint32_t>(args[0].getNumber());
            const float value = static_cast<float>(args[1].getNumber());
            fParameters[index] = value;
            fParameterLock = true; // avoid echo
            parameterHandlerSuper(args, origin);
        });

        notifyAll(reinterpret_cast<Client>(origin), "parameterChanged", args);
    });

#if DISTRHO_PLUGIN_WANT_STATE
    // Broadcast state updates to all clients except the originating one
    const MethodHandler& stateHandlerSuper = getMethodHandler("setState");
    setMethodHandler("setState", 2, [this, stateHandlerSuper](const Variant& args, uintptr_t origin) {
        queue([this, stateHandlerSuper, args, origin] {
            const String key = args[0].getString();
            const String value = args[1].getString();
            fStates[key.buffer()] = value.buffer();
            stateHandlerSuper(args, origin);
        });

        notifyAll(reinterpret_cast<Client>(origin), "stateChanged", args);
    });
#endif

    // Custom method for exchanging UI-only messages between clients
    setMethodHandler("broadcast", 1, [this](const Variant& args, uintptr_t origin) {
        notifyAll(reinterpret_cast<Client>(origin), "messageReceived", args);
    });

#if HIPHOP_UI_ZEROCONF
    setMethodHandler("isZeroconfPublished", 0, [this](const Variant&, uintptr_t origin) {
        notify(origin, "isZeroconfPublished", { fZeroconf.isPublished() });
    });

    setMethodHandler("setZeroconfPublished", 1, [this](const Variant& args, uintptr_t /*origin*/) {
        fZeroconfPublish = args[0].getBoolean();
        setState("_zc_published", fZeroconfPublish ? "true" : "false");
        zeroconfStateUpdated();
    });

    setMethodHandler("getZeroconfId", 0, [this](const Variant&, uintptr_t origin) {
        notify(origin, "getZeroconfId", { fZeroconfId });
    });

    setMethodHandler("getZeroconfName", 0, [this](const Variant&, uintptr_t origin) {
        notify(origin, "getZeroconfName", { fZeroconfName });
    });

    setMethodHandler("setZeroconfName", 1, [this](const Variant& args, uintptr_t /*origin*/) {
        fZeroconfName = args[0].getString();
        setState("_zc_name", fZeroconfName);
        zeroconfStateUpdated();
    });
#else
    setMethodHandler("isZeroconfPublished", 0, [this](const Variant&, uintptr_t origin) {
        notify(origin, "isZeroconfPublished", { false });
    });

    setMethodHandler("getZeroconfName", 0, [this](const Variant&, uintptr_t origin) {
        notify(origin, "getZeroconfName", { "" });
    });
#endif

    setMethodHandler("getPublicUrl", 0, [this](const Variant&, uintptr_t origin) {
        notify(origin, "getPublicUrl", { getPublicUrl() });
    });

    setMethodHandler("ping", 0, [this](const Variant&, uintptr_t origin) {
        notify(origin, "pong");
    });
}

void NetworkUI::initServer()
{
    fServer.init(fPort, this);
    fThread = new WebServerThread(&fServer);
    d_stderr(LOG_TAG " : server up @ %s", getPublicUrl().buffer());
}

int NetworkUI::findAvailablePort()
{
    // Ports are not reused during process lifetime unless SO_REUSEADDR is set.
    // Once a plugin binds to a port it is safe to assume that other plugin
    // instances will not claim the same port. A single plugin can reuse a port
    // by enabling DISTRHO_PLUGIN_WANT_STATE so it can be stored in a DPF state.

    const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        d_stderr(LOG_TAG " : failed socket(), errno %d", errno);
        return -1;
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    int port = -1, i = FIRST_PORT;

    while ((port == -1) && (i < 65535)) {
        addr.sin_port = htons(i);

        if (bind(sockfd, (const sockaddr*)&addr, sizeof(addr)) == 0) {
            port = i;
        } else {
            if (IS_EADDRINUSE()) {
                i++;
            } else {
                d_stderr(LOG_TAG " : failed bind(), errno %d", errno);
                break;
            }
        }
    }

    if (CLOSE_SOCKET(sockfd) == -1) {
        d_stderr(LOG_TAG " : failed close(), errno %d", errno);
    }

    if (port == -1) {
        d_stderr2(LOG_TAG " : could not find available port");
    } else {
        //d_stderr(LOG_TAG " : found available port %d", port);
    }

    return port;
}

#if HIPHOP_UI_ZEROCONF
void NetworkUI::zeroconfStateUpdated()
{
    if (fZeroconfPublish && ! fZeroconfId.isEmpty() && ! fZeroconfName.isEmpty()) {
        fZeroconf.publish(fZeroconfName, "_http._tcp", fPort, {
            { "dpfuri", DISTRHO_PLUGIN_URI },
            { "instanceid", fZeroconfId } // ID is useless unless plugin state is made persistent
        });
    } else {
        fZeroconf.unpublish();
    }
}
#endif

void NetworkUI::handleWebServerConnect(Client client)
{
    queue([this, client] {
        // Send all current parameters and states
        for (ParameterMap::const_iterator it = fParameters.cbegin(); it != fParameters.cend(); ++it) {
            const Variant args = { it->first, it->second };
            notify(reinterpret_cast<uintptr_t>(client), "parameterChanged", args);
        }

        for (StateMap::const_iterator it = fStates.cbegin(); it != fStates.cend(); ++it) {
            const Variant args = { it->first.c_str(), it->second.c_str() };
            notify(reinterpret_cast<uintptr_t>(client), "stateChanged", args);
        }

        onClientConnected(client);
    });
}

int NetworkUI::handleWebServerRead(Client client, const ByteVector& data)
{
#if defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
    handleMessage(Variant::fromBSON(data, /*asArray*/true), reinterpret_cast<uintptr_t>(client));
#else
    (void)client;
    (void)data;
#endif
    return 0;
}

int NetworkUI::handleWebServerRead(Client client, const char* data)
{
#if defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
    handleMessage(Variant::fromJSON(data), reinterpret_cast<uintptr_t>(client));
#else
    (void)client;
    (void)data;
#endif
    return 0;
}

#if defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
String NetworkUI::getMethodSignature(const Variant& args)
{
    return WebUIBase::getMethodSignature(args);
    //return String(args[1].getNumber());
}

void NetworkUI::setMethodSignature(Variant& args, String method)
{
    WebUIBase::setMethodSignature(args, method);
    //args.insertArrayItem(1, djb2hash(method));
}
#endif

void NetworkUI::notifyAll(Client exclude, const char* method, Variant args)
{
    args.insertArrayItem(0, "UI");
    setMethodSignature(args, String(method));
    broadcastMessage(args, exclude);
}

uint32_t NetworkUI::djb2hash(const char* str)
{
    uint32_t hash = 5381;
    uint32_t c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

WebServerThread::WebServerThread(WebServer* server) noexcept
    : fServer(server)
    , fRun(true)
{
    startThread();
}

WebServerThread::~WebServerThread() noexcept
{
    fRun = false;
    fServer->cancel();
    stopThread(-1 /*wait forever*/);
}

void WebServerThread::run() noexcept
{
    while (fRun) {
#if defined(DISTRHO_OS_WINDOWS)
        // Telling serve() to block creates lags during new connections setup
        fServer->serve(false);
        Sleep(1);
#else
        fServer->serve(true);
#endif
    }
}
