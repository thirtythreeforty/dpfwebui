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

#ifndef WASM_C_API
#define WASM_C_API

#include "src/DistrhoDefines.h"

#include "extra/macro.h"

#ifdef HIPHOP_WASM_DLL
# include "extra/Path.hpp"
# ifdef DISTRHO_OS_WINDOWS
#  include <libloaderapi.h>
# endif
#endif

#ifdef HIPHOP_WASM_RUNTIME_WAMR
# include "wasm_c_api.h"
#elif HIPHOP_WASM_RUNTIME_WASMER
# ifdef DISTRHO_OS_WINDOWS
#  define WASM_API_EXTERN // allow to link against static lib
# endif
# include "wasm.h"
# include "wasmer.h"
#else
# error "Invalid WebAssembly runtime specified"
#endif

// WASM C API convention. See Ownership section in wasm_c_api.h.
#define own

START_NAMESPACE_DISTRHO

#ifdef HIPHOP_WASM_DLL
# define DLL_SYMBOL(name,type) reinterpret_cast<type>(getLibrarySymbol(name))
#endif

#if defined(__GNUC__) && (__GNUC__ >= 9)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

// This class allows to easily link against a static or dynamic WASM library.
// Only a subset of the C API is implemented.

class WasmCApi
{
public:
#ifdef HIPHOP_WASM_DLL
    WasmCApi()
    {
        String path = Path::getPluginLibrary() + "\\" XSTR(HIPHOP_WASM_DLL);
# ifdef DISTRHO_OS_WINDOWS
        fDllHandle = LoadLibrary(path);
# endif
    }

    ~WasmCApi()
    {
        if (fDllHandle != nullptr) {
# ifdef DISTRHO_OS_WINDOWS
            FreeLibrary(fDllHandle);
# endif
            fDllHandle = nullptr;
        }
    }
#endif

    //
    // Engine
    //

    inline own wasm_engine_t* wasm_engine_new(void)
    {
#ifdef HIPHOP_WASM_DLL
        typedef own wasm_engine_t* (*FuncType)();
        return DLL_SYMBOL(__FUNCTION__,FuncType)();
#else
        return ::wasm_engine_new();
#endif
    }

    inline void wasm_engine_delete(own wasm_engine_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(own wasm_engine_t*);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        ::wasm_engine_delete(arg0);
#endif
    }

    //
    // Store
    //

    inline own wasm_store_t* wasm_store_new(wasm_engine_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef own wasm_store_t* (*FuncType)(wasm_engine_t*);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        return ::wasm_store_new(arg0);
#endif
    }

    inline void wasm_store_delete(own wasm_store_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(own wasm_store_t*);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        return ::wasm_store_delete(arg0);
#endif
    }

    //
    // Instance
    //

    inline own wasm_instance_t* wasm_instance_new(wasm_store_t* arg0, const wasm_module_t* arg1,
                                            const wasm_extern_vec_t * arg2, own wasm_trap_t** arg3)
    {
#ifdef HIPHOP_WASM_DLL
        typedef own wasm_instance_t* (*FuncType)(wasm_store_t*, const wasm_module_t*,
                                                    const wasm_extern_vec_t*, own wasm_trap_t**);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0, arg1, arg2, arg3);
#else
        return ::wasm_instance_new(arg0, arg1, arg2, arg3);
#endif
    }

    inline void wasm_instance_delete(own wasm_instance_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(own wasm_instance_t*);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        ::wasm_instance_delete(arg0);
#endif
    }

    inline void wasm_instance_exports(const wasm_instance_t* arg0, own wasm_extern_vec_t* arg1)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(const wasm_instance_t*, own wasm_extern_vec_t*);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0, arg1);
#else
        ::wasm_instance_exports(arg0, arg1);
#endif
    }

    //
    // Byte vector
    //

    inline void wasm_byte_vec_new_uninitialized(own wasm_byte_vec_t* arg0, size_t arg1)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(own wasm_byte_vec_t*, size_t);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0, arg1);
#else
        ::wasm_byte_vec_new_uninitialized(arg0, arg1);
