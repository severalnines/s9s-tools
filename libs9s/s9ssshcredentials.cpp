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
#include "s9ssshcredentials.h"

S9sSshCredentials::S9sSshCredentials() :
    S9sObject()
{
}

S9sSshCredentials::~S9sSshCredentials()
{
}
        
S9sSshCredentials &
S9sSshCredentials::operator=(
        const S9sVariantMap &rhs)
{
    S9sObject::operator=(rhs);
    return *this;
}

void
S9sSshCredentials::setUserName(
        const S9sString &value) 
{
    setProperty("user_name", value);
}

void
S9sSshCredentials::setPassword(
        const S9sString &value) 
{
    setProperty("password", value);
}

void
S9sSshCredentials::setPublicKeyFilePath(
        const S9sString &value) 
{
    setProperty("public_key_file", value);
}

void
S9sSshCredentials::setPort(
        const int value)
{
    setProperty("port", value);
}

void
S9sSshCredentials::setTimeout(
        const int value)
{
    setProperty("timeout", value);
}

void
S9sSshCredentials::setTtyForSudo(
        const bool value)
{
    setProperty("tty_for_sudo", value);
}
