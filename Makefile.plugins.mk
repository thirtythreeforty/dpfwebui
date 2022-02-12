# Filename: Makefile.plugins.mk
# Author:   oss@lucianoiam.com

# ------------------------------------------------------------------------------
# Basic setup

HIPHOP_ROOT_PATH   := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
HIPHOP_SRC_PATH    ?= $(HIPHOP_ROOT_PATH)/hiphop/src
HIPHOP_VENDOR_PATH ?= $(HIPHOP_ROOT_PATH)/vendor

DPF_PATH       ?= $(HIPHOP_ROOT_PATH)/dpf
DPF_TARGET_DIR ?= bin
DPF_BUILD_DIR  ?= build
DPF_GIT_BRANCH ?= develop

ifeq ($(HIPHOP_PROJECT_VERSION),)
$(error HIPHOP_PROJECT_VERSION is not set)
endif

ifneq ($(HIPHOP_AS_DSP_PATH),)
AS_DSP = true
endif

ifneq ($(HIPHOP_WEB_UI_PATH),)
WEB_UI = true
endif

NPM_OPT_SET_PATH = true

# ------------------------------------------------------------------------------
# Determine build environment

TARGET_MACHINE := $(shell gcc -dumpmachine)

ifneq (,$(findstring linux,$(TARGET_MACHINE)))
LINUX = true
endif
ifneq (,$(findstring apple,$(TARGET_MACHINE)))
MACOS = true
endif
ifneq (,$(findstring mingw,$(TARGET_MACHINE)))
WINDOWS = true
endif
ifneq (,$(findstring MINGW,$(MSYSTEM)))
MSYS_MINGW = true
endif

ifeq ($(MSYS_MINGW),true)
ifeq ($(shell cmd /c "net.exe session 1>NUL 2>NUL || exit /b 1"; echo $$?),1)
#$(info Run MSYS as administrator for real symlink support)
MSYS_MINGW_SYMLINKS = :
else
MSYS_MINGW_SYMLINKS = export MSYS=winsymlinks:nativestrict
endif
endif

# ------------------------------------------------------------------------------
# Utility for determining plugin library path. Use the standard plugin resources
# path when available. User defined TARGETS variable becomes available only
# *after* inclusion of this Makefile hence the usage of the 'test' command.

TEST_LV2 = test -d $(TARGET_DIR)/$(NAME).lv2
TEST_VST3 = test -d $(TARGET_DIR)/$(NAME).vst3
TEST_VST2_LINUX = test -f $(TARGET_DIR)/$(NAME)-vst.so
TEST_VST2_MACOS = test -d $(TARGET_DIR)/$(NAME).vst
TEST_VST2_WINDOWS = test -f $(TARGET_DIR)/$(NAME)-vst.dll
TEST_JACK_LINUX_OR_MACOS = test -f $(TARGET_DIR)/$(NAME)
TEST_JACK_WINDOWS = test -f $(TARGET_DIR)/$(NAME).exe
TEST_NOBUNDLE = $(TEST_VST2_WINDOWS) || $(TEST_VST2_LINUX) \
                || $(TEST_JACK_LINUX_OR_MACOS) || $(TEST_JACK_WINDOWS)

LIB_DIR_LV2 = $(TARGET_DIR)/$(NAME).lv2/lib
LIB_DIR_VST3 = $(TARGET_DIR)/$(NAME).vst3/Contents/Resources
LIB_DIR_VST2_MACOS = $(TARGET_DIR)/$(NAME).vst/Contents/Resources
LIB_DIR_NOBUNDLE = $(TARGET_DIR)/$(NAME)-lib

# ------------------------------------------------------------------------------
# Add optional support for AssemblyScript DSP

ifeq ($(AS_DSP),true)
HIPHOP_FILES_DSP  = plugin/PluginEx.cpp \
                    plugin/WasmHostPlugin.cpp \
                    plugin/WasmRuntime.cpp
ifeq ($(LINUX),true)
HIPHOP_FILES_DSP += LinuxPath.cpp
endif
ifeq ($(MACOS),true)
HIPHOP_FILES_DSP += MacPath.mm
endif
ifeq ($(WINDOWS),true)
HIPHOP_FILES_DSP += WindowsPath.cpp
endif

FILES_DSP += $(HIPHOP_FILES_DSP:%=$(HIPHOP_SRC_PATH)/%)
endif

