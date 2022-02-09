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
#if DISTRHO_PLUGIN_WANT_STATE
    fMemoryIn.create();
    setState("_shm_plugin_to_ui", fMemoryIn.getDataFilename());
    fMemoryOut.create();
    setState("_shm_ui_to_plugin", fMemoryOut.getDataFilename());
#endif
}

#if DISTRHO_PLUGIN_WANT_STATE
void UIEx::writeSharedMemory(const char* metadata, const unsigned char* data, size_t size)
{
    char* p = reinterpret_cast<char*>(fMemoryOut.getDataPointer());

    std::strcpy(p, metadata);

    char temp[1024];
    std::strcpy(temp, p);

    d_stderr("writeSharedMemory() metadata=%s size=%d", metadata, size);
}

void UIEx::replaceWasmBinary(const unsigned char* data, size_t size)
{
    writeSharedMemory("_wasm_bin", data, size);
}
#endif // DISTRHO_PLUGIN_WANT_STATE
