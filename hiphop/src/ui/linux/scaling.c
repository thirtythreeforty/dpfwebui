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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include "scaling.h"

int primary_monitor_scale_factor()
{
    GdkMonitor* monitor = gdk_display_get_primary_monitor(gdk_display_get_default());
    return gdk_monitor_get_scale_factor(monitor);
}

float device_pixel_ratio()
{
    // Favor GDK scale factor, like set by Gnome Shell display settings.

    int k = primary_monitor_scale_factor();
    
    if (k > 1) {
        return (float)k;
    }

    // Simulate Chromium device pixel ratio https://wiki.debian.org/MonitorDPI
    // Chromium will use the ratio between Xft/DPI (as set through XSETTINGS)
    // and the DPI reported by the X server (through xdpyinfo) as a scaling
    // factor to be used. GTK scale factor is also taken in account by Chromium.

    return xft_dpi() / display_dpi() * gtk_env_scale();
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
                float dpi = atof(ret.addr);

                if (dpi > 0) {
                    return dpi;
                }
            }
        }
    }

    return 96.f;
}

float display_dpi()
{
    Display* display = XOpenDisplay(NULL);
    return ((float)(DisplayWidth(display, 0)) * 25.4f /*mm to inch*/)
         / ((float)(DisplayWidthMM(display, 0)));
}

float gtk_env_scale()
{
    const char* s = getenv("GDK_SCALE");
    int d;

    if ((s != 0) && (sscanf(s, "%d", &d) == 1)) {
        return (float)d;
    } else {
        s = getenv("GDK_DPI_SCALE");
        float f;

        if ((s != 0) && (sscanf(s, "%f", &f) == 1)) {
            return f;
        }
    }

    return 1.f;
}
