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

#include <stdexcept>
#include <string>

#include "extra/BSONVariant.hpp"

USE_NAMESPACE_DISTRHO

BSONVariant::BSONVariant() noexcept
    : fType(BSON_TYPE_NULL)
    , fArray(nullptr)
{}

BSONVariant::BSONVariant(bool b) noexcept
    : fType(BSON_TYPE_BOOL)
    , fBool(b)
{}

BSONVariant::BSONVariant(double d) noexcept
    : fType(BSON_TYPE_DOUBLE)
    , fDouble(d)
{}

BSONVariant::BSONVariant(String s) noexcept
    : fType(BSON_TYPE_UTF8)
{
    fString = new char[s.length() + 1];
    std::strcpy(fString, s.buffer());
}

BSONVariant::BSONVariant(const BinaryData& data) noexcept
    : fType(BSON_TYPE_BINARY)
{
    fData = new BinaryData(data.begin(), data.end());
}

BSONVariant::BSONVariant(int32_t i) noexcept
    : fType(BSON_TYPE_INT32)
    , fInt(i)
{}

BSONVariant::BSONVariant(uint32_t i) noexcept
    : fType(BSON_TYPE_INT32)
    , fInt(static_cast<int32_t>(i))
{}

BSONVariant::BSONVariant(float f) noexcept
    : fType(BSON_TYPE_DOUBLE)
    , fDouble(static_cast<double>(f))
{}

BSONVariant::BSONVariant(const char* s) noexcept
    : fType(BSON_TYPE_UTF8)
{
    fString = new char[std::strlen(s) + 1];
    std::strcpy(fString, s);
}

BSONVariant::BSONVariant(std::initializer_list<BSONVariant> l) noexcept
    : fType(BSON_TYPE_ARRAY)
    , fArray(bson_new())
{
    for (std::initializer_list<BSONVariant>::const_iterator it = l.begin(); it != l.end(); ++it) {
        pushArrayItem(*it);
    }
}

BSONVariant::~BSONVariant()
{
    destroy();
}

BSONVariant::BSONVariant(const BSONVariant& v) noexcept
{
    copy(v);
}

BSONVariant& BSONVariant::operator=(const BSONVariant& v) noexcept
{
    destroy();
    copy(v);

    return *this;
}

BSONVariant::BSONVariant(BSONVariant&& v) noexcept
{
    move(std::move(v));
}

BSONVariant& BSONVariant::operator=(BSONVariant&& v) noexcept
{
    if (this != &v) {
        destroy();
        move(std::move(v));
    }

   return *this;
}

BSONVariant BSONVariant::createArray() noexcept
{
    return BSONVariant(BSON_TYPE_ARRAY, bson_new());
}

BSONVariant BSONVariant::createObject() noexcept
{
    return BSONVariant(BSON_TYPE_DOCUMENT, bson_new());
}

bool BSONVariant::isNull() const noexcept
{
    return fType == BSON_TYPE_NULL;
}

bool BSONVariant::isBoolean() const noexcept
{
    return fType == BSON_TYPE_BOOL;
}

bool BSONVariant::isNumber() const noexcept
{
    return (fType == BSON_TYPE_INT32) || (fType == BSON_TYPE_DOUBLE);
}

bool BSONVariant::isString() const noexcept
{
    return fType == BSON_TYPE_UTF8;
}

bool BSONVariant::isBinaryData() const noexcept
{
    return fType == BSON_TYPE_BINARY;
}

bool BSONVariant::isArray() const noexcept
{
    return fType == BSON_TYPE_ARRAY;
}

bool BSONVariant::isObject() const noexcept
{
    return fType == BSON_TYPE_DOCUMENT;
}

