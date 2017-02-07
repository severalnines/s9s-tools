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
#pragma once

#include "S9sString"
#include "S9sVariantMap"

/**
 * A simple class to parse URLs.
 */
class S9sUrl
{
    public:
        S9sUrl();
        S9sUrl(const S9sString &stringRep);

        S9sString protocol() const { return m_protocol; };
        bool hasProtocol() const { return !m_protocol.empty(); };
        S9sString hostName() const { return m_hostName; };
        int port() const { return m_port; };
        bool hasPort() const { return m_hasPort; };
        
        S9sVariant property(const S9sString &key) const;

    protected:
        enum ParseState
        {
            StartState,
            MayBeProtocol,
            MaybeProtocolSeparator,
            ProtocolSeparator,
            MaybeUserName,
            PropertyName,
            PropertyValue,
            PortString,
        };

        S9sString parseStateToString(const S9sUrl::ParseState state);
        bool parse(const S9sString      &input);

    private:
        S9sString     m_origString;
        S9sString     m_protocol;
        S9sString     m_hostName;
        int           m_port;
        bool          m_hasPort;
        S9sVariantMap m_properties;

        friend class UtS9sUrl;
};
