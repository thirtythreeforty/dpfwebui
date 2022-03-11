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

#include "distrho/extra/String.hpp"

START_NAMESPACE_DISTRHO

class Zeroconf
{
public:
    Zeroconf()
        : fPublished(false)
    {}

    ~Zeroconf()
    {
        unpublish();
    }

    bool isPublished() const noexcept
    {
        return fPublished;
    }

    void publish(String& url) noexcept // TODO - args?
    {
        (void)url;
#if DISTRHO_OS_LINUX

        // TODO
        // https://linux.die.net/man/1/avahi-publish

#elif DISTRHO_OS_MAC

        // TODO
        // https://developer.apple.com/library/archive/samplecode/DNSSDObjects/Introduction/Intro.html

#elif DISTRHO_OS_WINDOWS

        // TODO

        // if (Windows < 10) return

        // https://docs.microsoft.com/en-us/windows/win32/api/windns/nf-windns-dnsserviceregister
        // https://stackoverflow.com/questions/66474722/use-multicast-dns-when-network-cable-is-unplugged

#endif
    }

    void unpublish() noexcept
    {
        if (! fPublished) {
            return;
        }

#if DISTRHO_OS_LINUX

        // TODO

#elif DISTRHO_OS_MAC

        // TODO

#elif DISTRHO_OS_WINDOWS

        // TODO
        // if (Windows < 10) return

#endif
    }

private:
    bool fPublished;

};

END_NAMESPACE_DISTRHO

#endif // ZEROCONF_HPP
