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
#include "s9smessage.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sMessage::S9sMessage()
{
}

S9sMessage::S9sMessage(
        const S9sVariantMap &properties) :
    m_properties(properties)
{
}

S9sMessage::~S9sMessage()
{
}

S9sMessage &
S9sMessage::operator=(
        const S9sVariantMap &rhs)
{
    m_properties = rhs;
    
    return *this;
}

const S9sVariantMap &
S9sMessage::toVariantMap() const
{
    return m_properties;
}

bool
S9sMessage::hasFileName() const
{
    return m_properties.contains("fileName");
}

S9sString 
S9sMessage::fileName() const
{
    if (m_properties.contains("fileName"))
        return m_properties.at("fileName").toString();

    return S9sString();
}

bool
S9sMessage::hasLineNumber() const
{
    return m_properties.contains("lineNumber");
}

int
S9sMessage::lineNumber() const
{
    if (m_properties.contains("lineNumber"))
        return m_properties.at("lineNumber").toInt();

    return -1;
}

S9sString
S9sMessage::message() const
{
    if (m_properties.contains("message"))
        return m_properties.at("message").toString();

    return S9sString();
}

bool 
S9sMessage::isError() const
{
    if (m_properties.contains("severity"))
    {
        S9sString severityString = m_properties.at("severity").toString();
        
        if (severityString == "error")
            return true;
    }

    return false;
}

S9sString
S9sMessage::termColorString() const
{
    S9sString retval;

    if (hasFileName() && hasLineNumber())
    {
        if (isError())
        {
            retval.sprintf("%s%s%s:%d:%s%s%s",
                    XTERM_COLOR_BLUE, STR(fileName()), TERM_NORMAL,
                    lineNumber(),
                    XTERM_COLOR_RED, STR(message()), TERM_NORMAL);
        } else {
            retval.sprintf("%s%s%s:%d:%s",
                    XTERM_COLOR_BLUE, STR(fileName()), TERM_NORMAL,
                    lineNumber(),
                    STR(message()));
        }
    } else {
        if (isError())
        {
            retval.sprintf("%s%s%s",
                    XTERM_COLOR_RED, STR(message()), TERM_NORMAL);
        } else {
            retval.sprintf("%s",
                    STR(message()));
        }
    }

    return retval;
}

S9sString
S9sMessage::toString() const
{
    S9sString retval;

    if (hasFileName() && hasLineNumber())
    {
        retval.sprintf("%s:%d:%s",
                STR(fileName()), lineNumber(), STR(message()));
    } else {
        retval.sprintf("%s",
                STR(message()), TERM_NORMAL);
    }

    return retval;
}
