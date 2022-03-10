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

import DISTRHO from './dpf'

// This plugin has no functionality other than providing metadata

export default class HotSwapExamplePlugin extends DISTRHO.Plugin implements DISTRHO.PluginInterface {

    getLabel(): string {
        return 'Hot Swap'
    }

    getMaker(): string {
        return 'Luciano Iam'
    }

    getLicense(): string {
        return 'GPLv3'
    }

    getVersion(): u32 {
        return DISTRHO.d_version(1, 0, 0)
    }

    getUniqueId(): i64 {
        return DISTRHO.d_sconst('HHhs')
    }

    activate(): void {
        // empty implementation
    }

    run(inputs: Float32Array[], outputs: Float32Array[], midiEvents: DISTRHO.MidiEvent[]): void {
        // empty implementation
    }

    /**
     * All interface methods must be implemented
     */

    initParameter(index: u32, parameter: DISTRHO.Parameter): void {
        // empty implementation
    }

    getParameterValue(index: u32): f32 {
        return 0 // empty implementation
    }

    setParameterValue(index: u32, value: f32): void {
        // empty implementation
    }

    initProgramName(index: u32, programName: DISTRHO.String): void {
        // empty implementation
    }

    loadProgram(index: u32): void {
        // empty implementation
    }

    initState(index: u32, state: DISTRHO.State): void {
        // empty implementation
    }

    setState(key: string, value: string): void {
        // empty implementation
    }
    
    getState(key: string): string {
        return '' // empty implementation
    }

    deactivate(): void {
        // empty implementation
    }

}
