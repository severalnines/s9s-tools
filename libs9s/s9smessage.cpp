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
#define WARNING
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

/**
 * \returns The numerical ID of the message if there is a message ID or -1 if
 *   there is no ID.
 */
int
S9sMessage::messageId() const
{
    if (m_properties.contains("message_id"))
        return m_properties.at("message_id").toInt();

    return -1;
}

/**
 * \returns The 'created' date and time property.
 */
S9sDateTime
S9sMessage::created() const
{
    S9sDateTime retval;

    if (m_properties.contains("created"))
        retval.parse( m_properties.at("created").toString());

    return retval;
}

S9sString
S9sMessage::severity() const
{
    S9sString      status;
        
    if (m_properties.contains("message_status"))
        status = m_properties.at("message_status").toString();

    if (status == "JOB_SUCCESS")
        // XTERM_COLOR_GREEN TERM_NORMAL
        return "MESSAGE";
    else if (status == "JOB_WARNING")
        // XTERM_COLOR_YELLOW TERM_NORMAL
        return "WARNING";
    else if (status == "JOB_FAILED")
        // XTERM_COLOR_RED TERM_NORMAL
        return "FAILURE";

    return status;
}

S9sString
S9sMessage::message() const
{
    if (m_properties.contains("message"))
        return m_properties.at("message").toString();
    else if (m_properties.contains("message_text"))
        return m_properties.at("message_text").toString();

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

S9sString
S9sMessage::toString(
        const S9sString &formatString) const
{
    S9sString retval;
    S9sString tmp;
    char      c;
    S9sString partFormat;
    bool      percent = false;
    bool      escaped = false;
    
    for (uint n = 0; n < formatString.size(); ++n)
    {
        c = formatString[n];
        
        if (c == '%')
        {
            percent    = true;
            partFormat = "%";
            continue;
        } else if (c == '\\')
        {
            escaped = true;
            continue;
        }

        if (escaped)
        {
            switch (c)
            {
                case 'n':
                    retval += '\n';
                    break;

                case 't':
                    retval += '\t';
                    break;
            }
        } else if (percent)
        {
            switch (c)
            {
                case 'L':
                    // The line number.
                    partFormat += 'd';
                    //S9S_WARNING("  partFormat: '%s'", STR(partFormat));
                    tmp.sprintf(STR(partFormat), lineNumber());
                    retval += tmp;
                    break;
                
                case 'I':
                    // The message ID.
                    partFormat += 'd';
                    //S9S_WARNING("  partFormat: '%s'", STR(partFormat));
                    tmp.sprintf(STR(partFormat), messageId());
                    retval += tmp;
                    break;

                case 'M':
                    // The message in color.
                    partFormat += 's';
                    tmp.sprintf(
                            STR(partFormat), 
                            STR(S9sString::html2ansi(message())));
                    retval += tmp;
                    break;

                case 'C':
                    // The 'created' date&time.
                    partFormat += 's';
                    tmp.sprintf(
                            STR(partFormat), 
                            STR(created().toString(
                                    S9sDateTime::MySqlLogFileFormat)));
                    retval += tmp;
                    break;
                
                case 'T':
                    // The 'created' time.
                    partFormat += 's';
                    tmp.sprintf(
                            STR(partFormat), 
                            STR(created().toString(
                                    S9sDateTime::LongTimeFormat)));
                    retval += tmp;
                    break;

                case 'S':
                    // The severity or status.
                    partFormat += 's';
                    tmp.sprintf(
                            STR(partFormat), 
                            STR(severity()));
                    retval += tmp;
                    break;

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case '-':
                case '+':
                case '.':
                    partFormat += c;
                    continue;
            }
        } else {
            retval += c;
        }

        percent = false;
        escaped    = false;
    }

    return retval;
}

