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

#include "extra/PluginEx.hpp"

#if HIPHOP_ENABLE_SHARED_MEMORY
# define DUMMY __COUNTER__
#endif

#define STATE_COUNT __COUNTER__

PluginEx::PluginEx(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount)
    : Plugin(parameterCount, programCount, stateCount + STATE_COUNT /*internal state*/)
{}

PluginEx::~PluginEx()
{
#if HIPHOP_ENABLE_SHARED_MEMORY
    fMemory.close();
#endif
}

#if HIPHOP_ENABLE_SHARED_MEMORY
size_t PluginEx::getSharedMemorySize() const noexcept
{
    return fMemory.out.getSizeBytes();
}

bool PluginEx::writeSharedMemory(const char* metadata, const unsigned char* data, size_t size)
{
    if (fMemory.out.write(metadata, data, size)) {
        // UI picks up data periodically
        return true;
    } else {
        d_stderr2("Could not write shared memory (plugin->ui)");
        return false;
    }
}

void PluginEx::setState(const char* key, const char* value)
{
    if (std::strcmp(key, "_shmem") == 0) {
        if (std::strstr(value, "init_p2ui:") == value) {
            if (fMemory.out.connect(value + 10) == nullptr) {
                d_stderr2("Could not connect to shared memory (plugin->ui)");
            }
        } else if (std::strstr(value, "init_ui2p:") == value) {
            if (fMemory.in.connect(value + 10) == nullptr) {
                d_stderr2("Could not connect to shared memory (ui->plugin)");
            }
        } else if (std::strstr(value, "data_ui2p") == value) {
            if (! fMemory.in.isRead()) { // check not needed, do it for correctness
                sharedMemoryChanged(fMemory.in.getMetadata(), fMemory.in.getDataPointer(),
                                    fMemory.in.getDataSize());
                fMemory.in.setRead();
            }
        } else if (std::strstr(value, "deinit") == value) {
            fMemory.close();
        }
    }
}
#endif // HIPHOP_ENABLE_SHARED_MEMORY
