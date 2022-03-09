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

class TeleportExampleUI extends DISTRHO.UI {

    constructor() {
        super();

        this.dom = ['overscan', 'main', 'control', 'qr'].reduce((res, id) => {
            res[id] = document.getElementById(id);
            return res;
        }, {});

        // Automatically display a modal view when connection is lost
        helper.enableDisconnectionModal(this);
        
        // Setup view to suit environment
        if (env.plugin) {
            this._setupForPluginEmbeddedWebview();
        } else {
            this.dom.overscan.style.background = 'rgba(0,0,0,0.5)';
            this.dom.main.removeChild(this.dom.qr);

            if (env.remote) {
                this._setupForRemoteWebClient();
            } else if (env.dev) {
                this._setupForDevelopment();
            }
        }
    }

    messageChannelOpen() {
        //helper.showQRCodeModal(this);
        const w = env.plugin ? 1 : 0.6;

        // FIXME - message channel still not implemented
        if (!env.plugin) {
            this.dom.main.style.width = '480px';
            this.dom.main.style.height = w * 320 + 'px';
            document.body.style.visibility = 'visible';
            return;
        }

        helper.setElementToPluginUISize(this, this.dom.main, 1, w).then(() => {
            document.body.style.visibility = 'visible';
        });
    }

    _setupForPluginEmbeddedWebview() {
        const msg = document.createTextNode('UI running in embedded web view');
        this.dom.control.appendChild(msg);

        helper.getQRCodeElement(this).then((el) => {
            this.dom.qr.appendChild(el);
        });
    }

    _setupForRemoteWebClient() {
        const msg = document.createTextNode('UI running in remote client');
        this.dom.control.appendChild(msg);
    }

    _setupForDevelopment() {
        // Directly Open Source mode ;)
        const msg = document.createTextNode('This program cannot be run in DOS mode');
        this.dom.control.appendChild(msg);
    }

}
