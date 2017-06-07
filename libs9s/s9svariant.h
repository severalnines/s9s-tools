/*
 * Severalnines Tools
 * Copyright (C) 2016  Severalnines AB
 *
 * This file is part of s9s-tools.
 *
 * s9s-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * S9sTools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with S9sTools. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "s9sunion.h"
#include "S9sString"

class S9sNode;
class S9sVariantMap;
class S9sVariantList;

class S9sVariant
{
    public:
        enum TextFormat
        {
            NotSpecified       = 0,
            StringFormat, 
            GeneralNumber,
            IntegerNumber,
            ScientificNumber,
            TwoDecimalNumber,
            FourDecimalNumber,
            HighPrecDecimalNumber,
            IntegerPercent,
            TwoDecimalPercent,
            ShortTime,
            LongTime,
            ShortDate,
            DateTime,
            ElapsedTime,
            ElapsedTimeMicros,
            Boolean,
            Celsius,
            Kelvin,
            Bytes,
            BytesShort,
            BytesPerSecShort,
        };

        inline S9sVariant();
        S9sVariant(const S9sVariant &orig);
        inline S9sVariant(const int integerValue);
        inline S9sVariant(const ulonglong ullValue);
        inline S9sVariant(const double doubleValue);
        inline S9sVariant(const bool boolValue);
        inline S9sVariant(const char *stringValue);
        inline S9sVariant(const std::string &stringValue);
        inline S9sVariant(const S9sString &stringValue);
        S9sVariant(const S9sNode &nodeValue);
        S9sVariant(const S9sAccount &accountValue);
        
        S9sVariant(const S9sVariantMap &mapValue);
        S9sVariant(const S9sVariantList &listValue);

        virtual ~S9sVariant();

        S9sVariant &operator=(const S9sVariant &rhs);
        bool operator==(const S9sVariant &rhs) const;
        bool operator!=(const S9sVariant &rhs) const;
        S9sVariant &operator+=(const S9sVariant &rhs);
        S9sVariant &operator/=(const int rhs);
        bool operator<(const S9sVariant &rhs) const;
        bool operator>(const S9sVariant &rhs) const;
        S9sVariant &operator[] (const S9sString &index);
        S9sVariant &operator[] (const int &index);

        S9sBasicType type() const { return m_type; };
        S9sString typeName() const;
        
        bool isInvalid() const { return m_type == Invalid; };
        bool isBoolean() const { return m_type == Bool; };
        bool isInt() const { return m_type == Int; };
        bool isULongLong() const { return m_type == Ulonglong; };
        bool isDouble() const { return m_type == Double; };
        bool isNumber() const { 
            return isInt() || isULongLong() || isDouble(); }; 
        bool isString() const { return m_type == String; };
        bool isVariantMap() const { return m_type == Map; };
        bool isVariantList() const { return m_type == List; };
        bool isNode() const { return m_type == Node; };
        bool isAccount() const { return m_type == Account; };

        int size() const;

        int toInt(const int defaultValue = 0) const;
        ulonglong toULongLong(ulonglong defaultValue = 0ull) const; 
        time_t toTimeT() const;
        bool toBoolean(const bool defaultValue = false) const;
        double toDouble(const double defaultValue = 0.0) const;
        double toGBytes() const;
        double toTBytes() const;

        S9sString toString() const;
        S9sString toString(const S9sVariant::TextFormat format) const;

        const S9sVariantMap &toVariantMap() const;
        const S9sNode &toNode() const;
        const S9sAccount &toAccount() const;
        const S9sVariantList &toVariantList() const;

        bool contains(const S9sVariant &value) const;
        bool contains(const S9sString &key) const;
        bool contains(const char *key) const;
        void clear();

    protected:
        static bool fuzzyCompare(double first, double second);
        void additionWithOverflow(const int arg1, const int arg2);

    private:
        static const S9sVariantMap  sm_emptyMap;
        static const S9sVariantList sm_emptyList;

    private:
        S9sBasicType    m_type;
        S9sUnion        m_union;
};

inline 
S9sVariant::S9sVariant() :
    m_type(Invalid)
{
}

inline 
S9sVariant::S9sVariant(
        const int integerValue) :
    m_type(Int)
{
    m_union.iVal = integerValue;
}

inline 
S9sVariant::S9sVariant(
        const ulonglong ullValue) :
    m_type (Ulonglong)
{
    m_union.ullVal = ullValue;
}

inline 
S9sVariant::S9sVariant(
        const double doubleValue) :
    m_type(Double)
{
    m_union.dVal = doubleValue;
}

inline 
S9sVariant::S9sVariant(
        const bool boolValue) :
    m_type (Bool)
{
    m_union.bVal = boolValue;
}

inline 
S9sVariant::S9sVariant(
        const char *stringValue) :
    m_type (String)
{
    if (stringValue == NULL)
        m_union.stringValue = new S9sString;
    else
        m_union.stringValue = new S9sString(stringValue);
}

inline 
S9sVariant::S9sVariant(
        const std::string &stringValue) :
    m_type (String)
{
    m_union.stringValue = new S9sString(stringValue);
}

inline 
S9sVariant::S9sVariant(
        const S9sString &stringValue) :
    m_type (String)
{
    m_union.stringValue = new S9sString(stringValue);
}

