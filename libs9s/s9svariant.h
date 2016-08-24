/* 
 * Copyright (C) 2011-2015 severalnines.com
 */
#pragma once

#include "s9sunion.h"
#include "S9sString"

class S9sVariantMap;
class S9sVariantList;

class S9sVariant
{
    public:
        inline S9sVariant();
        S9sVariant(const S9sVariant &orig);
        inline S9sVariant(const int integerValue);
        inline S9sVariant(const ulonglong ullValue);
        inline S9sVariant(double doubleValue);
        inline S9sVariant(const bool boolValue);
        inline S9sVariant(const char *stringValue);
        inline S9sVariant(const std::string &stringValue);
        inline S9sVariant(const S9sString &stringValue);
        
        S9sVariant(const S9sVariantMap &mapValue);
        S9sVariant(const S9sVariantList &listValue);

        virtual ~S9sVariant();

        S9sVariant &operator=(const S9sVariant &rhs);
        S9sVariant &operator[] (const S9sString &index);

        bool isInvalid() const { return m_type == Invalid; };
        bool isInt() const { return m_type == Int; };
        bool isULongLong() const { return m_type == Ulonglong; };
        bool isDouble() const { return m_type == Double; };
        int toInt(const int defaultValue = 0) const;
        ulonglong toULongLong(ulonglong defaultValue = 0ull) const;
        
        bool isNumber() const { 
            return isInt() || isULongLong() || isDouble(); }; 
        bool isString() const { return m_type == String; };
        bool isVariantMap() const { return m_type == Map; };

        bool toBoolean(const bool defaultValue = false) const;
        double toDouble(const double defaultValue = 0.0) const;
        S9sString toString() const;
        const S9sVariantMap &toVariantMap() const;
        const S9sVariantList &toVariantList() const;

        void clear();

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
        double doubleValue) :
    m_type (Double)
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

