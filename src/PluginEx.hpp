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

#include "DistrhoPlugin.hpp"
#include "util/SharedMemoryEx.hpp"

#if HIPHOP_ENABLE_SHARED_MEMORY
# if ! DISTRHO_PLUGIN_WANT_STATE
#  error Shared memory support requires DISTRHO_PLUGIN_WANT_STATE
# endif
#endif 

START_NAMESPACE_DISTRHO

// This class adds some goodies to DISTRHO::Plugin like shared memory support

class PluginEx : public Plugin
{
public:
    PluginEx(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount);
    virtual ~PluginEx();

#if HIPHOP_ENABLE_SHARED_MEMORY
    void writeSharedMemory(const char* metadata /*C str*/, const unsigned char* data, size_t size);

    void setState(const char* key, const char* value) override;
#endif // HIPHOP_ENABLE_SHARED_MEMORY

#if HIPHOP_ENABLE_SHARED_MEMORY
protected:
    virtual void sharedMemoryChanged(const char* metadata, const unsigned char* data, size_t size) 
    {
        (void)metadata;
        (void)data;
        (void)size;
    }
#endif // HIPHOP_ENABLE_SHARED_MEMORY

private:
#if HIPHOP_ENABLE_SHARED_MEMORY
    DuplexSharedMemory<unsigned char,1048576/*1 MiB*/> fMemory;
#endif // HIPHOP_ENABLE_SHARED_MEMORY

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEx)

};

END_NAMESPACE_DISTRHO

#endif  // PLUGIN_EX_HPP