# ------------------------------------------------------------------------------
# Add optional support for web UI

ifeq ($(WEB_UI),true)
HIPHOP_FILES_UI  = ui/UIEx.cpp \
                   ui/AbstractWebHostUI.cpp \
                   ui/AbstractWebView.cpp \
                   ui/JsValue.cpp
ifeq ($(LINUX),true)
HIPHOP_FILES_UI += LinuxPath.cpp \
                   ui/linux/LinuxWebHostUI.cpp \
                   ui/linux/ChildProcessWebView.cpp \
                   ui/linux/IpcChannel.cpp \
                   ui/linux/ipc.c
endif
ifeq ($(MACOS),true)
HIPHOP_FILES_UI += MacPath.mm \
                   ui/macos/MacWebHostUI.mm \
                   ui/macos/CocoaWebView.mm
endif
ifeq ($(WINDOWS),true)
HIPHOP_FILES_UI += WindowsPath.cpp \
                   ui/windows/WindowsWebHostUI.cpp \
                   ui/windows/EdgeWebView.cpp \
                   ui/windows/WebView2EventHandler.cpp \
                   ui/windows/cJSON.c
endif

FILES_UI += $(HIPHOP_FILES_UI:%=$(HIPHOP_SRC_PATH)/%)
endif

# ------------------------------------------------------------------------------
# Optional support for macOS universal binaries, keep this before DPF include.
ifeq ($(MACOS),true)
HIPHOP_MACOS_UNIVERSAL ?= false
ifeq ($(HIPHOP_MACOS_UNIVERSAL),true)
# Non CPU-specific optimization flags, see DPF Makefile.base.mk
NOOPT = true
MACOS_UNIVERSAL_FLAGS = -arch x86_64 -arch arm64
CFLAGS += $(MACOS_UNIVERSAL_FLAGS)
CXXFLAGS += $(MACOS_UNIVERSAL_FLAGS)
LDFLAGS += $(MACOS_UNIVERSAL_FLAGS)
endif
endif

# ------------------------------------------------------------------------------
# Include DPF Makefile for plugins

ifeq (,$(wildcard $(DPF_PATH)/Makefile))
_ := $(shell git submodule update --init --recursive)
endif

ifneq (,$(DPF_GIT_BRANCH))
ifeq (,$(findstring $(DPF_GIT_BRANCH),$(shell git -C $(DPF_PATH) branch --show-current)))
_ := $(shell git -C $(DPF_PATH) checkout $(DPF_GIT_BRANCH))
endif
endif

ifeq ($(WEB_UI),true)
UI_TYPE = external
endif

include $(DPF_PATH)/Makefile.plugins.mk

# ------------------------------------------------------------------------------
# Add shared build flags

BASE_FLAGS += -I$(HIPHOP_ROOT_PATH)/hiphop -I$(HIPHOP_SRC_PATH) -I$(DPF_PATH) \
              -DPLUGIN_BIN_BASENAME=$(NAME) \
              -DHIPHOP_PROJECT_ID_HASH=$(shell echo $(NAME):$(HIPHOP_PROJECT_VERSION) \
                 | shasum -a 256 | head -c 8)
ifeq ($(LINUX),true)
BASE_FLAGS += -lrt
endif
ifeq ($(MACOS),true)
# This is needed otherwise expect crashes on older macOS when compiling on newer
# systems. Minimum supported target is High Sierra when WKWebView was introduced.
# Warn: ... was built for newer macOS version (11.0) than being linked (10.13)
BASE_FLAGS += -mmacosx-version-min=10.13
endif

# ------------------------------------------------------------------------------
# Add build flags for AssemblyScript DSP dependencies

ifeq ($(AS_DSP),true)

BASE_FLAGS += -DHIPHOP_ENABLE_WASM_PLUGIN=1

ifeq ($(WINDOWS),true)
HIPHOP_WASM_RUNTIME ?= wamr
else
HIPHOP_WASM_RUNTIME ?= wasmer
endif

HIPHOP_ENABLE_WASI ?= false

ifeq ($(HIPHOP_ENABLE_WASI),true)
ifeq ($(HIPHOP_WASM_RUNTIME),wamr)
$(error WAMR C API does not support WASI)
endif
BASE_FLAGS += -DHIPHOP_ENABLE_WASI
endif

