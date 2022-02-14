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
        
        const target = document.getElementById('target');
        const hint = document.getElementById('hint');
        const selectFile = document.getElementById('select-file');

        selectFile.addEventListener('change', (_) => {
            if (selectFile.files.length > 0) {
                this.onFileSelected(selectFile.files[0]);
            }
        });
        
        if (DISTRHO.quirks.noDragAndDrop) {
            hint.innerHTML = 'Load<br>optimized.wasm';
            target.style.border = 'none';
        } else {       
            hint.innerHTML = 'Drag and drop<br>optimized.wasm';
            selectFile.style.display = 'none';

            target.addEventListener('dragover', (ev) => ev.preventDefault());
            
            target.addEventListener('dragenter', (_) => {
                target.classList.add('hover');
            });

            target.addEventListener('dragleave', (_) => {
                target.classList.remove('hover');
            });

            target.addEventListener('drop', (ev) => {
                target.classList.remove('hover');

                ev.preventDefault();

                if (ev.dataTransfer.items.length > 0) {
                    const item = ev.dataTransfer.items[0];
                    if (item.kind == 'file') {
                        this.onFileSelected(item.getAsFile());
                    }
                }
            });
        }

        // This is perfectly valid but it will play notes only while the UI is
        // visible. See alternate version in HotSwapExamplePlugin.cpp.
        //setInterval(() => {
        //    const ampt = [69 /*A*/, 71 /*C*/, 72 /*D*/, 73 /*E*/, 75 /*G*/];
        //    this.sendNote(1, ampt[getRandomInt(0, 4)], 127);
        //}, 500 /*120 BPM*/);
    }

    onFileSelected(file) {
        const reader = new FileReader;

        reader.onload = (_) => {
            const data = new Uint8Array(reader.result);
            console.log(`Send file of ${data.length} bytes to Plugin instance`);
            this.sideloadWasmBinary(data);
        };

        reader.readAsArrayBuffer(file);
    }

}

function getRandomInt(min, max) {
    min = Math.ceil(min);
    max = Math.floor(max);

    return Math.floor(Math.random() * (max - min + 1)) + min;
}
