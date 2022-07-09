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

#ifndef PATH_HPP
#define PATH_HPP

#if DISTRHO_OS_LINUX
# include <cstring>
# include <pwd.h>
# include <unistd.h>
# include <sys/stat.h>
#endif
#if DISTRHO_OS_MAC
# include <cstring>
# include <dlfcn.h>
# include <libgen.h>
# include <sys/stat.h>
# include <sysdir.h>
# include <wordexp.h>
#endif
#if DISTRHO_OS_WINDOWS
# include <cstring>
# include <errhandlingapi.h>
# include <shlobj.h>
# include <shlwapi.h>
# include <shtypes.h>
#endif

#include "distrho/extra/String.hpp"

#include "extra/macro.h"

START_NAMESPACE_DISTRHO

enum class PluginFormat { Unknown, Jack, LV2, VST2, VST3 };

namespace PathSubdirectory {

    const String bundleLibrary = String("lib");
    const String nonBundleLibrary = String(XSTR(HIPHOP_PLUGIN_BIN_BASENAME) "-lib");
    const String cache = String("cache");

}

struct Path
{
    static const char* getPluginBinary() noexcept
    {
        static String filename;
#ifdef DISTRHO_OS_WINDOWS
        CHAR buf[MAX_PATH];
        buf[0] = '\0';
        GetModuleFileNameA((HINSTANCE)&__ImageBase, buf, sizeof(buf));
        filename = buf;
#else
        // dladdr() works for executables and dynamic libs on Linux and macOS
        Dl_info info;
        dladdr((void *)&__PRETTY_FUNCTION__, &info);
        char buf[PATH_MAX];
        filename = realpath(info.dli_fname, buf);
#endif
        return filename;
    }

    static String getPluginLibrary() noexcept
    {
        String path = String(getPluginBinary());
        path.truncate(path.rfind(DISTRHO_OS_SEP));

        switch (getPluginFormat()) {
            case PluginFormat::LV2:
                return path + DISTRHO_OS_SEP_STR + PathSubdirectory::bundleLibrary;
            case PluginFormat::VST2:
#if defined(DISTRHO_OS_LINUX) || defined(DISTRHO_OS_WINDOWS)
                return path + DISTRHO_OS_SEP_STR + PathSubdirectory::nonBundleLibrary;
#endif
            case PluginFormat::VST3:
                return path.truncate(path.rfind(DISTRHO_OS_SEP)) + DISTRHO_OS_SEP_STR + "Resources";
            default:
                break;
        }

        return path + DISTRHO_OS_SEP_STR + PathSubdirectory::nonBundleLibrary;
    }

    static PluginFormat getPluginFormat() noexcept
    {
        PluginFormat format = PluginFormat::Unknown;

#if DISTRHO_OS_LINUX
        char exePath[PATH_MAX];
        exePath[0] = '\0';
        ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
        if (len == -1) {
            d_stderr2("Could not determine executable path");
            return format;
        }

        if (strcmp(exePath, getPluginBinary()) == 0) {
            format = PluginFormat::Jack;
        } else {
            void* handle = dlopen(imgPath, RTLD_LAZY | RTLD_NOLOAD);
            String path(dirname(imgPath)); 

            if (handle != 0) {
                if ((dlsym(handle, "lv2ui_descriptor") != 0) || dlsym(handle, "lv2_descriptor") != 0) {
                    format = PluginFormat::LV2;
                } else if (dlsym(handle, "main") != 0) {
                    format = PluginFormat::VST2;
                } else if (dlsym(handle, "GetPluginFactory") != 0) {
                    format = PluginFormat::VST2;
                }

                dlclose(handle);
            }
        }
#elif DISTRHO_OS_MAC
        char filename[PATH_MAX];
        strcpy(filename, getPluginBinary());
        String path(dirname(filename));
        void* handle = dlopen(filename, RTLD_LAZY | RTLD_NOLOAD);

        // dlopen() returns 0 for the standalone executable on macOS
        if (handle == 0) {
            format = PluginFormat::Jack;
        } else {
            if ((dlsym(handle, "lv2_descriptor") != 0) || dlsym(handle, "lv2ui_descriptor") != 0) {
                format = PluginFormat::LV2;
            } else if (dlsym(handle, "VSTPluginMain") != 0) {
                format = PluginFormat::VST2;
            } else if (dlsym(handle, "GetPluginFactory") != 0) {
                format = PluginFormat::VST2;
            }

            dlclose(handle);
        }
#elif DISTRHO_OS_WINDOWS
        // HISTANCE and HMODULE are interchangeable
        // https://stackoverflow.com/questions/2126657/how-can-i-get-hinstance-from-a-dll
        HMODULE hm = (HMODULE)&__ImageBase;

        if ((GetProcAddress(hm, "lv2_descriptor") != 0) || (GetProcAddress(hm, "lv2ui_descriptor") != 0)) {
            format = PluginFormat::LV2;
        } else if (GetProcAddress(hm, "VSTPluginMain") != 0) {
            format = PluginFormat::VST2;
        } else if (GetProcAddress(hm, "GetPluginFactory") != 0) {
            format = PluginFormat::VST3;
        } else {
            format = PluginFormat::Jack;
        }
#endif
        return format;
    }

