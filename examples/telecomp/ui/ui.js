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

            if (env.remote) {
                this._setupForRemoteWebClient();
            } else if (env.dev) {
                this._setupForDevelopment();
            }
        }
    }

    messageChannelOpen() {
        const main = document.getElementById('main');

        // FIXME - WS message channel still not implemented
        if (!env.plugin) {
            main.style.width = '480px';
            main.style.height = '192px';
            document.body.style.minWidth = '480px';
            document.body.style.minHeight = '192px';
            document.body.style.visibility = 'visible';
            return;
        }

        // Do not let the UI take up all available space in a web browser,
        helper.setSizeToUIInitSize(this, main).then(() => {
            document.body.style.visibility = 'visible';
        });
    }

    _setupForPluginEmbeddedWebview() {
        const main = document.getElementById('main');

        let msg = document.createTextNode('UI running in embedded web view');
        main.appendChild(msg);

        main.appendChild(helper.getMirrorButtonElement(this, {fill: '#000'}));
    }

    _setupForRemoteWebClient() {
        const msg = document.createTextNode('UI running in remote client');
        document.getElementById('main').appendChild(msg);
    }

    _setupForDevelopment() {
        // Directly Open Source mode ;)
        const msg = document.createTextNode('This program cannot be run in DOS mode');
        document.getElementById('main').appendChild(msg);
    }

}
