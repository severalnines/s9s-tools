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
#include "s9svariant.h"
#include "S9sVariantMap"
#include "S9sVariantList"

#include <errno.h>
#include <strings.h>
#include <stdlib.h>
#include <climits>
#include <cmath>
#include <limits> 

#include "S9sNode"
#include "S9sAccount"
#include "S9sDateTime"

#define DEBUG
#define WARNING
#include "s9sdebug.h"

static const double tbyte = 1024.0 * 1024.0 * 1024.0 * 1024.0;
static const double gbyte = 1024.0 * 1024.0 * 1024.0;
static const double mbyte = 1024.0 * 1024.0;
static const double kbyte = 1024.0;

const S9sVariantMap  S9sVariant::sm_emptyMap;
const S9sVariantList S9sVariant::sm_emptyList = S9sVariantList();
static const S9sNode             sm_emptyNode;
static const S9sAccount          sm_emptyAccount;

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC visibility push(hidden)
#endif

S9sVariant::S9sVariant(
        const S9sVariant &orig)
{
    m_type         = orig.m_type;

    switch (m_type)
    {
        case Invalid:
        case Int:
        case Ulonglong:
        case Double:
        case Bool:
            /* We don't need to copy here. */
            m_union = orig.m_union;
            break;
        
        case String:
            m_union.stringValue = new S9sString(*orig.m_union.stringValue);
            break;

        case List:
            m_union.listValue = new S9sVariantList(*orig.m_union.listValue);
            break;

        case Map:
            m_union.mapValue = new S9sVariantMap(*orig.m_union.mapValue);
            break;

        case Node:
            m_union.nodeValue = new S9sNode(*orig.m_union.nodeValue);
            break;

        case Account:
            m_union.accountValue = new S9sAccount(*orig.m_union.accountValue);
    }
}

S9sVariant::S9sVariant(
        const S9sNode &nodeValue) :
    m_type (Node)
{
    m_union.nodeValue = new S9sNode(nodeValue);
}

S9sVariant::S9sVariant(
        const S9sAccount &accountValue) :
    m_type (Account)
{
    m_union.accountValue = new S9sAccount(accountValue);
}

S9sVariant::S9sVariant(
        const S9sVariantMap &mapValue) :
    m_type(Map)
{
    m_union.mapValue = new S9sVariantMap(mapValue);
}

S9sVariant::S9sVariant(
        const S9sVariantList &listValue) :
    m_type(List)
{
    m_union.listValue = new S9sVariantList(listValue);
}

S9sVariant::~S9sVariant()
{
    clear();
}

/**
 * Assignment operator for the S9sVariant class that accepts an other S9sVariant
 * object as right hand side argument.
 */
S9sVariant &
S9sVariant::operator=(
        const S9sVariant &rhs)
{
    if (this == &rhs)
        return *this;

    clear();

    m_type         = rhs.m_type;
    
    switch (m_type)
    {
        case Invalid:
        case Int:
        case Ulonglong:
        case Double:
        case Bool:
            /* We don't need to copy here. */
            m_union = rhs.m_union;
            break;
        
        case String:
            m_union.stringValue = new S9sString(*rhs.m_union.stringValue);
            break;

        case List:
            m_union.listValue = new S9sVariantList(*rhs.m_union.listValue);
            break;

        case Map:
            m_union.mapValue = new S9sVariantMap(*rhs.m_union.mapValue);
            break;

        case Node:
            m_union.nodeValue = new S9sNode(*rhs.m_union.nodeValue);
            break;

        case Account:
            m_union.accountValue = new S9sAccount(*rhs.m_union.accountValue);
            break;
    }
    
    return *this;
}

