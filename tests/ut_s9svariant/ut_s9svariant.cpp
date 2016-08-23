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
    PERFORM_TEST(testToDouble,  retval);
    PERFORM_TEST(testToBoolean, retval);
    PERFORM_TEST(testToInt,     retval);

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

/**
 * Testing the toDouble() function.
 */
bool
UtS9sVariant::testToDouble()
{
    S9S_COMPARE(S9sVariant(42.0).toDouble(), 42.0);
    S9S_COMPARE(S9sVariant(42).toDouble(), 42.0);
    S9S_COMPARE(S9sVariant(42ull).toDouble(), 42.0);
    S9S_COMPARE(S9sVariant("42").toDouble(), 42.0);

    return true;
}

bool
UtS9sVariant::testToBoolean()
{
    S9S_COMPARE(S9sVariant("yes").toBoolean(), true);
    S9S_COMPARE(S9sVariant("true").toBoolean(), true);
    S9S_COMPARE(S9sVariant("on").toBoolean(), true);
    S9S_COMPARE(S9sVariant("t").toBoolean(), true);

    S9S_COMPARE(S9sVariant("no").toBoolean(), false);
    S9S_COMPARE(S9sVariant("false").toBoolean(), false);
    S9S_COMPARE(S9sVariant("off").toBoolean(), false);
    S9S_COMPARE(S9sVariant("f").toBoolean(), false);
    
    S9S_COMPARE(S9sVariant(1).toBoolean(), true);
    S9S_COMPARE(S9sVariant(10).toBoolean(), true);
    
    S9S_COMPARE(S9sVariant(0).toBoolean(), false);
    
    S9S_COMPARE(S9sVariant("yes").toBoolean(false), true);
    S9S_COMPARE(S9sVariant("true").toBoolean(false), true);
    S9S_COMPARE(S9sVariant("on").toBoolean(false), true);
    S9S_COMPARE(S9sVariant("t").toBoolean(false), true);

    S9S_COMPARE(S9sVariant("no").toBoolean(true), false);
    S9S_COMPARE(S9sVariant("false").toBoolean(true), false);
    S9S_COMPARE(S9sVariant("off").toBoolean(true), false);
    S9S_COMPARE(S9sVariant("f").toBoolean(true), false);
    
    S9S_COMPARE(S9sVariant("1").toBoolean(false), true);
    S9S_COMPARE(S9sVariant("10").toBoolean(false), true);
    
    S9S_COMPARE(S9sVariant("0").toBoolean(true), false);
    
    // An invalid variant defaults to false, and this should be used to simplify
    // the code.
    S9sVariant var1;
    S9S_COMPARE(var1.toBoolean(), false);
    return true;
}

/**
 * Testing the toInt() method.
 */
bool
UtS9sVariant::testToInt()
{
    // An invalid variant defaults to 0, and this should be used to simplify the
    // code.
    S9sVariant var1;
    S9S_COMPARE(var1.toInt(), 0);
    S9S_COMPARE(var1.toInt(3), 3);

    var1 = 42.42;
    S9S_COMPARE(var1.toInt(), 42);
    S9S_COMPARE(var1.toInt(3), 42);
    
    var1 = 42ull;
    S9S_COMPARE(var1.toInt(), 42);
    S9S_COMPARE(var1.toInt(3), 42);
    
    var1 = "42";
    S9S_COMPARE(var1.toInt(), 42);
    S9S_COMPARE(var1.toInt(3), 42);

    return true;
}


S9S_UNIT_TEST_MAIN(UtS9sVariant)

