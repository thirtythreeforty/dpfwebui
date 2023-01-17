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

#ifndef BSON_VARIANT_HPP
#define BSON_VARIANT_HPP

#include <initializer_list>

#include "distrho/extra/String.hpp"

#include "VariantHelper.hpp"
#include "bson.h"

START_NAMESPACE_DISTRHO

class BSONVariant
{
public:
    // Constructors
    BSONVariant() noexcept;
    BSONVariant(bool b) noexcept;
    BSONVariant(double d) noexcept;
    BSONVariant(String s) noexcept;

    // Convenience constructors for plugin code
    BSONVariant(uint32_t i) noexcept;
    BSONVariant(float f) noexcept;
    BSONVariant(const char* s) noexcept;
    BSONVariant(const BinaryData& data) noexcept;
    BSONVariant(std::initializer_list<BSONVariant> l) noexcept;

    // Copy and move semantics
    BSONVariant(const BSONVariant& v) noexcept;
    BSONVariant& operator=(const BSONVariant& v) noexcept;
    BSONVariant(BSONVariant&& v) noexcept;
    BSONVariant& operator=(BSONVariant&& v) noexcept;

    // Factory methods
    static BSONVariant createArray() noexcept;
    static BSONVariant createObject() noexcept;

    // Destructor
    ~BSONVariant();

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
    BSONVariant getArrayItem(int idx) const noexcept;
    BSONVariant getObjectItem(const char* key) const noexcept;
    BSONVariant operator[](int idx) const noexcept;
    BSONVariant operator[](const char* key) const noexcept;

    // Setters
    void pushArrayItem(const BSONVariant& value) noexcept;
    void setArrayItem(int idx, const BSONVariant& value) /*noexcept*/;
    void insertArrayItem(int idx, const BSONVariant& value) /*noexcept*/;
    void setObjectItem(const char* key, const BSONVariant& value) /*noexcept*/;

    // Operations on arrays
    BSONVariant sliceArray(int start, int end = -1) const
    {
        return ::sliceVariantArray(*this, start, end);
    }

    // Arithmetic operators
    BSONVariant& operator+=(const BSONVariant& other)
    {
        return ::joinVariantArrays(*this, other);
    }

    friend BSONVariant operator+(BSONVariant lhs, const BSONVariant& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    BinaryData toBSON() const;
    static BSONVariant fromBSON(const BinaryData& data, bool asArray) noexcept;

private:
    BSONVariant(bson_type_t type, bson_t* array) noexcept;

    void copy(const BSONVariant& v) noexcept;
    void move(BSONVariant&& v) noexcept;
    void destroy() noexcept;

    bson_type_t fType;

    union {
        bool        fBool;
        double      fDouble;
        char*       fString;
        BinaryData* fData;
        bson_t*     fArray;
    };

};

END_NAMESPACE_DISTRHO

#endif // BSON_VARIANT_HPP
