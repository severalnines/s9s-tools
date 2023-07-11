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

#include "s9sstring.h"

class S9sRsaKeyPrivate;

class S9sRsaKey
{
    public:
        S9sRsaKey();
        S9sRsaKey(const S9sRsaKey &orig);

        virtual ~S9sRsaKey();
        S9sRsaKey &operator=(const S9sRsaKey &rhs);

        bool isValid() const;

        bool signRsaSha256(
                const S9sString     &input,
                S9sString           &signature);

        bool loadKeyFromFile(const S9sString &path);
        bool generateKeyPair();

        bool saveKeys(
                const S9sString &privateKeyPath,
                const S9sString &publicKeyPath,
                S9sString       &errorString);

    private:
        S9sRsaKeyPrivate    *m_priv;
};

