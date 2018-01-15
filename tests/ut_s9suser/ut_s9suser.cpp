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
#include "ut_s9suser.h"

#include "S9sUser"
#include <cstdio>
#include <cstring>

#define DEBUG
#include "s9sdebug.h"

UtS9sUser::UtS9sUser()
{
    S9S_DEBUG("");
}

UtS9sUser::~UtS9sUser()
{
}

bool
UtS9sUser::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testConstruct,   retval);

    return retval;
}

/**
 * This test will check the various types of constructors the S9sUser class
 * have.
 */
bool
UtS9sUser::testConstruct()
{
    S9sVariantMap theMap;
    S9sUser       user;
    const S9sString jsonString = 
    "{\n"
    "    'class_name': 'CmonUser',\n"
    "    'email_address': 'warrior@ds9.com',\n"
    "    'first_name': 'Worf',\n"
    "    'groups': [ \n"
    "    {\n"
    "        'class_name': 'CmonGroup',\n"
    "        'group_id': 4,\n"
    "        'group_name': 'ds9'\n"
    "    } ],\n"
    "    'title': 'Lt.',\n"
    "    'user_id': 12,\n"
    "    'user_name': 'worf'\n"
    "}\n";

    // Creating a map and setting up the user accordingly.
    S9S_VERIFY(theMap.parse(STR(jsonString)));
    user = theMap;

    S9S_COMPARE(user.userName(),          "worf");
    S9S_COMPARE(user.emailAddress(),      "warrior@ds9.com");
    S9S_COMPARE(user.firstName(),         "Worf");
    S9S_COMPARE(user.lastName(),          "");
    S9S_COMPARE(user.middleName(),        "");
    S9S_COMPARE(user.title(),             "Lt.");
    S9S_COMPARE(user.jobTitle(),          "");
    S9S_COMPARE(user.groupNames(),        "ds9");
    S9S_COMPARE(user.fullName(),          "Lt. Worf");
    S9S_COMPARE(user.ownerName(),         "");
    S9S_COMPARE(user.groupOwnerName(),    "");
    S9S_COMPARE(user.lastLoginString(),   "");
    S9S_COMPARE(user.createdString(),     "");
    S9S_COMPARE(user.failedLoginString(), "");
    S9S_COMPARE(user.isDisabled(),        false);
    S9S_COMPARE(user.isSuspended(),       false);

    return true;
}


S9S_UNIT_TEST_MAIN(UtS9sUser)


