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

#include <cstdio>
#include <cstring>
#include <iostream>

#include "WasmRuntime.hpp"

#define MAX_STRING_SIZE    1024
#define MAX_HOST_FUNCTIONS 1024

WasmRuntime::WasmRuntime()
    : fEngine(nullptr)
    , fStore(nullptr)
    , fModule(nullptr)
    , fInstance(nullptr)
#if HIPHOP_PLUGIN_WASM_WASI
    , fWasiEnv(nullptr)
#endif
{
    std::memset(&fExportsVec, 0, sizeof(fExportsVec));

    fEngine = fLib.wasm_engine_new();
    if (fEngine == nullptr) {
        throw wasm_runtime_exception("wasm_engine_new() failed");
    }

    fStore = fLib.wasm_store_new(fEngine); 
    if (fStore == nullptr) {
        fLib.wasm_engine_delete(fEngine);
        throw wasm_runtime_exception("wasm_store_new() failed");
    }

#if defined(HIPHOP_WASM_RUNTIME_WAMR)
    sWamrEngineRefCount++;
#endif
}

WasmRuntime::~WasmRuntime()
{
    if (hasInstance()) {
        destroyInstance();
    }

    if (fStore != nullptr) {
        fLib.wasm_store_delete(fStore);
        fStore = nullptr;
    }

#if defined(HIPHOP_WASM_RUNTIME_WAMR)
    if (--sWamrEngineRefCount > 0) {
        // Calling wasm_engine_delete() also tears down the full WAMR runtime.
        // There is ongoing discussion on how to fix this:
        // https://github.com/bytecodealliance/wasm-micro-runtime/pull/1001
        return;
    }
#endif

    if (fEngine != nullptr) {
        fLib.wasm_engine_delete(fEngine);
        fEngine = nullptr;
    }
}

void WasmRuntime::load(const char* modulePath)
{
    if (hasInstance()) {
        destroyInstance();
    }

    std::FILE* file = std::fopen(modulePath, "rb");
    if (file == nullptr) {
        throw wasm_module_exception("Error opening module file");
    }

    std::fseek(file, 0L, SEEK_END);
    const size_t fileSize = std::ftell(file);
    std::fseek(file, 0L, SEEK_SET);

    wasm_byte_vec_t moduleBytes;
    fLib.wasm_byte_vec_new_uninitialized(&moduleBytes, fileSize);
    
    size_t bytesRead = std::fread(moduleBytes.data, 1, fileSize, file);
    std::fclose(file);

    if (bytesRead != fileSize) {
        fLib.wasm_byte_vec_delete(&moduleBytes);
        throw wasm_module_exception("Error reading module file");
    }

    // WINWASMERBUG : Following call crashes some hosts on Windows when using
    //                the Wasmer runtime, does not affect WAMR. See bugs.txt.
    fModule = fLib.wasm_module_new(fStore, &moduleBytes);
    fLib.wasm_byte_vec_delete(&moduleBytes);

    if (fModule == nullptr) {
        throw wasm_runtime_exception("wasm_module_new() failed");
    }
}

void WasmRuntime::load(const unsigned char* moduleData, size_t size)
{
    if (hasInstance()) {
        destroyInstance();
    }

    wasm_byte_vec_t moduleBytes;
    fLib.wasm_byte_vec_new_uninitialized(&moduleBytes, size);

    std::memcpy(moduleBytes.data, moduleData, size);

    fModule = fLib.wasm_module_new(fStore, &moduleBytes);
    fLib.wasm_byte_vec_delete(&moduleBytes);

    if (fModule == nullptr) {
        throw wasm_runtime_exception("wasm_module_new() failed");
    }
}

bool WasmRuntime::hasInstance()
{
    return fInstance != nullptr;
}