ifeq ($(HIPHOP_WASM_RUNTIME),wamr)
BASE_FLAGS += -I$(WAMR_PATH)/core/iwasm/include -DHIPHOP_WASM_RUNTIME_WAMR
LINK_FLAGS += -L$(WAMR_BUILD_DIR) -lvmlib
ifeq ($(LINUX),true)
LINK_FLAGS += -lpthread
endif
ifeq ($(WINDOWS),true)
LINK_FLAGS += -lWs2_32 -lShlwapi
endif
endif

ifeq ($(HIPHOP_WASM_RUNTIME),wasmer)
BASE_FLAGS += -I$(WASMER_PATH)/include -DHIPHOP_WASM_RUNTIME_WASMER
LINK_FLAGS += -L$(WASMER_PATH)/lib -lwasmer
ifeq ($(LINUX),true)
LINK_FLAGS += -lpthread -ldl
endif
ifeq ($(MACOS),true)
LINK_FLAGS += -framework AppKit 
endif
ifeq ($(WINDOWS),true)
LINK_FLAGS += -Wl,-Bstatic -lWs2_32 -lBcrypt -lUserenv -lShlwapi
endif
endif

endif

# ------------------------------------------------------------------------------
# Add build flags for web UI dependencies

ifeq ($(WEB_UI),true)
ifeq ($(HIPHOP_PRINT_TRAFFIC),true)
BASE_FLAGS += -DHIPHOP_PRINT_TRAFFIC
endif
ifeq ($(LINUX),true)
LINK_FLAGS += -lpthread -ldl
endif
ifeq ($(MACOS),true)
LINK_FLAGS += -framework WebKit 
endif
ifeq ($(WINDOWS),true)
BASE_FLAGS += -I$(EDGE_WEBVIEW2_PATH)/build/native/include -Wno-unknown-pragmas
LINK_FLAGS += -L$(EDGE_WEBVIEW2_PATH)/build/native/x64 \
              -lole32 -lShlwapi -lMfplat -lksuser -lmfuuid -lwmcodecdspuuid \
              -static-libgcc -static-libstdc++ -Wl,-Bstatic \
              -lstdc++ -lpthread
endif
endif

# ------------------------------------------------------------------------------
# Print some info

TARGETS += info

info:
	@echo "Building $(NAME)"

# ------------------------------------------------------------------------------
# Dependency - Build DPF Graphics Library

ifneq ($(WEB_UI),true)
LIBDGL_PATH = $(DPF_PATH)/build/libdgl.a
TARGETS += $(LIBDGL_PATH)

ifeq ($(HIPHOP_MACOS_UNIVERSAL),true)
	DGL_MAKE_FLAGS = dgl NOOPT=true DGL_FLAGS="$(MACOS_UNIVERSAL_FLAGS)" \
					 DGL_LIBS="$(MACOS_UNIVERSAL_FLAGS)"
endif

$(LIBDGL_PATH):
	make -C $(DPF_PATH) $(DGL_MAKE_FLAGS)
endif

# ------------------------------------------------------------------------------
# Dependency - Clone and build WAMR

ifeq ($(AS_DSP),true)
ifeq ($(HIPHOP_WASM_RUNTIME),wamr)
WAMR_PATH = $(HIPHOP_VENDOR_PATH)/wasm-micro-runtime
WAMR_BUILD_DIR = ${WAMR_PATH}/build
WAMR_LIB_PATH = $(WAMR_BUILD_DIR)/libvmlib.a
WAMR_GIT_URL = https://github.com/bytecodealliance/wasm-micro-runtime
#WAMR_GIT_TAG = set this after PR #1000 gets included in a new release

# Disable HW_BOUND_CHECK feature because it does not compile on MinGW. On Linux
# and macOS it must be also disabled to prevent crashes during initialization of
# additional plugin instances, ie. after the first plugin instance has been
# successfully created. WAMR was not designed to run in plugin environments.
# Disable WASI because it is not available through the C API.
WAMR_CMAKE_ARGS = -DWAMR_DISABLE_HW_BOUND_CHECK=1 -DWAMR_BUILD_LIBC_WASI=0

ifeq ($(WINDOWS),true)
# Use the C version of invokeNative() instead of ASM until MinGW build is fixed
WAMR_CMAKE_ARGS += -G"Unix Makefiles" -DWAMR_BUILD_INVOKE_NATIVE_GENERAL=1
endif

