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

#define DEBUG
//#define WARNING
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
    m_hasPort(false)
{
    S9sString  theString = stringRep;
    S9sRegExp  protocolRegExp("(.+)://(.*)");
    S9sRegExp  regexp("([^:]+):([0-9]+)");

    if (protocolRegExp == theString)
    {
        //S9S_WARNING("protocolRegExp[0] = '%s'", STR(protocolRegExp[0]));
        //S9S_WARNING("protocolRegExp[1] = '%s'", STR(protocolRegExp[1]));
        //S9S_WARNING("protocolRegExp[2] = '%s'", STR(protocolRegExp[2]));

        m_protocol = protocolRegExp[1];
        theString  = protocolRegExp[2];
    }

    if (regexp == theString)
    {
        //S9S_WARNING("regexp[0] = '%s'", STR(regexp[0]));
        //S9S_WARNING("regexp[1] = '%s'", STR(regexp[1]));
        //S9S_WARNING("regexp[2] = '%s'", STR(regexp[2]));

        m_hostName = regexp[1];
        m_port     = regexp[2].toInt();
        m_hasPort  = true;
    } else {
        m_hostName = theString;
    }
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

    S9S_DEBUG("");
    for (int n = 0;;)
    {
        c = input.c_str()[n];

        S9S_DEBUG("%-26s n = %02d c = '%c'", 
                STR(parseStateToString(state)), n, c);

        switch (state)
        {
            case StartState:
                if (c == '\0')
                {
                    return true;
                } else {
                    state = MayBeProtocol;
                }
                break;

            case MayBeProtocol:
                if (c == '\0')
                {
                    return true;
                } else if (c == ':')
                {
                    state = MaybeProtocolSeparator;
                    n++;
                } else {
                    tmpString += c;
                    ++n;
                }
                break;

            case MaybeProtocolSeparator:
                if (c == '\0')
                {
                    return false;
                } else if (c == '/')
                {
                    state = ProtocolSeparator;
                    n++;
                } else {
                    return false;
                }
                break;

            case ProtocolSeparator:
                if (c == '\0')
                {
                    return false;
                } else if (c == '/') 
                {
                    // The second '/', we finished reading the protocol.
                    protocol = tmpString;
                    tmpString.clear();
                    S9S_DEBUG("protocol -> '%s'", STR(protocol));

                    n++;
                    state = MaybeUserName;
                }
                break;

            case MaybeUserName:
                if (c == '\0')
                {
                    hostName = tmpString;
                    S9S_DEBUG("hostName -> '%s'", STR(hostName));

                    m_origString = input;
                    m_protocol   = protocol;
                    m_hostName   = hostName;
                    m_port       = -1;
                    m_hasPort    = false;
                    return true;
                } else if (c == '?') 
                {
                    hostName = tmpString;
                    S9S_DEBUG("hostName -> '%s'", STR(hostName));
                    
                    ++n;
                    state = PropertyName;
                } else {
                    tmpString += c;
                    ++n;
                }
                break;

            case PropertyName:
                if (c == 0)
                {
                    return false;
                } else if (c == '=')
                {
                    state = PropertyValue;
                    n++;
                } else {
                    propertyName += c;
                    n++;
                }
                break;

            case PropertyValue:
                if (c == 0)
                {
                    S9S_DEBUG("name     -> '%s'", STR(propertyName));
                    S9S_DEBUG("value    -> '%s'", STR(propertyValue));
                    m_origString = input;
                    m_protocol   = protocol;
                    m_hostName   = hostName;
                    m_port       = -1;
                    m_hasPort    = false;

                    return true;
                } else {
                    propertyValue += c;
                    n++;
                }
                break;
        }
    }

    return true;
}



