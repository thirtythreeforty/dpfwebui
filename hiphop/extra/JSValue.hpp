/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2023 Luciano Iam <oss@lucianoiam.com>
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

#include "distrho/extra/String.hpp"

// BSON defines a lot more data types than JSON, like for example binary data.
// Making the backend transparent requires lots of work and it is unnecessary.
#if defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
# include "cJSON.h"
#elif defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
# include "bson.h"
#else
# error JSValue backend not configured
#endif

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

    // Copy and move semantics
    JSValue(const JSValue& v) noexcept;
    JSValue& operator=(const JSValue& v) noexcept;
    JSValue(JSValue&& v) noexcept;
    JSValue& operator=(JSValue&& v) noexcept;

    // Factory methods
    static JSValue createArray() noexcept;
    static JSValue createObject() noexcept;

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
    JSValue getArrayItem(int idx) const noexcept;
    JSValue getObjectItem(const char* key) const noexcept;
    JSValue operator[](int idx) const noexcept;
    JSValue operator[](const char* key) const noexcept;

    // Setters
    void pushArrayItem(const JSValue& value) noexcept;
    void setArrayItem(int idx, const JSValue& value) noexcept;
    void insertArrayItem(int idx, const JSValue& value) noexcept;
    void setObjectItem(const char* key, const JSValue& value) noexcept;

    // Operations on arrays
    JSValue sliceArray(int start, int end = -1) const noexcept;

    // Arithmetic operators
    JSValue& operator+=(const JSValue& other);
    friend JSValue operator+(JSValue lhs, const JSValue& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    // Type casting operators
    operator bool()   const noexcept { return getBoolean(); }
    operator double() const noexcept { return getNumber(); }
    operator String() const noexcept { return getString(); }

    // Serialization/deserialization
#if defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
    String toJSON(bool format = false) const noexcept;
    static JSValue fromJSON(const char* jsonText) noexcept;
#elif defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
    const uint8_t* toBSON(size_t* size) const noexcept;
    static JSValue fromBSON(const uint8_t *data, size_t size, bool asArray) noexcept;
#endif

private:

#if defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
    JSValue(cJSON* impl) noexcept;

    cJSON* fImpl;
#elif defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
    JSValue(bson_t* impl, bson_type_t type) noexcept;

    void copy(const JSValue& v) noexcept;

    bson_t*     fImpl;
    bson_type_t fType;

    union {
        bool   fBool;
        double fDouble;
        char*  fString;
    } fScalar;
#endif

};

END_NAMESPACE_DISTRHO

#endif // JS_VALUE_HPP
