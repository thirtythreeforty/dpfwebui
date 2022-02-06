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

#ifndef SPIN_LOCK_HPP
#define SPIN_LOCK_HPP

#include <atomic>
#include <unistd.h>

#include "src/DistrhoDefines.h"

START_NAMESPACE_DISTRHO

class SpinLock
{
public:
    SpinLock() noexcept : fFlag(false)
    {}

    void lock(int usec) noexcept
    {
        while (fFlag.test_and_set(std::memory_order_acquire)) {
            if (usec > 0) {
                usleep(static_cast<useconds_t>(usec));
            }
        }
    }

    void unlock() noexcept
    {
        fFlag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag fFlag;

};

class ScopedSpinLock
{
public:
    ScopedSpinLock(SpinLock& lock, int usec)
        : fLock(&lock)
    {
        fLock->lock(usec);
    }

    ~ScopedSpinLock()
    {
        fLock->unlock();
    }

private:
    SpinLock* fLock;

};

END_NAMESPACE_DISTRHO

#endif  // SPIN_LOCK_HPP
