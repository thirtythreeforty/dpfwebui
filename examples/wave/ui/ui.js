/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2023 Luciano Iam <oss@lucianoiam.com>
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

const env = DISTRHO.env;

class WaveExampleUI extends DISTRHO.UI {

    constructor() {
        super();

        this._sampleRate = 0;
        this._sampleCount = 0;

        // Receive low latency visualization data sent using local transport,
        // skipping network overhead. Plugin embedded web view only.

        if (env.plugin) {
            window.host.addMessageListener(data => {
                data.samples = DISTRHO.Base64.decode(data.samples.$binary);
                this._fixmeProcessVisualizationData(data);
            });
        }

        document.body.style.visibility = 'visible';
    }

    sampleRateChanged(newSampleRate) {
        this._sampleRate = newSampleRate;
    }

    async messageChannelOpen() {
        this._sampleRate = await this.getSampleRate();
    }

    onVisualizationData(data) {
        // Receive higher latency visualization data sent through WebSocket

        if (! env.plugin) {
            data.samples = data.samples.buffer;
            this._fixmeProcessVisualizationData(data);
        }
    }

    _fixmeProcessVisualizationData(data) {
        this._sampleCount += data.samples.length;
        const s = `Sample count: ${this._sampleCount}`;
        document.getElementById('output').innerText = s;

        // TODO

    }

}
