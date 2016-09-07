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
#pragma once

#include "s9sconfigfile.h"

class S9sConfigFilePrivate
{
    public:
        S9sConfigFilePrivate();
        ~S9sConfigFilePrivate();
        
        void ref();
        int unRef();

    private:
        int             m_referenceCounter;
        S9s::Syntax     m_syntax;

        S9sString       name;
        S9sString       filename;
        int             hostalive;

        S9sString      m_content;
        S9sString      m_fullpath;
        ulonglong       m_crc;
        int             m_size;
        // other misc. flags from/to the DB&RPC
        bool            m_hasChange;
        ulonglong       m_timeStamp;
        int             m_includeLevel;       
        S9sVariantList  m_searchGroups;
        
        S9sClusterConfigParseContext *m_parseContext;

        friend class S9sConfigFile;
};