void WasmRuntime::createInstance(WasmFunctionMap hostFunctions)
{
    if (hasInstance()) {
        destroyInstance();
    }

    char name[MAX_STRING_SIZE];

#if HIPHOP_PLUGIN_WASM_WASI
    // Build a map of WASI imports
    // Call to wasi_get_imports() fails because of missing host imports, use
    // wasi_get_unordered_imports() https://github.com/wasmerio/wasmer/issues/2450

    wasi_config_t* config = wasi_config_new("dpf");
    fWasiEnv = wasi_env_new(config);

    if (fWasiEnv == nullptr) {
        throw wasm_runtime_exception("wasi_env_new() failed");
    }

    wasmer_named_extern_vec_t wasiImports;

    if (!wasi_get_unordered_imports(fStore, fModule, fWasiEnv, &wasiImports)) {
        throw wasm_runtime_exception("wasi_get_unordered_imports() failed");
    }

    std::unordered_map<std::string, int> wasiImportIndex;

    for (size_t i = 0; i < wasiImports.size; i++) {
        const wasmer_named_extern_t *ne = wasiImports.data[i];
        const wasm_name_t *wn = wasmer_named_extern_name(ne);
        std::memcpy(name, wn->data, wn->size);
        name[wn->size] = '\0';
        wasiImportIndex[name] = i;
    }
#endif // HIPHOP_ENABLE_WASI

    // Build module imports vector

    wasm_importtype_vec_t importTypes;
    fLib.wasm_module_imports(fModule, &importTypes);
    wasm_extern_vec_t imports;
    fLib.wasm_extern_vec_new_uninitialized(&imports, importTypes.size);

    std::unordered_map<std::string, int> importIndex;
    bool moduleNeedsWasi = false;

    for (size_t i = 0; i < importTypes.size; i++) {
        const wasm_name_t *wn = fLib.wasm_importtype_name(importTypes.data[i]);
        std::memcpy(name, wn->data, wn->size);
        name[wn->size] = '\0';
        importIndex[name] = i;

#if HIPHOP_PLUGIN_WASM_WASI
        if (wasiImportIndex.find(name) != wasiImportIndex.end()) {
            const wasmer_named_extern_t* ne = wasiImports.data[wasiImportIndex[name]];
            imports.data[i] = const_cast<wasm_extern_t *>(wasmer_named_extern_unwrap(ne));
        }
#endif
        if (!moduleNeedsWasi) {
            wn = fLib.wasm_importtype_module(importTypes.data[i]);
            std::memcpy(name, wn->data, wn->size);
            name[wn->size] = '\0';
            if (std::strstr(name, "wasi_") == name) { // eg, wasi_snapshot_preview1
                moduleNeedsWasi = true;
            }
        }
    }

    fLib.wasm_importtype_vec_delete(&importTypes);

#if HIPHOP_PLUGIN_WASM_WASI
    if (!moduleNeedsWasi) {
        throw wasm_module_exception("WASI is enabled but module is not WASI compliant");
    }
#else
    if (moduleNeedsWasi) {
        throw wasm_module_exception("WASI is not enabled but module requires WASI");
    }
#endif

    // Insert host functions into imports vector

    // Avoid reallocation to ensure pointers to elements remain valid through engine lifetime
    fHostFunctions.reserve(MAX_HOST_FUNCTIONS);

    for (WasmFunctionMap::const_iterator it = hostFunctions.begin(); it != hostFunctions.end(); ++it) {
        fHostFunctions.push_back(it->second.function);

        wasm_valtype_vec_t params;
        toCValueTypeVector(it->second.params, &params);
        wasm_valtype_vec_t result;
        toCValueTypeVector(it->second.result, &result);

        const wasm_functype_t* funcType = fLib.wasm_functype_new(&params, &result);
        wasm_func_t* func = fLib.wasm_func_new_with_env(fStore, funcType, WasmRuntime::callHostFunction,
                                                        &fHostFunctions.back(), nullptr);
        imports.data[importIndex[it->first]] = fLib.wasm_func_as_extern(func);

        fLib.wasm_valtype_vec_delete(&result);
        fLib.wasm_valtype_vec_delete(&params);
    }

    // Create instance and start WASI

    fInstance = fLib.wasm_instance_new(fStore, fModule, &imports, nullptr);

    fLib.wasm_extern_vec_delete(&imports);

    if (fInstance == nullptr) {
        throw wasm_runtime_exception("wasm_instance_new() failed");
    }
    
#if HIPHOP_PLUGIN_WASM_WASI
    wasm_func_t* wasiStart = wasi_get_start_function(fInstance);
    
    if (wasiStart == nullptr) {
        throw wasm_runtime_exception("wasi_get_start_function() failed");
    }

    wasm_val_vec_t empty_val_vec = WASM_EMPTY_VEC;
    fLib.wasm_func_call(wasiStart, &empty_val_vec, &empty_val_vec);
    fLib.wasm_func_delete(wasiStart);
#endif

    // Build a map of externs indexed by name

    fExportsVec.size = 0;
    fLib.wasm_instance_exports(fInstance, &fExportsVec);
    wasm_exporttype_vec_t exportTypes;
    fLib.wasm_module_exports(fModule, &exportTypes);

    for (size_t i = 0; i < fExportsVec.size; i++) {
        const wasm_name_t *wn = fLib.wasm_exporttype_name(exportTypes.data[i]);
        std::memcpy(name, wn->data, wn->size);
        name[wn->size] = '\0';
        fModuleExports[name] = fExportsVec.data[i];
    }

    fLib.wasm_exporttype_vec_delete(&exportTypes);
}

void WasmRuntime::destroyInstance()
{
    if (fModule != nullptr) {
        fLib.wasm_module_delete(fModule);
        fModule = nullptr;
    }

#if HIPHOP_PLUGIN_WASM_WASI
    if (fWasiEnv != nullptr) {
        wasi_env_delete(fWasiEnv);
        fWasiEnv = nullptr;
    }
#endif

    if (fExportsVec.size != 0) {
        fLib.wasm_extern_vec_delete(&fExportsVec);
        fExportsVec.size = 0;
    }

    if (fInstance != nullptr) {
        fLib.wasm_instance_delete(fInstance);
        fInstance = nullptr;
    }

    fHostFunctions.clear();
    fModuleExports.clear();
}

