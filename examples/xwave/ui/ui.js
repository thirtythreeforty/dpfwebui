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

import '/dpf.js';
import { WaveformElement } from '/thirdparty/x-waveform.js'

function main() {
    new XWaveExampleUI;
}

const MAX_BUFFER_SIZE = 1048576  // ~22s @ 8-bit 48000 Hz

class XWaveExampleUI extends DISTRHO.UI {

    constructor() {
        super();

        this._sampleRate = 0;
        this._sampleBuffer = [];
        this._prevFrameTimeMs = 0;
        this._isEmbeddedWebView = DISTRHO.env.plugin;

        this._initView();
    }

    sampleRateChanged(newSampleRate) {
        this._sampleRate = newSampleRate;
        this._startVisualization();
    }

    async messageChannelOpen() {
        this.sampleRateChanged(await this.getSampleRate());
    }

    // Receive higher latency visualization data sent through WebSocket
    onVisualizationData(data) {
        if (! this._isEmbeddedWebView) {
            this._addSamples(data.samples.buffer);
        }
    }

    _initView() {
        window.customElements.define('x-waveform', WaveformElement);
        this._waveform = new WaveformElement;
        this._waveform.setAttribute('autoresize', '');
        document.body.prepend(this._waveform);

        document.body.style.visibility = 'visible';
    }

    _startVisualization() {
        // Register to receive low latency visualization data sent using local
        // transport, skipping network overhead. Plugin embedded web view only.
        if (this._isEmbeddedWebView) {
            window.host.addMessageListener(data => {
                this._addSamples(DISTRHO.Base64.decode(data.samples.$binary));
            });
        }

        this._animate(0);
    }

    _addSamples(/*Uint8Array*/loResSamples) {
        const floatSamples = new Float32Array(loResSamples).map(s => s / 255);
        this._sampleBuffer.push(...floatSamples);

        // Limit buffer size in case animation stops running
        const length = this._sampleBuffer.length;

        if (length > MAX_BUFFER_SIZE) {
            this._sampleBuffer.splice(0, length - MAX_BUFFER_SIZE);
        }
    }

    _animate(timestampMs) {
        if (this._prevFrameTimeMs > 0) {
            const deltaMs = timestampMs - this._prevFrameTimeMs,
                  nSamples = Math.ceil(deltaMs / 1000 * this._sampleRate);

            this._waveform.analyserData = this._sampleBuffer.splice(0, nSamples);

            if (! this._isEmbeddedWebView) {
                console.log(`DBG : sr=${this._sampleRate} dt=${deltaMs}ms \
                            pop=${nSamples} left=${this._sampleBuffer.length}`);
            }
        }

        this._prevFrameTimeMs = timestampMs;
        
        window.requestAnimationFrame(t => { this._animate(t) });
    }

}

main();
