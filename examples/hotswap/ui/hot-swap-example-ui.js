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

class HotSwapExampleUI extends DISTRHO.UI {

    constructor() {
        super();

        document.body.style.visibility = 'visible';

        const sendNote = document.getElementById('send-note');

        sendNote.addEventListener('click', (_) => {
            this.sendNote(1, getRandomInt(69 - 12, 69 + 12), 127);
        });

        const target = document.getElementById('target');
        
        target.addEventListener('dragover', (ev) => ev.preventDefault());
        
        target.addEventListener('dragenter', (_) => {
            target.classList.add('hover');
            sendNote.style.pointerEvents = 'none';
        });

        target.addEventListener('dragleave', (_) => {
            target.classList.remove('hover');
            sendNote.style.pointerEvents = '';
        });

        target.addEventListener('drop', (ev) => {
            target.classList.remove('hover');
            sendNote.style.pointerEvents = '';
            ev.preventDefault();
            this.dropHandler(ev);
        });
    }

    dropHandler(ev) {
        if (ev.dataTransfer.items.length == 0) {
            return;
        }

        const item = ev.dataTransfer.items[0];

        if (item.kind !== 'file') {
            return;
        }

        const reader = new FileReader;

        reader.onload = (ev) => {
            const data = new Uint8Array(event.target.result);
            console.log(`Read file with size ${data.length}`);
            this.sideloadWasmBinary(data);
        };

        reader.readAsArrayBuffer(item.getAsFile());
    }

}

function getRandomInt(min, max) {
    min = Math.ceil(min);
    max = Math.floor(max);

    return Math.floor(Math.random() * (max - min + 1)) + min;
}
