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

#include <string>
#include <unordered_map>
#include <vector>

#include "distrho/extra/String.hpp"

#include "cJSON.h"

START_NAMESPACE_DISTRHO

class JSValue
{
public:
    typedef std::vector<JSValue> array;
    typedef std::unordered_map<std::string,JSValue> object;

    // Constructors
    JSValue() noexcept;
    JSValue(bool b) noexcept;
    JSValue(double d) noexcept;
    JSValue(String s) noexcept;
    JSValue(const JSValue& v);

    // Convenience constructors for plugin code
    JSValue(uint32_t i) noexcept;
    JSValue(float f) noexcept;
    JSValue(const char* s) noexcept;

    // Factory methods
    static JSValue createArray() noexcept;
    static JSValue createObject() noexcept;

    // Destructor
    ~JSValue();

    // Getters
    bool    isNull() const noexcept;
    bool    isBoolean() const noexcept;
    bool    getBoolean() const noexcept;
    bool    isNumber() const noexcept;
    double  getNumber() const noexcept;
    bool    isString() const noexcept;
    String  getString() const noexcept;
    bool    isArray() const noexcept;
    array&  getArray() const;
    bool    isObject() const noexcept;
    object& getObject() const;

    // Type casting operators
    operator bool()   const { return getBoolean(); }
    operator double() const { return getNumber(); }
    operator String() const { return getString(); }
    operator array()  const { return getArray(); }
    operator object() const { return getObject(); }

    // Serialization/deserialization
    String toJSON(bool format = false) noexcept;
    static JSValue fromJSON(const char* jsonText) noexcept;

    // Helper method for plugin code
    static String arrayToJSON(const array& a, bool format = false) noexcept;

private:
    JSValue(cJSON* json, bool createContainer) noexcept;

    void cJSONConnectTree() noexcept;
    void cJSONDisconnectTree() noexcept;

    cJSON* fStorage;
    void*  fContainer;

};

END_NAMESPACE_DISTRHO

#endif // JS_VALUE_HPP
