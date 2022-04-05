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

// Colors
// attack #800000
// release #008001
// threshold #feff00
// ratio #1500ff
// knee #ff00ff
// slew #ff02ff

class TeleCompExampleUI extends DISTRHO.UI {

    constructor() {
        super();

        // Automatically display a modal view when connection is lost
        helper.enableOfflineModal(this);
        
        // Setup view to suit environment
        if (env.plugin) {
            this._setupForPluginEmbeddedWebview();
        } else {
            document.getElementById('overscan').style.background = 'rgba(0,0,0,0.5)';

            if (env.network) {
                this._setupForRemoteClient();
            } else if (env.dev) {
                this._setupForDevelopment();
            }
        }
    }

    messageChannelOpen() {
        if (env.dev) {
            document.body.style.visibility = 'visible';
            return;
        }
        
        // Do not let the UI take up all available space in a web browser
        const main = document.getElementById('main');

        helper.setSizeToUIInitSize(this, main).then(() => {
            document.body.style.visibility = 'visible';
        });
    }

    _setupForPluginEmbeddedWebview() {
        const main = document.getElementById('main');
        main.appendChild(helper.getMirrorButtonElement(this, {fill: '#000'}));
    }

    _setupForRemoteClient() {

        // TODO

    }

    _setupForDevelopment() {
        // Directly Open Source mode ;)
        const main = document.getElementById('main');
        //main.innerHTML = '';
        main.appendChild(document.createTextNode('This program cannot be run in DOS mode'));
    }

}
