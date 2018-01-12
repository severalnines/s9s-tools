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

/**
 * A class that represents a user on the controller. 
 */
class S9sUser
{
    public:
        S9sUser();
        S9sUser(const S9sVariantMap &properties);
        S9sUser(const S9sString &stringRep);

        virtual ~S9sUser();

        S9sUser &operator=(const S9sVariantMap &rhs);

        bool hasProperty(const S9sString &key) const;
        S9sVariant property(const S9sString &name) const;
        void setProperty(const S9sString &name, const S9sString &value);

        const S9sVariantMap &toVariantMap() const;
        S9sString toString() const;

        void setProperties(const S9sVariantMap &properties);

        S9sString userName() const;
        S9sString emailAddress(const S9sString &defaultValue = "") const;
        int userId() const;
        S9sString firstName() const;
        S9sString lastName() const;
        S9sString middleName() const;
        S9sString title() const;
        S9sString jobTitle() const;
        void setGroup(const S9sString &groupName);
        S9sString groupNames(const S9sString separator = ",") const;
        bool isMemberOf(const S9sString &groupName) const;

        void setPublicKey(
                const S9sString &name,
                const S9sString &key);

        S9sString fullName() const;
        S9sString ownerName() const;
        S9sString groupOwnerName() const;
        S9sString lastLoginString(const S9sString &defaultValue) const;
        S9sString createdString(const S9sString &defaultValue) const;
        S9sString failedLoginString(const S9sString &defaultValue) const;
        bool isDisabled() const;
        bool isSuspended() const;
        int nFailedLogins() const;


        S9sString toString(
                const bool       syntaxHighlight,
                const S9sString &formatString) const;

    private:
        S9sVariantMap    m_properties;
};
