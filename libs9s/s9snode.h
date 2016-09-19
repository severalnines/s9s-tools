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

#include "S9sVariantMap"
#include "S9sUrl"

/**
 * A class that represents a node/host/server. 
 */
class S9sNode
{
    public:
        S9sNode();
        S9sNode(const S9sVariantMap &properties);
        S9sNode(const S9sString &stringRep);

        virtual ~S9sNode();

        S9sNode &operator=(const S9sVariantMap &rhs);

        const S9sVariantMap &toVariantMap() const;
        void setProperties(const S9sVariantMap &properties);

        S9sString protocol() const { return m_url.protocol(); };
        S9sString className() const;
        S9sString name() const;
        S9sString hostName() const;
        S9sString alias() const;
        
        bool hasPort() const;
        int port() const;

        S9sString hostStatus() const;
        S9sString nodeType() const;
        S9sString version() const;
        S9sString message() const;
        bool isMaintenanceActive() const;

    private:
        S9sVariantMap    m_properties;
        S9sUrl           m_url;
};
