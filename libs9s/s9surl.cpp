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
#include "s9surl.h"

#include "S9sRegExp"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sUrl::S9sUrl() :
    m_port(0),
    m_hasPort(false)
{
}

S9sUrl::S9sUrl(
        const S9sString &stringRep) :
    m_origString(stringRep),
    m_port(0),
    m_hasPort(false),
    m_parseCursor(0)
{
#if 0
    S9sString  theString = stringRep;
    S9sRegExp  protocolRegExp("(.+)://(.*)");
    S9sRegExp  regexp("([^:]+):([0-9]+)");

    // If the string starts with a protocol we remove that and continue with the
    // remaining part.
    if (protocolRegExp == theString)
    {
        m_protocol = protocolRegExp[1];
        theString  = protocolRegExp[2];
    }

    // If the string contains a port.
    if (regexp == theString)
    {
        m_hostName = regexp[1];
        m_port     = regexp[2].toInt();
        m_hasPort  = true;
    } else {
        m_hostName = theString;
    }
#else
    parse(stringRep);
#endif
}

void 
S9sUrl::setPort(
        int value)
{
    m_port    = value;
    m_hasPort = true;
}

int 
S9sUrl::port() const 
{ 
    return m_port; 
}

bool 
S9sUrl::hasPort() const 
{
    return m_hasPort; 
}

/**
 * \returns The single line error string if the parsing found and error, the
 *   empty string otherwise.
 */
S9sString
S9sUrl::errorString() const
{
    return m_errorString;
}

/**
 * \returns The original string that was parsed to get the URL object.
 */
S9sString
S9sUrl::origString() const
{
    return m_origString;
}

/**
 * \returns A multi line error string or the empty string if no error found
 *   while parsing.
 *
 * It helps to understand what's wrong with an URL string if we have a detailed
 * error showing the exact location of the error. This function returns an error
 * string that provides this kind of details. Here are some examples:

Expected port number.
10.10.1.120:3306extra
                ^

Expected '/' or port number.
10.10.1.120:some
            ^
 */
S9sString
S9sUrl::fullErrorString() const
{
    S9sString retval;

    if (!m_errorString.empty())
    {
        retval += m_errorString;
        retval += '\n';
        retval += m_origString;
        retval += '\n';

        for (int n = 0; n < m_parseCursor; ++n)
            retval += ' ';

        retval += "^\n";
    }

    return retval;
}

void
S9sUrl::setProperties(
        const S9sVariantMap &values)
{
    m_properties = values;
}

const S9sVariantMap &
S9sUrl::properties() const
{
    return m_properties;
}

/**
 * \returns The property for the given key or the null variant.
 *
 * Should the URL contain keys and values (e.g. "proxysql://10.10.10.23?db_username=bob&db_password=b0b&db_database='*.*'&db_privs='SELECT,INSERT,UPDATE'") 
 * this function can be used to retrieve such values by their names.
 */
S9sVariant 
S9sUrl::property(
        const S9sString &key) const
{
    if (m_properties.contains(key))
        return m_properties.at(key);

    return S9sVariant();
}

S9sString
S9sUrl::parseStateToString(
        const S9sUrl::ParseState state)
{
    switch (state)
    {
        case StartState:
            return "StartState";

        case MayBeProtocol:
            return "MayBeProtocol";

        case MaybeProtocolSeparator:
            return "MaybeProtocolSeparator";

        case ProtocolSeparator:
            return "ProtocolSeparator";

        case MaybeUserName:
            return "MaybeUserName";

        case PropertyName:
            return "PropertyName";

        case PropertyValue:
            return "PropertyValue";

        case PortString:
            return "PortString";
    }

    return "UNKNOWN";
}

/**
 *
 */
