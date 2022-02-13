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

#include <random>

#include "distrho/extra/Sleep.hpp"
#include "distrho/extra/Thread.hpp"

#include "WasmHostPlugin.hpp"

class HotSwapExamplePlugin;

struct PlayNotesThread : public Thread
{
    void run() override;

    bool fRun;
    HotSwapExamplePlugin* fPlugin;
};

class HotSwapExamplePlugin : public WasmHostPlugin
{
public:
    HotSwapExamplePlugin()
        : WasmHostPlugin(16 /*parameters*/, 0 /*programs*/, 0 /*states*/)
        , fInjectedNote(0)
    {
        fWorker.fPlugin = this;
    }

    // There is no equivalent of UI::sendNote() for Plugin
    void injectNote(uint8_t note)
    {
        fInjectedNote = note;
    }

    void activate() override
    {
        WasmHostPlugin::activate();

        fWorker.fRun = true; 
        fWorker.startThread();
    }

    void deactivate() override
    {
        WasmHostPlugin::deactivate();

        fWorker.fRun = false;
        fWorker.stopThread(-1 /*wait forever*/);
    }

    void run(const float** inputs, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        MidiEvent noteEvent;

        if (fInjectedNote > 0) {
            if (midiEventCount == 0) {
                noteEvent.frame = 0; // hardcoded position
                noteEvent.size = 3;
                noteEvent.data[0] = 0x90; // note on, ch 1
                noteEvent.data[1] = fInjectedNote;
                noteEvent.data[2] = 0x7f; // vel 127
                noteEvent.dataExt = nullptr;
                midiEvents = &noteEvent;
                midiEventCount = 1;
            }

            fInjectedNote = 0;
        }

        WasmHostPlugin::run(inputs, outputs, frames, midiEvents, midiEventCount);
    }

    // See dsp/assembly/plugin.ts for more Plugin method implementations

private:
    PlayNotesThread fWorker;
    uint8_t         fInjectedNote;

};

void PlayNotesThread::run()
{
    // C++11 random numbers generator
    std::random_device rd;
    std::default_random_engine rgen(rd());
    std::uniform_int_distribution<uint8_t> unid(0, 4);

    // A minor pentatonic
    const uint8_t ampt[5] = {69 /*A*/, 71 /*C*/, 72 /*D*/, 73 /*E*/, 75 /*G*/};

    while (fRun) {
        fPlugin->injectNote(ampt[unid(rgen)]);
        d_msleep(500); // 120 BPM
    }
}

Plugin* DISTRHO::createPlugin()
{
    return new HotSwapExamplePlugin();
}
