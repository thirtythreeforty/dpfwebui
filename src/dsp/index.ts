/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021 Luciano Iam <oss@lucianoiam.com>
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

import DISTRHO from './distrho-plugin'
import PluginImpl from './plugin'

// The interface defined in this file is private to the framework and optimized
// for Wasmer integration. Do not use it for creating plugins, use the public
// interface provided by distrho-plugin.ts instead.
// As of Jul '21 AssemblyScript strings format has not completely settled down.
// https://github.com/AssemblyScript/assemblyscript/issues/1653. Use C-style
// strings (UTF-8, null terminated) for all interfaces exposed by this module
// to keep it future-proof and perform all string conversions in AssemblyScript.

const pluginInstance = new PluginImpl

// These are external functions implemented by the host. They are declared here
// instead of the caller module (distrho-plugin.ts) to keep all interfaces to
// the host in a single place (index.ts) and also to make sure all declared
// functions show up in the module imports table.

declare function _get_samplerate(): f32
declare function _get_time_position(): void
declare function _write_midi_event(): bool

// Re-export host functions including glue code to support distrho-plugin.ts

export { _get_samplerate as glue_get_samplerate }

export function glue_write_midi_event(midiEvent: DISTRHO.MidiEvent): bool {
    let midiOffset: i32 = 0
    raw_midi_events.setUint32(midiOffset, midiEvent.frame, /* LE */ true)
    midiOffset += 4
    raw_midi_events.setUint32(midiOffset, midiEvent.data.length, /* LE */ true)
    midiOffset += 4
    for (let i = 0; i < midiEvent.data.length; ++i) {
        raw_midi_events.setUint8(midiOffset, midiEvent.data[i])
        midiOffset++
    }
    return _write_midi_event()
}

export function glue_get_time_position(): DISTRHO.TimePosition {
    _get_time_position()
    let pos = new DISTRHO.TimePosition
    pos.playing = <bool>_rw_int32_1
    pos.frame = _rw_int64_1
    return pos
}

// Keep _get_label(), _get_maker() and _get_license() as function exports. They
// could be replaced with globals initialized to these function return values
// for simpler implementation, but in the future index.ts will be automatically
// injected into the Wasm VM (just like done with ui.js for the web view) and
// plugin implementations moved to "linked modules". Under such scheme the
// guarantee that pluginInstance is already init'd at this point no longer holds.

export function _get_label(): ArrayBuffer {
    return _wtf16_to_c_string(pluginInstance.getLabel())
}

export function _get_maker(): ArrayBuffer {
    return _wtf16_to_c_string(pluginInstance.getMaker())
}

export function _get_license(): ArrayBuffer {
    return _wtf16_to_c_string(pluginInstance.getLicense())
}

export function _get_version(): u32 {
    return pluginInstance.getVersion()
}

export function _get_unique_id(): i64 {
    return pluginInstance.getUniqueId()
}

// See explanation below for the odd value return convention
    
export function _init_parameter(index: u32): void {
    const parameter = new DISTRHO.Parameter
    pluginInstance.initParameter(index, parameter)

    _rw_int32_1 = parameter.hints
    _ro_string_1 = _wtf16_to_c_string(parameter.name)
    _rw_float32_1 = parameter.ranges.def
    _rw_float32_2 = parameter.ranges.min
    _rw_float32_3 = parameter.ranges.max
}

export function _get_parameter_value(index: u32): f32 {
    return pluginInstance.getParameterValue(index)
}

export function _set_parameter_value(index: u32, value: f32): void {
    pluginInstance.setParameterValue(index, value)
}

export function _init_program_name(index: u32): ArrayBuffer {
    let programName = new DISTRHO.StringWrapper
    pluginInstance.initProgramName(index, programName)
    return _wtf16_to_c_string(programName.string)
}

export function _load_program(index: u32): void {
    pluginInstance.loadProgram(index)
}

export function _init_state(index: u32): void {
    let stateKey = new DISTRHO.StringWrapper
    let defaultStateValue = new DISTRHO.StringWrapper

    pluginInstance.initState(index, stateKey, defaultStateValue)

    _ro_string_1 = _wtf16_to_c_string(stateKey.string)
    _ro_string_2 = _wtf16_to_c_string(defaultStateValue.string)
}

export function _set_state(key: ArrayBuffer, value: ArrayBuffer): void {
    pluginInstance.setState(_c_to_wtf16_string(key), _c_to_wtf16_string(value))
}