    static String getUserData() noexcept
    {
#if DISTRHO_OS_LINUX
        String path;
        struct passwd *pw = getpwuid(getuid());
        path += pw->pw_dir;
        path += "/.config/" XSTR(HIPHOP_PLUGIN_BIN_BASENAME);
        mkdir(path, 0777);
        path += "/" + PathSubdirectory::cache;
        mkdir(path, 0777);

        return path;
#elif DISTRHO_OS_MAC
        // Getting system directories without calling Objective-C
        // https://zameermanji.com/blog/2021/7/7/getting-standard-macos-directories/
        // https://www.manpagez.com/man/3/sysdir/
        char cachesPath[PATH_MAX];

        sysdir_search_path_enumeration_state state =
            sysdir_start_search_path_enumeration(SYSDIR_DIRECTORY_CACHES, SYSDIR_DOMAIN_MASK_USER);
        state = sysdir_get_next_search_path_enumeration(state, cachesPath);
        if (state == 0) {
            d_stderr2("Could not determine user caches directory");
            return String();
        }

        wordexp_t exp_result;
        wordexp(cachesPath, &exp_result, 0); // tilde expansion
        String path = String(exp_result.we_wordv[0]) + "/" XSTR(HIPHOP_PLUGIN_BIN_BASENAME);
        wordfree(&exp_result);

        mkdir(path, 0777);

        return path;
#elif DISTRHO_OS_WINDOWS
        // Get path inside user files folder: C:\Users\< USERNAME >\AppData\Local\PluginName\cache
        char dataPath[MAX_PATH];
        const HRESULT result = SHGetFolderPathA(0, CSIDL_LOCAL_APPDATA, 0, SHGFP_TYPE_DEFAULT, dataPath);
        
        if (FAILED(result)) {
            d_stderr2("Could not determine user app data folder - %x", result);
            return String();
        }

        String path = String(dataPath) + "\\" XSTR(HIPHOP_PLUGIN_BIN_BASENAME) "\\" + PathSubdirectory::cache;

        // Append host executable name to the cache path otherwise WebView2 controller initialization
        // fails with HRESULT 0x8007139f when trying to load plugin into more than a single host
        // simultaneously due to permissions. C:\Users\< USERNAME >\AppData\Local\PluginName\cache\< HOST_BIN >
        char exePath[MAX_PATH];
        
        if (GetModuleFileNameA(0, exePath, sizeof(exePath)) == 0) {
            d_stderr2("Could not determine host executable path - %x", GetLastError());
            return String();
        }

        LPSTR exeFilename = PathFindFileNameA(exePath);

        // The following call relies on a further Windows library called Pathcch, which is implemented
        // in api-ms-win-core-path-l1-1-0.dll and requires Windows 8.
        // Since the minimum plugin target is Windows 7 it is acceptable to use a deprecated function.
        //PathCchRemoveExtension(exeFilename, sizeof(exeFilename));
        PathRemoveExtensionA(exeFilename);
        path += "\\";
        path += exeFilename;

        return path;
#endif
    }

}; // struct path 

END_NAMESPACE_DISTRHO

#endif  // PATH_HPP
