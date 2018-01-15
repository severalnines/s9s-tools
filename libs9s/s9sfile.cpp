/*
 * Severalnines Tools
 * Copyright (C) 2017  Severalnines AB
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
#include "s9sfile.h"
#include "s9sfile_p.h"

#include "S9sVariantList"

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>

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
        S9sString homeDir = getenv("HOME");

        m_priv->m_fileName.erase(0, 2);
        m_priv->m_path.erase(0, 2);
        m_priv->m_path = homeDir + dirSeparator() + m_priv->m_path;
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
 * Returns the full path of the specified file.
 * (the ./ and ~/ are substituted)
 */
S9sString
S9sFile::path() const
{
    return m_priv->m_path;
}

bool
S9sFile::fileExists(
        const S9sString &path)
{
    struct stat buffer;
    bool        retval = false;

    retval = stat(STR(path), &buffer) == 0;

    return retval;
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

void
S9sFile::close()
{
    m_priv->close();
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

bool
S9sFile::writeTxtFile(
        const S9sString   &content)
{
	mode_t   mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int      fileDescriptor;
	ssize_t  nBytes;
	int      errorCode;

	fileDescriptor = open(STR(m_priv->m_path),
			O_WRONLY | O_CREAT | O_TRUNC, mode);
	if (fileDescriptor < 0)
	{
		m_priv->m_errorString.sprintf(
				"Error opening '%s' for writing: %m",
				STR(m_priv->m_path));
		return false;
	}

	nBytes = safeWrite(fileDescriptor,
		(void*) STR(content), content.size());
	if (nBytes < (ssize_t) content.size())
	{
		m_priv->m_errorString.sprintf(
				"Error writing file '%s': %m",
				STR(m_priv->m_path));
		::close(fileDescriptor);
		return false;
	}

	errorCode = ::close(fileDescriptor);
	if (errorCode != 0)
	{
		m_priv->m_errorString.sprintf(
				"Error closing file '%s': %m",
				STR(m_priv->m_path));
		return false;
	}

	return true;
}

/**
 * Prints formatted string into a file. Pretty easy to create and fill text
 * files using this function.
 */
bool
S9sFile::fprintf(
        const char *formatString,
        ...)
{
    int  nChars;

    m_priv->m_errorString.clear();

    /*
     * If the file is not open we open it here.
     */
    if (m_priv->m_outputStream == NULL)
    {
        m_priv->m_outputStream = fopen(STR(m_priv->m_path), "a");
        if (!m_priv->m_outputStream)
        {
            m_priv->m_errorString.sprintf(
                    "Unable to open '%s' for writing: %m",
                    STR(m_priv->m_path));
            return false;
        }
    }

    /*
     * Printing into the file.
     */
    va_list arguments;
    va_start(arguments, formatString);

    nChars = vfprintf(m_priv->m_outputStream, formatString, arguments);
    if (nChars <= 0)
        m_priv->m_errorString.sprintf("Error writing file: %m.");

    va_end(arguments);

    return nChars >= 0;
}


S9sString 
S9sFile::errorString() const
{
    return m_priv->m_errorString;
}

S9sFilePath 
S9sFile::currentWorkingDirectory()
{
#ifdef _GNU_SOURCE
    char        *buffer = get_current_dir_name();
#else
    size_t sz = 0;
    char * buffer = getcwd(NULL, sz);
#endif

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

void
S9sFile::listFiles (
        const S9sFilePath &directoryPath,
        S9sVariantList    &files,
        bool               prependPath,
        bool               recursive,
        bool               includeDirs)
{
    DIR             *directory;
    S9sVariantList   subDirs;

    if ((directory = opendir(STR(directoryPath))))
    {
        dirent *entry = NULL;
        while ((entry = readdir (directory)) != NULL)
        {
            S9sString fullPath;
            struct stat ss;

            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
                continue;

            fullPath = buildPath(directoryPath, S9sString(entry->d_name));

            if (stat(STR(fullPath), &ss) == 0 && S_ISDIR(ss.st_mode))
            {
                subDirs << S9sString (entry->d_name);

                // we might don't want the dirs in the returned list
                if (! includeDirs)
                    continue;
            }

            if (prependPath) 
            {
                files << S9sVariant(fullPath);
            } else {
                files << S9sVariant(entry->d_name);
            }
        }

        closedir(directory);
    }

    if (recursive)
    {
        // ok lets go trough on the sub-dirs one-by-one
        for (uint idx = 0; idx < subDirs.size (); ++idx)
        {
            S9sString path = buildPath(directoryPath, subDirs[idx].toString());
            listFiles (path, files, prependPath, recursive, includeDirs);
        }
    }
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

/**
 * Same as write(3), but this repeats the request if it was interrupted by a
 * signal.
 */
ssize_t
S9sFile::safeWrite (
    int     fileDescriptor, 
    void   *buffer, 
    size_t  bufferSize)
{
    ssize_t retval;
  
    do {
        retval = ::write(fileDescriptor, buffer, bufferSize);
    } while (retval == -1 && errno == EINTR);

    return retval;
}

