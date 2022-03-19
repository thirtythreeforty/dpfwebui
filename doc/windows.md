NOTES FOR WINDOWS
-----------------

1. Native binaries can be built by cross-compiling on Linux or by installing the
   MinGW toolchain on Windows. For the latter option first download MSYS2 from
   https://www.msys2.org/ and then follow website instructions on how to install
   the mingw-64 GCC.


2. For building the plugin the Edge WebView2 SDK is required. Makefile will try
   to download the SDK if the NuGet tool is already present. If not, NuGet will
   be automatically downloaded for MinGW but on Linux that could need extra work
   https://docs.microsoft.com/en-us/nuget/install-nuget-client-tools
   Some distros like Ubuntu already offer it: sudo apt install nuget

   Running MinGW on Windows 7 can cause nuget.exe to fail due to a TLS error, in
   such case here is the fix: https://github.com/NuGet/NuGetGallery/issues/8176

   The SDK can also be downloaded using the free Visual Studio Community IDE:

   - Create Project
   - Right click on solution
   - Manage NuGet packages
   - Find and install Microsoft.Web.WebView2
   - Copy < SOLUTION_DIR >/packages/Microsoft.Web.WebView2.< VERSION > to
     < PLUGIN_DIR >/lib/Microsoft.Web.WebView2  (version suffix stripped)


3. For running the plugin on Windows 10 or earlier, the Microsoft Edge WebView2
   Runtime must be installed https://developer.microsoft.com/microsoft-edge/webview2


4. Node.js is required for building the plugin AssemblyScript files. There is no
   official binary package for MinGW. Makefile will try to download the regular
   Windows version if npm is absent. https://github.com/msys2/MINGW-packages
   provides a recipe for building Node.js but as of Jul '21 it seems broken.

   ```Bash
   pacman -S mingw-w64-x86_64-python2 mingw-w64-x86_64-nasm \
      mingw-w64-x86_64-c-ares mingw-w64-x86_64-http-parser \
      mingw-w64-x86_64-nghttp2 mingw-w64-x86_64-libuv winpty \
      mingw-w64-x86_64-icu
   git clone https://github.com/msys2/MINGW-packages
   cd MINGW-packages/mingw-w64-nodejs
   makepkg
   ```


5. The default WebAssembly runtime is WAMR, however the static library built by
   MinGW GCC (libvmlib.a) seems to be broken when the WAMR ahead-of-time (AOT)
   feature is enabled. For debugging purposes look [here](https://github.com/bytecodealliance/wasm-micro-runtime/blob/25fc006c3359e0788b42bc9a11923f8ffbe29577/core/iwasm/aot/aot_runtime.c#L1542)
   and [here](https://github.com/bytecodealliance/wasm-micro-runtime/blob/52b6c73d9c2dee4973271a5cb1e2b9242a7a975b/core/iwasm/common/arch/invokeNative_general.c#L10).
   To overcome this issue plugins link against a MSVC DLL instead which is
   downloaded by the Makefile. Here are the steps for building it from source:

   - Install [Visual Studio Community](https://visualstudio.microsoft.com/downloads/)
   - Install [CMake for Windows](https://cmake.org/download/)
   - Install [Git for Windows](https://github.com/git-for-windows/git/releases/)
   - Open Git bash and build
   ```Bash
     git clone https://github.com/bytecodealliance/wasm-micro-runtime
     cd wasm-micro-runtime/product-mini/platforms/windows
     mkdir build
     cd build
     cmake .. -DWAMR_BUILD_LIB_PTHREAD=1 -DWAMR_BUILD_AOT=1 \
              -DWAMR_BUILD_INTERP=0 -DWAMR_BUILD_LIBC_UVWASI=0
     cmake --build . --config Release
   ```
   - Look for libiwasm.dll in ./Release

   Note that WAMR_BUILD_LIBC_UVWASI=0 is required otherwise plugins will crash.
   Official instructions for building the DLL can be found [here](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/build_wamr.md).


6. Building the WAMR compiler (wamrc) on MinGW requires CMake from pacman
   package 'mingw-w64-x86_64-cmake' and NSIS. CMake from package 'cmake' will
   not work. Package dlfcn is also required otherwise -ldl link switch fails.
   Updating all MinGW packages before starting is also recommended.

   ```Bash
   pacman -R cmake
   pacman -Syu
   pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-nsis mingw-w64-x86_64-dlfcn
   ```

   More details [here](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/build_wamr.md#MinGW)


7. The official binary distribution of Wasmer requires MSVC. To overcome this
   problem the Makefile downloads a custom build that is compatible with MinGW.
   If rebuilding Wasmer from source is needed, these are the steps:

   ```Bash
   wget -O rustup-init.exe https://win.rustup.rs/
   ./rustup-init   (select host triple: x86_64-pc-windows-gnu)
   export PATH="/c/Users/< USERNAME >/.cargo/bin:$PATH"
   git clone https://github.com/wasmerio/wasmer
   make -C wasmer
   mkdir -p hiphop/deps/wasmer/include
   cp wasmer/lib/c-api/tests/wasm-c-api/include/wasm.h hiphop/deps/wasmer/include
   cp wasmer/lib/c-api/wasmer.h hiphop/deps/wasmer/include
   mkdir -p hiphop/deps/wasmer/lib
   cp wasmer/target/release/libwasmer.a hiphop/deps/wasmer/lib
   ```
   
   More details [here](https://stackoverflow.com/questions/47379214/step-by-step-instruction-to-install-rust-and-cargo-for-mingw-with-msys2)

   As of Feb '22 this runtime makes plugins crash on some hosts like Ableton
   Live and Carla.


8. Instructions on how to build libwasmer.a with debug symbols can be found here
   https://github.com/wasmerio/wasmer/issues/2571

   ```Bash
   RUSTFLAGS="-C target-feature=+crt-static" cargo build \
     --manifest-path lib/c-api/Cargo.toml --no-default-features \
     --features wat,universal,dylib,staticlib,wasi,middlewares \
     --features cranelift
   ```

   Then for debugging with Carla:

   ```Bash
   gdb --args python ~/carla/source/frontend/carla
   set breakpoint pending on
   break wasm_module_new
   ```
