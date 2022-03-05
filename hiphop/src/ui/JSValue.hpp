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

#ifndef JS_VALUE_HPP
#define JS_VALUE_HPP

#include <ostream>
#include <unordered_map>
#include <vector>

#include "distrho/extra/String.hpp"

START_NAMESPACE_DISTRHO

class JSValue
{
public:
    enum Type {
        TNull,
        TBool,
        TNumber,
        TString,
        TArray,
        TObject
    };

    JSValue() noexcept
        : fType(TNull)
        , fBoolean(false)
        , fNumber(0)
    {}

    JSValue(bool b) noexcept
        : fType(TBool)
        , fBoolean(b)
        , fNumber(0)
    {}

    JSValue(double d) noexcept
        : fType(TNumber)
        , fBoolean(false)
        , fNumber(d)
    {}

    JSValue(String s) noexcept
        : fType(TString)
        , fBoolean(false)
        , fNumber(0)
        , fString(s)
    {}

    //
    // Convenience constructors
    //

    JSValue(uint32_t i) noexcept
        : fType(TNumber)
        , fBoolean(false)
        , fNumber(static_cast<double>(i))
    {}

    JSValue(float f) noexcept
        : fType(TNumber)
        , fBoolean(false)
        , fNumber(static_cast<double>(f))
    {}

    JSValue(const char *s) noexcept
        : fType(TString)
        , fBoolean(false)
        , fNumber(0)
        , fString(String(s))
    {}

    //
    // Getters
    //

    bool    isNull()     const noexcept { return fType == TNull; }
    Type    getType()    const noexcept { return fType; }
    bool    getBoolean() const noexcept { return fBoolean; }
    double  getNumber()  const noexcept { return fNumber; }
    String  getString()  const noexcept { return fString; }
    //array&  getArray()   noexcept { return fArray; }
    //object& getObject()  noexcept { return fObject; }

    //
    // Type casting operators
    //

    operator bool()   const noexcept { return fBoolean; }
    operator double() const noexcept { return fNumber; }
    operator String() const noexcept { return fString; }
    //operator array()  noexcept { return fArray; }
    //operator object() noexcept { return fObject; }

private:
    Type   fType;
    bool   fBoolean;
    double fNumber;
    String fString;
    //void*  fArray;
    //void*  fObject;

};

END_NAMESPACE_DISTRHO

typedef std::vector<DISTRHO::JSValue> JSArray;
typedef std::unordered_map<const char*,DISTRHO::JSValue> JSObject;

std::ostream& operator<<(std::ostream &os, const DISTRHO::JSValue &val);

#endif // JS_VALUE_HPP
