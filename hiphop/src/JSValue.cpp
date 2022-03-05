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
    : fType(TNull)
    , fContainer(nullptr)
    , fContainerOwn(false)
    , fBoolean(false)
    , fNumber(0)
{}

JSValue::JSValue(bool b) noexcept
    : fType(TBoolean)
    , fContainer(nullptr)
    , fContainerOwn(false)
    , fBoolean(b)
    , fNumber(0)
{}

JSValue::JSValue(double d) noexcept
    : fType(TNumber)
    , fContainer(nullptr)
    , fContainerOwn(false)
    , fBoolean(false)
    , fNumber(d)
{}

JSValue::JSValue(String s) noexcept
    : fType(TString)
    , fContainer(nullptr)
    , fContainerOwn(false)
    , fBoolean(false)
    , fNumber(0)
    , fString(s)
{}

JSValue::JSValue(uint32_t i) noexcept
    : fType(TNumber)
    , fContainer(nullptr)
    , fContainerOwn(false)
    , fBoolean(false)
    , fNumber(static_cast<double>(i))
{}

JSValue::JSValue(float f) noexcept
    : fType(TNumber)
    , fContainer(nullptr)
    , fContainerOwn(false)
    , fBoolean(false)
    , fNumber(static_cast<double>(f))
{}

JSValue::JSValue(const char *s) noexcept
    : fType(TString)
    , fContainer(nullptr)
    , fContainerOwn(false)
    , fBoolean(false)
    , fNumber(0)
    , fString(String(s))
{}

JSValue::JSValue(const array& a) noexcept
    : fType(TArray)
    , fContainer(static_cast<void*>(const_cast<array*>(&a)))
    , fContainerOwn(false)
    , fBoolean(false)
    , fNumber(0)
{}

JSValue::~JSValue()
{
    if (fContainerOwn) {
        if (fType == TArray) {
            delete &getArray();
        } else if (fType == TObject) {
            delete &getObject();
        }
    }
}

bool JSValue::isNull() const noexcept
{
    return fType == TNull;
}

JSValue::type JSValue::getType() const noexcept
{
    return fType;
}

bool JSValue::getBoolean() const
{
    if (fType != TBoolean) {
        throw std::runtime_error("Value type is not boolean");
    }

    return fBoolean;
}

double JSValue::getNumber() const
{
    if (fType != TNumber) {
        throw std::runtime_error("Value type is not number");
    }

    return fNumber;
}

String JSValue::getString() const
{
    if (fType != TString) {
        throw std::runtime_error("Value type is not string");
    }

    return fString;
}

JSValue::array& JSValue::getArray() const
{
    if (fType != TNull) {
        if (fType != TArray) {
            throw std::runtime_error("Value type is not array");
        }
    } else {
        fType = TArray;
        fContainer = static_cast<void*>(new array());
        fContainerOwn = true;
    }

    return *reinterpret_cast<array*>(fContainer);
}

JSValue::object& JSValue::getObject() const
{
    if (fType != TNull) {
        if (fType != TObject) {
            throw std::runtime_error("Value type is not object");
        }
    } else {
        fType = TObject;
        fContainer = static_cast<void*>(new object());
        fContainerOwn = true;
    }

    return *reinterpret_cast<object*>(fContainer);
}

String JSValue::toJSON(bool format)
{
    cJSON* json = toCJSON();
    char* s = format ? cJSON_Print(json) : cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    String jsonText = String(s);
    cJSON_free(s);

    return jsonText;
}

JSValue JSValue::fromJSON(const String& jsonText)
{
    cJSON* json = cJSON_Parse(jsonText);
    JSValue value = JSValue(json);
    cJSON_Delete(json);

    return value;
}

cJSON* JSValue::toCJSON() const noexcept
{
    cJSON* json;

    switch(fType) {
        case TBoolean:
            json = fBoolean ? cJSON_CreateTrue() : cJSON_CreateFalse();
            break;
        case TNumber:
            json = cJSON_CreateNumber(fNumber);
            break;
        case TString:
            json = cJSON_CreateString(fString);
            break;
        case TArray: {
            json = cJSON_CreateArray();
            JSValue::array& array = getArray();
            for (JSValue::array::iterator it = array.begin(); it != array.end(); ++it) {
                cJSON_AddItemToArray(json, it->toCJSON());
            }
            break;
        }
        case TObject: {
            json = cJSON_CreateObject();
            JSValue::object& object = getObject();
            for (JSValue::object::iterator it = object.begin(); it != object.end(); ++it) {
                cJSON_AddItemToObject(json, it->first, it->second.toCJSON());
            }
            break;
        }
        default:
            json = cJSON_CreateNull();
            break;
    }

    return json;
}

JSValue::JSValue(cJSON* json) noexcept
    : fType(TNull)
    , fContainer(nullptr)
    , fContainerOwn(false)
    , fBoolean(false)
    , fNumber(0)
{
    if (cJSON_IsFalse(json)) {
        fType = TBoolean;
        fBoolean = false;
    } else if (cJSON_IsTrue(json)) {
        fType = TBoolean;
        fBoolean = true;
    } else if (cJSON_IsNumber(json)) {
        fType = TNumber;
        fNumber = cJSON_GetNumberValue(json);
    } else if (cJSON_IsString(json)) {
        fType = TString;
        fString = cJSON_GetStringValue(json);
    } else if (cJSON_IsArray(json)) {
        JSValue::array& array = getArray();
        cJSON* elem;
        cJSON_ArrayForEach(elem, json) {
            array.push_back(JSValue(elem));
        }
    } else if (cJSON_IsObject(json)) {
        JSValue::object& object = getObject();
        cJSON* elem;
        cJSON_ArrayForEach(elem, json) {
            object[elem->string] = JSValue(elem);
        }
    } else {
        // null
    }
}
