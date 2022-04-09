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

#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include "scaling.h"

float device_pixel_ratio()
{
    // Simulate Chromium device pixel ratio https://wiki.debian.org/MonitorDPI
    // Chromium will use the ratio between Xft/DPI (as set through XSETTINGS)
    // and the DPI reported by the X server (through xdpyinfo) as a scaling
    // factor to be used. GTK scale factor is also taken in account by Chromium.

    float k = (float)opt_gdk_scale();

    if (k == 0) {
        k = opt_gdk_dpi_scale();
    
        if (k == 0) {
            k = 1.f;
        }
    }

    return xft_dpi() / x_display_dpi() * k;
}

float xft_dpi()
{
    Display* display = XOpenDisplay(NULL);
    XrmInitialize();

    char* rms = XResourceManagerString(display);

    if (rms != NULL) {
        XrmDatabase sdb = XrmGetStringDatabase(rms);

        if (sdb != NULL) {
            char* type = NULL;
            XrmValue ret;

            if (XrmGetResource(sdb, "Xft.dpi", "String", &type, &ret)
                    && (ret.addr != NULL) && (type != NULL)
                    && (strncmp("String", type, 6) == 0)) {
                float dpi = (float)atof(ret.addr);

                if (dpi > 0) {
                    return dpi;
                }
            }
        }
    }

    return 96.f;
}

float x_display_dpi()
{
    Display* display = XOpenDisplay(NULL);
    return ((float)(DisplayWidth(display, 0)) * 25.4f /*mm to inch*/)
         / ((float)(DisplayWidthMM(display, 0)));
}

int opt_gdk_scale()
{
    const char* s = getenv("GDK_SCALE");

    if (s != NULL) {
        return atoi(s);
    }

    return 0;
}

float opt_gdk_dpi_scale()
{
    const char* s = getenv("GDK_DPI_SCALE");

    if (s != NULL) {
        return (float)atof(s);
    }

    return 0;
}