String BSONVariant::asString() const noexcept
{
    switch (fType) {
        case BSON_TYPE_NULL:
            return String("null");
        case BSON_TYPE_BOOL:
            return String(fBool ? "true" : "false");
        case BSON_TYPE_INT32:
            return String(fInt);
        case BSON_TYPE_INT64:
        case BSON_TYPE_DOUBLE:
            return String(fDouble);
        case BSON_TYPE_UTF8:
            return String(fString);
        case BSON_TYPE_BINARY:
            return String("[BinaryData]");
        case BSON_TYPE_ARRAY:
        case BSON_TYPE_DOCUMENT:
            return String("[Object]");
        default:
            break;
    }

    return String();
}

bool BSONVariant::getBoolean() const noexcept
{
    return fString;
}

double BSONVariant::getNumber() const noexcept
{
    if (fType == BSON_TYPE_INT32) {
        return static_cast<double>(fInt);
    }

    if (fType == BSON_TYPE_DOUBLE) {
        return fDouble;
    }

    return 0;
}

String BSONVariant::getString() const noexcept
{
    return String(fString);
}

BinaryData BSONVariant::getBinaryData() const noexcept
{
    return *fData;
}

int BSONVariant::getArraySize() const noexcept
{
    return bson_count_keys(fArray);
}

BSONVariant BSONVariant::getArrayItem(int idx) const noexcept
{
    return get(fArray, String(idx).buffer());
}

BSONVariant BSONVariant::getObjectItem(const char* key) const noexcept
{
    return get(fArray, key);
}

BSONVariant BSONVariant::operator[](int idx) const noexcept
{
    return getArrayItem(idx);
}

BSONVariant BSONVariant::operator[](const char* key) const noexcept
{
    return get(fArray, key);
}

void BSONVariant::pushArrayItem(const BSONVariant& value) noexcept
{
    String key(bson_count_keys(fArray));
    set(fArray, key.buffer(), value);
}

void BSONVariant::setArrayItem(int idx, const BSONVariant& value) noexcept
{
    String key(idx);
    set(fArray, key.buffer(), value);
}

void BSONVariant::insertArrayItem(int idx, const BSONVariant& value) noexcept
{
    if (fArray == nullptr) {
        return;
    }

    bson_iter_t iter;

    if (! bson_iter_init(&iter, fArray)) {
        return;
    }

    int keyCount = static_cast<int>(bson_count_keys(fArray));

    if (idx > keyCount) {
        return;
    }

    if (idx == keyCount) {
        pushArrayItem(value);
        return;
    }

    bson_t* newArr = bson_new();
    int newIdx;

    while (bson_iter_next(&iter)) {
        try {
            newIdx = std::stoi(bson_iter_key(&iter));
        } catch (std::invalid_argument const& ex) {
            bson_destroy(newArr);
            return;
        }

        if (newIdx >= idx) {
            newIdx++;
        }

        bson_append_iter(newArr, String(newIdx).buffer(), -1, &iter);
    }

    bson_destroy(fArray);
    fArray = newArr;

    set(fArray, String(idx).buffer(), value);
}

void BSONVariant::setObjectItem(const char* key, const BSONVariant& value) noexcept
{
    set(fArray, key, value);
}

BinaryData BSONVariant::toBSON() const noexcept
{
    if (fArray == nullptr) {
        return BinaryData();
    }

    const uint8_t* data = bson_get_data(fArray);   
    
    return BinaryData(data, data + fArray->len);
}

BSONVariant BSONVariant::fromBSON(const BinaryData& data, bool asArray) noexcept
{
    bson_t* array = bson_new_from_data(data.data(), data.size());

    if (array == nullptr) {
        return BSONVariant();
    }

    return BSONVariant(asArray ? BSON_TYPE_ARRAY : BSON_TYPE_DOCUMENT, array);
}

BSONVariant::BSONVariant(bson_type_t type, bson_t* array) noexcept
    : fType(type)
    , fArray(array)
{}

