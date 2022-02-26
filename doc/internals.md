### AssemblyScript DSP

As an optional feature built on top of DPF's C++ `Plugin` class, it is also
possible to write plugins in the AssemblyScript language. While not strictly a
subset it is highly similar in syntax to [TypeScript](https://www.typescriptlang.org)
and specifically designed for targeting [WebAssembly](https://webassembly.org).
Plugins leveraging this feature embed the [WAMR](https://github.com/bytecodealliance/wasm-micro-runtime) runtime or optionally [Wasmer](https://github.com/wasmerio/wasmer)
for running precompiled AssemblyScript code at near native performance.

It is worth noting that code running on top of the Wasm runtime and the web view
are completely separated entities that only communicate through a key/value
pairs interface provided by DPF (states) or shared memory. Completely decoupled
DSP and UI is enforced by DPF and this project builds upon this design. The Wasm
runtime runs in parallel to the web view, and the latter will be loaded or
unloaded depending on the UI visibility state. There is no continuously running
hidden web view or similar hacks involved.

Hip-Hop provides two AssemblyScript files that must be included in user projects:

File      | Description
----------|-------------------------------------------------------------
dpf.ts    | Public plugin interface, used for writing plugins.
index.ts  | Private plugin interface, glue code between C++ and the public interface.

User DSP code should follow the standard AssemblyScript project format described
[here](https://www.assemblyscript.org/quick-start.html). AssemblyScript project
scaffold is created by running:

`npx asinit [DIRECTORY]`

Then append some custom arguments for the compiler in file `package.json`,
these are only needed by WAMR but also work with Wasmer:

```
asc assembly/index.ts --target [debug|release] --exportRuntime --use abort=
```

This scheme might be simplified in the future when "linked modules" (the Wasm
equivalent of DLLs) are implemented.

### Web view UI

- On Linux a WebKitGTK or Chromium Embedded Framework (CEF) web view instance
  runs in a child process.

- On macOS the system WKWebView is used.

- On Windows Edge WebView2 is used. Windows <= 10 users must first install a
runtime library from https://developer.microsoft.com/microsoft-edge/webview2.
Windows 11 already ships with this library.

Usage of JS frameworks is up to the developer. No web equivalent versions of the
DPF/DGL widgets are provided. There are some options available:

- Rely on stock HTML elements plus styling
- Browse the web for available toolkits like this one [here](https://github.com/DeutscheSoft/toolkit)
- Try my widgets library [here](https://github.com/lucianoiam/guinda). It
is incomplete and still under development as of Aug '21.
- Roll your own widgets. HTML5 offers plenty of tools, being SVG and canvas
worth looking into. Even a quick combination of images, stylesheets and little
code can do the job.

### Integration with the underlying C++ framework (DPF)

The project provides a basic C++ wrapper around the Wasm standardized C API,
implemented in `WasmRuntime.cpp`. It provides some methods to read and write
global variables, and for enabling cross-language function calls. This can be
useful for creating new hybrid features.

For the UI a small JS wrapper around the C++ `DISTRHO::UI` class is provided
for convenience. New integrations between C++ and JS code can be easily built
around a single function `window.webviewHost.postMessage()` that wraps the
platform specific calls:

Linux CEF:
`window.hostPostMessage()`

Linux GTK, Mac:
`window.webkit.messageHandlers.host.postMessage()`

Windows:
`window.chrome.webview.postMessage()`

In an attempt to keep the interface symmetrical and generic, `window.webviewHost`
is created as a `EventTarget` instance that can listened for events named
'message' on the JS side. This allows C++ to send messages by running the
following JS code:

`window.webviewHost.dispatchEvent(new CustomEvent('message',{detail:args}))`

The `DISTRHO::WebHostUI` and JS `DISTRHO.UI` classes use the above mechanism
to map some useful plugin methods, like the ones shown in the first code example
of the main README.

The bridge interface in a nutshell:

```
// Send from JS to C++

window.webviewHost.postMessage([...]);

void WebHostUI::webMessageReceived(const JsValueVector&) {

   // Receive in C++ from JS

}

// Send from C++ to JS

WebHostUI::webPostMessage({...});

window.webviewHost.addMessageListener((args) => {
    
    // Receive in JS from C++

});
```

Message arguments must be an array/vector containing values of primitive data
types. These values are wrapped by `DISTRHO::JsValue` instances. The following
JS types are supported: boolean, number, string. Any other types are mapped to
null.
