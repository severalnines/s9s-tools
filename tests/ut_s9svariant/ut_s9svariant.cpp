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
#include "ut_s9svariant.h"

#include "S9sVariant"
#include <cstdio>
#include <cstring>

#define DEBUG
#include "s9sdebug.h"

UtS9sVariant::UtS9sVariant()
{
    S9S_DEBUG("");
}

UtS9sVariant::~UtS9sVariant()
{
}

bool
UtS9sVariant::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testConstruct, retval);
    PERFORM_TEST(testAssign,    retval);

    return retval;
}

/**
 * This test will check the various types of constructors the CmonVariant class
 * have.
 */
bool
UtS9sVariant::testConstruct()
{
    S9sVariant var1;
    S9sVariant var2 = 12;
    S9sVariant var3 = true;
    S9sVariant var4 = "a string";
    S9sVariant var5 = std::string("another string");

    S9S_COMPARE(var1.isInvalid(), true);
    S9S_COMPARE(var2.isInvalid(), false);
    S9S_COMPARE(var2.toInt(), 12);
    S9S_COMPARE(var3.isInvalid(), false);
    S9S_COMPARE(var3.toBoolean(), true);
    S9S_COMPARE(var4.isInvalid(), false);
    S9S_COMPARE(var4.toString(), std::string("a string"));
    S9S_COMPARE(var5.isInvalid(), false);
    S9S_COMPARE(var5.toString(), std::string("another string"));

    //
    // Copy constructor...
    //
    S9sVariant var11(var1);
    S9sVariant var12(var2);
    S9sVariant var13(var3);
    S9sVariant var14(var4);
    S9sVariant var15(var5);
    S9S_COMPARE(var11.isInvalid(), true);
    S9S_COMPARE(var12.isInvalid(), false);
    S9S_COMPARE(var12.toInt(), 12);
    S9S_COMPARE(var13.isInvalid(), false);
    S9S_COMPARE(var13.toBoolean(), true);
    S9S_COMPARE(var14.isInvalid(), false);
    S9S_COMPARE(var14.toString(), std::string("a string"));
    S9S_COMPARE(var15.isInvalid(), false);
    S9S_COMPARE(var15.toString(), std::string("another string"));

    return true;
}

/**
 * Testing the assignment operator (operator=()).
 */
bool
UtS9sVariant::testAssign()
{
    S9sVariant var1;
    S9sVariant var2;
    S9sVariant var3;
    S9sVariant var4;
    S9sVariant var5;
    
    var2 = 12;
    var3 = true;
    var4 = "a string";
    var5 = std::string("another string");

    S9S_COMPARE(var1.isInvalid(), true);
    S9S_COMPARE(var2.isInvalid(), false);
    S9S_COMPARE(var2.toInt(), 12);
    S9S_COMPARE(var3.isInvalid(), false);
    S9S_COMPARE(var3.toBoolean(), true);
    S9S_COMPARE(var4.isInvalid(), false);
    S9S_COMPARE(var4.toString(), std::string("a string"));
    S9S_COMPARE(var5.isInvalid(), false);
    S9S_COMPARE(var5.toString(), std::string("another string"));

    return true;
}


S9S_UNIT_TEST_MAIN(UtS9sVariant)

