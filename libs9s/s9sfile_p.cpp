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
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar. If not, see <http://www.gnu.org/licenses/>.
 */

#include "s9sfile_p.h"

#include <stdio.h>

S9sFilePrivate::S9sFilePrivate() :
    m_referenceCounter(1),
    m_outputStream(0)
{
}

/**
 * Copy constructor for deep copy.
 */
S9sFilePrivate::S9sFilePrivate(
        const S9sFilePrivate &orig) :
    m_referenceCounter(1),
    m_fileName(orig.m_fileName),
    m_path(orig.m_path),
    m_errorString(orig.m_path),
    m_outputStream(NULL)
{
}

S9sFilePrivate::~S9sFilePrivate()
{
    close();
}

void 
S9sFilePrivate::ref()
{
	++m_referenceCounter;
}

int 
S9sFilePrivate::unRef()
{
	return --m_referenceCounter;
}

void
S9sFilePrivate::close()
{
    if (m_outputStream)
    {
        fclose(m_outputStream);
        m_outputStream = 0;
    }
}
