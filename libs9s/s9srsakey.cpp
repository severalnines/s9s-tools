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
#include "s9srsakey.h"
#include "s9srsakey_p.h"
#include "s9sfile.h"

S9sRsaKey::S9sRsaKey() :
    m_priv(new S9sRsaKeyPrivate)
{
}

S9sRsaKey::S9sRsaKey(
        const S9sRsaKey &orig) :
    m_priv(orig.m_priv)
{
    if (m_priv)
        m_priv->ref();
    else
        m_priv = new S9sRsaKeyPrivate;
}

S9sRsaKey::~S9sRsaKey()
{
    if (m_priv && m_priv->unRef() == 0)
    {
        delete m_priv;
        m_priv = 0;
    }
}

/**
 * Assignment operator that utilises the implicit sharing this class uses.
 */
S9sRsaKey &
S9sRsaKey::operator= (
        const S9sRsaKey &rhs)
{
    if (this == &rhs)
        return *this;

    if (m_priv && m_priv->unRef() == 0)
    {
        delete m_priv;
        m_priv = 0;
    }

    m_priv = rhs.m_priv;
    if (m_priv) 
    {
        m_priv->ref ();
    }

    return *this;
}

/**
 * Returns true if the class holds a valid RSA keypair
 */
bool
S9sRsaKey::isValid() const
{
    if (m_priv)
        return m_priv->isValid();

    return false;
}

bool
S9sRsaKey::signRsaSha256(
        const S9sString     &input,
        S9sString           &signature)
{
    return m_priv->signRsaSha256(input, signature);
}

bool
S9sRsaKey::loadKeyFromFile(
        const S9sString     &path)
{
    S9sFile file(path);
    return m_priv->loadFromFile (file.path());
}

bool
S9sRsaKey::generateKeyPair()
{
    return m_priv->generateKeyPair();
}

bool
S9sRsaKey::saveKeys(
        const S9sString &privateKeyPath,
        const S9sString &publicKeyPath,
        S9sString       &errorString)
{
    S9sFile privFile(privateKeyPath);
    S9sFile pubFile(publicKeyPath);

    if (!m_priv->saveKeys(privFile.path(), pubFile.path()))
    {
        errorString = m_priv->m_errorString;
        return false;
    }

    return true;
}

