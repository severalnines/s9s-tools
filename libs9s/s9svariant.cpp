/*
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "s9svariant.h"
#include "S9sVariantMap"

const S9sVariantMap S9sVariant::sm_emptyMap;

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

        
const S9sVariantMap &
S9sVariant::toVariantMap() const
{
    if (m_type == Map)
        return *m_union.mapValue;

    return sm_emptyMap;
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

        case String:
            delete m_union.stringValue;
            break;

        case Map:
            delete m_union.mapValue;
            break;

        case List:
        case Array:
            // Not yet implemented.
            break;
    }

    m_type = Invalid;
}
