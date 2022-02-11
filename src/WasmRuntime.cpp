/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021 Luciano Iam <oss@lucianoiam.com>
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

#include <iostream>

#include "WasmRuntime.hpp"

#define MAX_STRING_SIZE    1024
#define MAX_HOST_FUNCTIONS 1024

// TODO - check for correct usage of the Wasm C API focusing on memory leaks

WasmRuntime::WasmRuntime()
    : fStarted(false)
    , fEngine(nullptr)
    , fStore(nullptr)
    , fModule(nullptr)
    , fInstance(nullptr)
#ifdef HIPHOP_ENABLE_WASI
    , fWasiEnv(nullptr)
#endif // HIPHOP_ENABLE_WASI
{
    memset(&fExportsVec, 0, sizeof(fExportsVec));

    fEngine = wasm_engine_new();
    if (fEngine == nullptr) {
        throw wasm_runtime_exception("wasm_engine_new() failed");
    }

    fStore = wasm_store_new(fEngine); 
    if (fStore == nullptr) {
        throw wasm_runtime_exception("wasm_store_new() failed");
    }

#ifdef HIPHOP_WASM_RUNTIME_WAMR
    sWamrRefCount++;
#endif // HIPHOP_WASM_RUNTIME_WAMR
}

WasmRuntime::~WasmRuntime()
{
    stop();
    unload();

    if (fStore != nullptr) {
        wasm_store_delete(fStore);
        fStore = nullptr;
    }

#ifdef HIPHOP_WASM_RUNTIME_WAMR
    if (--sWamrRefCount > 0) {
        // Calling wasm_engine_delete() also tears down the WAMR runtime
        return;
    }
#endif

    if (fEngine != nullptr) {
        wasm_engine_delete(fEngine);
        fEngine = nullptr;
    }
}

void WasmRuntime::load(const char* modulePath)
{
    unload();

    FILE* file = fopen(modulePath, "rb");
    if (file == nullptr) {
        throw wasm_module_exception("Error opening Wasm module file");
    }

    fseek(file, 0L, SEEK_END);
    const size_t fileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    wasm_byte_vec_t moduleBytes;
    wasm_byte_vec_new_uninitialized(&moduleBytes, fileSize);
    
    size_t bytesRead = fread(moduleBytes.data, 1, fileSize, file);
    fclose(file);

    if (bytesRead != fileSize) {
        wasm_byte_vec_delete(&moduleBytes);
        throw wasm_module_exception("Error reading Wasm module file");
    }

    // WINWASMERBUG : Following call crashes some hosts on Windows when using
    //                the Wasmer runtime, does not affect WAMR. See bugs.txt.
    fModule = wasm_module_new(fStore, &moduleBytes);
    wasm_byte_vec_delete(&moduleBytes);

    if (fModule == nullptr) {
        throw wasm_runtime_exception("wasm_module_new() failed");
    }
}

void WasmRuntime::load(const unsigned char* moduleData, size_t size)
{
    unload();

    wasm_byte_vec_t moduleBytes;
    wasm_byte_vec_new_uninitialized(&moduleBytes, size);

    std::memcpy(moduleBytes.data, moduleData, size);

    fModule = wasm_module_new(fStore, &moduleBytes);
    wasm_byte_vec_delete(&moduleBytes);

    if (fModule == nullptr) {
        throw wasm_runtime_exception("wasm_module_new() failed");
    }
}

void WasmRuntime::unload()
{
    if (fModule != nullptr) {
        wasm_module_delete(fModule);
        fModule = nullptr;
    }
}

void WasmRuntime::start(WasmFunctionMap hostFunctions)
{
    char name[MAX_STRING_SIZE];

#ifdef HIPHOP_ENABLE_WASI
    // -------------------------------------------------------------------------
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
        memcpy(name, wn->data, wn->size);
        name[wn->size] = '\0';
        wasiImportIndex[name] = i;
    }
