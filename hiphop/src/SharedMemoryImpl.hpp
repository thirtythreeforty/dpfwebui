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

START_NAMESPACE_DISTRHO

// Total size 128 bytes
struct SharedMemoryState
{
    unsigned char readFlag; // atomic
    uint32_t      dataOffset;
    uint32_t      dataSize;
    char          token[119];
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
        a.token[0]   = b.token[0]   = '\0';

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

    bool isRead(int index) const noexcept
    {
        return getState(index).readFlag != 0;
    }

    void setRead(int index) noexcept
    {
        getState(index).readFlag = 1;
    }

    const char* getToken(int index) const noexcept
    {
        return getState(index).token;
    }

    size_t getDataOffset(int index) const noexcept
    {
        return static_cast<size_t>(getState(index).dataOffset);
    }

    size_t getDataSize(int index) const noexcept
    {
        return static_cast<size_t>(getState(index).dataSize);
    }

    S* getDataPointer() const noexcept
    {
        return fImpl.getDataPointer() + sizeof(SharedMemoryHeader);
    }

    const char* getDataFilename() const noexcept
    {
        return fImpl.getDataFilename();
    }

    bool write(int index, const S* data, size_t size, size_t offset, const char* token)
    {
        if (size > (getSize() - offset)) {
            return false;
        }
        
        SharedMemoryState& state = getState(index);

        std::memcpy(getDataPointer() + offset, data, sizeof(S) * size);
        
        state.dataOffset = static_cast<uint32_t>(offset);
        state.dataSize = static_cast<uint32_t>(size);

        if (token != nullptr) {
            std::strcpy(state.token, token);
        } else {
            state.token[0] = '\0';
        }
        
        state.readFlag = 0; // do it last

        return true;
    }
private:
    SharedMemoryState& getState(int index) const noexcept
    {
        return reinterpret_cast<SharedMemoryHeader*>(fImpl.getDataPointer())->state[index];
    }

    SharedMemory<S,N> fImpl;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatefulSharedMemory)
};

constexpr int kDirectionPluginToUI = 0;
constexpr int kDirectionUIToPlugin = 1;

typedef StatefulSharedMemory<unsigned char,HIPHOP_SHARED_MEMORY_SIZE> SharedMemoryImpl;

END_NAMESPACE_DISTRHO

#endif  // SHARED_MEMORY_IMPL_HPP
