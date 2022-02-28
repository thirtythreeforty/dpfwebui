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

#ifndef WEB_UI_HPP
#define WEB_UI_HPP

#include "src/DistrhoDefines.h"

#ifdef DISTRHO_OS_LINUX
# include "ui/linux/LinuxWebViewUI.hpp"
typedef LinuxWebViewUI WebUI;
#endif

#ifdef DISTRHO_OS_MAC
# include "ui/macos/MacWebViewUI.hpp"
typedef MacWebViewUI WebUI;
#endif

#ifdef DISTRHO_OS_WINDOWS
# include "ui/windows/WindowsWebViewUI.hpp"
typedef WindowsWebViewUI WebUI;
#endif

#endif  // WEB_UI_HPP