byte_t* WasmRuntime::getMemory(const WasmValue& wPtr)
{
    return fLib.wasm_memory_data(
        fLib.wasm_extern_as_memory(fModuleExports["memory"])
    ) + wPtr.of.i32;
}

char* WasmRuntime::getMemoryAsCString(const WasmValue& wPtr)
{
    return static_cast<char *>(getMemory(wPtr));
}

void WasmRuntime::copyCStringToMemory(const WasmValue& wPtr, const char* s)
{
    std::strcpy(getMemoryAsCString(wPtr), s);
}

WasmValue WasmRuntime::getGlobal(const char* name)
{
    wasm_val_t value;
    fLib.wasm_global_get(fLib.wasm_extern_as_global(fModuleExports[name]), &value);
    return value;
}

void WasmRuntime::setGlobal(const char* name, const WasmValue& value)
{
    fLib.wasm_global_set(fLib.wasm_extern_as_global(fModuleExports[name]), &value);
}

char* WasmRuntime::getGlobalAsCString(const char* name)
{
    return getMemoryAsCString(getGlobal(name));
}

WasmValueVector WasmRuntime::callFunction(const char* name, WasmValueVector params)
{
    const wasm_func_t* func = fLib.wasm_extern_as_func(fModuleExports[name]);

    // wasm_val_vec_t is implemented differently for each runtime type.
    // Is there a generic way to create an arbitrary sized instance?
    wasm_val_vec_t paramsVec;
    paramsVec.size = params.size();
    paramsVec.data = params.data();
#if defined(HIPHOP_WASM_RUNTIME_WAMR)
    paramsVec.num_elems = params.size();
    paramsVec.size_of_elem = sizeof(wasm_val_t);
#endif
    wasm_val_t resultArray[1] = { WASM_INIT_VAL };
    wasm_val_vec_t resultVec = WASM_ARRAY_VEC(resultArray);

    const wasm_trap_t* trap = fLib.wasm_func_call(func, &paramsVec, &resultVec);

    if (trap != nullptr) {
        std::string s = std::string("Failed call to function ") + name;

        wasm_message_t* wm = nullptr;
        fLib.wasm_trap_message(trap, wm);

        if (wm != nullptr) {
            s += std::string(" - trap message: ") + std::string(wm->data /*null terminated*/);
        }

        throw wasm_runtime_exception(s);
    }

    return WasmValueVector(resultVec.data, resultVec.data + resultVec.size);
}

WasmValue WasmRuntime::callFunctionReturnSingleValue(const char* name, WasmValueVector params)
{
    return callFunction(name, params)[0];
}

const char* WasmRuntime::callFunctionReturnCString(const char* name, WasmValueVector params)
{
    return getMemoryAsCString(callFunctionReturnSingleValue(name, params));
}

wasm_trap_t* WasmRuntime::callHostFunction(void* env, const wasm_val_vec_t* paramsVec, wasm_val_vec_t* resultVec)
{
    const WasmFunction* func = static_cast<WasmFunction *>(env);
    const WasmValueVector params (paramsVec->data, paramsVec->data + paramsVec->size);
    const WasmValueVector result = (*func)(params);

    for (size_t i = 0; i < resultVec->size; i++) {
        resultVec->data[i] = result[i];
    }

    return nullptr;
}

own void WasmRuntime::toCValueTypeVector(WasmValueKindVector kinds, own wasm_valtype_vec_t* out)
{
    int i = 0;
    const size_t size = kinds.size();
    wasm_valtype_t* typesArray[size];

    for (WasmValueKindVector::const_iterator it = kinds.cbegin(); it != kinds.cend(); ++it) {
        typesArray[i++] = fLib.wasm_valtype_new(*it);
    }

    fLib.wasm_valtype_vec_new(out, size, typesArray);
}

const char* WasmRuntime::WTF16ToCString(const WasmValue& wPtr)
{
    if (fModuleExports.find("wtf16_to_c_string") == fModuleExports.end()) {
        throw wasm_module_exception("Wasm module does not export function _wtf16_to_c_string");
    }

    return callFunctionReturnCString("wtf16_to_c_string", { wPtr });
}

WasmValue WasmRuntime::CToWTF16String(const char* s)
{
    if (fModuleExports.find("c_to_wtf16_string") == fModuleExports.end()) {
        throw wasm_module_exception("Wasm module does not export function _c_to_wtf16_string");
    }

    const WasmValue wPtr = getGlobal("_rw_string_0");

    copyCStringToMemory(wPtr, s);

    return callFunctionReturnSingleValue("c_to_wtf16_string", { wPtr });
}

#if defined(HIPHOP_WASM_RUNTIME_WAMR)
int WasmRuntime::sWamrEngineRefCount = 0;
#endif
