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

JSValue::JSValue(std::initializer_list<JSValue> l) noexcept
    : fImpl(cJSON_CreateArray())
{
    for (std::initializer_list<JSValue>::const_iterator it = l.begin(); it != l.end(); ++it) {
        push(*it);
    }
}

JSValue::JSValue(const JSValue& v) noexcept
{
    fImpl = cJSON_Duplicate(v.fImpl, true/*recurse*/);
}

JSValue& JSValue::operator=(const JSValue& v) noexcept
{
    cJSON_Delete(fImpl);
    fImpl = cJSON_Duplicate(v.fImpl, true/*recurse*/);

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

JSValue::JSValue(const vector& v) noexcept
    : fImpl(cJSON_CreateArray())
{
    for (vector::const_iterator it = v.begin(); it != v.end(); ++it) {
        push(*it);
    }
}

JSValue::vector JSValue::toVector() noexcept
{
    vector v;
    int size = cJSON_GetArraySize(fImpl);

    for (int i = 0; i < size; ++i) {
        v.push_back(JSValue(cJSON_Duplicate(cJSON_GetArrayItem(fImpl, i), true)));
    }

    return v;
}

JSValue::~JSValue()
{
    if (fImpl != nullptr) {
        cJSON_Delete(fImpl);
        fImpl = nullptr;
    }
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

JSValue JSValue::get(int idx) const noexcept
{
    return JSValue(cJSON_Duplicate(cJSON_GetArrayItem(fImpl, idx), true));
}

JSValue JSValue::get(const char* key) const noexcept
{
    return JSValue(cJSON_Duplicate(cJSON_GetObjectItem(fImpl, key), true));
}

void JSValue::push(const JSValue& value)
{
    cJSON_AddItemToArray(fImpl, cJSON_Duplicate(value.fImpl, true));
}

void JSValue::set(const char* key, const JSValue& value)
{
    cJSON_AddItemToObject(fImpl, key, cJSON_Duplicate(value.fImpl, true));
}

String JSValue::toJSON(bool format) noexcept
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
