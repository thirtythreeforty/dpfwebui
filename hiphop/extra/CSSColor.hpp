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

#ifndef CSS_COLOR_HPP
#define CSS_COLOR_HPP

#include <cstdlib>
#include <cstring>

#include "src/DistrhoDefines.h"

START_NAMESPACE_DISTRHO

struct CSSColor
{
    // Modified version of DGL Color::fromHTML() that adds alpha support.
    // Original source in dpf/dgl/Color.hpp 
    static uint32_t fromHex(const char* rgba) noexcept
    {
        uint32_t fallback = 0xff; // solid black
        DISTRHO_SAFE_ASSERT_RETURN(rgba != nullptr && rgba[0] != '\0', fallback);

        if (rgba[0] == '#')
            ++rgba;
        DISTRHO_SAFE_ASSERT_RETURN(rgba[0] != '\0', fallback);

        std::size_t rgblen = std::strlen(rgba);
        DISTRHO_SAFE_ASSERT_RETURN(rgblen == 3 || rgblen == 6 || rgblen == 8, fallback);

        char rgbtmp[5] = { '0', 'x', '\0', '\0', '\0' };
        uint32_t r, g, b, a;

        if (rgblen == 3)
        {
            rgbtmp[2] = rgba[0];
            r = static_cast<uint32_t>(std::strtol(rgbtmp, nullptr, 16)) * 17;

            rgbtmp[2] = rgba[1];
            g = static_cast<uint32_t>(std::strtol(rgbtmp, nullptr, 16)) * 17;

            rgbtmp[2] = rgba[2];
            b = static_cast<uint32_t>(std::strtol(rgbtmp, nullptr, 16)) * 17;

            a = 0xff;
        }
        else
        {
            rgbtmp[2] = rgba[0];
            rgbtmp[3] = rgba[1];
            r = static_cast<uint32_t>(std::strtol(rgbtmp, nullptr, 16));

            rgbtmp[2] = rgba[2];
            rgbtmp[3] = rgba[3];
            g = static_cast<uint32_t>(std::strtol(rgbtmp, nullptr, 16));

            rgbtmp[2] = rgba[4];
            rgbtmp[3] = rgba[5];
            b = static_cast<uint32_t>(std::strtol(rgbtmp, nullptr, 16));

            if (rgblen == 8) {
                rgbtmp[2] = rgba[6];
                rgbtmp[3] = rgba[7];
                a = static_cast<uint32_t>(std::strtol(rgbtmp, nullptr, 16));
            } else {
                a = 0xff;
            }
        }

        return (r << 24) | (g << 16) | (b << 8) | a;
    }

};

END_NAMESPACE_DISTRHO

#endif // CSS_COLOR_HPP
