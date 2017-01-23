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
