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

#include <stdexcept>

#include "extra/JSValue.hpp"

USE_NAMESPACE_DISTRHO

JSValue::JSValue() noexcept
    : fStorage(cJSON_CreateNull())
    , fContainer(nullptr)
{}

JSValue::JSValue(bool b) noexcept
    : fStorage(b ? cJSON_CreateTrue() : cJSON_CreateFalse())
    , fContainer(nullptr)
{}

JSValue::JSValue(double d) noexcept
    : fStorage(cJSON_CreateNumber(d))
    , fContainer(nullptr)
{}

JSValue::JSValue(String s) noexcept
    : fStorage(cJSON_CreateString(s))
    , fContainer(nullptr)
{}

JSValue::JSValue(uint32_t i) noexcept
    : fStorage(cJSON_CreateNumber(static_cast<double>(i)))
    , fContainer(nullptr)
{}

JSValue::JSValue(float f) noexcept
    : fStorage(cJSON_CreateNumber(static_cast<double>(f)))
    , fContainer(nullptr)
{}

JSValue::JSValue(const char* s) noexcept
    : fStorage(cJSON_CreateString(s))
    , fContainer(nullptr)
{}

JSValue::JSValue(const array& a) noexcept
    : fStorage(cJSON_CreateArray())
    , fContainer(static_cast<void*>(new array(a)))
{}

JSValue::JSValue(const JSValue& v) noexcept
{
    copy(v);
}

JSValue& JSValue::operator=(const JSValue& v)
{
    clear();
    copy(v);

    return *this;
}

JSValue JSValue::createArray() noexcept
{
    return JSValue(cJSON_CreateArray(), false/*copy*/, true/*createCont*/);
}

JSValue JSValue::createObject() noexcept
{
    return JSValue(cJSON_CreateObject(), false/*copy*/, true/*createCont*/);
}

JSValue::~JSValue()
{
    clear();
}

bool JSValue::isNull() const noexcept
{
    return cJSON_IsNull(fStorage);
}

bool JSValue::isBoolean() const noexcept
{
    return cJSON_IsBool(fStorage);
}

bool JSValue::getBoolean() const noexcept
{
    return cJSON_IsTrue(fStorage);
}

bool JSValue::isNumber() const noexcept
{
    return cJSON_IsNumber(fStorage);
}

double JSValue::getNumber() const noexcept
{
    return cJSON_GetNumberValue(fStorage);
}

bool JSValue::isString() const noexcept
{
    return cJSON_IsString(fStorage);
}

String JSValue::getString() const noexcept
{
    return String(cJSON_GetStringValue(fStorage));
}

bool JSValue::isArray() const noexcept
{
    return cJSON_IsArray(fStorage);
}

JSValue::array& JSValue::getArray() const
{
    if (! isArray()) {
        throw std::runtime_error("Value is not array");
    }

    return *reinterpret_cast<array*>(fContainer);
}

bool JSValue::isObject() const noexcept
{
    return cJSON_IsObject(fStorage);
}

JSValue::object& JSValue::getObject() const
{
    if (! isObject()) {
        throw std::runtime_error("Value is not object");
    }

    return *reinterpret_cast<object*>(fContainer);
}

String JSValue::toJSON(bool format) noexcept
{
    // Populate cJSON arrays and objects
    cJSONConnectTree();

    char* s = format ? cJSON_Print(fStorage) : cJSON_PrintUnformatted(fStorage);
    String jsonText = String(s);
    cJSON_free(s);

    // Empty cJSON arrays and objects:
    // - toJSON() could be called multiple times
    // - Avoid double freeing in ~JSValue() because cJSON_Delete() is recursive,
    //   cJSON objects lifetime must be only managed by its JSValue wrapper.
    cJSONDisconnectTree();

    return jsonText;
}

JSValue JSValue::fromJSON(const char* jsonText) noexcept
{
    return JSValue(cJSON_Parse(jsonText), true/*copy*/, true/*createCont*/);
}

JSValue::JSValue(cJSON* json, bool copy, bool createContainer) noexcept
    : fStorage(copy ? cJSON_Duplicate(json, false/*recurse*/) : json)
    , fContainer(nullptr)
{
    if (! createContainer) {
        return;
    }
    if (cJSON_IsArray(json)) {
        array* a = new array();
        fContainer = static_cast<void*>(a);
        cJSON* elem;
        cJSON_ArrayForEach(elem, json) {
            a->push_back(JSValue(elem, true/*copy*/, true/*createCont*/));
        }
    } else if (cJSON_IsArray(json)) {
        object* o = new object();
        fContainer = static_cast<void*>(o);
        cJSON* elem;
        cJSON_ArrayForEach(elem, json) {
            (*o)[elem->string] = JSValue(elem, true/*copy*/, true/*createCont*/);
        }
    }
}

void JSValue::cJSONConnectTree() noexcept
{
    if (isArray()) {
        JSValue::array& array = getArray();
        for (JSValue::array::iterator it = array.begin(); it != array.end(); ++it) {
            cJSON_AddItemReferenceToArray(fStorage, it->fStorage);
            it->cJSONConnectTree();
        }
    } else if (isObject()) {
        JSValue::object& object = getObject();
        for (JSValue::object::iterator it = object.begin(); it != object.end(); ++it) {
            cJSON_AddItemReferenceToObject(fStorage, it->first.c_str(), it->second.fStorage);
            it->second.cJSONConnectTree();
        }
    }
}

void JSValue::cJSONDisconnectTree() noexcept
{
    if (isArray()) {
        JSValue::array& array = getArray();
        for (size_t i = 0, size = array.size(); i < size; ++i) {
            cJSON_DetachItemFromArray(fStorage, i);
            array[i].cJSONDisconnectTree();
        }
    } else if (isObject()) {
        JSValue::object& object = getObject();
        for (JSValue::object::iterator it = object.begin(); it != object.end(); ++it) {
            cJSON_DetachItemFromObject(fStorage, it->first.c_str());
            it->second.cJSONDisconnectTree();
        }
    }
}

void JSValue::clear() noexcept
{
    if (fContainer != nullptr) {
        if (isArray()) {
            delete &getArray();
        } else if (isObject()) {
            delete &getObject();
        }

        fContainer = nullptr;
    }

    if (fStorage != nullptr) {
        cJSON_Delete(fStorage);
        fStorage = nullptr;
    }
}

void JSValue::copy(const JSValue& v) noexcept
{
    fStorage = cJSON_Duplicate(v.fStorage, false/*recurse*/);

    if (v.isArray()) {
        fContainer = static_cast<void*>(new array(v.getArray()));
    } else if (v.isObject()) {
        fContainer = static_cast<void*>(new object(v.getObject()));
    }
}
