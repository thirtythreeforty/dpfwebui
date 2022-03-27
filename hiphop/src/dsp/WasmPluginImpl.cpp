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

#include <cstring>
#include <stdexcept>

#include "WasmPluginImpl.hpp"
#include "extra/Path.hpp"

#if defined(HIPHOP_WASM_BINARY_COMPILED)
# if defined(__arm__)
#  define WASM_BINARY_FILE "aarch64.aot"
# else
#  define WASM_BINARY_FILE "x86_64.aot"
# endif
#else
# define WASM_BINARY_FILE "optimized.wasm"
#endif

USE_NAMESPACE_DISTRHO

WasmPlugin::WasmPlugin(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount,
                                std::shared_ptr<WasmRuntime> runtime)
    : PluginEx(parameterCount, programCount, stateCount)
    , fActive(false)
{   
    if (runtime != nullptr) {
        fRuntime = runtime;
        return; // caller initializes runtime
    }

    fRuntime.reset(new WasmRuntime());

    try {
        const String path = Path::getPluginLibrary() + "/dsp/" WASM_BINARY_FILE;
        fRuntime->load(path);
        onModuleLoad();
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

#define ERROR_STR "Error"
#define CHECK_INSTANCE() checkInstance(__FUNCTION__)
#define SCOPED_RUNTIME_LOCK() ScopedSpinLock lock(fRuntimeLock)

const char* WasmPlugin::getLabel() const
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        return fRuntime->callFunctionReturnCString("get_label");
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return ERROR_STR;
    }
}

const char* WasmPlugin::getMaker() const
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        return fRuntime->callFunctionReturnCString("get_maker");
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return ERROR_STR;
    }
}

const char* WasmPlugin::getLicense() const
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        return fRuntime->callFunctionReturnCString("get_license");
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return ERROR_STR;
    }
}

uint32_t WasmPlugin::getVersion() const
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        return fRuntime->callFunctionReturnSingleValue("get_version").of.i32;
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return 0;
    }
}

int64_t WasmPlugin::getUniqueId() const
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        return fRuntime->callFunctionReturnSingleValue("get_unique_id").of.i64;
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return 0;
    }
}

void WasmPlugin::initParameter(uint32_t index, Parameter& parameter)
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        fRuntime->callFunction("init_parameter", { MakeI32(index) });
        parameter.hints      = fRuntime->getGlobal("_rw_int32_0").of.i32;
        parameter.name       = fRuntime->getGlobalAsCString("_ro_string_0");
        parameter.ranges.def = fRuntime->getGlobal("_rw_float32_0").of.f32;
        parameter.ranges.min = fRuntime->getGlobal("_rw_float32_1").of.f32;
        parameter.ranges.max = fRuntime->getGlobal("_rw_float32_2").of.f32;
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

float WasmPlugin::getParameterValue(uint32_t index) const
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        return fRuntime->callFunctionReturnSingleValue("get_parameter_value",
            { MakeI32(index) }).of.f32;
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return 0;
    }
}

void WasmPlugin::setParameterValue(uint32_t index, float value)
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        fRuntime->callFunction("set_parameter_value", { MakeI32(index), MakeF32(value) });
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
void WasmPlugin::initProgramName(uint32_t index, String& programName)
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        programName = fRuntime->callFunctionReturnCString("init_program_name", { MakeI32(index) });
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

void WasmPlugin::loadProgram(uint32_t index)
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        fRuntime->callFunction("load_program", { MakeI32(index) });
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}
#endif // DISTRHO_PLUGIN_WANT_PROGRAMS

#if DISTRHO_PLUGIN_WANT_STATE
void WasmPlugin::initState(uint32_t index, State& state)
{
    PluginEx::initState(index, state);
    
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        fRuntime->callFunction("init_state", { MakeI32(index) });
        const char* key = fRuntime->getGlobalAsCString("_ro_string_0");

        // Do not overwrite PluginEx internal states
        if (std::strlen(key) == 0) {
            return;
        }

        state.key          = key;
        state.defaultValue = fRuntime->getGlobalAsCString("_ro_string_1");
        state.label        = fRuntime->getGlobalAsCString("_ro_string_2");
        state.description  = fRuntime->getGlobalAsCString("_ro_string_3");
        state.hints        = fRuntime->getGlobal("_rw_int32_0").of.i32;
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

void WasmPlugin::setState(const char* key, const char* value)
{
    PluginEx::setState(key, value);

    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        const WasmValue wkey = fRuntime->getGlobal("_rw_string_0");
        fRuntime->copyCStringToMemory(wkey, key);
        const WasmValue wval = fRuntime->getGlobal("_rw_string_1");
        fRuntime->copyCStringToMemory(wval, value);
        fRuntime->callFunction("set_state", { wkey, wval });
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

#if DISTRHO_PLUGIN_WANT_FULL_STATE
String WasmPlugin::getState(const char* key) const
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        const WasmValue wkey = fRuntime->getGlobal("_rw_string_0");
        fRuntime->copyCStringToMemory(wkey, key);
        const char* val = fRuntime->callFunctionReturnCString("get_state", { wkey });

        return String(val);
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return String();
    }
}
#endif

