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

#include <openssl/rsa.h>

class S9sRsaKeyPrivate
{
    public:
        S9sRsaKeyPrivate();
        ~S9sRsaKeyPrivate();

        void ref();
        int unRef();

    private:
        void release();
        bool isValid() const { return m_rsa; }

        bool loadFromFile(const S9sString &path);
        bool generateKeyPair();

        bool signRsaSha256(
                const S9sString     &input,
                S9sString           &signature);

        bool saveKeys(
            const S9sString &privateKeyPath,
            const S9sString &publicKeyPath);

    private:
        RSA            *m_rsa;
        S9sString       m_errorString;
        int             m_referenceCounter;

    friend class S9sRsaKey;
};