bool 
S9sVariant::operator== (
        const S9sVariant &rhs) const
{
    if (isInt() && rhs.isInt())
    {
        return toInt() == rhs.toInt();
    } else if (isULongLong() && rhs.isULongLong()) 
    {
        return toULongLong() == rhs.toULongLong();
    } else if (isDouble() && rhs.isDouble()) 
    {
        return fuzzyCompare(toDouble(), rhs.toDouble());
    } else if (isString() && rhs.isString())
    {
        return toString() == rhs.toString();
    } else if (isNumber() && rhs.isNumber())
    {
        return fuzzyCompare(toDouble(), rhs.toDouble());
    } else if (isBoolean() && rhs.isBoolean())
    {
        return toBoolean() == rhs.toBoolean();
    } else if ((isString() && !rhs.isString())
            || (!isString() && rhs.isString()))
    {
        // It seems that comparing a string with other than a string returning
        // true is rather counterintuitive. 
        return false;
    } else {
        //S9S_WARNING("TBD: (%s)%s == (%s)%s", 
        //        STR(toString()), STR(typeName()),
        //        STR(rhs.toString()), STR(rhs.typeName()));
        return false;
    }

    return false;
}

bool 
S9sVariant::operator!= (
        const S9sVariant &rhs) const
{
    if (isInt() && rhs.isInt())
    {
        return toInt() != rhs.toInt();
    } else if (isULongLong() && rhs.isULongLong()) 
    {
        return toULongLong() != rhs.toULongLong();
    } else if (isDouble() && rhs.isDouble()) 
    {
        return !fuzzyCompare(toDouble(), rhs.toDouble());
    } else if (isString() && rhs.isString())
    {
        return toString() != rhs.toString();
    } else if (isNumber() && rhs.isNumber())
    {
        return !fuzzyCompare(toDouble(), rhs.toDouble());
    } else if (isBoolean() && rhs.isBoolean())
    {
        return toBoolean() != rhs.toBoolean();
    } else if ((isString() && !rhs.isString())
            || (!isString() && rhs.isString()))
    {
        // It seems that comparing a string with other than a string returning
        // true is rather counterintuitive. 
        return false;
    } else {
        //S9S_WARNING("TBD: (%s)%s == (%s)%s", 
        //        STR(toString()), STR(typeName()),
        //        STR(rhs.toString()), STR(rhs.typeName()));
        return false;
    }

    return false;
}

S9sVariant & 
S9sVariant::operator+=(
        const S9sVariant &rhs) 
{
    if (this->isInvalid())
    {
        *this = rhs;
    } else if (this->isInt() && rhs.isInt())
    {
        additionWithOverflow(toInt(), rhs.toInt());
    } else if (this->isNumber() && rhs.isNumber())
    {
        *this = toDouble() + rhs.toDouble();
    } else {
        *this = toString() + rhs.toString();
    }

    return *this;
}

S9sVariant &
S9sVariant::operator/=(
        const int rhs)
{
    if (isInt())
    {
        *this = toInt() / rhs;
    } else if (isULongLong())
    {
        *this = toULongLong() / rhs;
    } else if (isNumber())
    {
        *this = toDouble() / rhs;
    }

    return *this;
}

bool 
S9sVariant::operator< (
        const S9sVariant &rhs) const
{
    if (isInt() && rhs.isInt())
        return toInt() < rhs.toInt();
    else if (isULongLong() && rhs.isULongLong())
        return toULongLong() < rhs.toULongLong();
    else if (isNumber() && rhs.isNumber())
        return toDouble() < rhs.toDouble();
    else if (isString() && rhs.isString())
        return toString() < rhs.toString();

    return false;
}

bool 
S9sVariant::operator> (
        const S9sVariant &rhs) const
{
    if (rhs.isInvalid())
    {
        return true;
    } else if (isInt() && rhs.isInt())
    {
        return toInt() > rhs.toInt();
    } else if (isULongLong() && rhs.isULongLong())
    {
        return toULongLong() > rhs.toULongLong();
    } else if (isNumber() && rhs.isNumber())
    {
        return toDouble() > rhs.toDouble();
    } else if (isString() && rhs.isString())
    {
        return toString() > rhs.toString();
    }

    return false;
}

S9sVariant &
S9sVariant::operator[] (
        const int &index)
{
    if (m_type == Invalid)
    {
        *this = S9sVariantList();
        return this->operator[](index);
    } else if (m_type == List)
    {
        return m_union.listValue->S9sVariantList::operator[](index);
    }
    
    S9S_WARNING("");
    S9S_WARNING("Unhandled type %s", STR(typeName()));
    S9S_WARNING("*** value: %s", STR(toString()));
    assert(false);
}

