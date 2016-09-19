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
#include "ut_s9surl.h"

#include "S9sUrl"

//#define DEBUG
#include "s9sdebug.h"

UtS9sUrl::UtS9sUrl()
{
}

UtS9sUrl::~UtS9sUrl()
{
}

bool
UtS9sUrl::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testCreate01,      retval);

    return retval;
}

/**
 * Testing the constructor that parse the string representation.
 */
bool
UtS9sUrl::testCreate01()
{
    S9sUrl url1("hostname.eu");

    S9S_COMPARE(url1.hostName(), "hostname.eu");
    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sUrl)
