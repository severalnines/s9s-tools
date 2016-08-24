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
#include "ut_s9svariantmap.h"

#include "S9sVariantMap"

//#define DEBUG
#include "s9sdebug.h"

UtS9sVariantMap::UtS9sVariantMap()
{
    S9S_DEBUG("");
}

UtS9sVariantMap::~UtS9sVariantMap()
{
}

bool
UtS9sVariantMap::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testCreate,        retval);
    PERFORM_TEST(testAssignMap,     retval);

    return retval;
}

/**
 * Testing the constructors.
 */
bool
UtS9sVariantMap::testCreate()
{
    S9sVariantMap theMap;

    S9S_VERIFY(theMap.empty());
    S9S_COMPARE(theMap.size(), 0);

    return true;
}

/**
 *
 */
bool
UtS9sVariantMap::testAssignMap()
{
    S9sVariant     variant;
    S9sVariant     other;
    S9sVariantMap  theMap;

    theMap["life"]       = 42;
    theMap["universe"]   = 42;
    theMap["everything"] = 42;

    variant = S9sVariant(theMap);
    S9S_COMPARE(variant.typeName(), "map");
    S9S_VERIFY(variant.isVariantMap());
    S9S_COMPARE(variant["life"], 42);
    S9S_COMPARE(variant["universe"], 42);
    S9S_COMPARE(variant["everything"], 42);

    other = variant;
    S9S_COMPARE(other.typeName(), "map");
    S9S_VERIFY(other.isVariantMap());
    S9S_COMPARE(other["life"], 42);
    S9S_COMPARE(other["universe"], 42);
    S9S_COMPARE(other["everything"], 42);

    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sVariantMap)


