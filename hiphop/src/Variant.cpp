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

#include "extra/Variant.hpp"
#include "extra/Base64.hpp"

USE_NAMESPACE_DISTRHO

Variant Variant::sliceArray(int start, int end) const noexcept
{
    Variant arr = createArray();

    if (! isArray()) {
        return arr;
    }

    if ((start < 0) || (start == end)) {
        return arr;
    }

    const int size = getArraySize();

    if (start >= size) {
        return arr;
    }

    if ((end < 0)/*def value*/ || (end > size)) {
        end = size;
    }

    for (int i = start; i < end; ++i) {
        arr.pushArrayItem(getArrayItem(i));
    }

    return arr;
}

Variant& Variant::operator+=(const Variant& other)
{
    if (! isArray() || ! other.isArray()) {
        throw std::runtime_error("Only summing arrays is implemented");
    }

    const int size = other.getArraySize();

    for (int i = 0; i < size; ++i) {
        pushArrayItem(other.getArrayItem(i));
    }

    return *this;
}

#if defined(HIPHOP_MESSAGE_PROTOCOL_TEXT)
Variant::Variant() noexcept
    : fImpl(cJSON_CreateNull())
{}

Variant::Variant(bool b) noexcept
    : fImpl(b ? cJSON_CreateTrue() : cJSON_CreateFalse())
{}

Variant::Variant(double d) noexcept
    : fImpl(cJSON_CreateNumber(d))
{}

Variant::Variant(String s) noexcept
    : fImpl(cJSON_CreateString(s))
{}

Variant::Variant(uint32_t i) noexcept
    : fImpl(cJSON_CreateNumber(static_cast<double>(i)))
{}

Variant::Variant(float f) noexcept
    : fImpl(cJSON_CreateNumber(static_cast<double>(f)))
{}

Variant::Variant(const char* s) noexcept
    : fImpl(cJSON_CreateString(s))
{}

Variant::Variant(const BinaryData& data) noexcept
    : fImpl(cJSON_CreateString(String::asBase64(data.data(), data.size())))
{}

Variant::Variant(std::initializer_list<Variant> l) noexcept
    : fImpl(cJSON_CreateArray())
{
    for (std::initializer_list<Variant>::const_iterator it = l.begin(); it != l.end(); ++it) {
        pushArrayItem(*it);
    }
}

Variant::Variant(const Variant& v) noexcept
    : fImpl(cJSON_Duplicate(v.fImpl, true))
{}

Variant& Variant::operator=(const Variant& v) noexcept
{
    if (fImpl != nullptr) {
        cJSON_Delete(fImpl);
    }

    fImpl = cJSON_Duplicate(v.fImpl, true);

    return *this;
}

Variant::Variant(Variant&& v) noexcept
{
    fImpl = v.fImpl;
    v.fImpl = nullptr;
}

Variant& Variant::operator=(Variant&& v) noexcept
{
    if (this != &v) {
        if (fImpl != nullptr) {
            cJSON_Delete(fImpl);
        }

        fImpl = v.fImpl;
        v.fImpl = nullptr;
    }

   return *this;
}

Variant Variant::createArray() noexcept
{
    return Variant(cJSON_CreateArray());
}

Variant Variant::createObject() noexcept
{
    return Variant(cJSON_CreateObject());
}

Variant::~Variant()
{
    if (fImpl != nullptr) {
        cJSON_Delete(fImpl);
    }

    fImpl = nullptr;
}

bool Variant::isNull() const noexcept
{
    return cJSON_IsNull(fImpl);
}

bool Variant::isBoolean() const noexcept
{
    return cJSON_IsBool(fImpl);
}

bool Variant::isNumber() const noexcept
{
    return cJSON_IsNumber(fImpl);
}

bool Variant::isString() const noexcept
{
    return cJSON_IsString(fImpl);
}

bool Variant::isArray() const noexcept
{
    return cJSON_IsArray(fImpl);
}

bool Variant::isObject() const noexcept
{
    return cJSON_IsObject(fImpl);
}

bool Variant::getBoolean() const noexcept
{
    return cJSON_IsTrue(fImpl);
}

double Variant::getNumber() const noexcept
{
    return cJSON_GetNumberValue(fImpl);
}

String Variant::getString() const noexcept
{
    return String(cJSON_GetStringValue(fImpl));
}

Variant::BinaryData Variant::getBinaryData() const noexcept
{
    return d_getChunkFromBase64String(cJSON_GetStringValue(fImpl));
}

