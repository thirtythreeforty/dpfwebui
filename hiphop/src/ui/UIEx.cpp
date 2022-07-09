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

#include <string.h>

#include "extra/UIEx.hpp"

UIEx::UIEx(uint width, uint height)
    : UI(width, height)
{}

#if HIPHOP_SHARED_MEMORY_SIZE
bool UIEx::writeSharedMemory(const unsigned char* data, size_t size, size_t offset,
                             uint32_t hints)
{
    if (fMemory.write(kSharedMemoryWriteOriginUI, data, size, offset, hints)) {
        // Notify Plugin instance there is new data available for reading
        setState("_shmem_data", ""/*arbitrary non-null*/);
        return true;
    } else {
        d_stderr2("Could not write shared memory (ui->plugin)");
        return false;
    }
}

#if defined(HIPHOP_WASM_SUPPORT)
void UIEx::sideloadWasmBinary(const unsigned char* data, size_t size)
{
    // Send binary to the Plugin instance. This could be also achieved using the
    // state interface by first encoding data into something like Base64.
    
    writeSharedMemory(data, size, 0, kShMemHintInternal | kShMemHintWasmBinary);
}
#endif
#endif // HIPHOP_SHARED_MEMORY_SIZE

#if HIPHOP_SHARED_MEMORY_SIZE
void UIEx::uiIdle()
{
    // ExternalWindow does not implement the IdleCallback methods. If uiIdle()
    // is not fast enough for visualizations a custom timer solution needs to be
    // implemented, or DPF modified so the uiIdle() frequency can be configured.

    constexpr int origin = kSharedMemoryWriteOriginPlugin;

    if (fMemory.isCreatedOrConnected() && ! fMemory.isRead(origin)) {
        sharedMemoryChanged(fMemory.getDataPointer() + fMemory.getDataOffset(origin),
                            fMemory.getDataSize(origin), fMemory.getHints(origin));
        fMemory.setRead(origin);
    }
}
#endif

#if DISTRHO_PLUGIN_WANT_STATE
void UIEx::stateChanged(const char* key, const char* value)
{
    (void)key;
    (void)value;
#if HIPHOP_SHARED_MEMORY_SIZE
    if (std::strcmp(key, "_shmem_file") == 0) {
        if (fMemory.connect(value) != nullptr) {
            sharedMemoryReady();
        } else {
            d_stderr2("Could not connect to shared memory");
        }
    }
#endif
}
#endif
