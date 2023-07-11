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

#include "s9svariantmap.h"

/**
 * A class that represents an account. 
 */
class S9sAccount
{
    public:
        S9sAccount();
        S9sAccount(const S9sVariantMap &properties);
        S9sAccount(const S9sString &stringRep);

        virtual ~S9sAccount();

        S9sAccount &operator=(const S9sVariantMap &rhs);

        S9sString userName() const;
        void setUserName(const S9sString &value);

        S9sString password() const;
        S9sString passwordMasked() const;
        void setPassword(const S9sString &value);

        S9sString hostAllow() const;
        void setHostAllow(const S9sString &value);

        void setWithDatabase(bool value);

        void setPrivate(bool value = true);

        void setGrants(const S9sString &value);
        S9sString grants() const;
        S9sString grants(bool syntaxHighlight) const;

        void setError(const S9sString &value);
        bool hasError() const;
        S9sString errorString() const;

        int maxConnections() const;
        int connections() const;
        bool isExpired() const;

        const S9sVariantMap &toVariantMap() const;
        void setProperties(const S9sVariantMap &properties);

    protected:
        bool parseStringRep(const S9sString &input);

        void appendColorizedGrant(
                const S9sString &value,
                S9sString       &result) const;
        
        void appendColorizedTarget(
                const S9sString &value,
                S9sString       &result) const;

        void appendColorizedPrivileges(
                const S9sString &value,
                S9sString       &result) const;

        void appendColorizedPrivilege(
                const S9sString &value,
                S9sString       &result) const;

        bool parseUrlEncodedChar(
                const S9sString &input,
                size_t          &location,
                char            &theChar);

    private:
        S9sVariantMap    m_properties;

        friend class UtS9sAccount;
};