int Variant::getArraySize() const noexcept
{
    return cJSON_GetArraySize(fImpl);
}

Variant Variant::getArrayItem(int idx) const noexcept
{
    return Variant(cJSON_Duplicate(cJSON_GetArrayItem(fImpl, idx), true));
}

Variant Variant::getObjectItem(const char* key) const noexcept
{
    return Variant(cJSON_Duplicate(cJSON_GetObjectItem(fImpl, key), true));
}

Variant Variant::operator[](int idx) const noexcept
{
    return getArrayItem(idx);
}

Variant Variant::operator[](const char* key) const noexcept
{
    return getObjectItem(key);
}

void Variant::pushArrayItem(const Variant& value) noexcept
{
    cJSON_AddItemToArray(fImpl, cJSON_Duplicate(value.fImpl, true));
}

void Variant::setArrayItem(int idx, const Variant& value) /*noexcept*/
{
    cJSON_ReplaceItemInArray(fImpl, idx, cJSON_Duplicate(value.fImpl, true));
}

void Variant::insertArrayItem(int idx, const Variant& value) /*noexcept*/
{
    cJSON_InsertItemInArray(fImpl, idx, cJSON_Duplicate(value.fImpl, true));
}

void Variant::setObjectItem(const char* key, const Variant& value) /*noexcept*/
{
    if (cJSON_HasObjectItem(fImpl, key)) {
        cJSON_ReplaceItemInObject(fImpl, key, cJSON_Duplicate(value.fImpl, true));
    } else {
        cJSON_AddItemToObject(fImpl, key, cJSON_Duplicate(value.fImpl, true));
    }
}

String Variant::toJSON(bool format) const noexcept
{
    char* s = format ? cJSON_Print(fImpl) : cJSON_PrintUnformatted(fImpl);
    String jsonText = String(s);
    cJSON_free(s);

    return jsonText;
}

Variant Variant::fromJSON(const char* jsonText) noexcept
{
    return Variant(cJSON_Parse(jsonText));
}

Variant::Variant(cJSON* impl) noexcept
    : fImpl(impl)
{}
#endif // HIPHOP_MESSAGE_PROTOCOL_TEXT

#if defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
Variant::Variant() noexcept
    : fType(BSON_TYPE_NULL)
    , fArray(nullptr)
{}

Variant::Variant(bool b) noexcept
    : fType(BSON_TYPE_BOOL)
    , fBool(b)
{}

Variant::Variant(double d) noexcept
    : fType(BSON_TYPE_DOUBLE)
    , fDouble(d)
{}

Variant::Variant(String s) noexcept
    : fType(BSON_TYPE_UTF8)
{
    fString = new char[s.length() + 1];
    std::strcpy(fString, s.buffer());
}

Variant::Variant(uint32_t i) noexcept
    : fType(BSON_TYPE_DOUBLE)
    , fDouble(static_cast<double>(i))
{}

Variant::Variant(float f) noexcept
    : fType(BSON_TYPE_DOUBLE)
    , fDouble(static_cast<double>(f))
{}

Variant::Variant(const char* s) noexcept
    : fType(BSON_TYPE_UTF8)
{
    fString = new char[std::strlen(s) + 1];
    std::strcpy(fString, s);
}

Variant::Variant(const BinaryData& data) noexcept
    : fType(BSON_TYPE_BINARY)
{
    fData = new BinaryData(data.begin(), data.end());
}

Variant::Variant(std::initializer_list<Variant> l) noexcept
    : fType(BSON_TYPE_ARRAY)
    , fArray(bson_new())
{
    for (std::initializer_list<Variant>::const_iterator it = l.begin(); it != l.end(); ++it) {
        pushArrayItem(*it);
    }
}

Variant::Variant(const Variant& v) noexcept
{
    copy(v);
}

Variant& Variant::operator=(const Variant& v) noexcept
{
    destroy();
    copy(v);

    return *this;
}

Variant::Variant(Variant&& v) noexcept
{
    move(std::move(v));
}

Variant& Variant::operator=(Variant&& v) noexcept
{
    if (this != &v) {
        destroy();
        move(std::move(v));
    }

   return *this;
}

Variant Variant::createArray() noexcept
{
    return Variant(BSON_TYPE_ARRAY, bson_new());
}

Variant Variant::createObject() noexcept
{
    return Variant(BSON_TYPE_DOCUMENT, bson_new());
}

Variant::~Variant()
{
    destroy();
}

