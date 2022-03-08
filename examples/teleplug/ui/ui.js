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

class TeleplugExampleUI extends DISTRHO.UI {

    constructor() {
        super();

        // Automatically display a modal view when message channel closes
        DISTRHO.UIHelper.enableDisconnectionModal(this);

        const main = document.getElementById('main'); 

        if (DISTRHO.env.webview) {
            // Content to display in the plugin embedded web view
            const openBrowser = document.createElement('a');
            main.appendChild(openBrowser);

            this.getPublicUrl().then((url) => {
                openBrowser.href = '#'; // make it look like a link
                openBrowser.innerText = url;
                openBrowser.addEventListener('click', (ev) => {
                    this.openSystemWebBrowser(url);
                });
            });
        } else {
            // Content to display in external web clients
            const hello = document.createElement('div');
            hello.innerText = 'Hello external client';
            main.appendChild(hello);
        }

        document.body.style.visibility = 'visible';
    }

}
