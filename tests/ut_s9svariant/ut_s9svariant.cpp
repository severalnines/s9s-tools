/*
 * Severalnines Tools
 * Copyright (C) 2018  Severalnines AB
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

#include "s9svariant.h"
#include "s9svariantmap.h"
#include "s9svariantlist.h"
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

    PERFORM_TEST(testConstruct,   retval);
    PERFORM_TEST(testAssign,      retval);
    PERFORM_TEST(testToDouble,    retval);
    PERFORM_TEST(testToBoolean,   retval);
    PERFORM_TEST(testToInt,       retval);
    PERFORM_TEST(testToULongLong, retval);
    PERFORM_TEST(testOperators01, retval);
    PERFORM_TEST(testEqual,       retval);

    return retval;
}

/**
 * This test will check the various types of constructors the S9sVariant class
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

    S9sVariant var16(atof("42.2"));

    S9S_COMPARE(var16.typeName(), "double");
    S9S_COMPARE(var16.toString(), "42.2");

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

/**
 * Testing the toBoolean() method.
 */
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

    // With extra characters after the integer, the first field is only
    // considered.
    var1 = "24.42";
    S9S_COMPARE(var1.toInt(), 24);
    S9S_COMPARE(var1.toInt(3), 24);

    // When no conversion is possible at all, the default value must be
    // returned.
    var1 = "xxx";
    S9S_COMPARE(var1.toInt(), 0);
    S9S_COMPARE(var1.toInt(25), 25);

    return true;
}

/**
 * Testting the toULongLong() method.
 */
bool
UtS9sVariant::testToULongLong()
{
    S9S_COMPARE(S9sVariant("42").toULongLong(), 42ull);
    S9S_COMPARE(S9sVariant("").toULongLong(), 0ull);
    
    return true;
}

bool
UtS9sVariant::testOperators01()
{
    S9sVariant variant;

    variant = 0.125;
    S9S_COMPARE(variant.typeName(), "double");

    variant += 0.125;
    S9S_COMPARE(variant.typeName(), "double");

    return true;
}

bool
UtS9sVariant::testEqual()
{
    S9sVariant   var1 = 42;
    S9sVariant   var2 = 42;
    S9sVariant   var3 = 42.0;
    S9sVariant   var4 = "string";
    S9sVariant   var5 = "string";
    S9sVariant   var6 = "other string";
    S9sVariant   var7 = "42";
    S9sVariant   var8 = 42.1;

    S9S_VERIFY(var1 == var2)
    S9S_VERIFY(var1 == var3)
    S9S_VERIFY(var4 == var5)
    S9S_VERIFY(!(var5 == var6))
    S9S_VERIFY(!(var1 == var7))
    S9S_VERIFY(!(var1 == var8))
    S9S_VERIFY(!(var3 == var8))
    
    S9S_VERIFY(var1 == 42);
    S9S_VERIFY(var3 == 42.0);
    S9S_VERIFY(var3 == 42);
    S9S_VERIFY(var4 == "string");

    S9S_VERIFY(S9sVariant(true) == S9sVariant(true));
    S9S_VERIFY(S9sVariant(false) == S9sVariant(false));

    /*
     * Compare maps
     */
    S9sVariantMap map1, map2;
    S9S_VERIFY(S9sVariant(map1) == S9sVariant(map2));

    map1["key1"] = "value1";
    S9S_VERIFY(! ( S9sVariant(map1) == S9sVariant(map2)));
    S9S_VERIFY(    S9sVariant(map1) != S9sVariant(map2));

    map2["key1"] = "value1";
    S9S_VERIFY(    S9sVariant(map1) == S9sVariant(map2));
    S9S_VERIFY(! ( S9sVariant(map1) != S9sVariant(map2)));

    /*
     * Compare lists
     */
    S9sVariantList list1, list2;
    S9S_VERIFY(S9sVariant(list1) == S9sVariant(list2));

    list1.push_back("item1");
    list1.push_back(S9sVariant(1234));
    S9S_VERIFY(! ( S9sVariant(list1) == S9sVariant(list2)));
    S9S_VERIFY(    S9sVariant(list1) != S9sVariant(list2));

    list2.push_back("item1");
    list2.push_back(S9sVariant(1234));
    S9S_VERIFY(    S9sVariant(list1) == S9sVariant(list2));
    S9S_VERIFY(! ( S9sVariant(list1) != S9sVariant(list2)));

    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sVariant)

