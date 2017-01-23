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

#include "s9sconfigfile_p.h"

S9sConfigFilePrivate::S9sConfigFilePrivate() :
    m_referenceCounter(1),
    m_syntax(S9s::GenericConfigSyntax),
    hostalive(),
    m_crc(0),
    m_size(0),
    m_hasChange(false),
    m_timeStamp((ulonglong)time(0)),
    m_includeLevel(0),
    m_parseContext(NULL)
{
}

S9sConfigFilePrivate::~S9sConfigFilePrivate()
{
    if (m_parseContext)
    {
        delete m_parseContext;
        m_parseContext = NULL;
    }
}

void 
S9sConfigFilePrivate::ref()
{
	++m_referenceCounter;
}

int 
S9sConfigFilePrivate::unRef()
{
	return --m_referenceCounter;
}

