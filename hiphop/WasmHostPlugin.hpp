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

#ifndef WASM_HOST_PLUGIN_HPP
#define WASM_HOST_PLUGIN_HPP

#include <memory>

#include "extra/PluginEx.hpp"
#include "plugin/WasmRuntime.hpp"
#include "plugin/SpinLock.hpp"

START_NAMESPACE_DISTRHO

class WasmHostPlugin : public PluginEx
{
public:
    WasmHostPlugin(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount,
                    std::shared_ptr<WasmRuntime> runtime = nullptr);
    virtual ~WasmHostPlugin() {}

    const char* getLabel() const override;
    const char* getMaker() const override;
    const char* getLicense() const override;

    uint32_t getVersion() const override;
    int64_t  getUniqueId() const override;

    void  initParameter(uint32_t index, Parameter& parameter) override;
    float getParameterValue(uint32_t index) const override;
    void  setParameterValue(uint32_t index, float value) override;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void initProgramName(uint32_t index, String& programName) override;
    void loadProgram(uint32_t index) override;
#endif // DISTRHO_PLUGIN_WANT_PROGRAMS

#if DISTRHO_PLUGIN_WANT_STATE
    void   initState(uint32_t index, String& stateKey, String& defaultStateValue) override;
    void   setState(const char* key, const char* value) override;
#if DISTRHO_PLUGIN_WANT_FULL_STATE
    String getState(const char* key) const override;
#endif // DISTRHO_PLUGIN_WANT_FULL_STATE
#endif // DISTRHO_PLUGIN_WANT_STATE

    void activate() override;
    void deactivate() override;

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void run(const float** inputs, float** outputs, uint32_t frames,
                const MidiEvent* midiEvents, uint32_t midiEventCount) override;
#else
    void run(const float** inputs, float** outputs, uint32_t frames) override;
#endif // DISTRHO_PLUGIN_WANT_MIDI_INPUT

#if HIPHOP_ENABLE_SHARED_MEMORY
    void sharedMemoryChanged(const char* metadata, const unsigned char* data, size_t size) override;
    void loadWasmBinary(const unsigned char* data, size_t size);
#endif // HIPHOP_ENABLE_SHARED_MEMORY

    WasmValueVector getTimePosition(WasmValueVector params);
    WasmValueVector writeMidiEvent(WasmValueVector params);

private:
    void prepareAndStartRuntime();

    inline void checkRuntime(const char* caller) const;

    bool fActive;
    std::shared_ptr<WasmRuntime> fRuntime;
    mutable SpinLock             fRuntimeLock;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WasmHostPlugin)

};

END_NAMESPACE_DISTRHO

#endif  // WASM_HOST_PLUGIN_HPP