bool Variant::isNull() const noexcept
{
    return fType == BSON_TYPE_NULL;
}

bool Variant::isBoolean() const noexcept
{
    return fType == BSON_TYPE_BOOL;
}

bool Variant::isNumber() const noexcept
{
    return fType == BSON_TYPE_DOUBLE;
}

bool Variant::isString() const noexcept
{
    return fType == BSON_TYPE_UTF8;
}

bool Variant::isArray() const noexcept
{
    return fType == BSON_TYPE_ARRAY;
}

bool Variant::isObject() const noexcept
{
    return fType == BSON_TYPE_DOCUMENT;
}

bool Variant::getBoolean() const noexcept
{
    return fString;
}

double Variant::getNumber() const noexcept
{
    return fDouble;
}

String Variant::getString() const noexcept
{
    return String(fString);
}

Variant::BinaryData Variant::getBinaryData() const noexcept
{
    return *fData;
}

int Variant::getArraySize() const noexcept
{
    return bson_count_keys(fArray);
}

Variant Variant::getArrayItem(int idx) const noexcept
{
    return getObjectItem(String(idx).buffer());
}

Variant Variant::getObjectItem(const char* key) const noexcept
{
    bson_iter_t iter;
    bson_iter_init(&iter, fArray);

    if (! bson_iter_find(&iter, key)) {
        return Variant();
    }

    Variant v;
    bson_type_t type = bson_iter_type(&iter);

    switch (type) {
        case BSON_TYPE_NULL:
            break;
        case BSON_TYPE_BOOL:
            v = Variant(bson_iter_bool(&iter));
            break;
        case BSON_TYPE_INT32:
        case BSON_TYPE_INT64:
        case BSON_TYPE_DOUBLE:
            v = Variant(bson_iter_as_double(&iter));
            break;
        case BSON_TYPE_UTF8:
            v = Variant(bson_iter_utf8(&iter, nullptr));
            break;
        case BSON_TYPE_BINARY: {
            uint32_t len;
            const uint8_t* data;
            bson_iter_binary(&iter, nullptr, &len, &data);
            v = Variant(BinaryData(data, data + static_cast<size_t>(len)));
            break;
        }
        case BSON_TYPE_ARRAY:
        case BSON_TYPE_DOCUMENT: {
            uint32_t size;
            const uint8_t *data;
            bson_iter_array(&iter, &size, &data);
            v = Variant(type, bson_new_from_data(data, static_cast<size_t>(size)));
            break;
        }
        default:
            break;
    }

    return v;
}

Variant Variant::operator[](int idx) const noexcept
{
    return getArrayItem(idx);
}

Variant Variant::operator[](const char* key) const noexcept
{
    return getObjectItem(key);
}

void Variant::pushArrayItem(const Variant& value) noexcept
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

void Variant::setArrayItem(int /*idx*/, const Variant& /*value*/) /*noexcept*/
{
    throw std::runtime_error("setArrayItem() not implemented");
}

void Variant::insertArrayItem(int /*idx*/, const Variant& /*value*/) /*noexcept*/
{
    throw std::runtime_error("insertArrayItem() not implemented");
}

void Variant::setObjectItem(const char* /*key*/, const Variant& /*value*/) /*noexcept*/
{
    throw std::runtime_error("setObjectItem() not implemented");
}

Variant::BinaryData Variant::toBSON() const
{
    if ((fType != BSON_TYPE_ARRAY) && (fType != BSON_TYPE_DOCUMENT)) {
        throw std::runtime_error("toBSON() only works for array and document types");
    }

    const uint8_t* data = bson_get_data(fArray);
    
    return BinaryData(data, data + fArray->len);
}

Variant Variant::fromBSON(const BinaryData& data, bool asArray) noexcept
{
    bson_t* array = bson_new_from_data(data.data(), data.size());

    if (array == nullptr) {
        return Variant();
    }

    return Variant(asArray ? BSON_TYPE_ARRAY : BSON_TYPE_DOCUMENT, array);
}

Variant::Variant(bson_type_t type, bson_t* array) noexcept
    : fType(type)
    , fArray(array)
{}

void Variant::copy(const Variant& v) noexcept
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

void Variant::move(Variant&& v) noexcept
{
    fType = v.fType;
    fArray = v.fArray;
    v.fType = BSON_TYPE_EOD;
    v.fArray = nullptr;
}

void Variant::destroy() noexcept
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

#endif // HIPHOP_MESSAGE_PROTOCOL_BINARY
