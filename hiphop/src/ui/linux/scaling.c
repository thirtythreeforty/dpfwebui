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

    return xdpi_scale() * gdk_scale() * gdk_dpi_scale();
}

float xdpi_scale()
{
    return xft_dpi() / xdisplay_dpi();
}

float xft_dpi()
{
    Display* display = XOpenDisplay(NULL);
    XrmInitialize();

    char* rms = XResourceManagerString(display);
    float dpi = 96.f;

    if (rms != NULL) {
        XrmDatabase sdb = XrmGetStringDatabase(rms);

        if (sdb != NULL) {
            char* type = NULL;
            XrmValue ret;

            if (XrmGetResource(sdb, "Xft.dpi", "String", &type, &ret)
                    && (ret.addr != NULL) && (type != NULL)
                    && (strncmp("String", type, 6) == 0)) {
                float xftDpi = (float)atof(ret.addr);

                if (xftDpi > 0) {
                    dpi = xftDpi;
                }
            }
        }
    }

    XCloseDisplay(display);

    return dpi;
}

float xdisplay_dpi()
{
    Display* display = XOpenDisplay(NULL);
    return ((float)(DisplayWidth(display, 0)) * 25.4f /*mm to inch*/)
         / ((float)(DisplayWidthMM(display, 0)));
}

int gdk_scale()
{
    const char* s = getenv("GDK_SCALE");

    if (s != NULL) {
        int k = atoi(s);

        if (k > 0) {
            return k;
        }
    }

    return 1;
}

float gdk_dpi_scale()
{
    const char* s = getenv("GDK_DPI_SCALE");

    if (s != NULL) {
        float k = (float)atof(s);

        if (k > 0) {
            return k;
        }
    }

    return 1.f;
}
