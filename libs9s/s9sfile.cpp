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
#include "s9sfile.h"
#include "s9sfile_p.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define READ_BUFFER_SIZE 16384
#define DIRSEPARATOR '/'


//#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sFile::S9sFile() :
    m_priv(new S9sFilePrivate)
{
}

S9sFile::S9sFile(
        const S9sFilePath path) :
    m_priv(new S9sFilePrivate)
{
    m_priv->m_fileName = path;
    m_priv->m_path     = path;

    if (m_priv->m_path.startsWith("./"))
    {
        // File in the current working directory
        m_priv->m_fileName.erase(0, 2);
        m_priv->m_path.erase(0, 2);

        m_priv->m_path = 
            currentWorkingDirectory() + dirSeparator() + m_priv->m_path;

    } else if (m_priv->m_path.startsWith("/"))
    {
        // File name with a full path
        char *orig = strdup(STR(path));
        m_priv->m_fileName = basename(orig);
        free (orig);
    } else if (m_priv->m_path.startsWith("~/")) 
    {
        // FIXME: This might be a remote file and then we would need the home
        // directory of the remote user on the remote host, right? Anyway, this
        // works for us now.
        S9sString homeDir = getenv("HOME");

        m_priv->m_fileName.erase(0, 2);
        m_priv->m_path.erase(0, 2);

        m_priv->m_path = 
            homeDir + dirSeparator() + m_priv->m_path;
    } else if (!m_priv->m_path.empty()) 
    {
        // Just a regular file name, the path is the current working directory
        m_priv->m_path = 
            currentWorkingDirectory() + dirSeparator() + m_priv->m_path;
    }
}

S9sFile::S9sFile (
		const S9sFile &orig)
{
	m_priv = orig.m_priv;

	if (m_priv) 
		m_priv->ref();
}

S9sFile::~S9sFile()
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
S9sFile &
S9sFile::operator= (
		const S9sFile &rhs)
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
 * \returns true if a file exists as any directory entry (file, directory,
 *   character special file, anything)
 */
bool 
S9sFile::exists() const
{
    struct stat buffer;
    bool retval = false;

    m_priv->m_errorString.clear();
    retval = stat(STR(m_priv->m_path), &buffer) == 0;

    return retval;
}

bool
S9sFile::readTxtFile(
        S9sString &content)
{
    int      fileDescriptor;
    bool     retval = false;

    fileDescriptor = open(STR(m_priv->m_path), O_RDONLY); 
    if (fileDescriptor < 0)
    {
        m_priv->m_errorString.sprintf(
                "Error opening '%s' for reading: %m", 
                STR(m_priv->m_path));

        return false;
    }

    // content should be empty
    content.clear();

    char *buffer = new char[READ_BUFFER_SIZE];
    if (buffer == 0) 
    {
        m_priv->m_errorString.sprintf("Can't allocate memory");
        return false;
    }

    retval = true;
    ssize_t readBytes = 0;
    do
    {
        readBytes = safeRead(fileDescriptor, buffer, READ_BUFFER_SIZE);

        if (readBytes > 0) 
        {
            content += std::string(buffer, (size_t) readBytes);
        } else if (readBytes < 0) 
        {
            // reading failed
            m_priv->m_errorString.sprintf("read error: %m");
            retval = false;
            break;
        }
    } while (readBytes > 0);

    delete[] buffer;
    ::close(fileDescriptor);

    return retval;
}


S9sFilePath 
S9sFile::currentWorkingDirectory()
{
    char        *buffer = get_current_dir_name();
    S9sFilePath  retval = buffer;

    if (buffer)
        free(buffer);

    return retval;
}

/**
 * \returns the base name part of the file name
 *
 * For example if the file name is "/my.cnf" the return value will be "my.cnf".
 */
S9sFileName
S9sFile::basename(
        const S9sFilePath &filePath)
{
    S9sFileName retval = filePath;

    size_t lastsep = retval.find_last_of("/");
    if (lastsep != std::string::npos)
        retval = retval.substr(lastsep + 1);

    return retval;
}

S9sDirName
S9sFile::dirname(
        const S9sFilePath &fileName)
{
    S9sDirName retval = fileName;

    // remove the ending slashes first
    while (retval.size () > 1 && retval.at (retval.size () - 1) == '/')
        retval = retval.substr (0, retval.size () - 1);

    size_t lastsep = retval.find_last_of ("/");
    if (lastsep != std::string::npos)
        retval = retval.substr (0, lastsep + 1);

    return retval;
}
        
bool 
S9sFile::isAbsolutePath(
        const S9sFilePath &path)
{
    return !path.empty() && path[0] == '/';
}

S9sFilePath
S9sFile::buildPath(
        const S9sString &path1,
        const S9sString &path2)
{
    S9sFilePath retval;
    bool        needSeparator = 
        !path1.empty() && 
        path1[path1.length() - 1] != DIRSEPARATOR &&
        !path2.empty() && 
        path2[0] != DIRSEPARATOR;

    bool        removeSeparator = 
        !path1.empty() && 
        path1[path1.length() - 1] == DIRSEPARATOR &&
        !path2.empty() && 
        path2[0] == DIRSEPARATOR;

    retval += path1;

    if (removeSeparator)
        retval.resize(retval.size () - 1);

    if (needSeparator)
        retval += DIRSEPARATOR;

    retval += path2;

    return retval;
}


/**
 * Same as read(3), but this repeats the request if it was interrupted by a
 * signal.
 */
ssize_t
S9sFile::safeRead (
    int     fileDescriptor, 
    void   *buffer, 
    size_t  bufferSize)
{
    ssize_t retval;
  
    do {
        retval = ::read(fileDescriptor, buffer, bufferSize);
    } while (retval == -1 && errno == EINTR);

    return retval;
}
