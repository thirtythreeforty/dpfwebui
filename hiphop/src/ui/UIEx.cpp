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

UIEx::UIEx(uint width, uint height, bool automaticallyScaleAndSetAsMinimumSize)
    : UI(width, height, automaticallyScaleAndSetAsMinimumSize)
{}

#if HIPHOP_PLUGIN_WANT_SHARED_MEMORY
size_t UIEx::getSharedMemorySize() const noexcept
{
    return fMemory.out.getSizeBytes();
}

bool UIEx::writeSharedMemory(const char* metadata, const unsigned char* data, size_t size)
{
    if (fMemory.out.write(metadata, data, size)) {
        // Notify Plugin instance there is new data available for reading
        setState("_shmem_data", "");
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
    
    writeSharedMemory("_wasm_bin", data, size);
}
#endif
#endif // HIPHOP_PLUGIN_WANT_SHARED_MEMORY

#if HIPHOP_PLUGIN_WANT_SHARED_MEMORY
void UIEx::uiIdle()
{
    // ExternalWindow does not implement the IdleCallback methods. If uiIdle()
    // is not fast enough for visualizations a custom timer solution needs to be
    // implemented, or DPF modified so the uiIdle() frequency can be configured.

    if (fMemory.in.isCreatedOrConnected() && ! fMemory.in.isRead()) {
        sharedMemoryChanged(fMemory.in.getMetadata(), fMemory.in.getDataPointer(),
                            fMemory.in.getDataSize());
        fMemory.in.setRead();
    }
}
#endif

#if DISTRHO_PLUGIN_WANT_STATE
void UIEx::stateChanged(const char* key, const char* value)
{
    (void)key;
    (void)value;
#if HIPHOP_PLUGIN_WANT_SHARED_MEMORY
    if (std::strcmp(key, "_shmem_files") == 0) {
        String val = String(value);

        if (val.length() > 0) {
            size_t sep = val.find(';');
            String ui2p = String(val.buffer() + sep + 6);
            if (fMemory.out.connect(ui2p) == nullptr) {
                d_stderr2("Could not connect to shared memory (plugin->ui)");
            }

            val.truncate(sep);
            String p2ui = String(val.buffer() + 5);
            if (fMemory.in.connect(p2ui) == nullptr) {
                d_stderr2("Could not connect to shared memory (ui->plugin)");
            }
        }
    }
#endif
}
#endif
