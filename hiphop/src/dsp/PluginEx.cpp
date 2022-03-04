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

// This is horrible but __COUNTER__ alone cannot solve the problem
#if defined(HIPHOP_NETWORK_UI)
# define COUNT_0 1
#else
# define COUNT_0 0
#endif
#if HIPHOP_PLUGIN_WANT_SHARED_MEMORY // DistrhoPluginInfo.h
# define COUNT_1 2
#else
# define COUNT_1 0
#endif

#define INTERNAL_STATE_COUNT (COUNT_0 + COUNT_1)

PluginEx::PluginEx(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount)
    : Plugin(parameterCount, programCount, stateCount + INTERNAL_STATE_COUNT)
    // stateCount is the last user state index
#if defined(HIPHOP_NETWORK_UI)
    , fStateIndexWsPort(stateCount + __COUNTER__)
    , fWebServerPort(0)
#endif
#if HIPHOP_PLUGIN_WANT_SHARED_MEMORY
    , fStateIndexShMemFiles(stateCount + __COUNTER__)
    , fStateIndexShMemData(stateCount + __COUNTER__)
#endif
{}

#if HIPHOP_PLUGIN_WANT_SHARED_MEMORY
size_t PluginEx::getSharedMemorySize() const noexcept
{
    return fMemory.out.getSizeBytes();
}
#endif

#if HIPHOP_PLUGIN_WANT_SHARED_MEMORY
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
#endif

#if DISTRHO_PLUGIN_WANT_STATE
void PluginEx::initState(uint32_t index, String& stateKey, String& defaultStateValue)
{
    (void)index;
    (void)stateKey;
    (void)defaultStateValue;
#if defined(HIPHOP_NETWORK_UI)
    if (index == fStateIndexWsPort) {
        stateKey = "_ws_port";
        defaultStateValue = "-1";
    }
#endif
#if HIPHOP_PLUGIN_WANT_SHARED_MEMORY
    if (index == fStateIndexShMemFiles) {
        // Plugin creates the shared memory and lets the UI know how to find it
        // by storing the filenames in internal state. UI->Plugin changes are
        // picked up asynchronously via the DPF state callback. Plugin->UI
        // changes are detected by polling the shared memory read state flag.
        stateKey = "_shmem_files";

        if (fMemory.create()) {
            defaultStateValue = String("p2ui:") + fMemory.out.getDataFilename()
                            + String(";ui2p:") + fMemory.in.getDataFilename();
        } else {
            defaultStateValue = "";
            d_stderr2("Could not create shared memory");
        }
    } else if (index == fStateIndexShMemData) {
        stateKey = "_shmem_data";
        defaultStateValue = "";
    }
#endif
}

void PluginEx::setState(const char* key, const char* value)
{
    (void)key;
    (void)value;
#if defined(HIPHOP_NETWORK_UI)
    if (std::strcmp(key, "_ws_port") == 0) {
        fWebServerPort = std::atoi(value);
    }
#endif
#if HIPHOP_PLUGIN_WANT_SHARED_MEMORY
    if ((std::strcmp(key, "_shmem_data") == 0) && ! fMemory.in.isRead()) {
        sharedMemoryChanged(fMemory.in.getMetadata(), fMemory.in.getDataPointer(),
                            fMemory.in.getDataSize());
        fMemory.in.setRead();
    }
#endif
}
#endif // DISTRHO_PLUGIN_WANT_STATE
