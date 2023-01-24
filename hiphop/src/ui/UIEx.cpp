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

#include <string.h>

#include "extra/UIEx.hpp"

UIEx::UIEx(uint width, uint height)
    : UI(width, height)
{}

#if defined(HIPHOP_SHARED_MEMORY_SIZE)
uint8_t* UIEx::getSharedMemory() const noexcept
{
    return fMemory.getDataPointer();
}

bool UIEx::writeSharedMemory(const uint8_t* data, size_t size, size_t offset) noexcept
{
    uint8_t* ptr = fMemory.getDataPointer();

    if (ptr == nullptr) {
        return false;
    }

    std::memcpy(ptr + offset, data, size);

    String metadata = String(size) + String(';') + String(offset);
    setState("_shmem_data", metadata.buffer()); // notify Plugin

    return true;
}

void UIEx::stateChanged(const char* key, const char* value)
{
    if ((std::strcmp(key, "_shmem_file") == 0) && ! fMemory.isCreatedOrConnected()) {
        fMemory.connect(value);
        sharedMemoryReady();
    }
}
#endif // HIPHOP_SHARED_MEMORY_SIZE
