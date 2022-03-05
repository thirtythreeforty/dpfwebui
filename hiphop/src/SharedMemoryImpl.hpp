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

#ifndef SHARED_MEMORY_IMPL_HPP
#define SHARED_MEMORY_IMPL_HPP

#include "distrho/extra/String.hpp"
#include "SharedMemory.hpp"

#define DEFAULT_SHMEM_SIZE 1048576 //1 MiB

START_NAMESPACE_DISTRHO

// Total size 128 bytes
struct SharedMemoryHeader
{
    unsigned char readFlag; // atomic
    uint32_t      dataOffset;
    uint32_t      dataSize;
    char          token[119];
};

// This class wraps SharedMemory and adds a read state flag and metadata
template<class S, size_t N>
class StatefulSharedMemory
{
public:
    StatefulSharedMemory() {}
    virtual ~StatefulSharedMemory() {}

    bool create()
    {
        if (! fImpl.create()) {
            return false;
        }

        SharedMemoryHeader *hdr = getHeaderPointer();
        hdr->readFlag = 1;
        hdr->dataOffset = 0;
        hdr->dataSize = 0;
        hdr->token[0] = '\0';

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

    size_t getSize() const noexcept {
        return N;
    }

    size_t getSizeBytes() const noexcept {
        return N * sizeof(S);
    }

    bool isRead() const noexcept
    {
        return getHeaderPointer()->readFlag != 0;
    }

    void setRead() noexcept
    {
        getHeaderPointer()->readFlag = 1;
    }

    const char* getToken() const noexcept
    {
        return getHeaderPointer()->token;
    }

    size_t getDataOffset() const noexcept
    {
        return static_cast<size_t>(getHeaderPointer()->dataOffset);
    }

    size_t getDataSize() const noexcept
    {
        return static_cast<size_t>(getHeaderPointer()->dataSize);
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

    bool write(const S* data, size_t size, size_t offset, const char* token)
    {
        if (size > (getSize() - offset)) {
            return false;
        }
        
        SharedMemoryHeader *hdr = getHeaderPointer();

        if (hdr == nullptr) {
            return false;
        }

        std::memcpy(getDataPointer() + offset, data, sizeof(S) * size);
        
        hdr->dataOffset = static_cast<uint32_t>(offset);
        hdr->dataSize = static_cast<uint32_t>(size);

        if (token != nullptr) {
            std::strcpy(hdr->token, token);
        } else {
            hdr->token[0] = '\0';
        }
        
        hdr->readFlag = 0; // do it last

        return true;
    }
private:
    SharedMemoryHeader* getHeaderPointer() const noexcept
    {
        return reinterpret_cast<SharedMemoryHeader*>(fImpl.getDataPointer());
    }

    SharedMemory<S,N> fImpl;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatefulSharedMemory)
};

// This struct wraps two instances of StatefulSharedMemory
template<class S, size_t N>
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

    StatefulSharedMemory<S,N> in;
    StatefulSharedMemory<S,N> out;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DuplexSharedMemory)
};

typedef DuplexSharedMemory<unsigned char,DEFAULT_SHMEM_SIZE> SharedMemoryImpl;

END_NAMESPACE_DISTRHO

#endif  // SHARED_MEMORY_IMPL_HPP