S9sVariant &
S9sVariant::operator[] (
        const S9sString &index)
{
    if (m_type == Invalid)
    {
        *this = S9sVariantMap();
        return this->operator[](index);
    } else if (m_type == Map)
    {
        return m_union.mapValue->S9sMap<
                S9sString, S9sVariant>::operator[](index);
    } 
   
    S9S_WARNING("Unhandled type %s", STR(typeName()));
    S9S_WARNING("*** value: %s", STR(toString()));
    assert(false);
}

S9sString 
S9sVariant::typeName() const
{
    S9sString retval;

    switch (m_type)
    {
        case Invalid:
            retval = "invalid";
            break;

        case Int:
            retval = "int";
            break;

        case Ulonglong:
            retval = "ulonglong";
            break;

        case Double:
            retval = "double";
            break;

        case Bool:
            retval = "bool";
            break;
        
        case String:
            retval = "string";
            break;
        
        case Node:
            retval = "node";
            break;
        
        case Account:
            retval = "account";
            break;

        case List:
            retval = "list";
            break;

        case Map:
            retval = "map";
            break;
    }

    return retval;
}

const S9sNode &
S9sVariant::toNode() const
{
    switch (m_type)
    {
        case Invalid:
        case Int:
        case Ulonglong:
        case Double:
        case Bool:
        case String:
        case List:
        case Map:
        case Account:
            return sm_emptyNode;

        case Node:
            return *m_union.nodeValue;
    }
            
    return sm_emptyNode;
}

const S9sAccount &
S9sVariant::toAccount() const
{
    switch (m_type)
    {
        case Invalid:
        case Int:
        case Ulonglong:
        case Double:
        case Bool:
        case String:
        case List:
        case Map:
        case Node:
            return sm_emptyAccount;

        case Account:
            return *m_union.accountValue;
    }
            
    return sm_emptyAccount;
}

/**
 * \returns the reference to the S9sVariantMap held in the S9sVariant.
 */
const S9sVariantMap &
S9sVariant::toVariantMap() const
{
    switch (m_type)
    {
        case Invalid:
        case Int:
        case Ulonglong:
        case Double:
        case Bool:
        case String:
        case List:
            return sm_emptyMap;

        case Map:
            return *m_union.mapValue;

        case Node:
            return m_union.nodeValue->toVariantMap();

        case Account:
            return m_union.accountValue->toVariantMap();
    }
            
    return sm_emptyMap;
}

/**
 * \returns the reference to the S9sVariantMap held in the S9sVariant.
 */
const S9sVariantList &
S9sVariant::toVariantList() const
{
    switch (m_type)
    {
        case Invalid:
        case Int:
        case Ulonglong:
        case Double:
        case Bool:
        case String:
        case Map:
        case Node:
        case Account:
            return sm_emptyList;

        case List:
            return *m_union.listValue;
    }
            
    return sm_emptyList;
}

int
S9sVariant::size() const
{
    if (m_type == Invalid)
    {
        return 0;
    } else if (m_type == List)
    {
        return m_union.listValue->size();
    }
    
    S9S_WARNING("");
    S9S_WARNING("Unhandled type %s", STR(typeName()));
    S9S_WARNING("*** value: %s", STR(toString()));
    return 0;
}

/**
 * \param defaultValue the value to be returned if the variant can't be
 *   converted to an integer.
 * \returns the value in the variant converted to integer.
 *
 */
int
S9sVariant::toInt(
        const int defaultValue) const
{
    switch (m_type)
    {
        case Invalid:
            // The integer value defaults to 0 as a global int variable would.
            // You can rely on this.
            return defaultValue;

        case String:
            return toString().empty() ? defaultValue : atoi(toString().c_str());

        case Int:
            return m_union.iVal;

        case Double:
            return (int) m_union.dVal;

        case Ulonglong:
            // This is cheating, converting an ulonglong value to integer might
            // cause data loss.
            return (int) m_union.ullVal;

        case Bool:
            return m_union.bVal ? 1 : 0;

        case Map:
        case List:
        case Node:
        case Account:
            return defaultValue;
    }

    return defaultValue;
}

