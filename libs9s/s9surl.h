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
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "S9sString"

class S9sUrl
{
    public:
        S9sUrl(const S9sString &stringRep);

        S9sString hostName() const { return m_hostName; };
        int port() const { return m_port; };
        bool hasPort() const { return m_hasPort; };

    private:
        S9sString    m_origString;
        S9sString    m_protocol;
        S9sString    m_hostName;
        int          m_port;
        bool         m_hasPort;
};
