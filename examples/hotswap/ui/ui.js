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

        document.getElementById('play-note').addEventListener('click', (_) => {
            this.sendNote(1, getRandomInt(69 - 12, 69 + 12), 127);
        });

        const selectFile = document.getElementById('select-file');

        if (navigator.platform.indexOf('Mac') != -1) { // MACFILEINPUTBUG ?
            selectFile.style.display = 'none';
        } else {
            selectFile.addEventListener('change', (_) => {
                if (selectFile.files.length > 0) {
                    this.sideload(selectFile.files[0]);
                }
            });
        }

        if (DISTRHO.quirks.noDragAndDrop) { // LXDRAGDROPBUG ?
            document.getElementById('hint').style.display = 'none';
        } else {
            const buttons = document.getElementById('buttons');
            const target = document.getElementById('target');
            
            target.addEventListener('dragover', (ev) => ev.preventDefault());
            
            target.addEventListener('dragenter', (_) => {
                target.classList.add('hover');
                buttons.style.pointerEvents = 'none';
            });

            target.addEventListener('dragleave', (_) => {
                target.classList.remove('hover');
                buttons.style.pointerEvents = '';
            });

            target.addEventListener('drop', (ev) => {
                target.classList.remove('hover');
                buttons.style.pointerEvents = '';

                ev.preventDefault();

                if (ev.dataTransfer.items.length > 0) {
                    const item = ev.dataTransfer.items[0];
                    if (item.kind == 'file') {
                        this.sideload(item.getAsFile());
                    }
                }
            });
        }
    }

    sideload(file) {
        const reader = new FileReader;

        reader.onload = (_) => {
            const data = new Uint8Array(reader.result);
            this.sideloadWasmBinary(data);
            console.log(`Sent file with size ${data.length} to Plugin instance`);
        };

        reader.readAsArrayBuffer(file);
    }

}

function getRandomInt(min, max) {
    min = Math.ceil(min);
    max = Math.floor(max);

    return Math.floor(Math.random() * (max - min + 1)) + min;
}
