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
#include "ut_s9saccount.h"

#include "s9saccount.h"
#include "s9svariantmap.h"

#define DEBUG
#define WARNING
#include "s9sdebug.h"

UtS9sAccount::UtS9sAccount()
{
}

UtS9sAccount::~UtS9sAccount()
{
}

bool
UtS9sAccount::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testParse01,         retval);
    PERFORM_TEST(testParse02,         retval);
    PERFORM_TEST(testParse03,         retval);
    PERFORM_TEST(testParse04,         retval);
    PERFORM_TEST(testParse05,         retval);
    PERFORM_TEST(testCreate,          retval);
    PERFORM_TEST(testMap,             retval);

    return retval;
}

/**
 */
bool
UtS9sAccount::testParse01()
{
    S9sAccount account;
    bool       success;

    success = account.parseStringRep("pipas");
    S9S_VERIFY(success);
    S9S_COMPARE(account.userName(), "pipas");
    
    success = account.parseStringRep("'pipas'");
    S9S_VERIFY(success);
    S9S_COMPARE(account.userName(), "pipas");

    return true;
}

/**
 */
bool
UtS9sAccount::testParse02()
{
    S9sAccount account;
    bool       success;

    success = account.parseStringRep("'pipas'");
    S9S_VERIFY(success);
    S9S_COMPARE(account.userName(), "pipas");

    return true;
}

bool
UtS9sAccount::testParse03()
{
    S9sAccount account;
    bool       success;

    success = account.parseStringRep("pipas@1.2.3.4");
    S9S_VERIFY(success);
    S9S_COMPARE(account.userName(),  "pipas");
    S9S_COMPARE(account.hostAllow(), "1.2.3.4");

    return true;
}

bool
UtS9sAccount::testParse04()
{
    S9sAccount account;
    bool       success;

    success = account.parseStringRep("'pipas'@'1.2.3.4'");
    S9S_VERIFY(success);
    S9S_COMPARE(account.userName(),  "pipas");
    S9S_COMPARE(account.hostAllow(), "1.2.3.4");
    
    success = account.parseStringRep("'pipas@'1.2.3.4'");
    S9S_VERIFY(!success);
    S9S_VERIFY(account.hasError());
    
    success = account.parseStringRep("pipas@'1.2.3.4");
    S9S_VERIFY(!success);
    S9S_VERIFY(account.hasError());

    return true;
}

bool
UtS9sAccount::testParse05()
{
    S9sAccount account;
    bool       success;

    success = account.parseStringRep("pipas:pwd@1.2.3.4");
    S9S_VERIFY(success);
    S9S_COMPARE(account.userName(),  "pipas");
    S9S_COMPARE(account.password(),  "pwd");
    S9S_COMPARE(account.hostAllow(), "1.2.3.4");

    success = account.parseStringRep("pipas:pwd%40next@1.2.3.4");
    S9S_VERIFY(success);
    S9S_COMPARE(account.userName(),  "pipas");
    S9S_COMPARE(account.password(),  "pwd@next");
    S9S_COMPARE(account.hostAllow(), "1.2.3.4");
    
    success = account.parseStringRep("pipas%24:pwd@1.2.3.4");
    S9S_VERIFY(success);
    S9S_COMPARE(account.userName(),  "pipas$");
    S9S_COMPARE(account.password(),  "pwd");
    S9S_COMPARE(account.hostAllow(), "1.2.3.4");
    
    success = account.parseStringRep("laszlo%20pere:pwd@1.2.3.4");
    S9S_VERIFY(success);
    S9S_COMPARE(account.userName(),  "laszlo pere");
    S9S_COMPARE(account.password(),  "pwd");
    S9S_COMPARE(account.hostAllow(), "1.2.3.4");

    success = account.parseStringRep("'pipas':pwd@1.2.3.4");
    S9S_VERIFY(success);
    S9S_COMPARE(account.userName(),  "pipas");
    S9S_COMPARE(account.password(),  "pwd");
    S9S_COMPARE(account.hostAllow(), "1.2.3.4");

    success = account.parseStringRep("'pipas':'pw@d'@1.2.3.4");
    S9S_VERIFY(success);
    S9S_COMPARE(account.userName(),  "pipas");
    S9S_COMPARE(account.password(),  "pw@d");
    S9S_COMPARE(account.hostAllow(), "1.2.3.4");

    return true;
}

bool
UtS9sAccount::testCreate()
{
    S9sAccount account("pipas:pwd@1.2.3.4");

    S9S_COMPARE(account.userName(),  "pipas");
    S9S_COMPARE(account.password(),  "pwd");
    S9S_COMPARE(account.hostAllow(), "1.2.3.4");

    return true;
}

bool
UtS9sAccount::testMap()
{
    S9sAccount    account("pipas:pwd@1.2.3.4");
    S9sVariantMap map;
    S9sString     string;

    map["account"] = account;
    string = map.toString();

    //S9S_DEBUG("-> \n%s\n", STR(string));
    S9S_VERIFY(string.contains("\"class_name\": \"CmonAccount\""));
    S9S_VERIFY(string.contains("\"host_allow\": \"1.2.3.4\""));
    S9S_VERIFY(string.contains("\"password\": \"pwd\""));
    S9S_VERIFY(string.contains("\"user_name\": \"pipas\""));

    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sAccount)

