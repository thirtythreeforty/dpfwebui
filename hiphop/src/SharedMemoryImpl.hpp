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

#ifndef SHARED_MEMORY_IMPL_HPP
#define SHARED_MEMORY_IMPL_HPP

#include "distrho/extra/String.hpp"
#include "extra/SharedMemory.hpp"

START_NAMESPACE_DISTRHO

// Plugin code should leave MSB off
const uint32_t kShMemHintInternal = 0x8000;

enum {
    kShMemHintWasmBinary = kShMemHintInternal | 0x1
};

// Used for determining who should read changes to the shared memory
enum {
    kShMemWriteOriginPlugin = 0,
    kShMemWriteOriginUI     = 1
};

// Keep the read flag atomic
struct SharedMemoryState
{
    uint8_t  readFlag;
    size_t   dataOffset;
    size_t   dataSize;
    uint32_t hints;
};

// Two states for full duplex usage
struct SharedMemoryHeader
{
    SharedMemoryState state[2];
};

// This class wraps SharedMemory and adds a header
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

        SharedMemoryState& a = getState(0);
        SharedMemoryState& b = getState(1);

        a.readFlag   = b.readFlag   = 1;
        a.dataOffset = b.dataOffset = 0;
        a.dataSize   = b.dataSize   = 0;
        a.hints      = b.hints      = 0;

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

    bool isRead(int origin) const noexcept
    {
        return getState(origin).readFlag != 0;
    }

    void setRead(int origin) noexcept
    {
        getState(origin).readFlag = 1;
    }

    uint32_t getHints(int origin) const noexcept
    {
        return getState(origin).hints;
    }

    size_t getDataOffset(int origin) const noexcept
    {
        return getState(origin).dataOffset;
    }

    size_t getDataSize(int origin) const noexcept
    {
        return getState(origin).dataSize;
    }

    S* getDataPointer() const noexcept
    {
        return fImpl.getDataPointer() + sizeof(SharedMemoryHeader);
    }

    const char* getDataFilename() const noexcept
    {
        return fImpl.getDataFilename();
    }

    bool write(int origin, const S* data, size_t size, size_t offset, uint32_t hints)
    {
        if (size > (getSize() - offset)) {
            return false;
        }
        
        SharedMemoryState& state = getState(origin);

        std::memcpy(getDataPointer() + offset, data, sizeof(S) * size);
        
        state.dataOffset = offset;
        state.dataSize = size;
        state.hints = hints;
        state.readFlag = 0; // do it last

        return true;
    }
private:
    SharedMemoryState& getState(int origin) const noexcept
    {
        return reinterpret_cast<SharedMemoryHeader*>(fImpl.getDataPointer())->state[origin];
    }

    SharedMemory<S,N> fImpl;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatefulSharedMemory)
};

typedef StatefulSharedMemory<uint8_t,HIPHOP_SHARED_MEMORY_SIZE> SharedMemoryImpl;

END_NAMESPACE_DISTRHO

#endif  // SHARED_MEMORY_IMPL_HPP
