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

#ifndef STRING_HASH_HPP
#define STRING_HASH_HPP

#include <functional>

#include "extra/String.hpp"

namespace std
{
    template <>
    struct hash<String>
    {
        size_t operator()(String k) const
        {
            // http://www.cse.yorku.ca/~oz/hash.html
            const char* s = k.buffer();
            size_t h = 5381;
            int c;
            while ((c = *s++))
                h = ((h << 5) + h) + c;
            return h;
        }
    };
}

#endif  // STRING_HASH_HPP
