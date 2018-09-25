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
#include "ut_s9sfile.h"

#include "S9sFile"
#include "S9sEvent"
#include "S9sDateTime"

#include <cstdio>
#include <cstring>

#define DEBUG
#define WARNING
#include "s9sdebug.h"

UtS9sFile::UtS9sFile()
{
    S9S_DEBUG("");
    setlocale(LC_NUMERIC, getenv("C"));
    setlocale(LC_ALL,     getenv("C"));
}

UtS9sFile::~UtS9sFile()
{
}

bool
UtS9sFile::runTest(
        const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testConstruct,   retval);

    return retval;
}

/**
 *
 */
bool
UtS9sFile::testConstruct()
{
    S9sFile     inputFile("/home/pipas/events.json");
    S9sDateTime prevCreated;
    S9sDateTime thisCreated;
    S9sEvent event;
    int      nEvents = 0;
    bool     success;

    for (;;)
    {
        success = inputFile.readEvent(event);
        if (!success)
            break;

        if (nEvents == 0)
        {
            prevCreated = event.created();
            S9S_DEBUG("[%6d] ", nEvents);
        } else {
            double millis;
            S9S_UNUSED(millis);

            thisCreated = event.created();
            millis      = S9sDateTime::milliseconds(thisCreated, prevCreated);
            S9S_DEBUG("[%6d] %f", nEvents, millis);
            prevCreated = thisCreated;
        }

        ++nEvents;
    }

    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sFile)
