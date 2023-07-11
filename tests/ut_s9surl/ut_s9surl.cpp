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
 * s9s-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with s9s-tools. If not, see <http://www.gnu.org/licenses/>.
 */
#include "ut_s9surl.h"

#include "s9surl.h"

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
    PERFORM_TEST(testCreate02,      retval);
    PERFORM_TEST(testCreate03,      retval);
    PERFORM_TEST(testParse01,       retval);
    PERFORM_TEST(testParse02,       retval);
    PERFORM_TEST(testParse03,       retval);
    PERFORM_TEST(testParse04,       retval);
    PERFORM_TEST(testParse05,       retval);
    PERFORM_TEST(testParse06,       retval);
    PERFORM_TEST(testParse07,       retval);
    PERFORM_TEST(testParse08,       retval);

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

/**
 * Testing the constructor that parse the string representation.
 */
bool
UtS9sUrl::testCreate02()
{
    S9sUrl url("hostname.eu:8000");

    S9S_COMPARE(url.hostName(), "hostname.eu");
    S9S_VERIFY(url.hasPort());
    S9S_COMPARE(url.port(), 8000);

    return true;
}

/**
 * Testing the constructor that parse the string representation.
 */
bool
UtS9sUrl::testCreate03()
{
    S9sUrl url("https://hostname.eu:8000");

    S9S_COMPARE(url.hostName(), "hostname.eu");
    S9S_VERIFY(url.hasPort());
    S9S_COMPARE(url.port(), 8000);
    S9S_VERIFY(url.hasProtocol());
    S9S_COMPARE(url.protocol(), "https");

    return true;
}

bool
UtS9sUrl::testParse01()
{
    S9sUrl url;

    S9S_VERIFY(url.parse("https://10.10.1.120"));
    S9S_COMPARE(url.protocol(), "https");
    S9S_COMPARE(url.hostName(), "10.10.1.120");
    S9S_COMPARE(url.hasPort(),  false);

    return true;
}

bool
UtS9sUrl::testParse02()
{
    S9sUrl url;

    S9S_VERIFY(url.parse("proxysql://10.10.10.23?db_username=bob"));
    S9S_COMPARE(url.protocol(), "proxysql");
    S9S_COMPARE(url.hostName(), "10.10.10.23");
    S9S_COMPARE(url.hasPort(),  false);
    S9S_COMPARE(url.property("db_username"),  "bob");

    return true;
}

bool
UtS9sUrl::testParse03()
{
    S9sUrl url;

    S9S_VERIFY(url.parse("proxysql://10.10.10.23?db_username=bob&db_password=b0b&db_database='*.*'&db_privs='SELECT,INSERT,UPDATE'"));
    S9S_COMPARE(url.protocol(), "proxysql");
    S9S_COMPARE(url.hostName(), "10.10.10.23");
    S9S_COMPARE(url.hasPort(),  false);
    S9S_COMPARE(url.property("db_username"),  "bob");
    S9S_COMPARE(url.property("db_password"),  "b0b");
    S9S_COMPARE(url.property("db_database"),  "*.*");
    S9S_COMPARE(url.property("db_privs"),     "SELECT,INSERT,UPDATE");

    return true;
}

bool
UtS9sUrl::testParse04()
{
    S9sUrl url;

    S9S_VERIFY(url.parse("https://10.10.1.120:8080"));
    S9S_COMPARE(url.protocol(), "https");
    S9S_COMPARE(url.hostName(), "10.10.1.120");
    S9S_COMPARE(url.hasPort(),  true);
    S9S_COMPARE(url.port(),     8080);

    return true;
}

bool
UtS9sUrl::testParse05()
{
    S9sUrl url;

    S9S_VERIFY(url.parse("10.10.1.120"));
    S9S_COMPARE(url.hostName(), "10.10.1.120");
    S9S_COMPARE(url.hasPort(),  false);

    return true;
}

bool
UtS9sUrl::testParse06()
{
    S9sUrl url;

    S9S_VERIFY(url.parse("10.10.1.120:80"));
    S9S_COMPARE(url.hostName(), "10.10.1.120");
    S9S_COMPARE(url.hasPort(),  true);
    S9S_COMPARE(url.port(),     80);

    return true;
}

bool
UtS9sUrl::testParse07()
{
    S9sUrl url;

    S9S_VERIFY(!url.parse("10.10.1.120:"));
    printf("\n%s", STR(url.fullErrorString()));
    S9S_COMPARE(url.errorString(), "Expected '/' or port number.");
    
    S9S_VERIFY(!url.parse("10.10.1.120:some"));
    printf("\n%s", STR(url.fullErrorString()));
    S9S_COMPARE(url.errorString(), "Expected '/' or port number.");
    
    S9S_VERIFY(!url.parse("http://10.10.1.120:80?db_privs="));
    printf("\n%s", STR(url.fullErrorString()));
    S9S_COMPARE(url.errorString(), "Expected property value for 'db_privs'.");
    
    S9S_VERIFY(!url.parse("10.10.1.120:3306extra"));
    printf("\n%s", STR(url.fullErrorString()));
    S9S_COMPARE(url.errorString(), "Expected port number.");
    
    S9S_VERIFY(!url.parse(""));
    printf("\n%s", STR(url.fullErrorString()));
    S9S_COMPARE(url.errorString(), "Expected protocol or host name.");

    return true;
}

bool
UtS9sUrl::testParse08()
{
    S9sUrl url;

    S9S_VERIFY(url.parse("10.10.10.23?master"));
    S9S_COMPARE(url.hostName(), "10.10.10.23");
    S9S_VERIFY(!url.hasPort());
    S9S_VERIFY(url.hasProperty("master"));
    
    S9S_VERIFY(url.parse("10.10.10.23?master=true"));
    S9S_COMPARE(url.hostName(), "10.10.10.23");
    S9S_VERIFY(!url.hasPort());
    S9S_VERIFY(url.hasProperty("master"));
    
    S9S_VERIFY(url.parse("10.10.10.23:8080?master"));
    S9S_COMPARE(url.hostName(), "10.10.10.23");
    S9S_VERIFY(url.hasPort());
    S9S_COMPARE(url.port(), 8080);
    S9S_VERIFY(url.hasProperty("master"));
    
    S9S_VERIFY(url.parse("10.10.10.23:8080?master=true"));
    S9S_COMPARE(url.hostName(), "10.10.10.23");
    S9S_VERIFY(url.hasPort());
    S9S_COMPARE(url.port(), 8080);
    S9S_VERIFY(url.hasProperty("master"));

    return true;
}
    

S9S_UNIT_TEST_MAIN(UtS9sUrl)
