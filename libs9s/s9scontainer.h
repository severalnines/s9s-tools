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
 * s9s-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with s9s-tools. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "S9sVariantMap"
#include "S9sUrl"
#include "S9sCluster"

/**
 * A class that represents a node/host/server. 
 */
class S9sContainer
{
    public:
        S9sContainer();
        S9sContainer(const S9sVariantMap &properties);
        S9sContainer(const S9sString &stringRep);

        virtual ~S9sContainer();

        S9sContainer &operator=(const S9sVariantMap &rhs);

        bool hasProperty(const S9sString &key) const;
        S9sVariant property(const S9sString &name) const;
        void setProperty(const S9sString &name, const S9sString &value);

        const S9sVariantMap &toVariantMap() const;
        void setProperties(const S9sVariantMap &properties);

        S9sString aclString() const;
        S9sString alias() const;
        S9sString cdtPath() const;
        S9sString className() const;
        int containerId() const;
        S9sString hostname() const;
        S9sString ipAddress(const S9sString &defaultValue = "") const;
        S9sString ownerName() const;
        S9sString groupOwnerName() const;
        S9sString parentServerName() const;
        S9sString state() const;
        S9sString templateName() const;
        S9sString type() const;

    private:
        S9sVariantMap    m_properties;
        S9sUrl           m_url;
};
