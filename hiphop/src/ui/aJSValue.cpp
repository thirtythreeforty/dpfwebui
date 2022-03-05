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

#include "JSValue.hpp"

// Avoid clashing with macOS WebKit class JSValue by making namespace explicit

std::ostream& operator<<(std::ostream &os, const DISTRHO::JSValue &val) {
    switch (val.getType()) {
        case DISTRHO::JSValue::TNull:
            os << "null";
            break;

        case DISTRHO::JSValue::TBool:
            os << (val.getBoolean() ? "true" : "false");
            break;

        case DISTRHO::JSValue::TNumber: {
            const double d = val.getNumber();
            if (std::isnan(d)) {
                os << "NaN";
            } else if (std::isinf(d)) {
                os << (d < 0 ? "-Inf" : "Inf");
            } else {
                os << d;
            }
            break;
        }

        case DISTRHO::JSValue::TString: {
            const String& s = val.getString();
            const char *buf = s.buffer();
            const int len = s.length();
            os << '"';
            for (int i = 0; i < len; i++) {
                if (buf[i] != '"') {
                    os << buf[i];
                } else {
                    os << "\\\"";
                }
            }
            os << '"';
            break;
        }

        default:
            break;
    }

    return os;
}
