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

#ifndef JS_VALUE_HPP
#define JS_VALUE_HPP

#include <exception>
#include <ostream>
#include <unordered_map>
#include <vector>

#include "distrho/extra/String.hpp"

START_NAMESPACE_DISTRHO

class JSValue
{
public:
    typedef std::vector<JSValue> array;
    typedef std::unordered_map<const char*,JSValue> object;

    enum Type {
        TNull,
        TBoolean,
        TNumber,
        TString,
        TArray,
        TObject
    };

    //
    // Constructors
    //

    JSValue() noexcept
        : fType(TNull)
        , fBoolean(false)
        , fNumber(0)
        , fContainer(nullptr)
    {}

    JSValue(bool b) noexcept
        : fType(TBoolean)
        , fBoolean(b)
        , fNumber(0)
        , fContainer(nullptr)
    {}

    JSValue(double d) noexcept
        : fType(TNumber)
        , fBoolean(false)
        , fNumber(d)
        , fContainer(nullptr)
    {}

    JSValue(String s) noexcept
        : fType(TString)
        , fBoolean(false)
        , fNumber(0)
        , fString(s)
        , fContainer(nullptr)
    {}

    //
    // Convenience constructors
    //

    JSValue(uint32_t i) noexcept
        : fType(TNumber)
        , fBoolean(false)
        , fNumber(static_cast<double>(i))
        , fContainer(nullptr)
    {}

    JSValue(float f) noexcept
        : fType(TNumber)
        , fBoolean(false)
        , fNumber(static_cast<double>(f))
        , fContainer(nullptr)
    {}

    JSValue(const char *s) noexcept
        : fType(TString)
        , fBoolean(false)
        , fNumber(0)
        , fString(String(s))
        , fContainer(nullptr)
    {}

    //
    // Destructor
    //

    ~JSValue()
    {
        if (fType == TArray) {
            delete &getArray();
        } else if (fType == TObject) {
            delete &getObject();
        }
    }

    //
    // Getters
    //

    bool isNull() const noexcept
    {
        return fType == TNull;
    }

    Type getType() const noexcept
    {
        return fType;
    }

    bool getBoolean() const
    {
        if (fType != TBoolean) {
            throw std::runtime_error("Value type is not boolean");
        }

        return fBoolean;
    }

    double getNumber() const
    {
        if (fType != TNumber) {
            throw std::runtime_error("Value type is not number");
        }

        return fNumber;
    }

    String getString() const
    {
        if (fType != TString) {
            throw std::runtime_error("Value type is not string");
        }

        return fString;
    }
    
    array& getArray()
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

    object& getObject()
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

    //
    // Type casting operators
    //

    operator bool()   const noexcept { return fBoolean; }
    operator double() const noexcept { return fNumber; }
    operator String() const noexcept { return fString; }
    operator array()  noexcept { return getArray(); }
    operator object() noexcept { return getObject(); }

private:
    Type   fType;
    bool   fBoolean;
    double fNumber;
    String fString;
    void*  fContainer;

};

END_NAMESPACE_DISTRHO

std::ostream& operator<<(std::ostream &os, const DISTRHO::JSValue &val);

#endif // JS_VALUE_HPP