ifeq ($(SKIP_STRIPPING),true)
WAMR_BUILD_CONFIG = Debug
else
WAMR_BUILD_CONFIG = Release
endif

TARGETS += $(WAMR_LIB_PATH)

$(WAMR_LIB_PATH): $(WAMR_PATH)
	@echo "Building WAMR static library"
	@mkdir -p $(WAMR_BUILD_DIR) && cd $(WAMR_BUILD_DIR) \
		&& cmake .. $(WAMR_CMAKE_ARGS) && cmake --build . --config $(WAMR_BUILD_CONFIG)

WAMR_C_API_PATH = $(WAMR_PATH)/core/iwasm/common

$(WAMR_PATH):
	@mkdir -p $(HIPHOP_VENDOR_PATH)
	@git -C $(HIPHOP_VENDOR_PATH) clone $(WAMR_GIT_URL) \
		&& git -C $(WAMR_PATH) reset --hard 4bdeb90
	#@git -C $(HIPHOP_VENDOR_PATH) clone $(WAMR_GIT_URL) --branch $(WAMR_GIT_TAG) --depth 1
endif
endif

# ------------------------------------------------------------------------------
# Dependency - Download Wasmer static library

ifeq ($(AS_DSP),true)
ifeq ($(HIPHOP_WASM_RUNTIME),wasmer)
WASMER_PATH = $(HIPHOP_VENDOR_PATH)/wasmer
WASMER_VERSION = 2.1.1

TARGETS += $(WASMER_PATH)

ifeq ($(LINUX_OR_MACOS),true)
ifeq ($(LINUX),true)
WASMER_PKG_FILE = wasmer-linux-amd64.tar.gz
endif
ifeq ($(MACOS),true)
# There is no macOS universal binary of Wasmer, download both architectures and combine.
WASMER_PKG_FILE_INTEL = wasmer-darwin-amd64.tar.gz
WASMER_PKG_FILE_ARM = wasmer-darwin-arm64.tar.gz
ifeq ($(CPU_I386_OR_X86_64),true)
WASMER_PKG_FILE = $(WASMER_PKG_FILE_INTEL)
WASMER_PKG_FILE_2 = $(WASMER_PKG_FILE_ARM)
else
WASMER_PKG_FILE = $(WASMER_PKG_FILE_ARM)
WASMER_PKG_FILE_2 = $(WASMER_PKG_FILE_INTEL)
endif
endif
WASMER_URL = https://github.com/wasmerio/wasmer/releases/download
WASMER_PKG_URL = $(WASMER_URL)/$(WASMER_VERSION)/$(WASMER_PKG_FILE)
endif
ifeq ($(WINDOWS),true)
# Wasmer official Windows binary distribution requires MSVC, download a custom build for MinGW.
WASMER_PKG_FILE = wasmer-mingw-amd64-$(WASMER_VERSION).tar.gz
WASMER_PKG_URL = https://github.com/lucianoiam/hiphop/files/7796845/$(WASMER_PKG_FILE)
endif

# https://stackoverflow.com/questions/37038472/osx-how-to-statically-link-a-library-and-dynamically-link-the-standard-library
$(WASMER_PATH):
	@mkdir -p $(WASMER_PATH)
	@wget -4 -O /tmp/$(WASMER_PKG_FILE) $(WASMER_PKG_URL)
	@tar xzf /tmp/$(WASMER_PKG_FILE) -C $(WASMER_PATH)
ifeq ($(LINUX),true)
	@mv $(WASMER_PATH)/lib/libwasmer.so $(WASMER_PATH)/lib/libwasmer.so.ignore
endif
ifeq ($(MACOS),true)
	@mv $(WASMER_PATH)/lib/libwasmer.dylib $(WASMER_PATH)/lib/libwasmer.dylib.ignore
endif
	@rm /tmp/$(WASMER_PKG_FILE)
