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

#include <X11/Xresource.h>

#include "pixel_ratio.h"

float getDevicePixelRatio(Display* display, float tkScaleFactor)
{
    // Simulate Chromium device pixel ratio https://wiki.debian.org/MonitorDPI
    // Chromium will use the ratio between Xft/DPI (as set through XSETTINGS)
    // and the DPI reported by the X server (through xdpyinfo) as a scaling
    // factor to be used.

    XrmInitialize();

    float xftDpi = 96.f;
    char* rms;

    if (rms = XResourceManagerString(display)) {
    	XrmDatabase sdb;

        if (sdb = XrmGetStringDatabase(rms)) {
            char* type = NULL;
            XrmValue ret;

            if (XrmGetResource(sdb, "Xft.dpi", "String", &type, &ret)
                    && (ret.addr != NULL) && (type != NULL)
                    && (strncmp("String", type, 6) == 0)) {
            	float dpi = atof(ret.addr);
                if (dpi > 0) {
                    xftDpi = dpi;
                }
            }
        }
    }

    // X display DPI value as returned by xdpyinfo

    float dpyDpi = ((float)(DisplayWidth(display, 0)) * 25.4f /*mm to inch*/)
                 / ((float)(DisplayWidthMM(display, 0)));

    // Toolkit scale factor is also taken in account by Chromium.

    if (tkScaleFactor == 1.f) {
	    const char* s = getenv("GDK_SCALE");
	    int d;

	    if ((s != 0) && (sscanf(s, "%d", &d) == 1)) {
	        tkScaleFactor = (float)d;
	    } else {
		    s = getenv("GDK_DPI_SCALE");
		    float f;

		    if ((s != 0) && (sscanf(s, "%f", &f) == 1)) {
		        tkScaleFactor = f;
		    }
		}
	}

    return tkScaleFactor * xftDpi / dpyDpi;
}
