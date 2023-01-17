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

#ifndef VARIANT_HPP
#define VARIANT_HPP

#include <initializer_list>
#include <vector>

#include "distrho/extra/String.hpp"

// BSON defines a lot more data types than JSON, like for example binary data.
// Making the backend transparent requires lots of work and it is unnecessary.
#if defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
# include "cJSON.h"
#elif defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
# include "bson.h"
#else
# error Variant backend not configured
#endif

START_NAMESPACE_DISTRHO

class Variant
{
public:
    typedef std::vector<uint8_t> BinaryData;

    // Constructors
    Variant() noexcept;
    Variant(bool b) noexcept;
    Variant(double d) noexcept;
    Variant(String s) noexcept;

    // Convenience constructors for plugin code
    Variant(uint32_t i) noexcept;
    Variant(float f) noexcept;
    Variant(const char* s) noexcept;
    Variant(const BinaryData& data) noexcept;
    Variant(std::initializer_list<Variant> l) noexcept;

    // Copy and move semantics
    Variant(const Variant& v) noexcept;
    Variant& operator=(const Variant& v) noexcept;
    Variant(Variant&& v) noexcept;
    Variant& operator=(Variant&& v) noexcept;

    // Factory methods
    static Variant createArray() noexcept;
    static Variant createObject() noexcept;

    // Destructor
    ~Variant();

    // Getters
    bool isNull() const noexcept;
    bool isBoolean() const noexcept;
    bool isNumber() const noexcept;
    bool isString() const noexcept;
    bool isArray() const noexcept;
    bool isObject() const noexcept;

    bool       getBoolean() const noexcept;
    double     getNumber() const noexcept;
    String     getString() const noexcept;
    BinaryData getBinaryData() const noexcept;
    int        getArraySize() const noexcept;
    Variant    getArrayItem(int idx) const noexcept;
    Variant    getObjectItem(const char* key) const noexcept;
    Variant    operator[](int idx) const noexcept;
    Variant    operator[](const char* key) const noexcept;

    // Setters
    void pushArrayItem(const Variant& value) noexcept;
    void setArrayItem(int idx, const Variant& value) /*noexcept*/;
    void insertArrayItem(int idx, const Variant& value) /*noexcept*/;
    void setObjectItem(const char* key, const Variant& value) /*noexcept*/;

    // Operations on arrays
    Variant sliceArray(int start, int end = -1) const noexcept;

    // Arithmetic operators
    Variant& operator+=(const Variant& other);
    friend Variant operator+(Variant lhs, const Variant& rhs)
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
    static Variant fromJSON(const char* jsonText) noexcept;
#elif defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
    BinaryData toBSON() const;
    static Variant fromBSON(const BinaryData& data, bool asArray) noexcept;
#endif

private:

#if defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
    Variant(cJSON* impl) noexcept;

    cJSON* fImpl;
#elif defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
    Variant(bson_type_t type, bson_t* array) noexcept;

    void copy(const Variant& v) noexcept;
    void move(Variant&& v) noexcept;
    void destroy() noexcept;

    bson_type_t fType;

    union {
        bool        fBool;
        double      fDouble;
        char*       fString;
        BinaryData* fData;
        bson_t*     fArray;
    };

#endif

};

END_NAMESPACE_DISTRHO

#endif // VARIANT_HPP
