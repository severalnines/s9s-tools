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
#include "ut_s9sdatetime.h"

#include "s9sdatetime.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

UtS9sDateTime::UtS9sDateTime()
{
}

UtS9sDateTime::~UtS9sDateTime()
{
}

bool
UtS9sDateTime::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testCreate,          retval);

    return retval;
}

/**
 *
 */
bool
UtS9sDateTime::testCreate()
{
    S9sDateTime today = S9sDateTime::currentDateTime();
    S9sDateTime dateTime;
    S9sString   theString;

    bool        success;

    theString = 
        today.toString(S9sDateTime::MySqlLogFileDateFormat) + 
        "T07:13:54.000Z";

    S9S_DEBUG("-> '%s'", STR(theString));
    success = dateTime.parse(theString);
    S9S_VERIFY(success);

    S9S_DEBUG("");
    S9S_DEBUG("TzDateTimeFormat : %s", 
            STR(dateTime.toString(S9sDateTime::TzDateTimeFormat)));

    S9S_DEBUG("  LongTimeFormat : %s",
            STR(dateTime.toString(S9sDateTime::LongTimeFormat)));


    S9S_VERIFY(
            dateTime.toString(S9sDateTime::TzDateTimeFormat).
            contains("T07:13:54.000Z"));
    
    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sDateTime)

