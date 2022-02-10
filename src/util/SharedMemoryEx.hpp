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

#ifndef SHARED_MEMORY_EX_HPP
#define SHARED_MEMORY_EX_HPP

#include "extra/String.hpp"
#include "SharedMemory.hpp"

START_NAMESPACE_DISTRHO

// This class wraps SharedMemory and adds a read state flag and metadata
template<class S>
class TaggedSharedMemory
{
public:
    TaggedSharedMemory() {}
    virtual ~TaggedSharedMemory() {}

    bool create()
    {
        return fImpl.create();
    }

    S* connect(const char* const filename2)
    {
        return fImpl.connect(filename2);
    }

    void close()
    {
        fImpl.close();
    }

    bool isCreatedOrConnected() const noexcept
    {
        return fImpl.isCreatedOrConnected();
    }

    S* getDataPointer() const noexcept
    {
        return fImpl.getDataPointer();
    }

    const char* getDataFilename() const noexcept
    {
        return fImpl.getDataFilename();
    }
private:
    SharedMemory<S> fImpl;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TaggedSharedMemory)
};

// This struct wraps two instances of TaggedSharedMemory
template<class S>
struct DuplexSharedMemory
{
    DuplexSharedMemory() {}
    
    // Convenience method
    bool create()
    {
        if (! in.create()) {
            return false;
        }

        if (! out.create()) {
            in.close();
            return false;
        }

        return true;
    }

    void close()
    {
        if (in.isCreatedOrConnected()) {
            in.close();
        }

        if (out.isCreatedOrConnected()) {
            out.close();
        }
    }

    TaggedSharedMemory<S> in;
    TaggedSharedMemory<S> out;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DuplexSharedMemory)
};

END_NAMESPACE_DISTRHO

#endif  // SHARED_MEMORY_EX_HPP