export function _get_state(key: ArrayBuffer): ArrayBuffer {
    return _wtf16_to_c_string(pluginInstance.getState(_c_to_wtf16_string(key)))
}

export function _activate(): void {
    pluginInstance.activate()
}

export function _deactivate(): void {
    pluginInstance.deactivate()
}

let run_count = 0

export function _run(frames: u32, midiEventCount: u32): void {
    let inputs: Float32Array[] = []

    for (let i: i32 = 0; i < _rw_num_inputs; ++i) {
        inputs.push(Float32Array.wrap(_rw_input_block, i * frames * 4, frames))
    }

    let outputs: Float32Array[] = []

    for (let i: i32 = 0; i < _rw_num_outputs; ++i) {
        outputs.push(Float32Array.wrap(_rw_output_block, i * frames * 4, frames))
    }

    let midiEvents: DISTRHO.MidiEvent[] = []
    let midiOffset: i32 = 0
    
    for (let i: u32 = 0; i < midiEventCount; ++i) {
        let event = new DISTRHO.MidiEvent
        event.frame = raw_midi_events.getUint32(midiOffset, /* LE */ true)
        midiOffset += 4
        let size = raw_midi_events.getUint32(midiOffset, /* LE */ true)
        midiOffset += 4
        event.data = Uint8Array.wrap(_rw_midi_block, midiOffset, size)
        midiOffset += size
        midiEvents.push(event)
    }

    // Count arguments are redundant, they can be inferred from arrays length.

    pluginInstance.run(inputs, outputs, midiEvents)

    // Run AS garbage collector every N calls. Default TLSF + incremental GC
    // https://www.assemblyscript.org/garbage-collection.html#runtime-variants
    // TODO: This is apperently only needed on Windows to avoid segfault after
    //       a certain period of time. Need to investigate root cause.

    if ((run_count++ % 100) == 0) {
        __collect()
    }
}

// Number of inputs or outputs does not change during runtime so it makes sense
// to init both once instead of passing them as arguments on every call to run()

export let _rw_num_inputs: i32
export let _rw_num_outputs: i32

// Using exported globals instead of passing buffer arguments to run() allows
// for a simpler implementation by avoiding Wasm memory alloc on the host side.
// Audio block size should not exceed 64Kb, or 16384 frames of 32-bit float
// samples. Midi should not exceed 6Kb, or 512 events of 12 bytes (def. size)

const MAX_AUDIO_BLOCK_BYTES = 65536

export let _rw_input_block = new ArrayBuffer(MAX_AUDIO_BLOCK_BYTES)
export let _rw_output_block = new ArrayBuffer(MAX_AUDIO_BLOCK_BYTES)

const MAX_MIDI_EVENT_BYTES = 1536

export let _rw_midi_block = new ArrayBuffer(MAX_MIDI_EVENT_BYTES)

let raw_midi_events = new DataView(_rw_midi_block, 0, MAX_MIDI_EVENT_BYTES)

// AssemblyScript does not support multi-values yet. Export a couple of generic
// variables for returning complex data types like initParameter() requires.

export let _rw_int32_1: i32
export let _rw_int32_2: i32
export let _rw_int32_3: i32
export let _rw_int32_4: i32
export let _rw_int64_1: i64
export let _rw_int64_2: i64
export let _rw_int64_3: i64
export let _rw_int64_4: i64
export let _rw_float32_1: f32
export let _rw_float32_2: f32
export let _rw_float32_3: f32
export let _rw_float32_4: f32
export let _rw_float64_1: f64
export let _rw_float64_2: f64
export let _rw_float64_3: f64
export let _rw_float64_4: f64
export let _ro_string_1: ArrayBuffer
export let _ro_string_2: ArrayBuffer

// These are useful for passing strings from host to Wasm

const MAX_STRING_BYTES = 1024

export let _rw_string_1 = new ArrayBuffer(MAX_STRING_BYTES)
export let _rw_string_2 = new ArrayBuffer(MAX_STRING_BYTES)

// Functions for converting between AssemblyScript and C strings

export function _wtf16_to_c_string(s: string): ArrayBuffer {
    return String.UTF8.encode(s, /* null terminated */ true)
}

export function _c_to_wtf16_string(s: ArrayBuffer): string {
    const nullPos = Uint8Array.wrap(s).indexOf(0)
    return String.UTF8.decode(s.slice(0, nullPos))
}