#endif // HIPHOP_ENABLE_WASI

    // -------------------------------------------------------------------------
    // Build module imports vector

    wasm_importtype_vec_t importTypes;
    wasm_module_imports(fModule, &importTypes);
    wasm_extern_vec_t imports;
    wasm_extern_vec_new_uninitialized(&imports, importTypes.size);

    std::unordered_map<std::string, int> importIndex;
    bool moduleNeedsWasi = false;

    for (size_t i = 0; i < importTypes.size; i++) {
        const wasm_name_t *wn = wasm_importtype_name(importTypes.data[i]);
        memcpy(name, wn->data, wn->size);
        name[wn->size] = '\0';
        importIndex[name] = i;

#ifdef HIPHOP_ENABLE_WASI
        if (wasiImportIndex.find(name) != wasiImportIndex.end()) {
            const wasmer_named_extern_t* ne = wasiImports.data[wasiImportIndex[name]];
            imports.data[i] = const_cast<wasm_extern_t *>(wasmer_named_extern_unwrap(ne));
        }
#endif // HIPHOP_ENABLE_WASI
        if (!moduleNeedsWasi) {
            wn = wasm_importtype_module(importTypes.data[i]);
            memcpy(name, wn->data, wn->size);
            name[wn->size] = '\0';
            if (strstr(name, "wasi_") == name) { // eg, wasi_snapshot_preview1
                moduleNeedsWasi = true;
            }
        }
    }

    wasm_importtype_vec_delete(&importTypes);

#ifdef HIPHOP_ENABLE_WASI
    if (!moduleNeedsWasi) {
        throw wasm_module_exception("WASI is enabled but module is not WASI compliant");
    }
#else
    if (moduleNeedsWasi) {
        throw wasm_module_exception("WASI is not enabled but module requires WASI");
    }
#endif // HIPHOP_ENABLE_WASI

    // -------------------------------------------------------------------------
    // Insert host functions into imports vector

    // Avoid reallocation to ensure pointers to elements remain valid through engine lifetime
    fHostFunctions.reserve(MAX_HOST_FUNCTIONS);

    for (WasmFunctionMap::const_iterator it = hostFunctions.begin(); it != hostFunctions.end(); ++it) {
        fHostFunctions.push_back(it->second.function);

        wasm_valtype_vec_t params;
        toCValueTypeVector(it->second.params, &params);
        wasm_valtype_vec_t result;
        toCValueTypeVector(it->second.result, &result);

        const wasm_functype_t* funcType = wasm_functype_new(&params, &result);
        wasm_func_t* func = wasm_func_new_with_env(fStore, funcType, WasmRuntime::callHostFunction,
                                                    &fHostFunctions.back(), nullptr);
        imports.data[importIndex[it->first]] = wasm_func_as_extern(func);

        wasm_valtype_vec_delete(&result);
        wasm_valtype_vec_delete(&params);
    }

    // -------------------------------------------------------------------------
    // Create Wasm instance and start WASI

    fInstance = wasm_instance_new(fStore, fModule, &imports, nullptr);

    wasm_extern_vec_delete(&imports);

    if (fInstance == nullptr) {
        throw wasm_runtime_exception("wasm_instance_new() failed");
    }
    
#ifdef HIPHOP_ENABLE_WASI
    wasm_func_t* wasiStart = wasi_get_start_function(fInstance);
    
    if (wasiStart == nullptr) {
        throw wasm_runtime_exception("wasi_get_start_function() failed");
    }

    wasm_val_vec_t empty_val_vec = WASM_EMPTY_VEC;
    wasm_func_call(wasiStart, &empty_val_vec, &empty_val_vec);
    wasm_func_delete(wasiStart);
#endif // HIPHOP_ENABLE_WASI
    // -------------------------------------------------------------------------
    // Build a map of externs indexed by name

    fExportsVec.size = 0;
    wasm_instance_exports(fInstance, &fExportsVec);
    wasm_exporttype_vec_t exportTypes;
    wasm_module_exports(fModule, &exportTypes);

    for (size_t i = 0; i < fExportsVec.size; i++) {
        const wasm_name_t *wn = wasm_exporttype_name(exportTypes.data[i]);
        memcpy(name, wn->data, wn->size);
        name[wn->size] = '\0';
        fModuleExports[name] = fExportsVec.data[i];
    }

    wasm_exporttype_vec_delete(&exportTypes);

    // -------------------------------------------------------------------------
    // Startup complete

    fStarted = true;
}

