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

#ifndef JSON_VARIANT_HPP
#define JSON_VARIANT_HPP

#include <initializer_list>

#include "distrho/extra/String.hpp"

#include "VariantHelper.hpp"
#include "cJSON.h"

START_NAMESPACE_DISTRHO

class JSONVariant
{
public:
    // Constructors
    JSONVariant() noexcept;
    JSONVariant(bool b) noexcept;
    JSONVariant(double d) noexcept;
    JSONVariant(String s) noexcept;

    // Convenience constructors for plugin code
    JSONVariant(uint32_t i) noexcept;
    JSONVariant(float f) noexcept;
    JSONVariant(const char* s) noexcept;
    JSONVariant(const BinaryData& data) noexcept;
    JSONVariant(std::initializer_list<JSONVariant> l) noexcept;

    // Copy and move semantics
    JSONVariant(const JSONVariant& v) noexcept;
    JSONVariant& operator=(const JSONVariant& v) noexcept;
    JSONVariant(JSONVariant&& v) noexcept;
    JSONVariant& operator=(JSONVariant&& v) noexcept;

    // Factory methods
    static JSONVariant createArray() noexcept;
    static JSONVariant createObject() noexcept;

    // Destructor
    ~JSONVariant();

    // Getters
    bool isNull() const noexcept;
    bool isBoolean() const noexcept;
    bool isNumber() const noexcept;
    bool isString() const noexcept;
    bool isArray() const noexcept;
    bool isObject() const noexcept;

    bool        getBoolean() const noexcept;
    double      getNumber() const noexcept;
    String      getString() const noexcept;
    BinaryData  getBinaryData() const noexcept;
    int         getArraySize() const noexcept;
    JSONVariant getArrayItem(int idx) const noexcept;
    JSONVariant getObjectItem(const char* key) const noexcept;
    JSONVariant operator[](int idx) const noexcept;
    JSONVariant operator[](const char* key) const noexcept;

    // Setters
    void pushArrayItem(const JSONVariant& value) noexcept;
    void setArrayItem(int idx, const JSONVariant& value) /*noexcept*/;
    void insertArrayItem(int idx, const JSONVariant& value) /*noexcept*/;
    void setObjectItem(const char* key, const JSONVariant& value) /*noexcept*/;

    // Operations on arrays
    JSONVariant sliceArray(int start, int end = -1) const
    {
        return ::sliceVariantArray(*this, start, end);
    }

    // Arithmetic operators
    JSONVariant& operator+=(const JSONVariant& other)
    {
        return ::joinVariantArrays(*this, other);
    }

    friend JSONVariant operator+(JSONVariant lhs, const JSONVariant& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    // Type casting operators
    operator bool()   const noexcept { return getBoolean(); }
    operator double() const noexcept { return getNumber(); }
    operator String() const noexcept { return getString(); }

    String toJSON(bool format = false) const noexcept;
    static JSONVariant fromJSON(const char* jsonText) noexcept;

private:
    JSONVariant(cJSON* impl) noexcept;

    cJSON* fImpl;

};

END_NAMESPACE_DISTRHO

#endif // JSON_VARIANT_HPP
