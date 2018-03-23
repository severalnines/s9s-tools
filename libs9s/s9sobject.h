/*
 * Severalnines Tools
 * Copyright (C) 2018  Severalnines AB
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

class S9sObject
{
    public:
        S9sObject();
        S9sObject(const S9sObject &orig);
        S9sObject(const S9sVariantMap &properties);
        
        virtual ~S9sObject();
        
        virtual S9sObject &operator=(const S9sVariantMap &rhs);
        
        bool hasProperty(const S9sString &key) const;
        S9sVariant property(const S9sString &name) const;
        void setProperty(const S9sString &name, const S9sString &value);
        void setProperty(const S9sString &name, const bool value);
        void setProperty(const S9sString &name, const int value);
        void setProperty(const S9sString &name, const S9sVariantMap &value);
        void setProperties(const S9sVariantMap &properties);

        virtual const S9sVariantMap &toVariantMap() const;

        virtual S9sString className() const;

        virtual S9sString name() const;
        virtual S9sString name(const int columns) const;

        virtual S9sString id(const S9sString &defaultValue) const;
        virtual S9sString cdtPath() const;

        S9sString aclString() const;
        S9sString aclShortString() const;
        
        S9sString ownerName() const;
        S9sString groupOwnerName() const;
    protected:
        S9sVariantMap    m_properties;
};

