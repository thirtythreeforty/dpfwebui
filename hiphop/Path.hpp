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

#include "distrho/extra/String.hpp"
#if ! (DISTRHO_OS_LINUX && CEF_HELPER_BINARY)
  // Including this file when compiling the CEF helper would involve adding
  // lots of dependencies from DPF, helper only needs to call getCachesPath().
  // The GTK-based helper does not need to call functions in LinuxPath.cpp.
# include "DistrhoPluginUtils.hpp"
#endif

#if DISTRHO_OS_LINUX
# include <cstring>
# include <pwd.h>
# include <unistd.h>
# include <sys/stat.h>
#endif
#if DISTRHO_OS_MAC
# include <cstring>
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

#include "macro.h"

START_NAMESPACE_DISTRHO

namespace path {

    const String kBundleLibrarySubdirectory = String("lib");
    const String kNoBundleLibrarySubdirectory = String(XSTR(PLUGIN_BIN_BASENAME) "-lib");
    const String kCacheSubdirectory = String("cache");

    inline String getLibraryPath()
    {
#if DISTRHO_OS_LINUX
        String path = String(getBinaryFilename());
        path.truncate(path.rfind('/'));

        const char* format = getPluginFormatName();

        if (strcmp(format, "LV2") == 0) {
            return path + "/" + kBundleLibrarySubdirectory;
        } else if (strcmp(format, "VST2") == 0) {
            return path + "/" + kNoBundleLibrarySubdirectory;
        } else if (strcmp(format, "VST3") == 0) {
            return path.truncate(path.rfind('/')) + "/Resources";
        }

        return path + "/" + kNoBundleLibrarySubdirectory;
#endif
#if DISTRHO_OS_MAC
        String path = String(getBinaryFilename());
        path.truncate(path.rfind('/'));

        const char* format = getPluginFormatName();

        if (strcmp(format, "LV2") == 0) {
            return path + "/" + kBundleLibrarySubdirectory;
        } else if ((strcmp(format, "VST2") == 0) || (strcmp(format, "VST3") == 0)) {
            return path.truncate(path.rfind('/')) + "/Resources";
        }

        return path + "/" + kNoBundleLibrarySubdirectory;
#endif
#if DISTRHO_OS_WINDOWS
        String path = String(getBinaryFilename());
        path.truncate(path.rfind('\\'));

        const char* format = getPluginFormatName();

        if (strcmp(format, "LV2") == 0) {
            return path + "\\" + kBundleLibrarySubdirectory;
        } else if (strcmp(format, "VST2") == 0) {
            return path + "\\" + kNoBundleLibrarySubdirectory;
        } else if (strcmp(format, "VST3") == 0) {
            return path.truncate(path.rfind('\\')) + "\\Resources";
        }

        return path + "\\" + kNoBundleLibrarySubdirectory;
#endif
    }

    inline String getCachesPath()
    {
#if DISTRHO_OS_LINUX
        String path;
        struct passwd *pw = getpwuid(getuid());
        path += pw->pw_dir;
        path += "/.config/" XSTR(PLUGIN_BIN_BASENAME);
        mkdir(path, 0777);
        path += "/" + kCacheSubdirectory;
        mkdir(path, 0777);

        return path;
#endif
#if DISTRHO_OS_MAC
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
        String path = String(exp_result.we_wordv[0]) + "/" XSTR(PLUGIN_BIN_BASENAME);
        wordfree(&exp_result);

        mkdir(path, 0777);

        return path;
#endif
#if DISTRHO_OS_WINDOWS
        // Get path inside user files folder: C:\Users\< USERNAME >\AppData\Local\PluginName\cache
        char dataPath[MAX_PATH];
        const HRESULT result = SHGetFolderPathA(0, CSIDL_LOCAL_APPDATA, 0, SHGFP_TYPE_DEFAULT, dataPath);
        
        if (FAILED(result)) {
            d_stderr2("Could not determine user app data folder - %x", result);
            return String();
        }

        String path = String(dataPath) + "\\" XSTR(PLUGIN_BIN_BASENAME) "\\" + kCacheSubdirectory;

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

}

END_NAMESPACE_DISTRHO

#endif  // PATH_HPP
