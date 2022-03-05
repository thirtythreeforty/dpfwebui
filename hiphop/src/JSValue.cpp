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
    : fType(TNull)
    , fBoolean(false)
    , fNumber(0)
    , fContainer(nullptr)
{}

JSValue::JSValue(bool b) noexcept
    : fType(TBoolean)
    , fBoolean(b)
    , fNumber(0)
    , fContainer(nullptr)
{}

JSValue::JSValue(double d) noexcept
    : fType(TNumber)
    , fBoolean(false)
    , fNumber(d)
    , fContainer(nullptr)
{}

JSValue::JSValue(String s) noexcept
    : fType(TString)
    , fBoolean(false)
    , fNumber(0)
    , fString(s)
    , fContainer(nullptr)
{}

JSValue::JSValue(uint32_t i) noexcept
    : fType(TNumber)
    , fBoolean(false)
    , fNumber(static_cast<double>(i))
    , fContainer(nullptr)
{}

JSValue::JSValue(float f) noexcept
    : fType(TNumber)
    , fBoolean(false)
    , fNumber(static_cast<double>(f))
    , fContainer(nullptr)
{}

JSValue::JSValue(const char *s) noexcept
    : fType(TString)
    , fBoolean(false)
    , fNumber(0)
    , fString(String(s))
    , fContainer(nullptr)
{}

JSValue::~JSValue()
{
    if (fType == TArray) {
        delete &getArray();
    } else if (fType == TObject) {
        delete &getObject();
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
    }

    return *reinterpret_cast<object*>(fContainer);
}

JSValue JSValue::fromJSON(const String& jsonText)
{
    cJSON* json = cJSON_Parse(jsonText);
    JSValue value = JSValue(json);
    cJSON_free(json);

    return value;
}

String JSValue::toJSON(const JSValue& value, bool format)
{
    cJSON* json = value.toCJSON();
    char* s = format ? cJSON_Print(json) : cJSON_PrintUnformatted(json);
    cJSON_free(json);

    String jsonText = String(s);
    cJSON_free(s);

    return jsonText;
}

JSValue::JSValue(cJSON* json) noexcept
    : fType(TNull)
    , fBoolean(false)
    , fNumber(0)
    , fContainer(nullptr)
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
                cJSON* item = it->toCJSON();
                cJSON_AddItemToArray(json, item);
                cJSON_free(item);
            }
            break;
        }
        case TObject: {
            json = cJSON_CreateObject();
            JSValue::object& object = getObject();
            for (JSValue::object::iterator it = object.begin(); it != object.end(); ++it) {
                cJSON* item = it->second.toCJSON();
                cJSON_AddItemToObject(json, it->first, item);
                cJSON_free(item);
            }
            break;
        }
        default:
            json = cJSON_CreateNull();
            break;
    }

    return json;
}

std::ostream& operator<<(std::ostream &os, const JSValue &val) {
    switch (val.getType()) {
        case JSValue::TNull:
            os << "null";
            break;

        case JSValue::TBoolean:
            os << (val.getBoolean() ? "true" : "false");
            break;

        case JSValue::TNumber: {
            const double d = val.getNumber();
            if (std::isnan(d)) {
                os << "NaN";
            } else if (std::isinf(d)) {
                os << (d < 0 ? "-Inf" : "Inf");
            } else {
                os << d;
            }
            break;
        }

        case JSValue::TString: {
            const String& s = val.getString();
            const char *buf = s.buffer();
            const int len = s.length();
            os << '"';
            for (int i = 0; i < len; i++) {
                if (buf[i] != '"') {
                    os << buf[i];
                } else {
                    os << "\\\"";
                }
            }
            os << '"';
            break;
        }

        default:
            break;
    }

    return os;
}
