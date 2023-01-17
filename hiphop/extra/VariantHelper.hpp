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

#ifndef VARIANT_HELPER_HPP
#define VARIANT_HELPER_HPP

#include <stdexcept>
#include <vector>

#include "src/DistrhoDefines.h"

START_NAMESPACE_DISTRHO

typedef std::vector<uint8_t> BinaryData;

template<class T>
T sliceVariantArray(const T& a, int start, int end = -1)
{
    if (! a.isArray()) {
        throw std::runtime_error("sliceArray() requires an array argument");
    }

    T b = T::createArray();

    if (! a.isArray()) {
        return b;
    }

    if ((start < 0) || (start == end)) {
        return b;
    }

    const int size = a.getArraySize();

    if (start >= size) {
        return b;
    }

    if ((end < 0)/*def value*/ || (end > size)) {
        end = size;
    }

    for (int i = start; i < end; ++i) {
        b.pushArrayItem(a.getArrayItem(i));
    }

    return b;
}

template<class T>
T& joinVariantArrays(T& a, const T& b)
{
    if (! a.isArray() || ! b.isArray()) {
        throw std::runtime_error("joinArrays() requires two array arguments");
    }

    const int size = b.getArraySize();

    for (int i = 0; i < size; ++i) {
        a.pushArrayItem(b.getArrayItem(i));
    }

    return a;
}

END_NAMESPACE_DISTRHO

#endif // VARIANT_HELPER_HPP
