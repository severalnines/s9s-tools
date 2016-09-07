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
#include "s9sdir.h"
#include "s9sfile.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sDir::S9sDir()
{
}

S9sDir::S9sDir(
        const S9sFilePath path) :
    m_fileName(path),
    m_path(path)
{
    if (m_path.startsWith("./"))
    {
        // File in the current working directory
        m_fileName.erase(0, 2);
        m_path.erase(0, 2);

        m_path = 
            S9sFile::currentWorkingDirectory() + 
            S9sFile::dirSeparator() + m_path;
    } else if (m_path.startsWith("/"))
    {
        // File name with a full path
        char *orig = strdup(STR(path));
        m_fileName = S9sFile::basename(orig);
        free (orig);
    } else if (m_path.startsWith("~/")) 
    {
        S9sString homeDir = getenv("HOME");

        m_fileName.erase(0, 2);
        m_path.erase(0, 2);
        m_path = homeDir + S9sFile::dirSeparator() + m_path;
    } else {
        // Just a regular file name, the path is the current working directory
        m_path = 
            S9sFile::currentWorkingDirectory() + 
            S9sFile::dirSeparator() + m_path;
    }
}

S9sDir::~S9sDir()
{
}


bool
S9sDir::mkdir()
{
    int       retval;
    S9sString parentPath;

    if (m_path.endsWith("/"))
        parentPath = S9sFile::dirname(m_path.substr (0, m_path.size() - 1));
    else
        parentPath = S9sFile::dirname(m_path);

    // lets check if parentDir exists and create it when its necessary
    if (!parentPath.empty () &&
        parentPath != "/" &&
        parentPath != m_path &&
        !S9sDir::exists(parentPath))
    {
        S9sDir parentDir (parentPath);

        if (! parentDir.mkdir ()) {
            m_errorString = parentDir.errorString ();
            return false;
        }
    }

    retval = ::mkdir(STR(m_path), 0750);
    if (retval != 0)
    {
        m_errorString.sprintf(
                "Unable to create directory '%s': %m",
                STR(m_path));
        return false;
    }

    return true;
}

bool
S9sDir::exists() const
{
    return S9sDir::exists(path());
}

/**
 * \returns This method returns true only if the specified path is exists and it
 *   is a directory.
 */
bool
S9sDir::exists(
        const S9sString &path)
{
    struct stat ss;

    // this method works on every distro
    if (stat(STR (path), &ss) == 0 && S_ISDIR(ss.st_mode))
        return true;

    return false;
}