#endif // DISTRHO_PLUGIN_WANT_STATE

void WasmPlugin::activate()
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        fRuntime->callFunction("activate");
        fActive = true;
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

void WasmPlugin::deactivate()
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        fRuntime->callFunction("deactivate");
        fActive = false;
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void WasmPlugin::run(const float** inputs, float** outputs, uint32_t frames,
                                const MidiEvent* midiEvents, uint32_t midiEventCount)
{
#else
    void WasmPlugin::run(const float** inputs, float** outputs, uint32_t frames)
{
    const MidiEvent* midiEvents = 0;
    uint32_t midiEventCount = 0;
#endif // DISTRHO_PLUGIN_WANT_MIDI_INPUT
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

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

        fRuntime->callFunction("run", { MakeI32(frames), MakeI32(midiEventCount) });

        audioBlock = reinterpret_cast<float32_t *>(fRuntime->getMemory(
            fRuntime->getGlobal("_rw_output_block")));

        for (int i = 0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; i++) {
            memcpy(outputs[i], audioBlock + i * frames, frames * 4);
        }

        // Run AS GC on each _run() call for more deterministic memory mgmt.
        // This can help preventing dropouts when running at small buffer sizes.
        fRuntime->callFunction("__collect");
    } catch (const std::exception& ex) {
        //d_stderr2(ex.what());
    }
}

#if HIPHOP_SHARED_MEMORY_SIZE
void WasmPlugin::sharedMemoryChanged(const unsigned char* data, size_t size, uint32_t hints)
{
    if (hints & (kShMemHintInternal | kShMemHintWasmBinary)) {
        try {
            loadWasmBinary(data, size);
        } catch (const std::exception& ex) {
            d_stderr2(ex.what());
        }
    }
}

void WasmPlugin::loadWasmBinary(const unsigned char* data, size_t size)
{
    // No need to check if the runtime is running
    SCOPED_RUNTIME_LOCK();

    fRuntime->load(data, size);
    onModuleLoad();

    // This has no effect on the host parameters but might be needed by the
    // plugin code to properly initialize.
    for (uint32_t i = 0; i < 128; ++i) {
        fRuntime->callFunction("init_parameter", { MakeI32(i) });
    }

    if (fActive) {
        fRuntime->callFunction("activate");
    }
}
#endif // HIPHOP_SHARED_MEMORY_SIZE

WasmValueVector WasmPlugin::getTimePosition(WasmValueVector params)
{
    (void)params;
#if DISTRHO_PLUGIN_WANT_TIMEPOS
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        const TimePosition& pos = Plugin::getTimePosition();
        fRuntime->setGlobal("_rw_int32_0", MakeI32(pos.playing));
        fRuntime->setGlobal("_rw_int64_0", MakeI64(pos.frame));

        return {};
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return {};
    }
#else
    throw std::runtime_error("Called getTimePosition() without DISTRHO_PLUGIN_WANT_TIMEPOS");
#endif // DISTRHO_PLUGIN_WANT_TIMEPOS
}

WasmValueVector WasmPlugin::writeMidiEvent(WasmValueVector params)
{
    (void)params;
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

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

        return { MakeI32(writeMidiEvent(event)) };
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return { MakeI32(0) };
    }
#else
    throw std::runtime_error("Called writeMidiEvent() without DISTRHO_PLUGIN_WANT_MIDI_OUTPUT");
#endif // DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
}

void WasmPlugin::onModuleLoad()
{
    WasmFunctionMap hostFunc;

    hostFunc["get_samplerate"] = { {}, { WASM_F32 }, [this](WasmValueVector) -> WasmValueVector {
        return { MakeF32(getSampleRate()) };
    }};

    hostFunc["get_time_position"] = { {}, {}, 
        std::bind(&WasmPlugin::getTimePosition, this, std::placeholders::_1)
    };

    hostFunc["write_midi_event"] = { {}, { WASM_I32 }, 
        std::bind(&WasmPlugin::writeMidiEvent, this, std::placeholders::_1)
    };

    fRuntime->createInstance(hostFunc);

    fRuntime->setGlobal("_rw_num_inputs", MakeI32(DISTRHO_PLUGIN_NUM_INPUTS));
    fRuntime->setGlobal("_rw_num_outputs", MakeI32(DISTRHO_PLUGIN_NUM_OUTPUTS));
}

void WasmPlugin::checkInstance(const char* caller) const
{
    if (!fRuntime->hasInstance()) {
        throw std::runtime_error(std::string(caller) + "() : missing wasm instance");
    }
}
