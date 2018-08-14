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

#include "S9sString"

class S9sFilePrivate
{
    public:
        S9sFilePrivate();
        S9sFilePrivate(const S9sFilePrivate &orig);
        ~S9sFilePrivate();
        
        void ref();
        int unRef();

        void close();

    private:
        int                     m_referenceCounter;
        S9sFileName             m_fileName;
        S9sFilePath             m_path;
        mutable S9sString       m_errorString;
        FILE                   *m_outputStream;
        FILE                   *m_inputStream;
        ulonglong               m_lineNumber;
        
        friend class S9sFile;
};
