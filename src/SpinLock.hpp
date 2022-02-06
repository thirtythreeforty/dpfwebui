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

#include "src/DistrhoDefines.h"

#ifdef DISTRHO_OS_WINDOWS
void usleep(unsigned int usec)
{
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10 * (__int64)usec);

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}
#else
# include <unistd.h>
#endif

START_NAMESPACE_DISTRHO

class SpinLock
{
public:
    SpinLock() noexcept : fFlag(false)
    {}

    void lock(int usec = 0) noexcept
    {
        while (fFlag.test_and_set(std::memory_order_acquire)) {
            if (usec > 0) {
                usleep(usec);
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

END_NAMESPACE_DISTRHO

#endif  // SPIN_LOCK_HPP
