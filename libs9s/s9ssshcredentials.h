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

#include "S9sObject"

class S9sSshCredentials : public S9sObject
{
    public:
        S9sSshCredentials();

        virtual ~S9sSshCredentials();
       
        virtual S9sSshCredentials &operator=(const S9sVariantMap &rhs);

        virtual S9sString className() const;

        void setUserName(const S9sString &value);
        void setPassword(const S9sString &value);
        void setPublicKeyFilePath(const S9sString &value);
        void setPort(const int value);
        void setTimeout(const int value);
        void setTtyForSudo(const bool value);
};