/**
 * \param defaultValue the value to be returned if the variant can't be
 *   converted to unsigned long long.
 * \returns the value in the variant converted to unsigned long long.
 *
 */
ulonglong
S9sVariant::toULongLong(
        ulonglong defaultValue) const
{
    switch (m_type)
    {
        case Invalid:
            return defaultValue;

        case Ulonglong:
            return m_union.ullVal;

        case Int:
            return (ulonglong) m_union.iVal;

        case Double:
            return (ulonglong) m_union.dVal;

        case String:
            if (toString().empty())
                return defaultValue;

            return strtoull(toString().c_str(), NULL, 0);

        case Bool:
            return m_union.bVal ? 1ull : 0ull;

        case Map:
        case List:
        case Node:
        case Account:
            // FIXME: This is not yet implemented.
            return defaultValue;
    }

    return defaultValue;
}

time_t
S9sVariant::toTimeT() const
{
    return toULongLong(0ull);
}


/**
 * If the value can not be converted to a double value this function will return
 * 0.0.
 */
double
S9sVariant::toDouble(
        const double defaultValue) const
{
    double retval = defaultValue;

    switch (m_type)
    {
        case Map:
        case List:
        case Invalid:
        case Node:
        case Account:
            // The default value is already there.
            break;

        case Double:
            retval = m_union.dVal;
            break;

        case Int:
            retval = double(m_union.iVal);
            break;

        case Ulonglong:
            retval = double(m_union.ullVal);
            break;

        case String:
            errno = 0;
            retval = strtod(STR(toString()), NULL);

            if (errno != 0)
                retval = defaultValue;

            break;

        case Bool:
            retval = m_union.bVal ? 1.0 : 0.0;
            break;
    }

    return retval;
}

/**
 * \param defaultValue the value that shall be returned if the variant can't be
 *   converted to a boolean.
 * \returns the value from the variant converted to a boolean.
 *
 * This method recognizes all the usual strings used to denote boolean values
 * like "yes", "true", "T", "on".
 */
bool
S9sVariant::toBoolean(
        const bool defaultValue) const
{
    switch (m_type)
    {
        case Invalid:
            return defaultValue;

        case Bool:
            return m_union.bVal;

        case String:
            {
                std::string trimmed = toString().trim();

                if (trimmed.empty())
                    return defaultValue;
        
                if (!strcasecmp(trimmed.c_str(), "yes") ||
                    !strcasecmp(trimmed.c_str(), "true") ||
                    !strcasecmp(trimmed.c_str(), "on") ||
                    !strcasecmp(trimmed.c_str(), "t"))
                {
                    return true;
                }

                if (!strcasecmp(trimmed.c_str(), "no") ||
                    !strcasecmp(trimmed.c_str(), "false") ||
                    !strcasecmp(trimmed.c_str(), "off") ||
                    !strcasecmp(trimmed.c_str(), "f"))
                {
                    return false;
                }

                if (atoi(trimmed.c_str()) != 0) 
                    return true;
                else 
                    return false;
            }
            break;

        case Int:
            return m_union.iVal != 0;

        case Double:
            return m_union.dVal != 0.0;

        case Ulonglong:
            return m_union.ullVal != 0ull;

        case Map:
        case List:
        case Node:
        case Account:
            return defaultValue;
    }

    return defaultValue;
}


/**
 * This is the simplest method of the toString() family. It returns a short, one
 * line version of the value that is not necesserily a full representation, but
 * it is excellent to be shown in messages, logs, debug strings.
 */