void BSONVariant::copy(const BSONVariant& v) noexcept
{
    fType = v.fType;

    switch (v.fType) {
        case BSON_TYPE_BOOL:
            fBool = v.fBool;
            break;
        case BSON_TYPE_INT32:
            fInt = v.fInt;
            break;
        case BSON_TYPE_DOUBLE:
            fDouble = v.fDouble;
            break;
        case BSON_TYPE_UTF8:
            fString = new char[std::strlen(v.fString) + 1];
            std::strcpy(fString, v.fString);
            break;
        case BSON_TYPE_BINARY:
            fData = new BinaryData(v.fData->begin(), v.fData->end());
            break;
        case BSON_TYPE_ARRAY:
        case BSON_TYPE_DOCUMENT:
            fArray = bson_copy(v.fArray);
            break;
        default:
            break;
    }
}

void BSONVariant::move(BSONVariant&& v) noexcept
{
    fType = v.fType;
    fArray = v.fArray;
    v.fType = BSON_TYPE_EOD;
    v.fArray = nullptr;
}

void BSONVariant::destroy() noexcept
{
    switch (fType) {
        case BSON_TYPE_UTF8:
            delete[] fString;
            break;
        case BSON_TYPE_BINARY:
            delete fData;
            break;
        case BSON_TYPE_ARRAY:
        case BSON_TYPE_DOCUMENT:
            bson_destroy(fArray);
            break;
        default:
            break;
    }
}

BSONVariant BSONVariant::get(const bson_t* bson, const char* key) noexcept
{
    bson_iter_t iter;

    if ((bson == nullptr) || !bson_iter_init(&iter, bson)
            || !bson_iter_find(&iter, key)) {
        return BSONVariant();
    }

    BSONVariant v;
    bson_type_t type = bson_iter_type(&iter);

    switch (type) {
        case BSON_TYPE_NULL:
            break;
        case BSON_TYPE_BOOL:
            v = BSONVariant(bson_iter_bool(&iter));
            break;
        case BSON_TYPE_INT32:
            v = BSONVariant(bson_iter_int32(&iter));
            break;
        case BSON_TYPE_INT64:
        case BSON_TYPE_DOUBLE:
            v = BSONVariant(bson_iter_as_double(&iter));
            break;
        case BSON_TYPE_UTF8:
            v = BSONVariant(bson_iter_utf8(&iter, nullptr));
            break;
        case BSON_TYPE_BINARY: {
            uint32_t len;
            const uint8_t* data;
            bson_iter_binary(&iter, nullptr, &len, &data);
            v = BSONVariant(BinaryData(data, data + static_cast<size_t>(len)));
            break;
        }
        case BSON_TYPE_ARRAY:
        case BSON_TYPE_DOCUMENT: {
            uint32_t size;
            const uint8_t *data;
            bson_iter_array(&iter, &size, &data);
            v = BSONVariant(type, bson_new_from_data(data, static_cast<size_t>(size)));
            break;
        }
        default:
            break;
    }

    return v;
}

void BSONVariant::set(bson_t* bson, const char* key, const BSONVariant& value) noexcept
{
    if (bson == nullptr) {
        return;
    }

    switch (value.fType) {
        case BSON_TYPE_NULL:
            bson_append_null(bson, key, -1);
            break;
        case BSON_TYPE_BOOL:
            bson_append_bool(bson, key, -1, value.fBool);
            break;
        case BSON_TYPE_INT32:
            bson_append_int32(bson, key, -1, value.fInt);
            break;
        case BSON_TYPE_DOUBLE:
            bson_append_double(bson, key, -1, value.fDouble);
            break;
        case BSON_TYPE_UTF8:
            bson_append_utf8(bson, key, -1, value.fString, -1);
            break;
        case BSON_TYPE_BINARY:
            bson_append_binary(bson, key, -1, BSON_SUBTYPE_BINARY, value.fData->data(),
                                static_cast<uint32_t>(value.fData->size()));
            break;
        case BSON_TYPE_ARRAY:
            bson_append_array(bson, key, -1, value.fArray);
            break;
        case BSON_TYPE_DOCUMENT:
            bson_append_document(bson, key, -1, value.fArray);
            break;
        default:
            break;
    }
}
