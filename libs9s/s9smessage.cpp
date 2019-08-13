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

#include "S9sOptions"
#include "S9sFormatter"

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
    return 
        m_properties.contains("fileName") || 
        m_properties.contains("file_name");
}

S9sString 
S9sMessage::fileName() const
{
    if (m_properties.contains("fileName"))
    {
        return m_properties.at("fileName").toString();
    } else if (m_properties.contains("file_name"))
    {
        return m_properties.at("file_name").toString();
    } else if (m_properties.contains("log_origins"))
    {
        S9sVariantMap map = m_properties.at("log_origins").toVariantMap();
        return map["sender_file"].toString();
    }

    return S9sString();
}

S9sString
S9sMessage::hostName(
        const S9sString &defaultValue) const
{
    S9sString retval;

    retval = m_properties.valueByPath("log_specifics/host/hostname").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

#ifdef LOG_FUNCNAMES_TO_JOBLOG
bool
S9sMessage::hasFunctionName() const
{
    return 
        m_properties.contains("functionName") || 
        m_properties.contains("function_name");
}

S9sString 
S9sMessage::functionName() const
{
    S9sString str, retval;

    if (m_properties.contains("functionName"))
    {
        str = m_properties.at("functionName").toString();
    } else if (m_properties.contains("function_name"))
    {
        str = m_properties.at("function_name").toString();
    }

    // a HACK for now to shorten the function name for the console output
    if (!str.regMatch("::(.*)\\(", retval))
        retval = str;

    return retval;
}
#endif

bool
S9sMessage::hasLineNumber() const
{
    return 
        m_properties.contains("lineNumber") || 
        m_properties.contains("line_number");
}

int
S9sMessage::lineNumber() const
{
    if (m_properties.contains("lineNumber"))
    {
        return m_properties.at("lineNumber").toInt();
    } else if (m_properties.contains("line_number"))
    {
        return m_properties.at("line_number").toInt();
    } else if (m_properties.contains("log_origins"))
    {
        S9sVariantMap map = m_properties.at("log_origins").toVariantMap();
        return map["sender_line"].toInt();
    }


    return -1;
}

int 
S9sMessage::clusterId() const
{
    if (m_properties.contains("log_specifics"))
    {
        S9sVariantMap map = m_properties.at("log_specifics").toVariantMap();
        return map["cluster_id"].toInt(0);
    }

    return 0;
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
    
    // This is for log messages.
    if (m_properties.contains("log_id"))
        return m_properties.at("log_id").toInt();

    return -1;
}

S9sString
S9sMessage::logClass() const
{
    if (m_properties.contains("log_class"))
        return m_properties.at("log_class").toString();

    return S9sString();
}

int
S9sMessage::jobId() const
{
    if (m_properties.contains("job_id"))
        return m_properties.at("job_id").toInt();

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
    else if (m_properties.contains("severity"))
        status = m_properties.at("severity").toString();

    if (status == "JOB_SUCCESS")
        return "MESSAGE";
    else if (status == "JOB_WARNING" || status == "LOG_WARNING")
        return "WARNING";
    else if (status == "JOB_FAILED")
        return "FAILURE";
    else if (status == "JOB_DEBUG" || status == "LOG_DEBUG")
        return "DEBUG";
    else if (status == "LOG_ERR")
        return "ERROR";
    else if (status == "LOG_CRIT")
        return "CRITICAL";
    else if (status == "LOG_INFO")
        return "INFO";

    return status;
}

void
S9sMessage::wrap()
{
    if (m_properties.contains("message"))
    {
        S9sString messageText = m_properties.at("message").toString();

        messageText.replace("\n", "\\n");
        messageText.replace("\r", "\\r");
        m_properties["message"] = messageText;
    }
    
    if (m_properties.contains("message_text"))
    {
        S9sString messageText = m_properties.at("message_text").toString();

        messageText.replace("\n", "\\n");
        messageText.replace("\r", "\\r");
        m_properties["message_text"] = messageText;
    }
}

S9sString
S9sMessage::message() const
{
    if (m_properties.contains("message"))
    {
        return m_properties.at("message").toString();
    } else if (m_properties.contains("message_text"))
    {
        return m_properties.at("message_text").toString();
    } else if (m_properties.contains("log_specifics"))
    {
        S9sVariantMap map = m_properties.at("log_specifics").toVariantMap();
        return map["message_text"].toString();
    }

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
    S9sString    retval;

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

/**
 * \param syntaxHighlight Controls if the string will have colors or not.
 * \param formatString The formatstring with markup.
 * \returns The string representation according to the format string.
 *
 * Converts the message to a string using a special format string that may
 * contain field names of message properties.
 */
S9sString
S9sMessage::toString(
        const bool       syntaxHighlight,
        const S9sString &formatString) const
{
    S9sOptions  *options = S9sOptions::instance();
    S9sFormatter formatter;
    S9sString    retval;
    S9sString    tmp;
    char         c;
    S9sString    partFormat;
    bool         percent = false;
    bool         escaped = false;
    
    for (uint n = 0; n < formatString.size(); ++n)
    {
        c = formatString[n];
        
        if (c == '%' && !percent)
        {
            percent    = true;
            partFormat = "%";
            continue;
        } else if (c == '\\' && !escaped)
        {
            escaped = true;
            continue;
        }

        if (escaped)
        {
            switch (c)
            {
                case '\"':
                    retval += '\"';
                    break;

                case '\\':
                    retval += '\\';
                    break;
       
                case 'a':
                    retval += '\a';
                    break;

                case 'b':
                    retval += '\b';
                    break;

                case 'e':
                    retval += '\027';
                    break;

                case 'n':
                    retval += '\n';
                    break;

                case 'r':
                    retval += '\r';
                    break;

                case 't':
                    retval += '\t';
                    break;
            }
        } else if (percent)
        {
            switch (c)
            {
                case 'c':
                    // The 'log_class' property.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(logClass()));
                    retval += formatter.typeColorBegin();
                    retval += tmp;
                    retval += formatter.typeColorEnd();
                    break;

                case 'C':
                    // The 'created' date&time.
                    partFormat += 's';
                    tmp.sprintf(
                            STR(partFormat), 
                            STR(options->formatDateTime(created())));
                    retval += tmp;
                    break;
                
                case 'h':
                    // The related host name. 
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(hostName("-")));
                    retval += tmp;
                    break;
                
                case 'L':
                    // The line number.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), lineNumber());
                    retval += tmp;
                    break;
                
                case 'i':
                    // The message ID.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), clusterId());
                    retval += tmp;
                    break;
                
                case 'I':
                    // The message ID.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), messageId());
                    retval += tmp;
                    break;
                
                case 'J':
                    // The job ID.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), jobId());
                    retval += tmp;
                    break;

                case 'M':
                    // The message in color.
                    partFormat += 's';
                    
                    if (syntaxHighlight)
                    {
                        tmp.sprintf(
                                STR(partFormat), 
                                STR(S9sString::html2ansi(message())));
                    } else {
                        tmp.sprintf(
                                STR(partFormat), 
                                STR(S9sString::html2text(message())));
                    }

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

                    // FIXME: This is hackish.
                    if (syntaxHighlight)
                    {
                        if (severity() == "MESSAGE" || 
                                severity() == "DEBUG")
                        {
                            retval += XTERM_COLOR_GREEN;
                        } else if (severity() == "WARNING")
                            retval += XTERM_COLOR_YELLOW;
                        else if (severity() == "FAILURE" ||
                                 severity() == "ERROR"   ||
                                 severity() == "CRITICAL")
                        {
                            retval += XTERM_COLOR_RED;
                        }
                    }

                    retval += tmp;

                    retval += TERM_NORMAL;
                    break;

                case 'F':
                    // The file name in color.
                    partFormat += 's';
                    tmp.sprintf(
                            STR(partFormat), 
                            STR(fileName()));
                        
                    retval += XTERM_COLOR_BLUE;
                    retval += tmp;

                    retval += TERM_NORMAL;
                    break;

                case 'B':
                    // The base name in color.
                    partFormat += 's';
                    tmp.sprintf(
                            STR(partFormat), 
                            STR(fileName().baseName()));
                        
                    if (syntaxHighlight)
                        retval += XTERM_COLOR_BLUE;

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += TERM_NORMAL;
                    break;

#ifdef LOG_FUNCNAMES_TO_JOBLOG
                case 'P':
                    // The function (procedure) name in color.
                    partFormat += 's';
                    tmp.sprintf(
                            STR(partFormat), 
                            STR(functionName()));

                    retval += XTERM_COLOR_BLUE;
                    retval += tmp;

                    retval += TERM_NORMAL;
                    break;
#endif

                case '%':
                    retval += '%';
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

    return toVariantMap().toString(syntaxHighlight, retval);
}

