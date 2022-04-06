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

#include "extra/JSValue.hpp"

USE_NAMESPACE_DISTRHO

JSValue::JSValue() noexcept
    : fImpl(cJSON_CreateNull())
    , fOwn(true)
{}

JSValue::JSValue(bool b) noexcept
    : fImpl(b ? cJSON_CreateTrue() : cJSON_CreateFalse())
    , fOwn(true)
{}

JSValue::JSValue(double d) noexcept
    : fImpl(cJSON_CreateNumber(d))
    , fOwn(true)
{}

JSValue::JSValue(String s) noexcept
    : fImpl(cJSON_CreateString(s))
    , fOwn(true)
{}

JSValue::JSValue(uint32_t i) noexcept
    : fImpl(cJSON_CreateNumber(static_cast<double>(i)))
    , fOwn(true)
{}

JSValue::JSValue(float f) noexcept
    : fImpl(cJSON_CreateNumber(static_cast<double>(f)))
    , fOwn(true)
{}

JSValue::JSValue(const char* s) noexcept
    : fImpl(cJSON_CreateString(s))
    , fOwn(true)
{}

JSValue::JSValue(std::initializer_list<JSValue> l) noexcept
    : fImpl(cJSON_CreateArray())
    , fOwn(true)
{
    for (std::initializer_list<JSValue>::const_iterator it = l.begin(); it != l.end(); ++it) {
        pushArrayItem(*it);
    }
}

JSValue::JSValue(const JSValue& v) noexcept
    : fImpl(cJSON_Duplicate(v.fImpl, true/*recurse*/))
    , fOwn(true)
{}

JSValue& JSValue::operator=(const JSValue& v) noexcept
{
    if (fOwn) {
        cJSON_Delete(fImpl);
    }

    fImpl = cJSON_Duplicate(v.fImpl, true/*recurse*/);
    fOwn = true;

    return *this;
}

JSValue::JSValue(JSValue&& v) noexcept
{
    fImpl = v.fImpl;
    fOwn = v.fOwn;
    v.fImpl = nullptr;
    v.fOwn = false;
}

JSValue& JSValue::operator=(JSValue&& v) noexcept
{
    if (this != &v) {
        if (fOwn) {
            cJSON_Delete(fImpl);
        }

        fImpl = v.fImpl;
        fOwn = v.fOwn;
        v.fImpl = nullptr;
        v.fOwn = false;
    }

   return *this;
}

JSValue JSValue::createArray() noexcept
{
    return JSValue(cJSON_CreateArray(), true/*own*/);
}

JSValue JSValue::createObject() noexcept
{
    return JSValue(cJSON_CreateObject(), true/*own*/);
}

JSValue::~JSValue()
{
    if (fOwn && (fImpl != nullptr)) {
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

int JSValue::getArraySize() const noexcept
{
    return cJSON_GetArraySize(fImpl);
}

JSValue JSValue::getArrayItem(int idx) const noexcept
{
    return JSValue(cJSON_GetArrayItem(fImpl, idx), false/*own*/);
}

JSValue JSValue::getObjectItem(const char* key) const noexcept
{
    return JSValue(cJSON_GetObjectItem(fImpl, key), false/*own*/);
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
    cJSON_AddItemToArray(fImpl, cJSON_Duplicate(value.fImpl, true/*recurse*/));
}

void JSValue::insertArrayItem(int idx, const JSValue& value) noexcept
{
    cJSON_InsertItemInArray(fImpl, idx, cJSON_Duplicate(value.fImpl, true/*recurse*/));
}

void JSValue::setObjectItem(const char* key, const JSValue& value) noexcept
{
    cJSON_AddItemToObject(fImpl, key, cJSON_Duplicate(value.fImpl, true/*recurse*/));
}

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

String JSValue::toJSON(bool format) const noexcept
{
    char* s = format ? cJSON_Print(fImpl) : cJSON_PrintUnformatted(fImpl);
    String jsonText = String(s);
    cJSON_free(s);

    return jsonText;
}

JSValue JSValue::fromJSON(const char* jsonText) noexcept
{
    return JSValue(cJSON_Parse(jsonText), true/*own*/);
}

JSValue::JSValue(cJSON* impl, bool own) noexcept
    : fImpl(impl)
    , fOwn(own)
{}
