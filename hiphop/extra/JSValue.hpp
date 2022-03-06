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

#include <initializer_list>
#include <vector>

#include "distrho/extra/String.hpp"

#include "cJSON.h"

START_NAMESPACE_DISTRHO

class JSValue
{
public:
    // Constructors
    JSValue() noexcept;
    JSValue(bool b) noexcept;
    JSValue(double d) noexcept;
    JSValue(String s) noexcept;

    // Convenience constructors for plugin code
    JSValue(uint32_t i) noexcept;
    JSValue(float f) noexcept;
    JSValue(const char* s) noexcept;
    JSValue(std::initializer_list<JSValue> l) noexcept;

    // Copy constructor
    JSValue(const JSValue& v) noexcept;

    // Copy assignment
    JSValue& operator=(const JSValue& v) noexcept;

    // Factory methods
    static JSValue createArray() noexcept;
    static JSValue createObject() noexcept;

    // Conversion from/to STL vector, this will be eventually removed.
    typedef std::vector<JSValue> vector;
    JSValue(const vector& v) noexcept;
    vector toVector() noexcept;

    // Destructor
    ~JSValue();

    // Getters
    bool isNull() const noexcept;
    bool isBoolean() const noexcept;
    bool isNumber() const noexcept;
    bool isString() const noexcept;
    bool isArray() const noexcept;
    bool isObject() const noexcept;

    bool    getBoolean() const noexcept;
    double  getNumber() const noexcept;
    String  getString() const noexcept;
    int     getArraySize() const noexcept;
    JSValue get(int idx) const noexcept;
    JSValue get(const char* key) const noexcept;

    // Setters
    void push(const JSValue& value);
    void set(const char* key, const JSValue& value);

    // Type casting operators
    operator bool()   const { return getBoolean(); }
    operator double() const { return getNumber(); }
    operator String() const { return getString(); }

    // Serialization/deserialization
    String toJSON(bool format = false) noexcept;
    static JSValue fromJSON(const char* jsonText) noexcept;

private:
    JSValue(cJSON* impl) noexcept;

    cJSON* fImpl;

};

END_NAMESPACE_DISTRHO

#endif // JS_VALUE_HPP
