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

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

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
