### Web view UI

- On Linux a WebKitGTK or Chromium Embedded Framework (CEF) web view instance
  runs in a child process.

- On macOS the system WKWebView is used.

- On Windows Edge WebView2 is used. Windows <= 10 users must first install a
runtime library from https://developer.microsoft.com/microsoft-edge/webview2.
Windows 11 already ships with this library.

Usage of JS frameworks is up to the developer. No web equivalent versions of the
DPF/DGL widgets are provided. These are some available solutions:

- Look for libraries, for example [here](https://github.com/lucianoiam/guinda) and [here](https://github.com/DeutscheSoft/toolkit)
- Rely on stock HTML elements plus styling
- Create custom widgets, HTML offers plenty of tools like SVG, canvas and bitmap
images.

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
DSP and UI is enforced by DPF and this project builds upon that design. The Wasm
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