void WasmRuntime::stop()
{
    fStarted = false;

#ifdef HIPHOP_ENABLE_WASI
    if (fWasiEnv != nullptr) {
        wasi_env_delete(fWasiEnv);
        fWasiEnv = nullptr;
    }
#endif // HIPHOP_ENABLE_WASI

    if (fExportsVec.size != 0) {
        wasm_extern_vec_delete(&fExportsVec);
        fExportsVec.size = 0;
    }

    if (fInstance != nullptr) {
        wasm_instance_delete(fInstance);
        fInstance = nullptr;
    }

    fHostFunctions.clear();
    fModuleExports.clear();
}

byte_t* WasmRuntime::getMemory(const WasmValue& wPtr)
{
    return wasm_memory_data(wasm_extern_as_memory(fModuleExports["memory"])) + wPtr.of.i32;
}

char* WasmRuntime::getMemoryAsCString(const WasmValue& wPtr)
{
    return static_cast<char *>(getMemory(wPtr));
}

void WasmRuntime::copyCStringToMemory(const WasmValue& wPtr, const char* s)
{
    strcpy(getMemoryAsCString(wPtr), s);
}

WasmValue WasmRuntime::getGlobal(const char* name)
{
    WasmValue value;
    wasm_global_get(wasm_extern_as_global(fModuleExports[name]), &value);
    return value;
}

void WasmRuntime::setGlobal(const char* name, const WasmValue& value)
{
    wasm_global_set(wasm_extern_as_global(fModuleExports[name]), &value);
}

char* WasmRuntime::getGlobalAsCString(const char* name)
{
    return getMemoryAsCString(getGlobal(name));
}

WasmValueVector WasmRuntime::callFunction(const char* name, WasmValueVector params)
{
    const wasm_func_t* func = wasm_extern_as_func(fModuleExports[name]);

    // wasm_val_vec_t is implemented differently for each runtime type.
    // Is there a generic way to create an arbitrary sized instance?
    wasm_val_vec_t paramsVec;
#ifdef HIPHOP_WASM_RUNTIME_WASMER
    paramsVec.size = params.size();
    paramsVec.data = params.data();
#endif
#ifdef HIPHOP_WASM_RUNTIME_WAMR
    paramsVec.size = sizeof(WasmValue) * params.size();
    paramsVec.data = params.data();
    paramsVec.num_elems = params.size();
    paramsVec.size_of_elem = sizeof(WasmValue);
#endif
    wasm_val_t resultArray[1] = { WASM_INIT_VAL };
    wasm_val_vec_t resultVec = WASM_ARRAY_VEC(resultArray);

    const wasm_trap_t* trap = wasm_func_call(func, &paramsVec, &resultVec);

    if (trap != nullptr) {
        std::string s = std::string("Failed call to function") + name;

        wasm_message_t* wm = nullptr;
        wasm_trap_message(trap, wm);

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

void WasmRuntime::toCValueTypeVector(WasmValueKindVector kinds, wasm_valtype_vec_t* types)
{
    int i = 0;
    const size_t size = kinds.size();
    wasm_valtype_t* typesArray[size];

    for (WasmValueKindVector::const_iterator it = kinds.cbegin(); it != kinds.cend(); ++it) {
        typesArray[i++] = wasm_valtype_new(*it);
    }

    wasm_valtype_vec_new(types, size, typesArray);
}

const char* WasmRuntime::WTF16ToCString(const WasmValue& wPtr)
{
    if (fModuleExports.find("_wtf16_to_c_string") == fModuleExports.end()) {
        throw wasm_module_exception("Wasm module does not export function _wtf16_to_c_string");
    }

    return callFunctionReturnCString("_wtf16_to_c_string", { wPtr });
}

WasmValue WasmRuntime::CToWTF16String(const char* s)
{
    if (fModuleExports.find("_c_to_wtf16_string") == fModuleExports.end()) {
        throw wasm_module_exception("Wasm module does not export function _c_to_wtf16_string");
    }

    const WasmValue wPtr = getGlobal("_rw_string_1");

    copyCStringToMemory(wPtr, s);

    return callFunctionReturnSingleValue("_c_to_wtf16_string", { wPtr });
}

#ifdef HIPHOP_WASM_RUNTIME_WAMR
int WasmRuntime::sWamrRefCount = 0;
#endif // HIPHOP_WASM_RUNTIME_WAMR