ifeq ($(HIPHOP_MACOS_UNIVERSAL),true)
	@wget -4 -O /tmp/$(WASMER_PKG_FILE_2) $(WASMER_URL)/$(WASMER_VERSION)/$(WASMER_PKG_FILE_2)
	@mkdir -p /tmp/wasmer-2
	@tar xzf /tmp/$(WASMER_PKG_FILE_2) -C /tmp/wasmer-2
	@lipo -create $(WASMER_PATH)/lib/libwasmer.a /tmp/wasmer-2/lib/libwasmer.a \
		-o $(WASMER_PATH)/lib/libwasmer-universal.a
	@rm $(WASMER_PATH)/lib/libwasmer.a
	@mv $(WASMER_PATH)/lib/libwasmer-universal.a $(WASMER_PATH)/lib/libwasmer.a
	@rm /tmp/$(WASMER_PKG_FILE_2)
	@rm -rf /tmp/wasmer-2
else
endif
endif
endif

# ------------------------------------------------------------------------------
# Dependency - Download Node.js for MinGW

ifeq ($(AS_DSP),true)
ifeq ($(MSYS_MINGW),true)
NPM_OPT_SET_PATH = export PATH=$$PATH:/opt/node && export NODE_SKIP_PLATFORM_CHECK=1
ifeq (,$(wildcard /opt/node))
NPM_VERSION = 16.6.0
NPM_FILENAME = node-v$(NPM_VERSION)-win-x64.zip
NPM_URL = https://nodejs.org/dist/v$(NPM_VERSION)/$(NPM_FILENAME)

TARGETS += /opt/node/npm

/opt/node/npm:
	@echo Downloading Node.js
	@wget -4 -P /tmp $(NPM_URL)
	@unzip -o /tmp/$(NPM_FILENAME) -d /opt
	@mv /opt/$(basename $(NPM_FILENAME)) /opt/node
	@rm /tmp/$(NPM_FILENAME)
endif
endif
endif

# ------------------------------------------------------------------------------
# Dependency - Download Edge WebView2

ifeq ($(WEB_UI),true)
ifeq ($(WINDOWS),true)
EDGE_WEBVIEW2_PATH = $(HIPHOP_VENDOR_PATH)/Microsoft.Web.WebView2

TARGETS += $(EDGE_WEBVIEW2_PATH)

NUGET_URL = https://dist.nuget.org/win-x86-commandline/latest/nuget.exe

ifeq ($(MSYS_MINGW),true)
$(EDGE_WEBVIEW2_PATH): /usr/bin/nuget.exe
else
$(EDGE_WEBVIEW2_PATH):
ifeq (,$(shell which nuget 2>/dev/null))
$(error NuGet not found, try sudo apt install nuget or the equivalent for your distro)
endif
endif
	@echo Downloading Edge WebView2 SDK
	@mkdir -p $(HIPHOP_VENDOR_PATH)
	@eval $(MSYS_MINGW_SYMLINKS)
	@nuget install Microsoft.Web.WebView2 -OutputDirectory $(HIPHOP_VENDOR_PATH)
	@ln -rs $(EDGE_WEBVIEW2_PATH).* $(EDGE_WEBVIEW2_PATH)

/usr/bin/nuget.exe:
	@echo Downloading NuGet
	@wget -4 -P /usr/bin $(NUGET_URL)
endif
endif

# ------------------------------------------------------------------------------
# Dependency - Built-in JavaScript library include and polyfills

ifeq ($(WEB_UI),true)
UI_JS_PATH = $(HIPHOP_SRC_PATH)/ui/client/distrho-ui.js
UI_JS_INCLUDE_PATH = $(UI_JS_PATH).inc

TARGETS += $(UI_JS_INCLUDE_PATH)

$(UI_JS_INCLUDE_PATH): $(UI_JS_PATH)
	@echo 'R"JS(' > $(UI_JS_INCLUDE_PATH)
	@cat $(UI_JS_PATH) >> $(UI_JS_INCLUDE_PATH)
	@echo ')JS"' >> $(UI_JS_INCLUDE_PATH)

ifeq ($(MACOS),true)
POLYFILL_JS_PATH = $(HIPHOP_SRC_PATH)/ui/client/event-target-polyfill.js
POLYFILL_JS_INCLUDE_PATH = $(POLYFILL_JS_PATH).inc

TARGETS += $(POLYFILL_JS_INCLUDE_PATH)

$(POLYFILL_JS_INCLUDE_PATH): $(POLYFILL_JS_PATH)
	@echo 'R"JS(' > $(POLYFILL_JS_INCLUDE_PATH)
	@cat $(POLYFILL_JS_PATH) >> $(POLYFILL_JS_INCLUDE_PATH)
	@echo ')JS"' >> $(POLYFILL_JS_INCLUDE_PATH)

