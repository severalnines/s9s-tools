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

#include "S9sString"
#include <sys/types.h>

class S9sFilePrivate;

class S9sFile
{
    public:
        S9sFile();
        S9sFile(const S9sFilePath path);
        S9sFile(const S9sFile &orig);

        virtual ~S9sFile();
       
        S9sFile &operator=(const S9sFile &rhs);

        S9sString path() const;
        bool exists() const;
        void close();
        bool readTxtFile(S9sString &content);
        bool writeTxtFile(const S9sString &content);
        bool fprintf(const char *formatString, ...);

        S9sString errorString() const;

        static S9sFilePath currentWorkingDirectory();
        static S9sFileName basename(const S9sFilePath &filePath);
        static S9sDirName dirname(const S9sFilePath &fileName);
        static bool isAbsolutePath(const S9sFilePath &path);
        static S9sString dirSeparator() { return "/"; };

        static S9sFilePath 
            buildPath(
                const S9sString &path1,
                const S9sString &path2);

        static void
            listFiles (
                const S9sFilePath &directoryPath,
                S9sVariantList    &files,
                bool               prependPath = false,
                bool               recursive   = false,
                bool               includeDirs = false);

    private:
        ssize_t safeRead(
                int     fileDescriptor, 
                void   *buffer, 
                size_t  bufferSize);
        
        ssize_t safeWrite(
                int     fileDescriptor, 
                void   *buffer, 
                size_t  bufferSize);

    protected: 
        S9sFilePrivate    *m_priv;
};
