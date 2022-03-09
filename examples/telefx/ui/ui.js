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

class TeleFxExampleUI extends DISTRHO.UI {

    constructor() {
        super();

        this.dom = ['main', 'fx', 'qr'].reduce((res, id) => {
            res[id] = document.getElementById(id);
            return res;
        }, {});

        this.helper = DISTRHO.UIHelper;
        
        // Automatically display a modal view when connection is lost
        this.helper.enableDisconnectionModal(this);

        // Set fixed size for external clients
        this.helper.setElementToPluginUISize(this.dom.main, this);
        
        // Setup view to suit environment
        if (DISTRHO.env.webview) {
            this.setupForPluginEmbeddedWebview();
        } else if (DISTRHO.env.network) {
            this.setupForExternalWebClients();
        } else if (DISTRHO.env.dev) {
            this.setupForDevelopment();
        }

        document.body.style.visibility = 'visible';
    }

    setupForPluginEmbeddedWebview() {
        this.helper.getQRCodeElement(this).then((qr) => {
            this.dom.qr.appendChild(qr);
        });
    }

    setupForExternalWebClients() {
        const msg = document.createTextNode('Hello external client');
        this.dom.fx.appendChild(msg);
        this.dom.main.removeChild(this.dom.qr);
    }

    setupForDevelopment() {
        // Directly Open Source mode ;)
        const msg = document.createTextNode('This program cannot be run in DOS mode');
        this.dom.fx.appendChild(msg);
        this.dom.main.removeChild(this.dom.qr);
    }

}
