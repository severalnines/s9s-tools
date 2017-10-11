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

/**
 * A class that represents a hardware server.
 */
class S9sServer
{
    public:
        S9sServer();
        S9sServer(const S9sVariantMap &properties);

        virtual ~S9sServer();

        S9sServer &operator=(const S9sVariantMap &rhs);
                
        bool hasProperty(const S9sString &key) const;
        S9sVariant property(const S9sString &name) const;

        void setProperties(const S9sVariantMap &properties);

        S9sString hostName() const;
        S9sString alias() const;
        S9sString version() const;
        S9sString ipAddress() const;

        int nContainers() const;

        S9sString ownerName() const;
        S9sString groupOwnerName() const;
        S9sString className() const;
        S9sString model() const;

        S9sString osVersionString() const;
        S9sVariantList processorNames() const;
        S9sVariantList nicNames() const;
        S9sVariantList memoryBankNames() const;

    private:
        S9sVariantMap    m_properties;
};

