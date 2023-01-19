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

#include "extra/JSONVariant.hpp"
#include "extra/Base64.hpp"

USE_NAMESPACE_DISTRHO

JSONVariant::JSONVariant() noexcept
    : fImpl(cJSON_CreateNull())
{}

JSONVariant::JSONVariant(bool b) noexcept
    : fImpl(b ? cJSON_CreateTrue() : cJSON_CreateFalse())
{}

JSONVariant::JSONVariant(double d) noexcept
    : fImpl(cJSON_CreateNumber(d))
{}

JSONVariant::JSONVariant(String s) noexcept
    : fImpl(cJSON_CreateString(s))
{}

JSONVariant::JSONVariant(const BinaryData& data) noexcept
    : fImpl(cJSON_CreateString(String::asBase64(data.data(), data.size())))
{}

JSONVariant::JSONVariant(uint32_t i) noexcept
    : fImpl(cJSON_CreateNumber(static_cast<double>(i)))
{}

JSONVariant::JSONVariant(float f) noexcept
    : fImpl(cJSON_CreateNumber(static_cast<double>(f)))
{}

JSONVariant::JSONVariant(const char* s) noexcept
    : fImpl(cJSON_CreateString(s))
{}

JSONVariant::JSONVariant(std::initializer_list<JSONVariant> l) noexcept
    : fImpl(cJSON_CreateArray())
{
    for (std::initializer_list<JSONVariant>::const_iterator it = l.begin(); it != l.end(); ++it) {
        pushArrayItem(*it);
    }
}

JSONVariant::~JSONVariant()
{
    if (fImpl != nullptr) {
        cJSON_Delete(fImpl);
    }

    fImpl = nullptr;
}

JSONVariant::JSONVariant(const JSONVariant& v) noexcept
    : fImpl(cJSON_Duplicate(v.fImpl, true))
{}

JSONVariant& JSONVariant::operator=(const JSONVariant& v) noexcept
{
    if (fImpl != nullptr) {
        cJSON_Delete(fImpl);
    }

    fImpl = cJSON_Duplicate(v.fImpl, true);

    return *this;
}

JSONVariant::JSONVariant(JSONVariant&& v) noexcept
{
    fImpl = v.fImpl;
    v.fImpl = nullptr;
}

JSONVariant& JSONVariant::operator=(JSONVariant&& v) noexcept
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

JSONVariant JSONVariant::createArray() noexcept
{
    return JSONVariant(cJSON_CreateArray());
}

JSONVariant JSONVariant::createObject() noexcept
{
    return JSONVariant(cJSON_CreateObject());
}

bool JSONVariant::isNull() const noexcept
{
    return cJSON_IsNull(fImpl);
}

bool JSONVariant::isBoolean() const noexcept
{
    return cJSON_IsBool(fImpl);
}

bool JSONVariant::isNumber() const noexcept
{
    return cJSON_IsNumber(fImpl);
}

bool JSONVariant::isString() const noexcept
{
    return cJSON_IsString(fImpl);
}

bool JSONVariant::isBinaryData() const noexcept
{
    if (! isString()) {
        return false;
    }

    String s = getString();
    size_t len = s.length();
    const char *data = s.buffer();

    for (size_t i = 0; i < len; ++i) {
        if ( ! ((data[i] >= 'A') && (data[i] <= 'Z'))
            || ((data[i] >= 'a') && (data[i] <= 'z'))
            || ((data[i] >= '0') && (data[i] <= '9'))
            ||  (data[i] == '+')
            ||  (data[i] == '/')
            ||  (data[i] == '=') ) {
            return false;
        }

    }

    return true;
}

bool JSONVariant::isArray() const noexcept
{
    return cJSON_IsArray(fImpl);
}

bool JSONVariant::isObject() const noexcept
{
    return cJSON_IsObject(fImpl);
}

String JSONVariant::asString() const noexcept
{
    return toJSON();
}

bool JSONVariant::getBoolean() const noexcept
{
    return cJSON_IsTrue(fImpl);
}

double JSONVariant::getNumber() const noexcept
{
    return cJSON_GetNumberValue(fImpl);
}

String JSONVariant::getString() const noexcept
{
    return String(cJSON_GetStringValue(fImpl));
}

BinaryData JSONVariant::getBinaryData() const noexcept
{
    return d_getChunkFromBase64String(cJSON_GetStringValue(fImpl));
}

int JSONVariant::getArraySize() const noexcept
{
    return cJSON_GetArraySize(fImpl);
}

JSONVariant JSONVariant::getArrayItem(int idx) const noexcept
{
    return JSONVariant(cJSON_Duplicate(cJSON_GetArrayItem(fImpl, idx), true));
}

JSONVariant JSONVariant::getObjectItem(const char* key) const noexcept
{
    return JSONVariant(cJSON_Duplicate(cJSON_GetObjectItem(fImpl, key), true));
}

JSONVariant JSONVariant::operator[](int idx) const noexcept
{
    return getArrayItem(idx);
}

JSONVariant JSONVariant::operator[](const char* key) const noexcept
{
    return getObjectItem(key);
}

void JSONVariant::pushArrayItem(const JSONVariant& value) noexcept
{
    cJSON_AddItemToArray(fImpl, cJSON_Duplicate(value.fImpl, true));
}

void JSONVariant::setArrayItem(int idx, const JSONVariant& value) noexcept
{
    cJSON_ReplaceItemInArray(fImpl, idx, cJSON_Duplicate(value.fImpl, true));
}

void JSONVariant::insertArrayItem(int idx, const JSONVariant& value) noexcept
{
    cJSON_InsertItemInArray(fImpl, idx, cJSON_Duplicate(value.fImpl, true));
}

void JSONVariant::setObjectItem(const char* key, const JSONVariant& value) noexcept
{
    if (cJSON_HasObjectItem(fImpl, key)) {
        cJSON_ReplaceItemInObject(fImpl, key, cJSON_Duplicate(value.fImpl, true));
    } else {
        cJSON_AddItemToObject(fImpl, key, cJSON_Duplicate(value.fImpl, true));
    }
}

String JSONVariant::toJSON(bool format) const noexcept
{
    char* s = format ? cJSON_Print(fImpl) : cJSON_PrintUnformatted(fImpl);
    String jsonText = String(s);
    cJSON_free(s);

    return jsonText;
}

JSONVariant JSONVariant::fromJSON(const char* jsonText) noexcept
{
    return JSONVariant(cJSON_Parse(jsonText));
}

JSONVariant::JSONVariant(cJSON* impl) noexcept
    : fImpl(impl)
{}
