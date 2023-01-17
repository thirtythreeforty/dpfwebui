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

BSONVariant::BSONVariant(uint32_t i) noexcept
    : fType(BSON_TYPE_DOUBLE)
    , fDouble(static_cast<double>(i))
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

BSONVariant::BSONVariant(const BinaryData& data) noexcept
    : fType(BSON_TYPE_BINARY)
{
    fData = new BinaryData(data.begin(), data.end());
}

BSONVariant::BSONVariant(std::initializer_list<BSONVariant> l) noexcept
    : fType(BSON_TYPE_ARRAY)
    , fArray(bson_new())
{
    for (std::initializer_list<BSONVariant>::const_iterator it = l.begin(); it != l.end(); ++it) {
        pushArrayItem(*it);
    }
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

BSONVariant::~BSONVariant()
{
    destroy();
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
    return fType == BSON_TYPE_DOUBLE;
}

bool BSONVariant::isString() const noexcept
{
    return fType == BSON_TYPE_UTF8;
}

bool BSONVariant::isArray() const noexcept
{
    return fType == BSON_TYPE_ARRAY;
}

bool BSONVariant::isObject() const noexcept
{
    return fType == BSON_TYPE_DOCUMENT;
}

bool BSONVariant::getBoolean() const noexcept
{
    return fString;
}

double BSONVariant::getNumber() const noexcept
{
    return fDouble;
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
    return getObjectItem(String(idx).buffer());
}

BSONVariant BSONVariant::getObjectItem(const char* key) const noexcept
{
    bson_iter_t iter;
    bson_iter_init(&iter, fArray);

    if (! bson_iter_find(&iter, key)) {
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

BSONVariant BSONVariant::operator[](int idx) const noexcept
{
    return getArrayItem(idx);
}

BSONVariant BSONVariant::operator[](const char* key) const noexcept
{
    return getObjectItem(key);
}

void BSONVariant::pushArrayItem(const BSONVariant& value) noexcept
{
    String tmp(bson_count_keys(fArray));
    const char* key = tmp.buffer();

    switch (value.fType) {
        case BSON_TYPE_NULL:
            bson_append_null(fArray, key, -1);
            break;
        case BSON_TYPE_BOOL:
            bson_append_bool(fArray, key, -1, value.fBool);
            break;
        case BSON_TYPE_DOUBLE:
            bson_append_double(fArray, key, -1, value.fDouble);
            break;
        case BSON_TYPE_UTF8:
            bson_append_utf8(fArray, key, -1, value.fString, -1);
            break;
        case BSON_TYPE_BINARY:
            bson_append_binary(fArray, key, -1, BSON_SUBTYPE_BINARY, value.fData->data(),
                                static_cast<uint32_t>(value.fData->size()));
            break;
        case BSON_TYPE_ARRAY:
            bson_append_array(fArray, key, -1, value.fArray);
            break;
        case BSON_TYPE_DOCUMENT:
            bson_append_document(fArray, key, -1, value.fArray);
            break;
        default:
            break;
    }
}

void BSONVariant::setArrayItem(int /*idx*/, const BSONVariant& /*value*/) /*noexcept*/
{
    throw std::runtime_error("setArrayItem() not implemented");
}

void BSONVariant::insertArrayItem(int /*idx*/, const BSONVariant& /*value*/) /*noexcept*/
{
    throw std::runtime_error("insertArrayItem() not implemented");
}

void BSONVariant::setObjectItem(const char* /*key*/, const BSONVariant& /*value*/) /*noexcept*/
{
    throw std::runtime_error("setObjectItem() not implemented");
}

BinaryData BSONVariant::toBSON() const
{
    if ((fType != BSON_TYPE_ARRAY) && (fType != BSON_TYPE_DOCUMENT)) {
        throw std::runtime_error("toBSON() only works for array and document types");
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
