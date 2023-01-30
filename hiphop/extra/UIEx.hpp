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

#ifndef UI_EX_HPP
#define UI_EX_HPP

#include "DistrhoUI.hpp"

#if defined(HIPHOP_SHARED_MEMORY_SIZE)
# if ! DISTRHO_PLUGIN_WANT_STATE
#  error Shared memory support requires DISTRHO_PLUGIN_WANT_STATE
# endif
# include "extra/SharedMemory.hpp"
#endif 

START_NAMESPACE_DISTRHO

// This class adds some goodies to DISTRHO::UI like shared memory support

class UIEx : public UI
{
public:
    UIEx(uint width = 0, uint height = 0);
    virtual ~UIEx();

#if defined(HIPHOP_SHARED_MEMORY_SIZE)
    uint8_t* getSharedMemoryPointer() const noexcept;
    bool     writeSharedMemory(const uint8_t* data, size_t size, size_t offset = 0) noexcept;
#endif

protected:
#if defined(HIPHOP_SHARED_MEMORY_SIZE)
    void uiIdle() override;

    virtual void sharedMemoryCreated(uint8_t* ptr)
    {
        (void)ptr;
    }
#endif

private:
#if defined(HIPHOP_SHARED_MEMORY_SIZE)
    SharedMemory<uint8_t,HIPHOP_SHARED_MEMORY_SIZE> fMemory;
#endif

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIEx)

};

END_NAMESPACE_DISTRHO

#endif  // UI_EX_HPP
