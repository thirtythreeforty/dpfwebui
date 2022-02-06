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

#include <stdexcept>

#include "WasmHostPlugin.hpp"

#include "Path.hpp"

#define ERROR_STR "Error"

USE_NAMESPACE_DISTRHO

WasmHostPlugin::WasmHostPlugin(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount,
                                std::shared_ptr<WasmRuntime> runtime)
    : Plugin(parameterCount, programCount, stateCount)
{   
    if (runtime != nullptr) {
        fRuntime = runtime;
        return; // caller initializes runtime
    }

    fRuntime.reset(new WasmRuntime());

    try {
        const String path = path::getLibraryPath() + "/dsp/main.wasm";
        fRuntime->load(path);

        WasmFunctionMap hf; // host functions

        hf["_get_samplerate"] = { {}, { WASM_F32 }, [this](WasmValueVector) -> WasmValueVector {
            return { MakeF32(getSampleRate()) };
        }};

        hf["_get_time_position"] = { {}, {}, 
            std::bind(&WasmHostPlugin::getTimePosition, this, std::placeholders::_1) };

        hf["_write_midi_event"] = { {}, { WASM_I32 }, 
            std::bind(&WasmHostPlugin::writeMidiEvent, this, std::placeholders::_1) };

        fRuntime->start(hf);

        fRuntime->setGlobal("_rw_num_inputs", MakeI32(DISTRHO_PLUGIN_NUM_INPUTS));
        fRuntime->setGlobal("_rw_num_outputs", MakeI32(DISTRHO_PLUGIN_NUM_OUTPUTS));
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

const char* WasmHostPlugin::getLabel() const
{
    if (! lockRuntime()) {
        return ERROR_STR;
    }

    const char *res = fRuntime->callFunctionReturnCString("_get_label");
    unlockRuntime();
    
    return res;
}

const char* WasmHostPlugin::getMaker() const
{
    if (! lockRuntime()) {
        return ERROR_STR;
    }

    const char* res = fRuntime->callFunctionReturnCString("_get_maker");
    unlockRuntime();
    
    return res;
}

const char* WasmHostPlugin::getLicense() const
{
    if (! lockRuntime()) {
        return ERROR_STR;
    }

    const char* res = fRuntime->callFunctionReturnCString("_get_license");
    unlockRuntime();
    
    return res;
}

uint32_t WasmHostPlugin::getVersion() const
{
    if (! lockRuntime()) {
        return 0;
    }

    uint32_t res = fRuntime->callFunctionReturnSingleValue("_get_version").of.i32;
    unlockRuntime();
    
    return res;
}

int64_t WasmHostPlugin::getUniqueId() const
{
    if (! lockRuntime()) {
        return 0;
    }

    int64_t res = fRuntime->callFunctionReturnSingleValue("_get_unique_id").of.i64;
    unlockRuntime();
    
    return res;
}

void WasmHostPlugin::initParameter(uint32_t index, Parameter& parameter)
{
    if (! lockRuntime()) {
        return;
    }

    fRuntime->callFunction("_init_parameter", { MakeI32(index) });
    parameter.hints      = fRuntime->getGlobal("_rw_int32_1").of.i32;
    parameter.name       = fRuntime->getGlobalAsCString("_ro_string_1");
    parameter.ranges.def = fRuntime->getGlobal("_rw_float32_1").of.f32;
    parameter.ranges.min = fRuntime->getGlobal("_rw_float32_2").of.f32;
    parameter.ranges.max = fRuntime->getGlobal("_rw_float32_3").of.f32;

    unlockRuntime();
}

float WasmHostPlugin::getParameterValue(uint32_t index) const
{
    if (! lockRuntime()) {
        return 0;
    }

    float res = fRuntime->callFunctionReturnSingleValue("_get_parameter_value",
        { MakeI32(index) }).of.f32;
    unlockRuntime();
    
    return res;
}

void WasmHostPlugin::setParameterValue(uint32_t index, float value)
{
    if (! lockRuntime()) {
        return;
    }

    fRuntime->callFunction("_set_parameter_value", { MakeI32(index), MakeF32(value) });
    unlockRuntime();
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
void WasmHostPlugin::initProgramName(uint32_t index, String& programName)
{
    if (! lockRuntime()) {
        return;
    }

    programName = fRuntime->callFunctionReturnCString("_init_program_name", { MakeI32(index) });
    unlockRuntime();
}

void WasmHostPlugin::loadProgram(uint32_t index)
{
    if (! lockRuntime()) {
        return;
    }

    fRuntime->callFunction("_load_program", { MakeI32(index) });
    unlockRuntime();
}
#endif // DISTRHO_PLUGIN_WANT_PROGRAMS

#if DISTRHO_PLUGIN_WANT_STATE
void WasmHostPlugin::initState(uint32_t index, String& stateKey, String& defaultStateValue)
{
    if (! lockRuntime()) {
        return;
    }

    fRuntime->callFunction("_init_state", { MakeI32(index) });
    stateKey = fRuntime->getGlobalAsCString("_ro_string_1");
    defaultStateValue = fRuntime->getGlobalAsCString("_ro_string_2");

    unlockRuntime();
}

void WasmHostPlugin::setState(const char* key, const char* value)
{
    if (! lockRuntime()) {
        return;
    }

    const WasmValue wkey = fRuntime->getGlobal("_rw_string_1");
    fRuntime->copyCStringToMemory(wkey, key);
    const WasmValue wval = fRuntime->getGlobal("_rw_string_2");
    fRuntime->copyCStringToMemory(wval, value);
    fRuntime->callFunction("_set_state", { wkey, wval });

    unlockRuntime();
}

#if DISTRHO_PLUGIN_WANT_FULL_STATE
String WasmHostPlugin::getState(const char* key) const
{
    if (! lockRuntime()) {
        return String();
    }

    const WasmValue wkey = fRuntime->getGlobal("_rw_string_1");
    fRuntime->copyCStringToMemory(wkey, key);
    const char* val = fRuntime->callFunctionReturnCString("_get_state", { wkey });

    unlockRuntime();

    return String(val);
}
#endif // DISTRHO_PLUGIN_WANT_FULL_STATE

#endif // DISTRHO_PLUGIN_WANT_STATE

void WasmHostPlugin::activate()
{
    if (! lockRuntime()) {
        return;
    }

    fRuntime->callFunction("_activate");
    unlockRuntime();
}

void WasmHostPlugin::deactivate()
{
    if (! lockRuntime()) {
        return;
    }

    fRuntime->callFunction("_deactivate");
    unlockRuntime();
}

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void WasmHostPlugin::run(const float** inputs, float** outputs, uint32_t frames,
                                const MidiEvent* midiEvents, uint32_t midiEventCount)
{
#else
    void WasmHostPlugin::run(const float** inputs, float** outputs, uint32_t frames)
{
    const MidiEvent* midiEvents = 0;
    uint32_t midiEventCount = 0;
#endif // DISTRHO_PLUGIN_WANT_MIDI_INPUT
    if (! lockRuntime(false)) {
        return;
    }

    float32_t* audioBlock;

    audioBlock = reinterpret_cast<float32_t *>(fRuntime->getMemory(
        fRuntime->getGlobal("_rw_input_block")));

    for (int i = 0; i < DISTRHO_PLUGIN_NUM_INPUTS; i++) {
        memcpy(audioBlock + i * frames, inputs[i], frames * 4);
    }

    byte_t* midiBlock = fRuntime->getMemory(fRuntime->getGlobal("_rw_midi_block"));

    for (uint32_t i = 0; i < midiEventCount; i++) {
        *reinterpret_cast<uint32_t *>(midiBlock) = midiEvents[i].frame;
        midiBlock += 4;
        *reinterpret_cast<uint32_t *>(midiBlock) = midiEvents[i].size;
        midiBlock += 4;
        if (midiEvents[i].size > MidiEvent::kDataSize) {
            memcpy(midiBlock, midiEvents[i].dataExt, midiEvents[i].size);
        } else {
            memcpy(midiBlock, midiEvents[i].data, midiEvents[i].size);
        }
        midiBlock += midiEvents[i].size;
    }

    fRuntime->callFunction("_run", { MakeI32(frames), MakeI32(midiEventCount) });

    audioBlock = reinterpret_cast<float32_t *>(fRuntime->getMemory(
        fRuntime->getGlobal("_rw_output_block")));

    for (int i = 0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; i++) {
        memcpy(outputs[i], audioBlock + i * frames, frames * 4);
    }

    unlockRuntime();
}

WasmValueVector WasmHostPlugin::getTimePosition(WasmValueVector params)
{
    (void)params;
#if DISTRHO_PLUGIN_WANT_TIMEPOS
    if (! lockRuntime()) {
        return {};
    }
    const TimePosition& pos = Plugin::getTimePosition();
    fRuntime->setGlobal("_rw_int32_1", MakeI32(pos.playing));
    fRuntime->setGlobal("_rw_int64_1", MakeI64(pos.frame));
    unlockRuntime();
    return {};
#else
    throw std::runtime_error("Called getTimePosition() without DISTRHO_PLUGIN_WANT_TIMEPOS");
#endif // DISTRHO_PLUGIN_WANT_TIMEPOS
}

WasmValueVector WasmHostPlugin::writeMidiEvent(WasmValueVector params)
{
    (void)params;
    if (! lockRuntime()) {
        return { MakeI32(0) };
    }
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    MidiEvent event;
    byte_t* midiBlock = fRuntime->getMemory(fRuntime->getGlobal("_rw_midi_block"));

    event.frame = *reinterpret_cast<uint32_t *>(midiBlock);
    midiBlock += 4;
    event.size = *reinterpret_cast<uint32_t *>(midiBlock);
    midiBlock += 4;

    if (event.size > MidiEvent::kDataSize) {
        event.dataExt = reinterpret_cast<uint8_t *>(midiBlock);
    } else {
        memcpy(event.data, midiBlock, event.size);
        event.dataExt = 0;
    }

    WasmValueVector res = { MakeI32(writeMidiEvent(event)) };
    unlockRuntime();
    return res;
#else
    throw std::runtime_error("Called writeMidiEvent() without DISTRHO_PLUGIN_WANT_MIDI_OUTPUT");
#endif // DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
}

bool WasmHostPlugin::lockRuntime(bool wait) const
{
    if (!fRuntime->isStarted()) {
        d_stderr2("WebAssembly runtime is not running");
        return false;
    }

    // When wait is false the lock spins tightly around an atomic flag without
    // making any syscalls while spinning. This is safe for the audio thread,
    // all plugin calls except run() should take negligible time to complete.
    // When wait is true some wait time is introduced on each spin iteration,
    // this is acceptable for non-audio threads and also desired because run()
    // can take non-negligible time to complete.

    fRuntimeLock.lock(wait ? 100 /*usec*/: 0);

    return true;
}

void WasmHostPlugin::unlockRuntime() const
{
    fRuntimeLock.unlock();
}
