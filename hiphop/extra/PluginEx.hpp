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

#ifndef PLUGIN_EX_HPP
#define PLUGIN_EX_HPP

#include <map>

#include "DistrhoPlugin.hpp"

#if defined(HIPHOP_SHARED_MEMORY_SIZE)
# if ! DISTRHO_PLUGIN_WANT_STATE
#  error Shared memory support requires DISTRHO_PLUGIN_WANT_STATE
# endif
# include "extra/SharedMemory.hpp"
#endif 

START_NAMESPACE_DISTRHO

// This class adds some goodies to DISTRHO::Plugin like shared memory support

class PluginEx : public Plugin
{
public:
    PluginEx(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount);
    virtual ~PluginEx() {}

#if DISTRHO_PLUGIN_WANT_STATE
    void   initState(uint32_t index, State& state) override;
    void   setState(const char* key, const char* value) override;
# if DISTRHO_PLUGIN_WANT_FULL_STATE
    String getState(const char* key) const override;
# endif
#endif

#if defined(HIPHOP_SHARED_MEMORY_SIZE)
    uint8_t* getSharedMemoryPointer() const noexcept;
    bool     writeSharedMemory(const uint8_t* data, size_t size, size_t offset = 0) const noexcept;
#endif

protected:
#if defined(HIPHOP_SHARED_MEMORY_SIZE)
    virtual void sharedMemoryDisconnected() {}
    virtual void sharedMemoryConnected(uint8_t* ptr)
    {
        (void)ptr;
    }

    virtual void sharedMemoryWritten(uint8_t* data, size_t size, size_t offset)
    {
        (void)data;
        (void)size;
        (void)offset;
    }
#endif

private:
#if defined(HIPHOP_NETWORK_UI)
    uint32_t fStateIndexWsPort;
#endif
#if defined(HIPHOP_SHARED_MEMORY_SIZE)
    uint32_t fStateIndexShMemFile;
    uint32_t fStateIndexShMemData;
    SharedMemory<uint8_t,HIPHOP_SHARED_MEMORY_SIZE> fMemory;
#endif
#if HIPHOP_UI_ZEROCONF
    uint32_t fStateIndexZeroconfPublish;
    uint32_t fStateIndexZeroconfId;
    uint32_t fStateIndexZeroconfName;
#endif
#if DISTRHO_PLUGIN_WANT_FULL_STATE
    typedef std::map<String,String> StateMap;
    StateMap fState;
#endif
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEx)

};

END_NAMESPACE_DISTRHO

#endif  // PLUGIN_EX_HPP