#endif
    }

    inline void wasm_byte_vec_delete(own wasm_byte_vec_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(own wasm_byte_vec_t*);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        ::wasm_byte_vec_delete(arg0);
#endif
    }

    //
    // Module
    //

    inline own wasm_module_t* wasm_module_new(wasm_store_t* arg0, const wasm_byte_vec_t* arg1)
    {
#ifdef HIPHOP_WASM_DLL
        typedef own wasm_module_t* (*FuncType)(wasm_store_t*, const wasm_byte_vec_t*);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0, arg1);
#else
        return ::wasm_module_new(arg0, arg1);
#endif
    }

    inline void wasm_module_delete(own wasm_module_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(own wasm_module_t*);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        ::wasm_module_delete(arg0);
#endif
    }

    inline void wasm_module_imports(const wasm_module_t* arg0, own wasm_importtype_vec_t* arg1)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(const wasm_module_t*, own wasm_importtype_vec_t*);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0, arg1);
#else
        ::wasm_module_imports(arg0, arg1);
#endif
    }

    inline void wasm_module_exports(const wasm_module_t* arg0, own wasm_exporttype_vec_t* arg1)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(const wasm_module_t*, own wasm_exporttype_vec_t*);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0, arg1);
#else
        ::wasm_module_exports(arg0, arg1);
#endif
    }

    //
    // Import
    //

    inline const wasm_name_t* wasm_importtype_module(const wasm_importtype_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef const wasm_name_t* (*FuncType)(const wasm_importtype_t*);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        return ::wasm_importtype_module(arg0);
#endif
    }

    inline const wasm_name_t* wasm_importtype_name(const wasm_importtype_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef const wasm_name_t* (*FuncType)(const wasm_importtype_t*);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        return ::wasm_importtype_name(arg0);
#endif
    }

    inline void wasm_importtype_vec_delete(own wasm_importtype_vec_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(own wasm_importtype_vec_t*);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        ::wasm_importtype_vec_delete(arg0);
#endif
    }

    //
    // Export
    //

    inline const wasm_name_t* wasm_exporttype_name(const wasm_exporttype_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef const wasm_name_t* (*FuncType)(const wasm_exporttype_t*);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        return ::wasm_exporttype_name(arg0);
#endif
    }

    inline void wasm_exporttype_vec_delete(own wasm_exporttype_vec_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(own wasm_exporttype_vec_t*);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        ::wasm_exporttype_vec_delete(arg0);
#endif
    }

    //
    // Extern
    //

    inline void wasm_extern_vec_new_uninitialized(own wasm_extern_vec_t* arg0, size_t arg1)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(own wasm_extern_vec_t*, size_t);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0, arg1);
#else
        ::wasm_extern_vec_new_uninitialized(arg0, arg1);
#endif
    }

    inline void wasm_extern_vec_delete(own wasm_extern_vec_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(own wasm_extern_vec_t*);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        ::wasm_extern_vec_delete(arg0);
#endif
    }

    inline wasm_func_t* wasm_extern_as_func(wasm_extern_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef wasm_func_t* (*FuncType)(wasm_extern_t*);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        return ::wasm_extern_as_func(arg0);
#endif
    }

    inline wasm_global_t* wasm_extern_as_global(wasm_extern_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef wasm_global_t* (*FuncType)(wasm_extern_t*);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        return ::wasm_extern_as_global(arg0);
#endif
    }

    inline wasm_memory_t* wasm_extern_as_memory(wasm_extern_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef wasm_memory_t* (*FuncType)(wasm_extern_t*);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        return ::wasm_extern_as_memory(arg0);
#endif
    }

    //
    // Value
    //

    inline own wasm_valtype_t* wasm_valtype_new(wasm_valkind_t arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef own wasm_valtype_t* (*FuncType)(wasm_valkind_t);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        return ::wasm_valtype_new(arg0);
#endif
    }

    inline void wasm_valtype_vec_new(own wasm_valtype_vec_t* arg0, size_t arg1,
                                        own wasm_valtype_t* const arg2[])
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(own wasm_valtype_vec_t*, size_t, own wasm_valtype_t* const[]);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0, arg1, arg2);
#else
        ::wasm_valtype_vec_new(arg0, arg1, arg2);
