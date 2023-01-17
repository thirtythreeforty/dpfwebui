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

#include "extra/JSValue.hpp"
#include "extra/Base64.hpp"

USE_NAMESPACE_DISTRHO

JSValue JSValue::sliceArray(int start, int end) const noexcept
{
    JSValue arr = createArray();

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

JSValue& JSValue::operator+=(const JSValue& other)
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
JSValue::JSValue() noexcept
    : fImpl(cJSON_CreateNull())
{}

JSValue::JSValue(bool b) noexcept
    : fImpl(b ? cJSON_CreateTrue() : cJSON_CreateFalse())
{}

JSValue::JSValue(double d) noexcept
    : fImpl(cJSON_CreateNumber(d))
{}

JSValue::JSValue(String s) noexcept
    : fImpl(cJSON_CreateString(s))
{}

JSValue::JSValue(uint32_t i) noexcept
    : fImpl(cJSON_CreateNumber(static_cast<double>(i)))
{}

JSValue::JSValue(float f) noexcept
    : fImpl(cJSON_CreateNumber(static_cast<double>(f)))
{}

JSValue::JSValue(const char* s) noexcept
    : fImpl(cJSON_CreateString(s))
{}

JSValue::JSValue(const BinaryData& data) noexcept
    : fImpl(cJSON_CreateString(String::asBase64(data.data(), data.size())))
{}

JSValue::JSValue(std::initializer_list<JSValue> l) noexcept
    : fImpl(cJSON_CreateArray())
{
    for (std::initializer_list<JSValue>::const_iterator it = l.begin(); it != l.end(); ++it) {
        pushArrayItem(*it);
    }
}

JSValue::JSValue(const JSValue& v) noexcept
    : fImpl(cJSON_Duplicate(v.fImpl, true))
{}

JSValue& JSValue::operator=(const JSValue& v) noexcept
{
    if (fImpl != nullptr) {
        cJSON_Delete(fImpl);
    }

    fImpl = cJSON_Duplicate(v.fImpl, true);

    return *this;
}

JSValue::JSValue(JSValue&& v) noexcept
{
    fImpl = v.fImpl;
    v.fImpl = nullptr;
}

JSValue& JSValue::operator=(JSValue&& v) noexcept
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

JSValue JSValue::createArray() noexcept
{
    return JSValue(cJSON_CreateArray());
}

JSValue JSValue::createObject() noexcept
{
    return JSValue(cJSON_CreateObject());
}

JSValue::~JSValue()
{
    if (fImpl != nullptr) {
        cJSON_Delete(fImpl);
    }

    fImpl = nullptr;
}

bool JSValue::isNull() const noexcept
{
    return cJSON_IsNull(fImpl);
}

bool JSValue::isBoolean() const noexcept
{
    return cJSON_IsBool(fImpl);
}

bool JSValue::isNumber() const noexcept
{
    return cJSON_IsNumber(fImpl);
}

bool JSValue::isString() const noexcept
{
    return cJSON_IsString(fImpl);
}

bool JSValue::isArray() const noexcept
{
    return cJSON_IsArray(fImpl);
}

bool JSValue::isObject() const noexcept
{
    return cJSON_IsObject(fImpl);
}

bool JSValue::getBoolean() const noexcept
{
    return cJSON_IsTrue(fImpl);
}

double JSValue::getNumber() const noexcept
{
    return cJSON_GetNumberValue(fImpl);
}

String JSValue::getString() const noexcept
{
    return String(cJSON_GetStringValue(fImpl));
}

JSValue::BinaryData JSValue::getBinaryData() const noexcept
{
    return d_getChunkFromBase64String(cJSON_GetStringValue(fImpl));
}

int JSValue::getArraySize() const noexcept
{
    return cJSON_GetArraySize(fImpl);
}

JSValue JSValue::getArrayItem(int idx) const noexcept
{
    return JSValue(cJSON_Duplicate(cJSON_GetArrayItem(fImpl, idx), true));
}

JSValue JSValue::getObjectItem(const char* key) const noexcept
{
    return JSValue(cJSON_Duplicate(cJSON_GetObjectItem(fImpl, key), true));
}

JSValue JSValue::operator[](int idx) const noexcept
{
    return getArrayItem(idx);
}

JSValue JSValue::operator[](const char* key) const noexcept
{
    return getObjectItem(key);
}

void JSValue::pushArrayItem(const JSValue& value) noexcept
{
    cJSON_AddItemToArray(fImpl, cJSON_Duplicate(value.fImpl, true));
}

void JSValue::setArrayItem(int idx, const JSValue& value) /*noexcept*/
{
    cJSON_ReplaceItemInArray(fImpl, idx, cJSON_Duplicate(value.fImpl, true));
}

void JSValue::insertArrayItem(int idx, const JSValue& value) /*noexcept*/
{
    cJSON_InsertItemInArray(fImpl, idx, cJSON_Duplicate(value.fImpl, true));
}

void JSValue::setObjectItem(const char* key, const JSValue& value) /*noexcept*/
{
    if (cJSON_HasObjectItem(fImpl, key)) {
        cJSON_ReplaceItemInObject(fImpl, key, cJSON_Duplicate(value.fImpl, true));
    } else {
        cJSON_AddItemToObject(fImpl, key, cJSON_Duplicate(value.fImpl, true));
    }
}

String JSValue::toJSON(bool format) const noexcept
{
    char* s = format ? cJSON_Print(fImpl) : cJSON_PrintUnformatted(fImpl);
    String jsonText = String(s);
    cJSON_free(s);

    return jsonText;
}

JSValue JSValue::fromJSON(const char* jsonText) noexcept
{
    return JSValue(cJSON_Parse(jsonText));
}

JSValue::JSValue(cJSON* impl) noexcept
    : fImpl(impl)
{}
#endif // HIPHOP_MESSAGE_PROTOCOL_TEXT

#if defined(HIPHOP_MESSAGE_PROTOCOL_BINARY)
JSValue::JSValue() noexcept
    : fType(BSON_TYPE_NULL)
    , fArray(nullptr)
{}

JSValue::JSValue(bool b) noexcept
    : fType(BSON_TYPE_BOOL)
    , fBool(b)
{}

JSValue::JSValue(double d) noexcept
    : fType(BSON_TYPE_DOUBLE)
    , fDouble(d)
{}

JSValue::JSValue(String s) noexcept
    : fType(BSON_TYPE_UTF8)
{
    fString = new char[s.length() + 1];
    std::strcpy(fString, s.buffer());
}

JSValue::JSValue(uint32_t i) noexcept
    : fType(BSON_TYPE_DOUBLE)
    , fDouble(static_cast<double>(i))
{}

JSValue::JSValue(float f) noexcept
    : fType(BSON_TYPE_DOUBLE)
    , fDouble(static_cast<double>(f))
{}

JSValue::JSValue(const char* s) noexcept
    : fType(BSON_TYPE_UTF8)
{
    fString = new char[std::strlen(s) + 1];
    std::strcpy(fString, s);
}

JSValue::JSValue(const BinaryData& data) noexcept
    : fType(BSON_TYPE_BINARY)
{
    fData = new BinaryData(data.begin(), data.end());
}

JSValue::JSValue(std::initializer_list<JSValue> l) noexcept
    : fType(BSON_TYPE_ARRAY)
    , fArray(bson_new())
{
    for (std::initializer_list<JSValue>::const_iterator it = l.begin(); it != l.end(); ++it) {
        pushArrayItem(*it);
    }
}

JSValue::JSValue(const JSValue& v) noexcept
{
    copy(v);
}

JSValue& JSValue::operator=(const JSValue& v) noexcept
{
    destroy();
    copy(v);

    return *this;
}

JSValue::JSValue(JSValue&& v) noexcept
{
    move(std::move(v));
}

JSValue& JSValue::operator=(JSValue&& v) noexcept
{
    if (this != &v) {
        destroy();
        move(std::move(v));
    }

   return *this;
}

JSValue JSValue::createArray() noexcept
{
    return JSValue(BSON_TYPE_ARRAY, bson_new());
}

JSValue JSValue::createObject() noexcept
{
    return JSValue(BSON_TYPE_DOCUMENT, bson_new());
}

JSValue::~JSValue()
{
    destroy();
}

bool JSValue::isNull() const noexcept
{
    return fType == BSON_TYPE_NULL;
}

bool JSValue::isBoolean() const noexcept
{
    return fType == BSON_TYPE_BOOL;
}

bool JSValue::isNumber() const noexcept
{
    return fType == BSON_TYPE_DOUBLE;
}

bool JSValue::isString() const noexcept
{
    return fType == BSON_TYPE_UTF8;
}

bool JSValue::isArray() const noexcept
{
    return fType == BSON_TYPE_ARRAY;
}

bool JSValue::isObject() const noexcept
{
    return fType == BSON_TYPE_DOCUMENT;
}

bool JSValue::getBoolean() const noexcept
{
    return fString;
}

double JSValue::getNumber() const noexcept
{
    return fDouble;
}

String JSValue::getString() const noexcept
{
    return String(fString);
}

JSValue::BinaryData JSValue::getBinaryData() const noexcept
{
    return *fData;
}

int JSValue::getArraySize() const noexcept
{
    return bson_count_keys(fArray);
}

JSValue JSValue::getArrayItem(int idx) const noexcept
{
    return getObjectItem(String(idx).buffer());
}

JSValue JSValue::getObjectItem(const char* key) const noexcept
{
    bson_iter_t iter;
    bson_iter_init(&iter, fArray);

    if (! bson_iter_find(&iter, key)) {
        return JSValue();
    }

    JSValue v;
    bson_type_t type = bson_iter_type(&iter);

    switch (type) {
        case BSON_TYPE_NULL:
            break;
        case BSON_TYPE_BOOL:
            v = JSValue(bson_iter_bool(&iter));
            break;
        case BSON_TYPE_INT32:
        case BSON_TYPE_INT64:
        case BSON_TYPE_DOUBLE:
            v = JSValue(bson_iter_as_double(&iter));
            break;
        case BSON_TYPE_UTF8:
            v = JSValue(bson_iter_utf8(&iter, nullptr));
            break;
        case BSON_TYPE_BINARY: {
            uint32_t len;
            const uint8_t* data;
            bson_iter_binary(&iter, nullptr, &len, &data);
            v = JSValue(BinaryData(data, data + static_cast<size_t>(len)));
            break;
        }
        case BSON_TYPE_ARRAY:
        case BSON_TYPE_DOCUMENT: {
            uint32_t size;
            const uint8_t *data;
            bson_iter_array(&iter, &size, &data);
            v = JSValue(type, bson_new_from_data(data, static_cast<size_t>(size)));
            break;
        }
        default:
            break;
    }

    return v;
}

JSValue JSValue::operator[](int idx) const noexcept
{
    return getArrayItem(idx);
}

JSValue JSValue::operator[](const char* key) const noexcept
{
    return getObjectItem(key);
}

void JSValue::pushArrayItem(const JSValue& value) noexcept
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

void JSValue::setArrayItem(int /*idx*/, const JSValue& /*value*/) /*noexcept*/
{
    throw std::runtime_error("setArrayItem() not implemented");
}

void JSValue::insertArrayItem(int /*idx*/, const JSValue& /*value*/) /*noexcept*/
{
    throw std::runtime_error("insertArrayItem() not implemented");
}

void JSValue::setObjectItem(const char* /*key*/, const JSValue& /*value*/) /*noexcept*/
{
    throw std::runtime_error("setObjectItem() not implemented");
}

JSValue::BinaryData JSValue::toBSON() const
{
    if ((fType != BSON_TYPE_ARRAY) && (fType != BSON_TYPE_DOCUMENT)) {
        throw std::runtime_error("toBSON() only works for array and document types");
    }

    const uint8_t* data = bson_get_data(fArray);
    
    return BinaryData(data, data + fArray->len);
}

JSValue JSValue::fromBSON(const BinaryData& data, bool asArray) noexcept
{
    bson_t* array = bson_new_from_data(data.data(), data.size());

    if (array == nullptr) {
        return JSValue();
    }

    return JSValue(asArray ? BSON_TYPE_ARRAY : BSON_TYPE_DOCUMENT, array);
}

JSValue::JSValue(bson_type_t type, bson_t* array) noexcept
    : fType(type)
    , fArray(array)
{}

void JSValue::copy(const JSValue& v) noexcept
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

void JSValue::move(JSValue&& v) noexcept
{
    fType = v.fType;
    fArray = v.fArray;
    v.fType = BSON_TYPE_EOD;
    v.fArray = nullptr;
}

void JSValue::destroy() noexcept
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
