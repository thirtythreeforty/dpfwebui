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

// Total size 1KiB
struct SharedMemoryHeader
{
    unsigned char readFlag;
    uint32_t      dataSize;
    char          metadata[1005];
};

// This class wraps SharedMemory and adds a read state flag and metadata
template<class S>
class TaggedSharedMemory
{
public:
    TaggedSharedMemory() {}
    virtual ~TaggedSharedMemory() {}

    bool create()
    {
        if (! fImpl.create()) {
            return false;
        }

        getHeaderPointer()->readFlag = 1;

        return true;
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

    bool isRead() const noexcept
    {
        return getHeaderPointer()->readFlag != 0;
    }

    void setRead() noexcept
    {
        getHeaderPointer()->readFlag = 1;
    }

    const char* getMetadata() const noexcept
    {
        return getHeaderPointer()->metadata;
    }

    uint32_t getDataSize() const noexcept
    {
        return getHeaderPointer()->dataSize;
    }

    S* getDataPointer() const noexcept
    {
        return fImpl.getDataPointer() + sizeof(SharedMemoryHeader);
    }

    // creator-side only
    const char* getDataFilename() const noexcept
    {
        return fImpl.getDataFilename();
    }

    bool write(const char* metadata, const S* data, int32_t size)
    {
        SharedMemoryHeader *hdr = getHeaderPointer();

        if (hdr == nullptr) {
            return false;
        }

        std::strcpy(hdr->metadata, metadata);

        std::memcpy(getDataPointer(), data, sizeof(S) * size);
        hdr->dataSize = size;
        
        hdr->readFlag = 0; // do it last

        return true;
    }
private:
    SharedMemoryHeader* getHeaderPointer() const noexcept
    {
        return reinterpret_cast<SharedMemoryHeader*>(fImpl.getDataPointer());
    }

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
