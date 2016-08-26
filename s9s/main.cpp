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
#include "library.h"
#include "S9sOptions"

#include <stdio.h>

int main(int argc, char **argv)
{
    S9sOptions *options = S9sOptions::instance();
    bool        success;
    int         exitStatus;

    success = options->readOptions(&argc, argv);
    if (!success)
    {
        fprintf(stderr, "%s: %s\n\n", 
                STR(options->binaryName()),
                STR(options->errorString()));
        fflush(stderr);

        goto finalize;
    }

finalize:
    exitStatus = options->exitStatus();
    S9sOptions::uninit();

    return exitStatus;
}

