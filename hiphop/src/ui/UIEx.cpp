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

#include <cstring>

#include "extra/UIEx.hpp"

UIEx::UIEx(uint width, uint height)
    : UI(width, height)
{}

UIEx::~UIEx()
{
#if defined(HIPHOP_SHARED_MEMORY_SIZE)
    setState("_shmem_file", "");
#endif
}

#if defined(HIPHOP_SHARED_MEMORY_SIZE)
uint8_t* UIEx::getSharedMemoryPointer() const noexcept
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
    setState("_shmem_data", metadata.buffer());

    return true;
}

void UIEx::uiIdle()
{
    // setState() fails for VST3 when called from constructor
    if (! fMemory.isCreatedOrConnected() && fMemory.create()) {
        setState("_shmem_file", fMemory.getDataFilename());
        sharedMemoryPointerUpdated(fMemory.getDataPointer());
    }
}
#endif // HIPHOP_SHARED_MEMORY_SIZE