endif
endif

# ------------------------------------------------------------------------------
# Linux only - Build webview helper binary

ifeq ($(WEB_UI),true)
ifeq ($(LINUX),true)
LXWEBVIEW_TYPE ?= gtk

ifeq ($(LXWEBVIEW_TYPE),gtk)
BASE_FLAGS += -DLXWEBVIEW_GTK
endif
ifeq ($(LXWEBVIEW_TYPE),cef)
BASE_FLAGS += -DLXWEBVIEW_CEF
endif

HIPHOP_TARGET += lxhelper_bin

LXHELPER_NAME = ui-helper
LXHELPER_BUILD_DIR = $(BUILD_DIR)/helper

include $(HIPHOP_SRC_PATH)/linux/Makefile.$(LXWEBVIEW_TYPE).mk
endif
endif

# ------------------------------------------------------------------------------
# Mac only - Build Objective-C++ files

ifeq ($(MACOS),true)
$(BUILD_DIR)/%.mm.o: %.mm
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@
endif

# ------------------------------------------------------------------------------
# Windows only - Build resource files

ifeq ($(WINDOWS),true)
$(BUILD_DIR)/%.rc.o: %.rc
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	@windres --input $< --output $@ --output-format=coff
endif

# ------------------------------------------------------------------------------
# Post build - Copy Linux helper files

ifeq ($(WEB_UI),true)
ifeq ($(LINUX),true)
HIPHOP_TARGET += lxhelper_res

lxhelper_res:
	@echo "Copying UI helper files"
	@($(TEST_LV2) \
		&& mkdir -p $(LIB_DIR_LV2) \
		&& cp -ru $(LXHELPER_FILES) $(LIB_DIR_LV2) \
		) || true
	@($(TEST_VST3) \
		&& mkdir -p $(LIB_DIR_VST3) \
		&& cp -ru $(LXHELPER_FILES) $(LIB_DIR_VST3) \
		) || true
	@($(TEST_NOBUNDLE) \
		&& mkdir -p $(LIB_DIR_NOBUNDLE) \
		&& cp -ru $(LXHELPER_FILES) $(LIB_DIR_NOBUNDLE) \
		) || true
endif
endif

# ------------------------------------------------------------------------------
# Post build - Compile AssemblyScript project

ifneq ($(AS_DSP),)
ifneq ($(HIPHOP_AS_SKIP_FRAMEWORK_FILES),true)
HIPHOP_TARGET += framework_as

AS_ASSEMBLY_PATH = $(HIPHOP_AS_DSP_PATH)/assembly

framework_as:
	@test -f $(AS_ASSEMBLY_PATH)/index.ts \
		|| ln -s $(abspath $(HIPHOP_SRC_PATH)/plugin/client/index.ts) $(AS_ASSEMBLY_PATH)
	@test -f $(AS_ASSEMBLY_PATH)/distrho-plugin.ts \
		|| ln -s $(abspath $(HIPHOP_SRC_PATH)/plugin/client/distrho-plugin.ts) $(AS_ASSEMBLY_PATH)
endif

WASM_MODULE = optimized.wasm
WASM_SRC_PATH = $(HIPHOP_AS_DSP_PATH)/build/$(WASM_MODULE)

HIPHOP_TARGET += $(WASM_SRC_PATH)

$(WASM_SRC_PATH): $(AS_ASSEMBLY_PATH)/plugin.ts
	@echo "Building AssemblyScript project"
	@# npm --prefix fails on MinGW due to paths mixing \ and /
	@test -d $(HIPHOP_AS_DSP_PATH)/node_modules \
		|| (cd $(HIPHOP_AS_DSP_PATH) && $(NPM_OPT_SET_PATH) && npm install)
	@cd $(HIPHOP_AS_DSP_PATH) && $(NPM_OPT_SET_PATH) && npm run asbuild
endif

# ------------------------------------------------------------------------------
# Post build - Always copy AssemblyScript DSP binary

ifneq ($(AS_DSP),)
HIPHOP_TARGET += lib_dsp

