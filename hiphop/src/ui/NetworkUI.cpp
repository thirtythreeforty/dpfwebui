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

#define FIRST_PORT        49152 // first in dynamic/private range
#define TRANSFER_PROTOCOL "http"
#define LOG_TAG           "NetworkUI"

USE_NAMESPACE_DISTRHO

NetworkUI::NetworkUI(uint width, uint height)
    : WebUIBase(width, height)
{
#if defined(DISTRHO_OS_WINDOWS)
    WSADATA wsaData;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc != 0) {
        d_stderr2(LOG_TAG " : failed WSAStartup()");
        return;
    }
#endif
    fPort = findAvailablePort();

    if (fPort == -1) {
        d_stderr2(LOG_TAG " : could not find available port");
        return;
    }

    fServer.init(fPort);
    initHandlers();
}

NetworkUI::~NetworkUI()
{
#if defined(DISTRHO_OS_WINDOWS)
    WSACleanup();
#endif
}

String NetworkUI::getLocalUrl()
{
    return String(TRANSFER_PROTOCOL "://127.0.0.1:") + String(fPort);
}

String NetworkUI::getPublicUrl()
{
    String url;

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
        d_stderr(LOG_TAG " : failed connect(), errno %d", errno);
    }

    if (CLOSE_SOCKET(sockfd) == -1) {
        d_stderr(LOG_TAG " : failed close(), errno %d", errno);
    }

    return url;
}

void NetworkUI::uiIdle()
{
    fServer.process();
}

void NetworkUI::postMessage(const JsValueVector& args)
{
    // TODO - broadcast to all clients

    (void)args;
}

void NetworkUI::initHandlers()
{
    fHandler["getPublicUrl"] = std::make_pair(0, [this](const JsValueVector&) {
        postMessage({"UI", "getPublicUrl", getPublicUrl()});
    });
}

int NetworkUI::findAvailablePort()
{
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

    return port;
}