S9sString
S9sVariant::toString() const
{
    S9sString retval;

    if (m_type == String)
    {
        retval = *m_union.stringValue;
    } else if (m_type == Invalid)
    {
        // Nothing to do, empty string...
        ;
    } else if (m_type == Bool)
    {
        retval = m_union.bVal ? "true" : "false";
    } else if (m_type == Int)
    {
        retval.sprintf("%d", m_union.iVal);
    } else if (m_type == Ulonglong)
    {
        retval.sprintf("%llu", m_union.ullVal);
    } else if (m_type == Double)
    {
        retval.sprintf("%g", m_union.dVal);
    } else if (m_type == Map)
    {
        retval = toVariantMap().toString();
    } else if (m_type == Node)
    {
        retval = toNode().toVariantMap().toString();
    } else if (m_type == List)
    {
        const S9sVariantList &list = toVariantList();

        retval = "[";
        for (uint idx = 0; idx < list.size(); ++idx)
        {
            const S9sVariant &item = list[idx];
            if (idx > 0)
                retval += ", ";

            retval += item.toString();
        }
        retval += "]";
    #if 0
    } else if (m_type == Array)
    {
        // Converting an array into a string. This is only used in 
        // spreadsheets so we are adding some quotes too.
        CmonVariantArray array = toVariantArray();
        for (uint column = 0; column < array.columns(); ++column)
        {
            if (!retval.empty())
                retval += "; ";

            // Limiting the string length for speed. 
            if (column > 5)
            {
                //CMON_WARNING("TOO MANY COLUMNS");
                retval += "...";
                break;
            }

            for (uint row = 0; row < array.rows(); ++row)
            {
                CMON_DEBUG("%2d, %2d: '%s'", 
                        column, row, 
                        STR(array.at(column, row).toString()));

                if (!retval.empty() && !retval.endsWith("; "))
                    retval += ", ";

                // Limiting the string length for speed.
                if (row > 5)
                {
                    //CMON_WARNING("TOO MANY ROWS");
                    retval += "...";
                    break;
                }

                if (array.at(column, row).isString())
                    retval = retval + "\"" +
                        array.at(column, row).toString() + "\"";
                else
                    retval = retval + array.at(column, row).toString();
            }
        }

        retval = "{" + retval + "}";
    #endif
    } else {
        //CMON_WARNING("Not implemented for %s", STR(typeName()));
    }

    return retval;
}

/**
 * This is the method that converts the content into string while giving a
 * caller *some* flexibility on controlling the format. It is widely used in JS
 * and in the spreadsheet system although it gives the user only a limited
 * control.
 */
