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
bool UIEx::writeSharedMemory(const uint8_t* data, size_t size, size_t offset,
                             uint32_t hints)
{
    if (fMemory.isCreatedOrConnected()
            && fMemory.write(kShMemWriteOriginUI, data, size, offset, hints)) {
        // Notify Plugin instance there is new data available for reading
        setState("_shmem_data", ""/*arbitrary non-null*/);
        return true;
    }

    return false;
}

# if defined(HIPHOP_SUPPORT_WASM)
void UIEx::sideloadWasmBinary(const uint8_t* data, size_t size)
{
    // Send binary to the Plugin instance
    writeSharedMemory(data, size, 0, kShMemHintWasmBinary);
}
# endif
void UIEx::uiIdle()
{
    constexpr int origin = kShMemWriteOriginPlugin;

    if (fMemory.isCreatedOrConnected() && ! fMemory.isRead(origin)) {
        sharedMemoryChanged(fMemory.getDataPointer() + fMemory.getDataOffset(origin),
                            fMemory.getDataSize(origin), fMemory.getHints(origin));
        fMemory.setRead(origin);
    }
}

void UIEx::stateChanged(const char* key, const char* value)
{
    if ((std::strcmp(key, "_shmem_file") == 0) && ! fMemory.isCreatedOrConnected()) {
        fMemory.connect(value);
    }
}
#endif // HIPHOP_SHARED_MEMORY_SIZE