bool
S9sUrl::parse(
        const S9sString      &input)
{
    int            c;
    ParseState     state = StartState;
    S9sString      tmpString;
    S9sString      protocol;
    S9sString      hostName;
    S9sString      propertyName, propertyValue;
    S9sVariantMap  properties;
    S9sString      portString;

    S9S_DEBUG("");
    m_errorString.clear();
    m_origString = input;

    for (m_parseCursor = 0;;)
    {
        c = input.c_str()[m_parseCursor];

        S9S_DEBUG("%-26s m_parseCursor = %02d c = '%c'", 
                STR(parseStateToString(state)),
                m_parseCursor, c);

        switch (state)
        {
            case StartState:
                if (c == '\0')
                {
                    m_errorString = "Expected protocol or host name.";
                    return false;
                } else {
                    state = MayBeProtocol;
                }
                break;

            case MayBeProtocol:
                if (c == '\0')
                {
                    hostName     = tmpString;

                    m_protocol   = protocol;
                    m_hostName   = hostName;
                    m_port       = -1;
                    m_hasPort    = false;
                    return true;
                } else if (c == ':')
                {
                    state = MaybeProtocolSeparator;
                    m_parseCursor++;
                } else {
                    tmpString += c;
                    m_parseCursor++;
                }
                break;

            case MaybeProtocolSeparator:
                // In this state we already consumed a ':' character, so we
                // expect either a '/' for protocol separator or a port number.
                if (c == '\0')
                {
                    m_errorString = "Expected '/' or port number.";
                    return false;
                } else if (c == '/')
                {
                    state = ProtocolSeparator;
                    m_parseCursor++;
                } else if (c <= '9' && c >= '0') 
                {
                    hostName    = tmpString;
                    portString += c;
                    state       = PortString;
                    m_parseCursor++;
                } else {
                    m_errorString = "Expected '/' or port number.";
                    return false;
                }
                break;

            case ProtocolSeparator:
                if (c == '\0')
                {
                    m_errorString = "Expected '/' or port number.";
                    return false;
                } else if (c == '/') 
                {
                    // The second '/', we finished reading the protocol.
                    protocol = tmpString;
                    tmpString.clear();

                    m_parseCursor++;
                    state = MaybeUserName;
                }
                break;

            case MaybeUserName:
                if (c == '\0')
                {
                    hostName = tmpString;

                    m_protocol   = protocol;
                    m_hostName   = hostName;
                    m_port       = -1;
                    m_hasPort    = false;
                    return true;
                } else if (c == ':')
                {
                    hostName = tmpString;
                    m_parseCursor++;
                    state = PortString;
                } else if (c == '?') 
                {
                    hostName = tmpString;
                    m_parseCursor++;
                    state = PropertyName;
                } else {
                    tmpString += c;
                    m_parseCursor++;
                }
                break;

            case PortString:
                if (c == 0)
                {
                    if (portString.empty())
                    {
                        return false;
                    } else {
                        m_protocol   = protocol;
                        m_hostName   = hostName;
                        m_port       = portString.toInt();
                        m_hasPort    = true;
                    }

                    return true;
                } else if (c >= '0' && c <= '9')
                {
                    portString += c;
                    m_parseCursor++;
                } else if (c == '?')
                {
                    if (portString.empty())
                    {
                        // We started to read the port, but it is an empty
                        // string.
                        m_errorString = "Expected port number.";
                        return false;
                    } else {
                        m_protocol   = protocol;
                        m_hostName   = hostName;
                        m_port       = portString.toInt();
                        m_hasPort    = true;

                        state = PropertyName;
                        m_parseCursor++;
                    }
                } else {
                    m_errorString = "Expected port number.";
                    return false;
                }
                break;

            case PropertyName:
                if (c == 0)
                {
                    m_errorString = "Expected '='.";
                    return false;
                } else if (c == '=')
                {
                    state = PropertyValue;
                    m_parseCursor++;
                } else if (c == '&')
                {
                    // We just had a name, then &, a field separator. Let's
                    // store this as a boolean true value, easy to remember and
                    // use.
                    properties[propertyName] = true;
                    propertyName.clear();
                    propertyValue.clear();
                    state = PropertyName;
                    m_parseCursor++;
                } else {
                    propertyName += c;
                    m_parseCursor++;
                }
                break;

            case PropertyValue:
                if (c == '\0')
                {
                    if (propertyValue.empty())
                    {
                        m_errorString.sprintf(
                                "Expected property value for '%s'.",
                                STR(propertyName));
                        return false;
                    }

                    properties[propertyName] = propertyValue.unQuote();

                    m_protocol   = protocol;
                    m_hostName   = hostName;
                    m_port       = -1;
                    m_hasPort    = false;
                    m_properties = properties;

                    return true;
                } else if (c == '&')
                {
                    properties[propertyName] = propertyValue.unQuote();
                    propertyName.clear();
                    propertyValue.clear();
                    state = PropertyName;
                    m_parseCursor++;
                } else {
                    propertyValue += c;
                    m_parseCursor++;
                }
                break;
        }
    }

    return true;
}