#endif
    }

    inline void wasm_valtype_vec_delete(own wasm_valtype_vec_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(own wasm_valtype_vec_t*);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        ::wasm_valtype_vec_delete(arg0);
#endif
    }

    //
    // Function
    //

    inline own wasm_functype_t* wasm_functype_new(own wasm_valtype_vec_t* arg0,
                                                    own wasm_valtype_vec_t* arg1)
    {
#ifdef HIPHOP_WASM_DLL
        typedef own wasm_functype_t* (*FuncType)(own wasm_valtype_vec_t*, own wasm_valtype_vec_t*);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0, arg1);
#else
        return ::wasm_functype_new(arg0, arg1);
#endif
    }

    inline own wasm_func_t* wasm_func_new_with_env(wasm_store_t* arg0, const wasm_functype_t* arg1,
                                            wasm_func_callback_with_env_t arg2, void* arg3,
                                            void (*arg4)(void*))
    {
#ifdef HIPHOP_WASM_DLL
        typedef own wasm_func_t* (*FuncType)(wasm_store_t*, const wasm_functype_t*,
        wasm_func_callback_with_env_t, void*, void (*)(void*));
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0, arg1, arg2, arg3, arg4);
#else
        return ::wasm_func_new_with_env(arg0, arg1, arg2, arg3, arg4);
#endif
    }

    inline wasm_extern_t* wasm_func_as_extern(wasm_func_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef wasm_extern_t* (*FuncType)(wasm_func_t*);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        return ::wasm_func_as_extern(arg0);
#endif
    }

    inline own wasm_trap_t* wasm_func_call(const wasm_func_t* arg0, const wasm_val_vec_t* arg1,
                                            wasm_val_vec_t* arg2)
    {
#ifdef HIPHOP_WASM_DLL
        typedef own wasm_trap_t* (*FuncType)(const wasm_func_t*, const wasm_val_vec_t*,
                                            wasm_val_vec_t*);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0, arg1, arg2);
#else
        return ::wasm_func_call(arg0, arg1, arg2);
#endif
    }

    //
    // Memory
    //

    inline byte_t* wasm_memory_data(wasm_memory_t* arg0)
    {
#ifdef HIPHOP_WASM_DLL
        typedef byte_t* (*FuncType)(wasm_memory_t*);
        return DLL_SYMBOL(__FUNCTION__,FuncType)(arg0);
#else
        return ::wasm_memory_data(arg0);
#endif
    }

    //
    // Global
    //

    inline void wasm_global_get(const wasm_global_t* arg0, own wasm_val_t* arg1)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(const wasm_global_t*, own wasm_val_t*);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0, arg1);
#else
        ::wasm_global_get(arg0, arg1);
#endif
    }

    inline void wasm_global_set(wasm_global_t* arg0, const wasm_val_t* arg1)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(wasm_global_t* arg0, const wasm_val_t* arg1);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0, arg1);
#else
        ::wasm_global_set(arg0, arg1);
#endif
    }

    //
    // Trap
    //
    
    inline void wasm_trap_message(const wasm_trap_t* arg0, own wasm_message_t* arg1)
    {
#ifdef HIPHOP_WASM_DLL
        typedef void (*FuncType)(const wasm_trap_t*, own wasm_message_t*);
        DLL_SYMBOL(__FUNCTION__,FuncType)(arg0, arg1);
#else
        ::wasm_trap_message(arg0, arg1);
#endif
    }

#ifdef HIPHOP_WASM_DLL
private:
# ifdef DISTRHO_OS_WINDOWS
    inline void* getLibrarySymbol(const char* name)
    {
        return reinterpret_cast<void*>(GetProcAddress(fDllHandle, name));
    }

    HMODULE fDllHandle;
# endif
#endif
};

#if defined(__GNUC__) && (__GNUC__ >= 9)
# pragma GCC diagnostic pop
#endif

END_NAMESPACE_DISTRHO

#endif // WASM_C_API
