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

const env = DISTRHO.env, helper = DISTRHO.UIHelper;

// ZamCompX2Plugin.hpp@33
const PARAMETERS = [
    'attack', 'release', 'knee', 'ratio', 'threshold', 'makeup', 'slew',
    'stereo', 'sidechain', 'gain', 'output-level'
];

class TeleCompExampleUI extends DISTRHO.UI {

    constructor() {
        super();

        // Automatically display a modal view when connection is lost
        helper.enableOfflineModal(this);

        // Do not navigate when clicking credits and open system browser instead
        helper.enableSystemBrowser(this, document.querySelector('#credits > a'));
        
        // Setup view to suit environment
        const main = document.getElementById('main');

        if (env.plugin) {
            main.appendChild(helper.getQRButtonElement(this, {
                fill: '#000',
                id: 'qr-button',
                modal: {
                    id: 'qr-modal'
                }
            }));
        } else {
            main.style.borderRadius = '10px';
            document.getElementById('overscan').style.background = 'rgba(0,0,0,0.5)';

            if (env.dev) {
                // Directly Open the Source mode ;)
                main.innerHTML = 'This program cannot be run in DOS mode';
            }
        }

        // Connect knobs to plugin
        for (let i = 0; i < PARAMETERS.length; i++) {
            const knob = this._getKnob(i);
            if (knob) {
                knob.addEventListener('input', (ev) => {
                    this.setParameterValue(i, ev.target.value);
                });
            }
        }
    }

    parameterChanged(index, value) {
        const knob = this._getKnob(index);
        if (knob) {
            knob.value = value;
        }

        // Do not show UI until full initial parameter state has been received
        this._parameterChangeCount = this._parameterChangeCount || 0;
        if (++this._parameterChangeCount == PARAMETERS.length) {
            document.body.style.visibility = 'visible';
        }
    }

    _getKnob(parameterIndex) {
        return document.getElementById(`knob-${PARAMETERS[parameterIndex]}`);
    }

}
