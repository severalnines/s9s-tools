/*
 * Severalnines Tools
 * Copyright (C) 2017 Severalnines AB
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

#include "S9sString"

class S9sDir
{
    public:
        S9sDir();
        S9sDir(const S9sFilePath path);
        ~S9sDir();

        const S9sFilePath &path() const { return m_path; };
        const S9sString errorString() const { return m_errorString; };

        static bool exists(const S9sString &path);
        bool exists() const;

        bool mkdir();

    protected: 
        S9sFilePath           m_fileName;
        S9sFilePath           m_path;
        mutable S9sString     m_errorString;
};
