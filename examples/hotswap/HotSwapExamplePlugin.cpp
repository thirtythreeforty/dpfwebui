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

#include <functional>
#include <random>

#include "distrho/extra/Sleep.hpp"
#include "distrho/extra/Thread.hpp"

#include "WasmHostPlugin.hpp"

class TimerThread : public Thread
{
public:
    TimerThread(const uint intervalMs, const std::function<void()> callback) noexcept
        : fRun(false)
        , fIntervalMs(intervalMs)
        , fCallback(callback)
    {}

    void start() noexcept
    {
        fRun = true;
        startThread();
    }

    void stop() noexcept
    {
        fRun = false;
        stopThread(-1 /* wait forever*/);
    }

    void run() noexcept override
    {
        while (fRun) {
            if (fCallback) {
                fCallback();
            }

            d_msleep(fIntervalMs);
        }
    }

private:
    bool fRun;
    uint fIntervalMs;
    std::function<void()> fCallback;

};

class HotSwapExamplePlugin : public WasmHostPlugin
{
public:
    HotSwapExamplePlugin() noexcept
        : WasmHostPlugin(16 /*parameters*/, 0 /*programs*/, 0 /*states*/)
        , fNote(0)
        , fTimer(
            500 /*120 BPM*/,
            std::bind(&HotSwapExamplePlugin::playRandomNote, this)
          )
    {}

    void activate() noexcept override
    {
        WasmHostPlugin::activate();
        fTimer.start();
    }

    void deactivate() noexcept override
    {
        fTimer.stop();
        WasmHostPlugin::deactivate();
    }

    void run(const float** inputs, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) noexcept override
    {
        // There is no equivalent of UI::sendNote() for Plugin
        MidiEvent noteEvent;

        if (fNote > 0) {
            if (midiEventCount == 0) {
                noteEvent.frame = 0; // hardcoded position
                noteEvent.size = 3;
                noteEvent.data[0] = 0x90; // note on, ch 1
                noteEvent.data[1] = fNote;
                noteEvent.data[2] = 0x7f; // vel 127
                noteEvent.dataExt = nullptr;
                midiEvents = &noteEvent;
                midiEventCount = 1;
            }

            fNote = 0;
        }

        // Call run() in plugin.ts, see source file for more plugin methods.
        WasmHostPlugin::run(inputs, outputs, frames, midiEvents, midiEventCount);
    }

private:
    void playRandomNote() noexcept
    {
        // C++11 random numbers generator
        std::random_device rd;
        std::default_random_engine rgen(rd());
        std::uniform_int_distribution<uint8_t> unid(0, 4);

        // Pick a random note from the A minor pentatonic scale
        const uint8_t ampt[5] = {69 /*A*/, 71 /*C*/, 72 /*D*/, 73 /*E*/, 75 /*G*/};
        fNote = ampt[unid(rgen)];
    }

    uint8_t     fNote;
    TimerThread fTimer;

};

Plugin* DISTRHO::createPlugin()
{
    return new HotSwapExamplePlugin();
}
