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

#include "S9sVariantList"
#include "S9sFormat"
#include "S9sOptions"

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

    PERFORM_TEST(testCreate,        retval);
    PERFORM_TEST(testAssign,        retval);
    PERFORM_TEST(testUnQuote,       retval);
    PERFORM_TEST(testToUpper,       retval);
    PERFORM_TEST(testToInt,         retval);
    PERFORM_TEST(testReplace,       retval);
    PERFORM_TEST(testConcat,        retval);
    PERFORM_TEST(testStartsWith,    retval);
    PERFORM_TEST(testPrintf,        retval);
    PERFORM_TEST(testContains,      retval);
    PERFORM_TEST(testEscape,        retval);
    PERFORM_TEST(testSplit,         retval);
    PERFORM_TEST(testSizeString,    retval);
    PERFORM_TEST(testMilliseconds,  retval);

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

    theString = "25.3";
    S9S_VERIFY(!theString.looksInteger());
    S9S_COMPARE(theString.toDouble(), 25.3);

    S9S_DEBUG("-> '%s'", STR(S9sString::decimalSeparator()));
    return true;
}

/**
 * Testing the replace() method by replacing strings to strings (no regexp this
 * time).
 */
bool
UtS9sString::testReplace()
{
    S9sString string1;
    
    string1 = "one $ONE $TWO one $ONE";
    string1.replace("$ONE", "replace");
    S9S_COMPARE(string1, "one replace $TWO one replace");

    string1 = "one :one :one one";
    string1.replace("someone", "somebody");
    S9S_COMPARE(string1, "one :one :one one");

    string1 = "one :one :one one";
    string1.replace(":one", "two");
    S9S_COMPARE(string1, "one two two one");

    string1 = "one\r\n";
    string1.replace("\n", "");
    string1.replace("\r", "");
    S9S_COMPARE(string1, "one");

    string1 = "one\r\n";
    string1.replace("\r\n", "\n");
    S9S_COMPARE(string1, "one\n");
    return true;
}

/**
 * Testing the '+' operator on strings.
 */
bool
UtS9sString::testConcat()
{
    S9sString   string1 = "string1";
    S9sString   string2 = "string2";
    S9sString   string3;

    string3 = string1 + string2;
    S9S_COMPARE(string3, "string1string2");

    return true;
}

bool
UtS9sString::testStartsWith()
{
    S9sString   string1 = "string1";
    S9sString   string2 = "STRING2";
    S9sString   baseDir;


    S9S_COMPARE(string1.startsWith("s"), true);
    S9S_COMPARE(string1.startsWith("st"), true);
    S9S_COMPARE(string1.startsWith("stri"), true);
    S9S_COMPARE(string1.startsWith("string1"), true);
    S9S_COMPARE(string1.startsWith("S"), false);
    S9S_COMPARE(string1.startsWith("ST"), false);
    S9S_COMPARE(string1.startsWith("STR"), false);

    S9S_COMPARE(string2.startsWith("STR"), true);
    S9S_COMPARE(string2.startsWith("STRING"), true);

    baseDir = "/usr/local/mysql/something";
    S9S_COMPARE(baseDir.startsWith("/usr/local/mysql"), true);
    return true;
}

/**
 * Testing the sprintf() variadic function.
 */
bool
UtS9sString::testPrintf()
{
    int         integer (42);
    const char *theChar = "the char";
    S9sString  string1;

    string1.sprintf("*** integer %d", integer);
    S9S_COMPARE(string1, "*** integer 42");

    string1.sprintf("*** char %s", theChar);
    S9S_COMPARE(string1, "*** char the char");

    string1.sprintf("*** both %s %d", theChar, integer);
    S9S_COMPARE(string1, "*** both the char 42");

    return true;
}

/**
 * Testing the contains() function.
 */
bool
UtS9sString::testContains()
{
    S9sString string1("The string");

    S9S_COMPARE(string1.contains('s'), true);
    S9S_COMPARE(string1.contains('g'), true);
    S9S_COMPARE(string1.contains('T'), true);
    S9S_COMPARE(string1.contains(' '), true);
    S9S_COMPARE(string1.contains('I'), false);
    S9S_COMPARE(string1.contains('\\'), false);
    S9S_COMPARE(string1.contains('\''), false);
    S9S_COMPARE(string1.contains('\"'), false);

    S9S_COMPARE(string1.contains("string"), true);
    S9S_COMPARE(string1.contains("The"), true);
    S9S_COMPARE(string1.contains("nope"), false);

    return true;
}

/**
 * Testing the escape() function.
 */
