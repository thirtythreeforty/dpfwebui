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

const env = DISTRHO.env, uiHelper = DISTRHO.UIHelper;

class XWaveExampleUI extends DISTRHO.UI {

    constructor() {
        super();

        this._sampleRate = 0;
        this._sampleBuffer = [];
        this._prevFrameTimeMs = 0;

        this._initView();
    }

    sampleRateChanged(newSampleRate) {
        this._sampleRate = newSampleRate;
    }

    async messageChannelOpen() {
        this.sampleRateChanged(await this.getSampleRate());

        if (this._prevFrameTimeMs == 0) {
            this._startVisualization();
        }
    }

    // Receive higher latency visualization data sent through WebSocket
    onVisualizationData(data) {
        if (! env.plugin) {
            this._addSamples(data.samples.buffer);
        }
    }

    _initView() {
        if (env.plugin) {
            document.body.appendChild(uiHelper.getNetworkDetailsModalButton(this, {
                fill: '#fff',
                id: 'qr-button',
                modal: {
                    id: 'qr-modal'
                }
            }));
        } else {
            uiHelper.enableOfflineModal(this);
        }

        window.customElements.define('x-waveform', WaveformElement);
        this._waveform = new WaveformElement;
        document.body.prepend(this._waveform);
        this._waveform.setAttribute('autoresize', '');
        this._waveform.setAttribute('width', this._waveform.clientWidth / 4);

        document.body.style.visibility = 'visible';
    }

    _startVisualization() {
        // Register to receive low latency visualization data sent using local
        // transport, skipping network overhead. Plugin embedded web view only.
        if (env.plugin) {
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
                  numSamplesInFrame = Math.floor(deltaMs / 1000 * this._sampleRate),
                  binSize = Math.floor(numSamplesInFrame / 8),
                  numBins = Math.floor(numSamplesInFrame / binSize),
                  bins = new Float32Array(numBins);

            let k = 0;

            for (let i = 0; i < numBins; i++) {
                for (let j = 0; j < binSize; j++) {
                    bins[i] += this._sampleBuffer[k++];
                }
                bins[i] /= binSize;
            }

            this._sampleBuffer.splice(0, k);
            this._waveform.analyserData = bins;

            /*if (! env.plugin) {
                console.log(`DBG : sr=${this._sampleRate} dt=${deltaMs}ms \
                            pop=${k} left=${this._sampleBuffer.length}`);
            }*/
        }

        this._prevFrameTimeMs = timestampMs;

        window.requestAnimationFrame(t => { this._animate(t) });
    }

}

main();
