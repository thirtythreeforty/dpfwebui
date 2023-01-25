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

#include <cstdlib>

#include "extra/PluginEx.hpp"

// This is ugly but __COUNTER__ alone cannot solve the problem
#if defined(HIPHOP_NETWORK_UI)
# define COUNT_0 1
#else
# define COUNT_0 0
#endif
#if defined(HIPHOP_SHARED_MEMORY_SIZE) // DistrhoPluginInfo.h
# define COUNT_1 2
#else
# define COUNT_1 0
#endif
#if HIPHOP_UI_ZEROCONF // DistrhoPluginInfo.h
# define COUNT_2 3
#else
# define COUNT_2 0
#endif

#define INTERNAL_STATE_COUNT (COUNT_0 + COUNT_1 + COUNT_2)

#if HIPHOP_UI_ZEROCONF
#include <random>

std::string gen_uuid() {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<int> dist(0, 15);

    const char *v = "0123456789abcdef";
    const bool dash[] = { 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0 };

    std::string res;
    for (int i = 0; i < 16; i++) {
        if (dash[i]) res += "-";
        res += v[dist(rng)];
        res += v[dist(rng)];
    }

    return res;
}
#endif

PluginEx::PluginEx(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount)
    : Plugin(parameterCount, programCount, stateCount + INTERNAL_STATE_COUNT)
    // stateCount is the last user state index
#if defined(HIPHOP_NETWORK_UI)
    , fStateIndexWsPort(stateCount + __COUNTER__)
#endif
#if defined(HIPHOP_SHARED_MEMORY_SIZE)
    , fStateIndexShMemFile(stateCount + __COUNTER__)
    , fStateIndexShMemData(stateCount + __COUNTER__)
#endif
#if HIPHOP_UI_ZEROCONF
    , fStateIndexZeroconfPublish(stateCount + __COUNTER__)
    , fStateIndexZeroconfId(stateCount + __COUNTER__)
    , fStateIndexZeroconfName(stateCount + __COUNTER__)
#endif
{}

#if DISTRHO_PLUGIN_WANT_STATE
void PluginEx::initState(uint32_t index, State& state)
{
    (void)index;
    (void)state;
# if defined(HIPHOP_NETWORK_UI)
    if (index == fStateIndexWsPort) {
        state.key = "_ws_port";
        state.defaultValue = "-1";
    }
# endif
# if defined(HIPHOP_SHARED_MEMORY_SIZE)
    if (index == fStateIndexShMemFile) {
        state.key = "_shmem_file";
    } else if (index == fStateIndexShMemData) {
        state.key = "_shmem_data";
    }
# endif
# if HIPHOP_UI_ZEROCONF
    if (index == fStateIndexZeroconfPublish) {
        state.key = "_zc_publish";
        state.defaultValue = "true";
    } else if (index == fStateIndexZeroconfId) {
        state.key = "_zc_id";
        state.defaultValue = gen_uuid().c_str();
    } else if (index == fStateIndexZeroconfName) {
        state.key = "_zc_name";
        state.defaultValue = DISTRHO_PLUGIN_NAME;
    }
# endif
# if DISTRHO_PLUGIN_WANT_FULL_STATE
    fState[state.key] = state.defaultValue;
# endif
}

void PluginEx::setState(const char* key, const char* value)
{
    (void)key;
    (void)value;
# if defined(HIPHOP_SHARED_MEMORY_SIZE)
    // Do not persist _shmem_* values by returning before setting fState
    if ((std::strcmp("_shmem_file", key) == 0) && (value[0] != '\0')) {
        if (fMemory.isCreatedOrConnected()) {
            fMemory.close();
        }
        sharedMemoryPointerUpdated(fMemory.connect(value));
        return;
    } else if (std::strcmp(key, "_shmem_data") == 0) {
        char* v2;
        size_t size = std::strtol(value, &v2, 10);
        size_t offset = std::strtol(v2 + 1, nullptr, 10);
        sharedMemoryWritten(getSharedMemoryPointer(), size, offset);
        return;
    }
# endif
# if DISTRHO_PLUGIN_WANT_FULL_STATE
    fState[String(key)] = value;
# endif
}

# if DISTRHO_PLUGIN_WANT_FULL_STATE
String PluginEx::getState(const char* key) const
{
    StateMap::const_iterator it = fState.find(String(key));

    if (it == fState.end()) {
        return String();
    }
    
    return it->second;
}
# endif

#endif // DISTRHO_PLUGIN_WANT_STATE

#if defined(HIPHOP_SHARED_MEMORY_SIZE)
uint8_t* PluginEx::getSharedMemoryPointer() const noexcept
{
    return fMemory.getDataPointer();
}

bool PluginEx::writeSharedMemory(const uint8_t* data, size_t size, size_t offset) const noexcept
{
    uint8_t* ptr = fMemory.getDataPointer();

    if (ptr == nullptr) {
        return false;
    }

    std::memcpy(ptr + offset, data, size);

    return true;
}
#endif
