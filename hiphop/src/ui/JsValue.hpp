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

#include <ostream>
#include <vector>

#include "distrho/extra/String.hpp"

START_NAMESPACE_DISTRHO

class JsValue {
public:
    enum Type {
        TNull,
        TBool,
        TDouble,
        TString
    };

    JsValue() noexcept
        : fT(TNull)
        , fB(false)
        , fD(0)
    {}

    JsValue(bool b) noexcept
        : fT(TBool)
        , fB(b)
        , fD(0)
    {}

    JsValue(double d) noexcept
        : fT(TDouble)
        , fB(false)
        , fD(d)
    {}

    JsValue(String s) noexcept
        : fT(TString)
        , fB(false)
        , fD(0)
        , fS(s)
    {}

    //
    // Convenience constructors
    //

    JsValue(uint32_t i) noexcept
        : fT(TDouble)
        , fB(false)
        , fD(static_cast<double>(i))
    {}

    JsValue(float f) noexcept
        : fT(TDouble)
        , fB(false)
        , fD(static_cast<double>(f))
    {}

    JsValue(const char *s) noexcept
        : fT(TString)
        , fB(false)
        , fD(0)
        , fS(String(s))
    {}

    //
    // Getters
    //

    bool isNull() const noexcept
    {
        return fT == TNull;
    }

    Type getType() const noexcept
    {
        return fT;
    }

    bool getBool() const noexcept
    {
        return fB;
    }

    double getDouble() const noexcept
    {
        return fD;
    }

    String getString() const noexcept
    {
        return fS;
    }

    //
    // Type casting operators
    //

    operator bool() const noexcept
    {
        return fB;
    }

    operator double() const noexcept
    {
        return fD;
    }

    operator String() const noexcept
    {
        return fS;
    }

private:
    Type   fT;
    bool   fB;
    double fD;
    String fS;

};

END_NAMESPACE_DISTRHO

typedef std::vector<JsValue> JsValueVector;

std::ostream& operator<<(std::ostream &os, const JsValue &val);

#endif // JS_VALUE_HPP