S9sString
S9sVariant::toString(
        const S9sVariant::TextFormat format) const
{
    S9sString retval;

    switch (format)
    {
        case NotSpecified:
        case StringFormat:
            retval = toString();
            break;

        case GeneralNumber:
            if (isNumber())
                retval = toString();
            else
                retval = 0;
            break;

        case IntegerNumber:
            if (isDouble())
                retval.sprintf("%d", int(floor(toDouble() + 0.5)));
            else
                retval.sprintf("%d", toInt());

            break;

        case ScientificNumber:
            retval.sprintf("%g", toDouble());
            break;

        case TwoDecimalNumber:
            if (isNumber()) 
            {
                retval.sprintf("%.2f", toDouble());
            } else {
                retval = toString();
            }
            break;

        case FourDecimalNumber:
            retval.sprintf("%.4f", toDouble());
            break;

        case HighPrecDecimalNumber:
            retval.sprintf("%.16f", toDouble());
            break;

        case IntegerPercent:
            if (isDouble())
                retval.sprintf("%d%%", int(floor(100.0 * toDouble() + 0.5)));
            else
                retval.sprintf("%d%%", toInt() * 100);

            break;

        case TwoDecimalPercent:
            if (isNumber()) 
            {
                retval.sprintf("%6.2f%%", toDouble() * 100.0);
            } else {
                retval = toString();
            }
            break;

        case Celsius:
            if (isNumber()) 
            {
                retval.sprintf("%5.2f℃", toDouble());
            } else {
                retval = toString();
            }
            break;

        case Kelvin:
            if (isNumber()) 
            {
                retval.sprintf("%.2fK", toDouble());
            } else {
                retval = toString();
            }
            break;

        case ShortTime:
            retval = S9sDateTime(toTimeT()).toString(
                    S9sDateTime::ShortTimeFormat);
            break;

        case LongTime:
            retval = S9sDateTime(toTimeT()).toString(
                    S9sDateTime::LongTimeFormat);
            break;

        case ShortDate:
                retval = S9sDateTime(toTimeT()).toString(
                        S9sDateTime::ShortDateFormat);
            break;

        case DateTime:
                retval = S9sDateTime(toTimeT()).toString(
                        S9sDateTime::LocalDateTimeFormat);
            break;

        case ElapsedTime:
            if (isNumber())
            {
                int value = toInt() / 100;
                int minutes, seconds, hundredths;

                minutes  = value / 60;
                value   -= minutes * 60;
                seconds  = value;
                hundredths = toInt() - 100 * (toInt() / 100);
                retval.sprintf("%d:%02d.%02d", minutes, seconds, hundredths);
            } else {
                retval = toString();
            }
            break;

        case ElapsedTimeMicros:
            if (isNumber())
            {
                ulonglong value = toULongLong();
                
                int minutes, seconds, ms, us;
                if ( value < 1000)
                    retval.sprintf("<1 sec");
//                    retval.sprintf("%d us", value);
                else if ( value >= 1000 && value < 1000000)
                {
                    ms = value / 1000;
                    us = value - ms*1000 ;
                    S9S_UNUSED(us)
//                    retval.sprintf("%d.%d ms", ms, us);
                    retval.sprintf("<1 sec");
                }
                else
                {
                    value = value / 1000 / 1000; //we have seconds
                    minutes  = value / 60;
                    value   -= minutes * 60;
                    seconds  = value;
                    if (minutes >= 60)                        
                        retval.sprintf("%d m", minutes);
                    else if ( minutes > 0 && minutes < 60)
                        retval.sprintf("%d m %02d s", minutes, seconds);
                    else
                        retval.sprintf("%2d sec", seconds);
                }
            } else {
                retval = toString();
            }
            break;

        case Boolean:
                retval = toBoolean() ? "true" : "false";
            break;

        case Bytes:
            if (isString())
            {
                retval = toString();
            } else {
                double doubleVal = toDouble();

                if (fabs(doubleVal) >= tbyte)
                {
                    doubleVal /= tbyte;

                    retval.sprintf("%.2f TiB", doubleVal);
                } else if (fabs(doubleVal) >= gbyte)
                {
                    doubleVal /= gbyte;
                    retval.sprintf("%.2f GiB", doubleVal);
                } else if (fabs(doubleVal) >= mbyte)
                {
                    doubleVal /= mbyte;
                    retval.sprintf("%.2f MiB", doubleVal);
                } else if (fabs(doubleVal) >= kbyte)
                {
                    doubleVal /= kbyte;
                    retval.sprintf("%.2f KiB", doubleVal);
                } else {
                    int intVal = toInt();
                    retval.sprintf("%d B", intVal);
                }
            }
            break;

        case BytesShort:
            if (isString())
            {
                retval = toString();
            } else {
                double doubleVal = toDouble();

                if (fabs(doubleVal) >= tbyte)
                {
                    doubleVal /= tbyte;

                    if (doubleVal >= 10.0)
                        retval.sprintf("%.0fT", doubleVal);
                    else
                        retval.sprintf("%.1fT", doubleVal);
                } else if (fabs(doubleVal) >= gbyte)
                {
                    doubleVal /= gbyte;

                    if (doubleVal >= 10.0)
                        retval.sprintf("%.0fG", doubleVal);
                    else
                        retval.sprintf("%.1fG", doubleVal);
                } else if (fabs(doubleVal) >= mbyte)
                {
                    doubleVal /= mbyte;

                    if (doubleVal >= 10.0)
                        retval.sprintf("%.0fM", doubleVal);
                    else
                        retval.sprintf("%.1fM", doubleVal);
                } else if (fabs(doubleVal) >= kbyte)
                {
                    doubleVal /= kbyte;

                    if (doubleVal >= 10.0)
                        retval.sprintf("%.0fK", doubleVal);
                    else
                        retval.sprintf("%.1fK", doubleVal);
                } else {
                    int intVal = toInt();

                    retval.sprintf("%dB", intVal);
                }
            }

            break;

        case BytesPerSecShort:
            if (isString())
            {
                retval = toString();
            } else {
                double doubleVal = toDouble();

                if (fabs(doubleVal) >= tbyte)
                {
                    doubleVal /= tbyte;

                    if (doubleVal >= 10.0)
                        retval.sprintf("%.0fT/s", doubleVal);
                    else
                        retval.sprintf("%.1fT/s", doubleVal);
                } else if (fabs(doubleVal) >= gbyte)
                {
                    doubleVal /= gbyte;

                    if (doubleVal >= 10.0)
                        retval.sprintf("%.0fG/s", doubleVal);
                    else
                        retval.sprintf("%.1fG/s", doubleVal);
                } else if (fabs(doubleVal) >= mbyte)
                {
                    doubleVal /= mbyte;

                    if (doubleVal >= 10.0)
                        retval.sprintf("%.0fM/s", doubleVal);
                    else
                        retval.sprintf("%.1fM/s", doubleVal);
                } else if (fabs(doubleVal) >= kbyte)
                {
                    doubleVal /= kbyte;

                    if (doubleVal >= 10.0)
                        retval.sprintf("%.0fK/s", doubleVal);
                    else
                        retval.sprintf("%.1fK/s", doubleVal);
                } else {
                    int intVal = toInt();

                    retval.sprintf("%dB/s", intVal);
                }
            }

            break;

    }

    return retval;
}
        
