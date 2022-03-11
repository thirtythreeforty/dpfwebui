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

#ifndef ZEROCONF_HPP
#define ZEROCONF_HPP

#if DISTRHO_OS_LINUX
# include <cstdio>
# include <signal.h>
# include <spawn.h>
#elif DISTRHO_OS_MAC
# include <dns_sd.h>
# include <arpa/inet.h>
#elif DISTRHO_OS_WINDOWS
    // TODO
#endif

#include "src/DistrhoDefines.h"

#define SERVICE_TYPE "_http._tcp"

#if DISTRHO_OS_LINUX
extern char **environ;
#endif

START_NAMESPACE_DISTRHO

class Zeroconf
{
public:
    Zeroconf()
        : fPublished(false)
#if DISTRHO_OS_LINUX
        , fPid(0)
#elif DISTRHO_OS_MAC
        , fService(nullptr)
#elif DISTRHO_OS_WINDOWS
    // TODO
#endif
    {}

    ~Zeroconf()
    {
        unpublish();
    }

    bool isPublished() const noexcept
    {
        return fPublished;
    }

    void publish(const char* name, int port) noexcept
    {
#if DISTRHO_OS_LINUX
        char sport[10];
        std::sprintf(sport, "%d", port);
        const char *argv[] = {"avahi-publish", "-s", name, SERVICE_TYPE, sport, nullptr};
        const int status = posix_spawnp(&fPid, "avahi-publish", nullptr/*file_actions*/,
            nullptr/*attrp*/, const_cast<char* const*>(argv), environ);
        if (status == 0) {
            fPublished = true;
        }
#elif DISTRHO_OS_MAC
        DNSServiceErrorType err = DNSServiceRegister(&fService, 0/*flags*/,
            kDNSServiceInterfaceIndexAny, name, SERVICE_TYPE, nullptr/*domain*/,
            nullptr/*host*/, htons(port), 0/*txtLen*/, nullptr/*txtRecord*/,
            nullptr/*callBack*/, nullptr/*context*/);
        if (err == kDNSServiceErr_NoError) {
            fPublished = true;
        }
#elif DISTRHO_OS_WINDOWS

        // TODO
        // if (Windows < 10) return
        // https://docs.microsoft.com/en-us/windows/win32/api/windns/nf-windns-dnsserviceregister
        // https://stackoverflow.com/questions/66474722/use-multicast-dns-when-network-cable-is-unplugged

#endif
        if (! fPublished) {
            d_stderr2("Zeroconf : failed publish()");
        }
    }

    void unpublish() noexcept
    {
#if DISTRHO_OS_LINUX
        if (fPid != 0) {
            kill(fPid, SIGTERM);
            fPid = 0;
        }
#elif DISTRHO_OS_MAC
        if (fService != nullptr) {
            DNSServiceRefDeallocate(fService);
            fService = nullptr;
        }
#elif DISTRHO_OS_WINDOWS

        // TODO
        // if (Windows < 10) return

#endif
    }

private:
    bool fPublished;
#if DISTRHO_OS_LINUX
    pid_t fPid;
#elif DISTRHO_OS_MAC
    DNSServiceRef fService;
#elif DISTRHO_OS_WINDOWS
    // TODO
#endif

};

END_NAMESPACE_DISTRHO

#endif // ZEROCONF_HPP
