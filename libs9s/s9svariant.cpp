/*
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "s9svariant.h"
#include "S9sVariantMap"
#include "S9sVariantList"

#include <errno.h>
#include <strings.h>
#include <stdlib.h>

#define DEBUG
#include "s9sdebug.h"

const S9sVariantMap  S9sVariant::sm_emptyMap;
const S9sVariantList S9sVariant::sm_emptyList;

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
    }
    
    return *this;
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
            return sm_emptyList;

        case List:
            return *m_union.listValue;
    }
            
    return sm_emptyList;
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
            // FIXME: This is not yet implemented.
            return defaultValue;
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
            delete m_union.listValue;
            m_union.listValue = NULL;
            break;
    }

    m_type = Invalid;
}
