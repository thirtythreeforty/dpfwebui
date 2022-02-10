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

#include "UIEx.hpp"

UIEx::UIEx(uint width, uint height, bool automaticallyScaleAndSetAsMinimumSize)
    : UI(width, height, automaticallyScaleAndSetAsMinimumSize)
{
#if DISTRHO_PLUGIN_WANT_STATE && HIPHOP_ENABLE_SHARED_MEMORY
    // UI can be created and destroyed multiple times during Plugin lifecycle.
    // Letting the UI manage shared memory ensures this resource will exist only
    // when it is necessary. Create it here and let the Plugin instance know
    // via DPF state interface. UI--state-->Host--state-->Plugin. 
    fMemoryIn.create();
    setState("_shmem_init_p2ui", fMemoryIn.getDataFilename());
    fMemoryOut.create();
    setState("_shmem_init_ui2p", fMemoryOut.getDataFilename());
#endif // DISTRHO_PLUGIN_WANT_STATE && HIPHOP_ENABLE_SHARED_MEMORY
}

#if DISTRHO_PLUGIN_WANT_STATE && HIPHOP_ENABLE_SHARED_MEMORY
void UIEx::writeSharedMemory(const char* metadata, const unsigned char* data, size_t size)
{
    char* p = reinterpret_cast<char*>(fMemoryOut.getDataPointer());

    std::strcpy(p, metadata);

    char temp[1024];
    std::strcpy(temp, p);

    d_stderr("writeSharedMemory() metadata=%s size=%d", metadata, size);

    // Notify Plugin instance there is new data available for reading
    setState("_shmem_data_ui2p", metadata);
}

#if HIPHOP_ENABLE_WASM_PLUGIN
void UIEx::replaceWasmBinary(const unsigned char* data, size_t size)
{
    // Send binary to the Plugin instance. This could be also achieved using the
    // state interface by first encoding data into something like Base64.
    writeSharedMemory("_wasm_bin", data, size);
}
#endif // HIPHOP_ENABLE_WASM_PLUGIN
#endif // DISTRHO_PLUGIN_WANT_STATE && HIPHOP_ENABLE_SHARED_MEMORY