lib_dsp:
	@echo "Copying WebAssembly DSP binary"
	@($(TEST_LV2) \
		&& mkdir -p $(LIB_DIR_LV2)/dsp \
		&& cp -r $(WASM_SRC_PATH) $(LIB_DIR_LV2)/dsp/$(WASM_MODULE) \
		) || true
	@($(TEST_VST3) \
		&& mkdir -p $(LIB_DIR_VST3)/dsp \
		&& cp -r $(WASM_SRC_PATH) $(LIB_DIR_VST3)/dsp/$(WASM_MODULE) \
		) || true
	@($(TEST_VST2_MACOS) \
		&& mkdir -p $(LIB_DIR_VST2_MACOS)/dsp \
		&& cp -r $(WASM_SRC_PATH) $(LIB_DIR_VST2_MACOS)/dsp/$(WASM_MODULE) \
		) || true
	@($(TEST_NOBUNDLE) \
		&& mkdir -p $(LIB_DIR_NOBUNDLE)/dsp \
		&& cp -r $(WASM_SRC_PATH) $(LIB_DIR_NOBUNDLE)/dsp/$(WASM_MODULE) \
		) || true
endif

# ------------------------------------------------------------------------------
# Post build - Always copy web UI files

ifeq ($(WEB_UI),true)
HIPHOP_TARGET += lib_ui

lib_ui:
	@echo "Copying web UI files"
	@($(TEST_LV2) \
		&& mkdir -p $(LIB_DIR_LV2)/ui \
		&& cp -r $(HIPHOP_WEB_UI_PATH)/* $(LIB_DIR_LV2)/ui \
		) || true
	@($(TEST_VST3) \
		&& mkdir -p $(LIB_DIR_VST3)/ui \
		&& cp -r $(HIPHOP_WEB_UI_PATH)/* $(LIB_DIR_VST3)/ui \
		) || true
	@($(TEST_VST2_MACOS) \
		&& mkdir -p $(LIB_DIR_VST2_MACOS)/ui \
		&& cp -r $(HIPHOP_WEB_UI_PATH)/* $(LIB_DIR_VST2_MACOS)/ui \
		) || true
	@($(TEST_NOBUNDLE) \
		&& mkdir -p $(LIB_DIR_NOBUNDLE)/ui \
		&& cp -r $(HIPHOP_WEB_UI_PATH)/* $(LIB_DIR_NOBUNDLE)/ui \
		) || true

clean: clean_lib

clean_lib:
	@rm -rf $(LIB_DIR_NOBUNDLE)
endif

# ------------------------------------------------------------------------------
# Post build - Copy Windows Edge WebView2 DLL, currently only 64-bit is supported

ifeq ($(WEB_UI),true)
ifeq ($(WINDOWS),true)
HIPHOP_TARGET += edge_lib
WEBVIEW_DLL = $(EDGE_WEBVIEW2_PATH)/runtimes/win-x64/native/WebView2Loader.dll

edge_lib:
	@($(TEST_LV2) \
		&& mkdir -p $(LIB_DIR_LV2) \
		&& cp $(WEBVIEW_DLL) $(LIB_DIR_LV2) \
		) || true
	@($(TEST_VST3) \
		&& mkdir -p $(LIB_DIR_VST3) \
		&& cp $(WEBVIEW_DLL) $(LIB_DIR_VST3) \
		) || true
	@($(TEST_NOBUNDLE) \
		&& mkdir -p $(LIB_DIR_NOBUNDLE) \
		&& cp $(WEBVIEW_DLL) $(LIB_DIR_NOBUNDLE) \
		) || true
endif
endif

# ------------------------------------------------------------------------------
# Post build - Create LV2 manifest files

ifneq ($(CROSS_COMPILING),true)
CAN_GENERATE_TTL = true
else ifneq ($(EXE_WRAPPER),)
CAN_GENERATE_TTL = true
endif

ifeq ($(CAN_GENERATE_TTL),true)
HIPHOP_TARGET += lv2ttl

lv2ttl: $(DPF_PATH)/utils/lv2_ttl_generator
	@# TODO - generate-ttl.sh expects hardcoded directory bin/
	@cd $(DPF_TARGET_DIR)/.. && $(abspath $(DPF_PATH))/utils/generate-ttl.sh

$(DPF_PATH)/utils/lv2_ttl_generator:
	$(MAKE) -C $(DPF_PATH)/utils/lv2-ttl-generator
endif
