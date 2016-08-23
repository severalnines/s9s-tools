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
#include "ut_s9sstring.h"

#include <libs9s/library.h>
#include <cstdio>
#include <cstring>

//#define DEBUG
#include "s9sdebug.h"

UtS9sString::UtS9sString()
{
    S9S_DEBUG("");
}

UtS9sString::~UtS9sString()
{
}

bool
UtS9sString::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testCreate,  retval);
    PERFORM_TEST(testAssign,  retval);
    PERFORM_TEST(testUnQuote, retval);
    PERFORM_TEST(testToUpper, retval);
    PERFORM_TEST(testToInt,   retval);

    return retval;
}

/**
 * Testing the constructors.
 */
bool
UtS9sString::testCreate()
{
    std::string  stdString("stdString");
    S9sString   string1;
    S9sString   string2 = "string2";
    S9sString   string3 = stdString;

    S9S_VERIFY(string1.empty());
    S9S_COMPARE(string2, "string2");
    S9S_COMPARE(string3, stdString);

    return true;
}

/**
 * Testing the assignment operator.
 */
bool
UtS9sString::testAssign()
{
    S9sString string1 = "one";

    string1 = "two";
    S9S_COMPARE(string1, "two");
    return true;
}

/**
 * Testing the unQuote() method.
 */
bool
UtS9sString::testUnQuote()
{
    S9sString string1;

    string1 = "two";
    S9S_COMPARE(string1.unQuote(), "two");

    string1 = "'two'";
    S9S_COMPARE(string1.unQuote(), "two");

    string1 = "\"two\"";
    S9S_COMPARE(string1.unQuote(), "two");

    string1 = "\"two'";
    S9S_COMPARE(string1.unQuote(), "\"two'");

    string1 = "'two\"";
    S9S_COMPARE(string1.unQuote(), "'two\"");
    return true;
}

/**
 * Testing the toUpper() method.
 */
bool
UtS9sString::testToUpper()
{
    S9sString string1;

    string1 = "two";
    S9S_COMPARE(string1.toUpper(), "TWO");

    string1 = "'Two'";
    S9S_COMPARE(string1.toUpper(), "'TWO'");

    return true;
}

/**
 * Testing the toInt() method.
 */
bool
UtS9sString::testToInt()
{
    S9sString theString;
    
    theString = "42";
    S9S_COMPARE(theString.toInt(), 42);

    theString = "3.2.1";
    S9S_COMPARE(theString.toInt(), 3);
    
    theString = "25.3.2.1";
    S9S_COMPARE(theString.toInt(), 25);
    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sString)