bool 
S9sVariant::contains(
        const S9sVariant &value) const
{
    if (isVariantList())
    {
        for (uint idx = 0u; idx < m_union.listValue->size(); ++idx)
        {
            const S9sVariant &thisValue = (*m_union.listValue)[idx];

            if (thisValue == value)
                return true;
        }
    }

    return false;
}

bool
S9sVariant::contains(
        const S9sString &key) const
{
    if (m_type == Map)
    {
        return m_union.mapValue->contains(key);
    }

    return false;
}

bool
S9sVariant::contains(
        const char *key) const
{
    if (m_type == Map)
    {
        return m_union.mapValue->contains(key);
    }

    return false;
}

/**
 * Drops the value from the variant, sets its type to "Invalid" and releases all
 * resources that the variant might hold. This function is also called from the
 * destructor.
 */
void
S9sVariant::clear()
{
    switch (m_type) 
    {
        case Invalid:
        case Bool:
        case Int:
        case Ulonglong:
        case Double:
            // Nothing to do here.
            break;

        case String:
            delete m_union.stringValue;
            m_union.stringValue = NULL;
            break;

        case Map:
            delete m_union.mapValue;
            m_union.mapValue = NULL;
            break;

        case List:
            delete m_union.listValue;
            m_union.listValue = NULL;
            break;

        case Node:
            delete m_union.nodeValue;
            m_union.nodeValue = NULL;
            break;

        case Account:
            delete m_union.accountValue;
            m_union.accountValue = NULL;
            break;
    }

    m_type = Invalid;
}

bool 
S9sVariant::fuzzyCompare(
        const double first, 
        const double second)
{
    return std::fabs(first - second) < 
        // This is much more liberal
        // 1e-12;
        // It seems the error is usually much greater than epsilon.
        10 * std::numeric_limits<double>::epsilon();
}

/**
 * Sets the value of the variant to the sum of arg1 and arg2 considering the
 * case that the addition can lead to overflow.
 */
void 
S9sVariant::additionWithOverflow(
        const int arg1, 
        const int arg2)
{
    if (((arg1 ^ arg2) | 
                (((arg1 ^ (~(arg1 ^ arg2) & INT_MIN)) + arg2) ^ arg2)) >= 0)
    {
        // The result is a double because an int would overflow.
        this->operator=(ulonglong(arg1) + ulonglong(arg2));
    }
    else 
    {
        this->operator=(arg1 + arg2);
    }
}
