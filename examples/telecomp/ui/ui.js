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

const PARAMETERS = ['attack', 'release', 'knee', 'ratio', 'threshold', 'slew'];

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
            document.getElementById('overscan').style.background = 'rgba(0,0,0,0.5)';

            if (env.dev) {
                // Directly Open Source mode ;)
                main.innerHTML = 'This program cannot be run in DOS mode';
            }
        }

        // Connect knobs to plugin
        for (let i = 0; i < PARAMETERS.length; i++) {
            this._getKnob(i).addEventListener('input', (ev) => {
                this.setParameterValue(i, ev.target.value);
            });
        }
    }

    messageChannelOpen() {
        // Do not let the UI take up all available space in a web browser
        const main = document.getElementById('main');

        if (env.dev) {
            main.style.width = '640px';
            main.style.height = '128px';
            document.body.style.visibility = 'visible';
            return;
        }

        // Could just use the fixed values above, this is for demo purposes.
        helper.setSizeToUIInitSize(this, main).then(() => {
            document.body.style.visibility = 'visible';
        });
    }

    parameterChanged(index, value) {
        const knob = this._getKnob(index);
        if (knob) {
            knob.value = value;
        }
    }

    _getKnob(parameterIndex) {
        return document.getElementById(`knob-${PARAMETERS[parameterIndex]}`);
    }

}
