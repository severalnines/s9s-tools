/*
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "s9svariant.h"
#include "S9sVariantMap"

#include <errno.h>
#include <strings.h>
#include <stdlib.h>

#define DEBUG
#include "s9sdebug.h"

const S9sVariantMap S9sVariant::sm_emptyMap;

S9sVariant::S9sVariant(
        const S9sVariant &orig)
{
    m_type         = orig.m_type;
    m_union        = orig.m_union;

    if (m_type == String)
    {
        m_union.stringValue = new S9sString(*orig.m_union.stringValue);
    } else if (m_type == Map)
    {
        m_union.mapValue = new S9sVariantMap(*orig.m_union.mapValue);
    }
}


S9sVariant::S9sVariant(
        const S9sVariantMap &mapValue) :
    m_type(Map)
{
    m_union.mapValue = new S9sVariantMap(mapValue);
}


S9sVariant::~S9sVariant()
{
    clear();
}

S9sVariant &
S9sVariant::operator= (
        const S9sVariant &rhs)
{
    if (this == &rhs)
        return *this;

    clear();

    m_type         = rhs.m_type;
    m_union        = rhs.m_union;
    
    switch (m_type)
    {
        case String:
            m_union.stringValue = new S9sString(*rhs.m_union.stringValue);
            break;

        case Map:
            m_union.mapValue = new S9sVariantMap(*rhs.m_union.mapValue);
            break;

        default:
            /* nop */
            break;
    }
    
    return *this;
}

        
const S9sVariantMap &
S9sVariant::toVariantMap() const
{
    if (m_type == Map)
        return *m_union.mapValue;

    return sm_emptyMap;
}

int
S9sVariant::toInt(
        const int defaultValue) const
{
    if (m_type == Int)
    {
        return m_union.iVal;
    } else if (m_type == Double)
    {
        return (int) m_union.dVal;
    } else if (isString())
    {
        return toString().empty() ? 0 : atoi(toString().c_str());
    } else if (m_type == Ulonglong)
    {
        // This is cheating, converting an ulonglong value to integer might
        // cause data loss.
        return (int) m_union.ullVal;
    } else if (isInvalid())
    {
        // The integer value defaults to 0 as a global int variable would. You
        // can rely on this.
        return defaultValue;
    }

    return defaultValue;
}

ulonglong
S9sVariant::toULongLong(
        ulonglong defaultValue) const
{
    if (m_type == Ulonglong)
        return m_union.ullVal;
    else if (m_type == Int)
        return (ulonglong) m_union.iVal;
    else if (m_type == Double)
        return (ulonglong) m_union.dVal;
    else if (m_type == String)
    {
        if (toString().empty())
            return defaultValue;

        return strtoull(toString().c_str(), NULL, 0);
    }

    return defaultValue;
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

    if (m_type == Double)
    {
        retval = m_union.dVal;
    } else if (m_type == Int)
    {
        retval = double(m_union.iVal);
    } else if (m_type == Ulonglong)
    {
        retval = double(m_union.ullVal);
    } else if (m_type == String)
    {
        const S9sString &str = toString();
        errno = 0;
        retval = strtod(STR (str), NULL);
        if (errno != 0)
            return defaultValue;
    }

    return retval;
}

bool
S9sVariant::toBoolean(
        const bool defaultValue) const
{
    if (m_type == Bool)
        return m_union.bVal;

    if (isInvalid())
        return defaultValue;

    // From string
    if (isString())
    {
        std::string trimmed = toString().trim();

        if (trimmed.empty())
            return false;
        
        if (!strcasecmp(trimmed.c_str(), "yes") ||
            !strcasecmp(trimmed.c_str(), "true") ||
            !strcasecmp(trimmed.c_str(), "on") ||
            !strcasecmp(trimmed.c_str(), "t"))
            return true;

        if (!strcasecmp(trimmed.c_str(), "no") ||
            !strcasecmp(trimmed.c_str(), "false") ||
            !strcasecmp(trimmed.c_str(), "off") ||
            !strcasecmp(trimmed.c_str(), "f"))
            return false;

        if (atoi(trimmed.c_str()) != 0) 
            return true;
        else 
            return false;
    } else if (m_type == Int)
    {
        return m_union.iVal != 0;
    } else if (isNumber())
    {
        return toDouble() != 0.0;
    }

    // More to come.
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
        //CmonJSonMessage map = toVariantMap();
        //retval = map.toString();
    #if 0
    } else if (m_type == List)
    {
        const CmonVariantList &list = toVariantList();

        retval = "{";
        for (uint idx = 0; idx < list.size(); ++idx)
        {
            const CmonVariant &item = list[idx];
            if (idx > 0)
                retval += ", ";

            retval += item.toString();
        }
        retval += "}";
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
        case Array:
            // Not yet implemented.
            break;
    }

    m_type = Invalid;
}