bool
UtS9sString::testEscape()
{
    S9sString string1("The string");
    S9sString string2("It's ok");
    S9sString string3("'s ok");
    S9sString string4("ok'");
    S9sString string5("one\"two");
    S9sString string6("\"two");
    S9sString string7("one\"");
    S9sString string8("it\\'s");
    S9sString string9("one\\\"two");
    S9sString string10("it\\''s");

    S9S_COMPARE(string1.escape(), "The string");
    S9S_COMPARE(string2.escape(), "It\\'s ok");
    S9S_COMPARE(string3.escape(), "\\'s ok");
    S9S_COMPARE(string4.escape(), "ok\\'");
    S9S_COMPARE(string5.escape(), "one\\\"two");
    S9S_COMPARE(string6.escape(), "\\\"two");
    S9S_COMPARE(string7.escape(), "one\\\"");
    S9S_COMPARE(string8.escape(), "it\\'s");
    S9S_COMPARE(string9.escape(), "one\\\"two");
    S9S_COMPARE(string10.escape(), "it\\'\\'s");

    return true;
}

/**
 * This function will test the S9sString::split() function with various inputs.
 */
bool
UtS9sString::testSplit()
{
    S9sString      string;
    S9sVariantList list;

    //
    // Regular format...
    //
    string = "one;two;three";
    list = string.split();
    S9S_COMPARE(list.size(), 3);
    S9S_COMPARE(list[0], "one");
    S9S_COMPARE(list[1], "two");
    S9S_COMPARE(list[2], "three");

    //
    // Extra delimiters...
    //
    string = "one;two;three;";
    list = string.split();
    S9S_COMPARE(list.size(), 3);
    S9S_COMPARE(list[0], "one");
    S9S_COMPARE(list[1], "two");
    S9S_COMPARE(list[2], "three");

    string = ";one;two;three";
    list = string.split();
    S9S_COMPARE(list.size(), 3);
    S9S_COMPARE(list[0], "one");
    S9S_COMPARE(list[1], "two");
    S9S_COMPARE(list[2], "three");

    string = ";;one;;two;;;;three;";
    list = string.split();
    S9S_COMPARE(list.size(), 3);
    S9S_COMPARE(list[0], "one");
    S9S_COMPARE(list[1], "two");
    S9S_COMPARE(list[2], "three");

    //
    // Extra delimiters...
    //
    string = "one;two;three;";
    list = string.split();
    S9S_COMPARE(list.size(), 3);
    S9S_COMPARE(list[0], "one");
    S9S_COMPARE(list[1], "two");
    S9S_COMPARE(list[2], "three");

    string = ";one;two;three";
    list = string.split();
    S9S_COMPARE(list.size(), 3);
    S9S_COMPARE(list[0], "one");
    S9S_COMPARE(list[1], "two");
    S9S_COMPARE(list[2], "three");

    string = ";;one;;two;;;;three;";
    list = string.split();
    S9S_COMPARE(list.size(), 3);
    S9S_COMPARE(list[0], "one");
    S9S_COMPARE(list[1], "two");
    S9S_COMPARE(list[2], "three");

    string = ";;one,two;;;;three;";
    list = string.split(";,");
    S9S_COMPARE(list.size(), 3);
    S9S_COMPARE(list[0], "one");
    S9S_COMPARE(list[1], "two");
    S9S_COMPARE(list[2], "three");

    return true;
}

bool
UtS9sString::testSizeString()
{
    S9sOptions *options = S9sOptions::instance();

    options->setHumanReadable();

    S9S_COMPARE(S9sFormat::toSizeString(0ull),        "0");
    S9S_COMPARE(S9sFormat::toSizeString(949ull),      "949");
    S9S_COMPARE(S9sFormat::toSizeString(1024ull),     "1.0K");
    S9S_COMPARE(S9sFormat::toSizeString(10240ull),    "10K");
    S9S_COMPARE(S9sFormat::toSizeString(102400ull),   "100K");
    S9S_COMPARE(S9sFormat::toSizeString(1024 * 1024), "1.0M");
    S9S_COMPARE(S9sFormat::toSizeString(1024 * 1024 * 1024), "1.0G");
    S9S_COMPARE(S9sFormat::toSizeString(1024ull * 1024 * 1024 * 1024), "1.0T");

    return true;
}

bool
UtS9sString::testMilliseconds()
{
    S9sFormat format;

    format.setUnit(S9sFormat::UnitMs);
    format.setHumanReadable(false);

    S9S_COMPARE(format.toString(12.45), "12");
    S9S_COMPARE(format.toString(12.00), "12");
    S9S_COMPARE(format.toString(2756220.189), "2756220");
    
    format.setHumanReadable(true);
    S9S_COMPARE(format.toString(12.45),       "12.45ms");
    S9S_COMPARE(format.toString(1526.34),     "1.53s");
    S9S_COMPARE(format.toString(2756220.189), "2756s");

    S9S_COMPARE(format.toString(0.45),        "450us");
    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sString)

