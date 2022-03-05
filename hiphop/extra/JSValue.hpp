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

#include <exception>
#include <ostream>
#include <unordered_map>
#include <vector>

#include "distrho/extra/String.hpp"

START_NAMESPACE_DISTRHO

class JSValue
{
public:
    typedef std::vector<JSValue> array;
    typedef std::unordered_map<const char*,JSValue> object;

    enum type {
        TNull,
        TBoolean,
        TNumber,
        TString,
        TArray,
        TObject
    };

    // Constructors
    JSValue() noexcept;
    JSValue(bool b) noexcept;
    JSValue(double d) noexcept;
    JSValue(String s) noexcept;

    // Convenience constructors
    JSValue(uint32_t i) noexcept;
    JSValue(float f) noexcept;
    JSValue(const char *s) noexcept;

    // Destructor
    ~JSValue();

    // Getters
    bool    isNull()     const noexcept;
    type    getType()    const noexcept;
    bool    getBoolean() const;
    double  getNumber()  const;
    String  getString()  const;
    array&  getArray();
    object& getObject();

    // Type casting operators
    operator bool()   const noexcept { return fBoolean; }
    operator double() const noexcept { return fNumber; }
    operator String() const noexcept { return fString; }
    operator array()  noexcept { return getArray(); }
    operator object() noexcept { return getObject(); }

private:
    type   fType;
    bool   fBoolean;
    double fNumber;
    String fString;
    void*  fContainer;

};

END_NAMESPACE_DISTRHO

std::ostream& operator<<(std::ostream &os, const DISTRHO::JSValue &val);

#endif // JS_VALUE_HPP
