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

const DISTRHO = (() => {

class UI {

    // uint UI::getWidth()
    async getWidth() {
        return this._callAndExpectReply('getWidth');
    }

    // uint UI::getHeight()
    async getHeight() {
        return this._callAndExpectReply('getHeight');
    }

    // void UI::setWidth(uint width)
    setWidth(width) {
        this._call('setWidth', width);
    }

    // void UI::setHeight(uint height)
    setHeight(height) {
        this._call('setHeight', height);
    }

    // bool UI::isResizable()
    async isResizable() {
        return this._callAndExpectReply('isResizable');
    }

    // void UI::setSize(uint width, uint height)
    setSize(width, height) {
        this._call('setSize', width, height);
    }

    // void UI::sendNote(uint8_t channel, uint8_t note, uint8_t velocity)
    sendNote(channel, note, velocity) {
        this._call('sendNote', channel, note, velocity);
    }

    // void UI::editParameter(uint32_t index, bool started)
    editParameter(index, started) {
        this._call('editParameter', index, started);
    }

    // void UI::setParameterValue(uint32_t index, float value)
    setParameterValue(index, value) {
        this._call('setParameterValue', index, value);
    }

    // void UI::setState(const char* key, const char* value)
    setState(key, value) {
        this._call('setState', key, value);
    }

    // bool UI::requestStateFile(const char* key)
    requestStateFile(key) {
        this._call('requestStateFile', key);
    }

    // void UI::sizeChanged(uint width, uint height)
    sizeChanged(width, height) {
        // default empty implementation
    }

    // void UI::parameterChanged(uint32_t index, float value)
    parameterChanged(index, value) {
        // default empty implementation
    }

    // void UI::programLoaded(uint32_t index)
    programLoaded(index) {
        // default empty implementation
    }

    // void UI::stateChanged(const char* key, const char* value)
    stateChanged(key, value) {
        // default empty implementation
    }
   
    // bool ExternalWindow::isStandalone()
    async isStandalone() {
        return this._callAndExpectReply('isStandalone');
    }

    // Non-DPF method for grabbing or releasing the keyboard focus
    // void WebViewUI::setKeyboardFocus()
    setKeyboardFocus(focus) {
        this._call('setKeyboardFocus', focus);
    }

    // Non-DPF method for opening the default system browser
    // void WebViewUI::openSystemWebBrowser(String& url)
    openSystemWebBrowser(url) {
        this._call('openSystemWebBrowser', url);
    }

    // Non-DPF method that returns the UI width at initialization time
    // uint WebViewUI::getInitialWidth()
    async getInitialWidth() {
        return this._callAndExpectReply('getInitialWidth');
    }

    // Non-DPF method that returns the UI height at initialization time
    // uint WebViewUI::getInitialHeight()
    async getInitialHeight() {
        return this._callAndExpectReply('getInitialHeight');
    }

    // Non-DPF method for sending a message to the host
    // void WebViewUI::postMessage(const JSValue& args)
    postMessage(...args) {
        if (DISTRHO.env.webview) {
            window.host.postMessage(args);
        } else if (DISTRHO.env.network) {
            if (this._socket.readyState == WebSocket.OPEN) {
                this._socket.send(JSON.stringify(args));
            } else {
                console.log(`Cannot send message, socket state is ${this._socket.readyState}.`);
            }
        } else {
            console.log(`stub: postMessage(${args})`);
        }
    }

    // Non-DPF callback method for receiving messages from the host
    // void WebUIBase::onMessageReceived(const JSValue& args)
    messageReceived(args) {
        // default empty implementation
    }

    // Non-DPF local callback that fires when the message channel is open
    messageChannelOpen() {
        // default empty implementation
    }

    // Non-DPF local callback that fires when the message channel is closed
    messageChannelClosed() {
        // default empty implementation
    }

    // Non-DPF method that writes memory shared with DISTRHO::PluginEx instance
    // void UIEx::writeSharedMemory(const unsigned char* data, size_t size, size_t offset, const char* token)
    writeSharedMemory(data /*Uint8Array*/, offset /*Number*/, token /*String*/) {
        this._call('writeSharedMemory', base64EncArr(data), offset || 0, token);
    }

    // Non-DPF callback method that notifies when shared memory is ready to use
    // void UIEx::sharedMemoryReady()
    sharedMemoryReady() {
        // default empty implementation
    }

    // Non-DPF callback method that notifies when shared memory has been written
    // void UIEx::sharedMemoryChanged(const unsigned char* data, size_t size, const char* token)
    sharedMemoryChanged(data /*Uint8Array*/, token /*String*/) {
        // default empty implementation
    }

    // Non-DPF method that loads binary into DISTRHO::WasmPlugin instance
    // void UIEx::sideloadWasmBinary(const unsigned char* data, size_t size)
    sideloadWasmBinary(data /*Uint8Array*/) {
        this._call('sideloadWasmBinary', base64EncArr(data));
    }

    // Non-DPF method that returns the plugin UI public URL
    // String NetworkUI::getPublicUrl()
    async getPublicUrl() {
        return this._callAndExpectReply('getPublicUrl');
    }

    // Non-DPF local getter that returns measured latency in milliseconds
    get latency() {
        return this._latency;
    }

}

//
// Class split in two for clarity, UI implements the public interface only.
//
class UIImpl extends UI {

    constructor() {
        super();

        this._resolve = {};
        this._socket = null;
        this._latency = 0;
        this._pingSendTime = 0;

        if (DISTRHO.env.webview) {
            this._initLocalMessageChannel();

            // Call WebUI::ready() to let the host know the JS UI has completed
            // setting up. This causes the initialization message buffer to be
            // flushed so any messages generated while the web view was still
            // loading can be processed. Since this involves message passing, it
            // will not cause any UI methods to be triggered synchronously and
            // is safe to indirectly call from super() in subclass constructors.
            this._call('ready');

        } else if (DISTRHO.env.network) {
            this._initNetworkMessageChannel();
        }
    }

    // Initialize native C++/JS message channel for the local webview
    _initLocalMessageChannel() {
        window.host.addMessageListener(this._messageReceived.bind(this));
        this.messageChannelOpen();
    }

    // Initialize WebSockets-based message channel for network clients
    _initNetworkMessageChannel() {
        const reconnectPeriod = 3;
        const pingPeriod = 10;

        let reconnectTimer = null;
        let pingTimer = null;

        const open = () => {
            this._socket = new WebSocket(`ws://${document.location.host}`);

            this._socket.addEventListener('open', (ev) => {
                console.log('UI: connected');
                clearInterval(reconnectTimer);
                pingTimer = setInterval(this._ping.bind(this), 1000 * pingPeriod);
                this._ping();
                this.messageChannelOpen();
            });

            this._socket.addEventListener('close', (ev) => {
                console.log(`UI: reconnecting in ${reconnectPeriod} sec...`);
                clearInterval(pingTimer);
                clearInterval(reconnectTimer);
                reconnectTimer = setInterval(open, 1000 * reconnectPeriod);
                this.messageChannelClosed();
            });

            this._socket.addEventListener('message', (ev) => {
                this._messageReceived(JSON.parse(ev.data));
            });
        };

        open();
    }

    // Helper for calling UI methods
    _call(method, ...args) {
        this.postMessage('UI', method, ...args)
    }

    // Helper for supporting synchronous calls using promises
    _callAndExpectReply(method, ...args) {
        if (method in this._resolve) {
            this._resolve[method][1](); // reject previous
        }
        
        return new Promise((resolve, reject) => {
            this._resolve[method] = [resolve, reject];
            this._call(method, ...args);
        });
    }

    // Send a ping message to measure latency
    _ping() {
        this._pingSendTime = (new Date).getTime();
        this._call('ping');
    }

    // Compute latency when response to ping is received
    _pong() {
        this._latency = (new Date).getTime() - this._pingSendTime;
    }

    // Handle incoming message
    _messageReceived(args) {
        if (args[0] != 'UI') {
            this.messageReceived(args); // passthrough
            return;
        }

        const method = args[1];
        args = args.slice(2);

        if (method in this._resolve) {
            this._resolve[method][0](...args); // fulfill promise
            delete this._resolve[method];
        } else {
            this[method](...args); // call method
        }
    }

    // Helper for decoding received shared memory data
    _sharedMemoryChanged(b64Data /*String*/, token /*String*/) {
        this.sharedMemoryChanged(base64DecToArr(b64Data), token);
    }

}

//
// Public utility functions
//
class UIHelper {

    static enableDisconnectionModal(ui) {
        // Monkey patch UI message channel callbacks
        const openUiCallback = ui.messageChannelOpen;
        const closedUiCallback = ui.messageChannelClosed;

        ui.messageChannelOpen = () => {
            openUiCallback();

            // TODO
            console.log('TODO : hide disconnection modal');
        };

        ui.messageChannelClosed = () => {
            closedUiCallback();

            // TODO
            console.log('TODO : show disconnection modal');
        };
    }

    static showQRCodeModal() {

        // TODO

    }

}

//
// Private utility functions
//
class UIHelperPrivate {

    static applyUiTweaks() {
        // Disable context menu
        window.addEventListener('contextmenu', (ev) => {
            ev.preventDefault()
        });

        // Disable print
        window.addEventListener('keydown', (ev) => { 
            if ((ev.key == 'p') && (ev.ctrlKey || ev.metaKey)) {
                ev.preventDefault();
            }
        });

        const style = (css) => {
            document.head.insertAdjacentHTML('beforeend', `<style>${css}</style>`);
        };

        style('img { user-drag: none; -webkit-user-drag: none; }'); // disable image drag
        style('body { user-select: none; -webkit-user-select: none; }'); // disable selection
        style('body { touch-action: pan-x pan-y; }'); // disable pinch zoom
        style('body { overflow: hidden; }'); // disable overflow
    }

    static buildEnvObject() {
        let env = {
            network: window.location.protocol.indexOf('http') == 0
        };

        if (window.host !== undefined) {
            env.webview = true;
            if (window.host.env) {
                Object.assign(env, window.host.env);
                delete window.host.env;
            }
        } else {
            env.webview = false;
        }
        
        return Object.freeze(env);
    }

}

//
// Basic setup to make the web UI behave a bit more like a native UI 
//
UIHelperPrivate.applyUiTweaks();

//
// Return namespace, see const DISTRHO = ... definition above.
//
// DISTRHO {
//    UI:       class   Base class for UIs, approximately mirrors C++ version.
//    UIHelper: class   Web browser oriented utility functions
//    env: {            Information about the environment
//       network: bool  True when document loaded via HTTP
//       webview: bool  True when running in the plugin web view
//       ...            Additional fields defined by web views
//    }
// }
//
return {
    UI: UIImpl,
    UIHelper: UIHelper
    env: UIHelperPrivate.buildEnvObject()
};


/*\
|*|
|*|  Base64 / binary data / UTF-8 strings utilities
|*|
|*|  https://developer.mozilla.org/en-US/docs/Web/JavaScript/Base64_encoding_and_decoding
|*|
\*/

/* Array of bytes to Base64 string decoding */

function b64ToUint6 (nChr) {

  return nChr > 64 && nChr < 91 ?
      nChr - 65
    : nChr > 96 && nChr < 123 ?
      nChr - 71
    : nChr > 47 && nChr < 58 ?
      nChr + 4
    : nChr === 43 ?
      62
    : nChr === 47 ?
      63
    :
      0;

}

function base64DecToArr (sBase64, nBlocksSize) {

  var
    sB64Enc = sBase64.replace(/[^A-Za-z0-9\+\/]/g, ""), nInLen = sB64Enc.length,
    nOutLen = nBlocksSize ? Math.ceil((nInLen * 3 + 1 >> 2) / nBlocksSize) * nBlocksSize : nInLen * 3 + 1 >> 2, taBytes = new Uint8Array(nOutLen);

  for (var nMod3, nMod4, nUint24 = 0, nOutIdx = 0, nInIdx = 0; nInIdx < nInLen; nInIdx++) {
    nMod4 = nInIdx & 3;
    nUint24 |= b64ToUint6(sB64Enc.charCodeAt(nInIdx)) << 6 * (3 - nMod4);
    if (nMod4 === 3 || nInLen - nInIdx === 1) {
      for (nMod3 = 0; nMod3 < 3 && nOutIdx < nOutLen; nMod3++, nOutIdx++) {
        taBytes[nOutIdx] = nUint24 >>> (16 >>> nMod3 & 24) & 255;
      }
      nUint24 = 0;

    }
  }

  return taBytes;
}

/* Base64 string to array encoding */

function uint6ToB64 (nUint6) {

  return nUint6 < 26 ?
      nUint6 + 65
    : nUint6 < 52 ?
      nUint6 + 71
    : nUint6 < 62 ?
      nUint6 - 4
    : nUint6 === 62 ?
      43
    : nUint6 === 63 ?
      47
    :
      65;

}

function base64EncArr (aBytes) {

  var nMod3 = 2, sB64Enc = "";

  for (var nLen = aBytes.length, nUint24 = 0, nIdx = 0; nIdx < nLen; nIdx++) {
    nMod3 = nIdx % 3;
    if (nIdx > 0 && (nIdx * 4 / 3) % 76 === 0) { sB64Enc += "\n"; }
    nUint24 |= aBytes[nIdx] << (16 >>> nMod3 & 24);
    if (nMod3 === 2 || aBytes.length - nIdx === 1) {
      sB64Enc += String.fromCodePoint(uint6ToB64(nUint24 >>> 18 & 63), uint6ToB64(nUint24 >>> 12 & 63), uint6ToB64(nUint24 >>> 6 & 63), uint6ToB64(nUint24 & 63));
      nUint24 = 0;
    }
  }

  return sB64Enc.substr(0, sB64Enc.length - 2 + nMod3) + (nMod3 === 2 ? '' : nMod3 === 1 ? '=' : '==');

}

})(); // DISTRHO
