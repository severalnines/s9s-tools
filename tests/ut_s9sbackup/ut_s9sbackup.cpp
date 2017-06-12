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
#include "ut_s9sbackup.h"

#include "S9sBackup"
#include "S9sVariantMap"
#include "S9sRpcClient"
#include "S9sOptions"

#define DEBUG
#define WARNING
#include "s9sdebug.h"

static const char *backupJson1 = 
"{\n"
"    \"class_name\": \"CmonGaleraHost\",\n"
"    \"clusterid\": 1,\n"
"    \"configfile\": [ \"/etc/mysql/my.cnf\" ],\n"
"    \"datadir\": \"/var/lib/mysql/\",\n"
"    \"distribution\":\n" 
"    {\n"
"        \"codename\": \"xenial\",\n"
"        \"name\": \"ubuntu\",\n"
"        \"release\": \"16.04\",\n"
"        \"type\": \"debian\"\n"
"    },\n"
"    \"errormsg\": \"Up and running.\",\n"
"    \"galera\": \n"
"    {\n"
"        \"certsdepsdistance\": 0,\n"
"        \"clustersize\": 2,\n"
"        \"flowctrlpaused\": 0,\n"
"        \"flowctrlrecv\": 0,\n"
"        \"flowctrlsent\": 0,\n"
"        \"galerastatus\": \"Primary\",\n"
"        \"lastcommitted\": 7,\n"
"        \"localrecvqueueavg\": 0,\n"
"        \"localsendqueueavg\": 0.333333,\n"
"        \"localstatus\": 4,\n"
"        \"localstatusstr\": \"Synced\",\n"
"        \"ready\": \"ON\"\n"
"    },\n"
"    \"hostId\": 3,\n"
"    \"hostname\": \"192.168.1.189\",\n"
"    \"hoststatus\": \"CmonHostOnline\",\n"
"    \"ip\": \"192.168.1.189\",\n"
"    \"isgalera\": true,\n"
"    \"lastseen\": 1473936903,\n"
"    \"logfile\": \"/var/log/mysql/mysqld.log\",\n"
"    \"maintenance_mode_active\": true,\n"
"    \"message\": \"Up and running.\",\n"
"    \"mysqlstatus\": 0,\n"
"    \"nodeid\": 3,\n"
"    \"nodetype\": \"galera\",\n"
"    \"pid\": 8272,\n"
"    \"pidfile\": \"/var/lib/mysql/mysql.pid\",\n"
"    \"pingstatus\": 0,\n"
"    \"port\": 3306,\n"
"    \"replication_master\": \n"
"    {\n"
"        \"position\": \"\"\n"
"    },\n"
"    \"role\": \"none\",\n"
"    \"serverid\": 2,\n"
"    \"timestamp\": 1473936903,\n"
"    \"uptime\": 55,\n"
"    \"version\": \"5.6.30-76.3-56\",\n"
"    \"wallclock\": 1473936985,\n"
"    \"wallclocktimestamp\": 1473936847\n"
"}\n"
;

UtS9sBackup::UtS9sBackup()
{
}

UtS9sBackup::~UtS9sBackup()
{
}

bool
UtS9sBackup::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testCreate,          retval);
    PERFORM_TEST(testSetProperties,   retval);
    PERFORM_TEST(testAssign,          retval);

    return retval;
}

/**
 * Creating nodes from simple (non JSon) strings and checking if the properties
 * are set.
 */
bool
UtS9sBackup::testCreate()
{

    return true;
}

/**
 * Testing the S9sBackup::setProperties() method. 
 */
bool
UtS9sBackup::testSetProperties()
{
    S9sVariantMap theMap;
    S9sBackup     theBackup;

    S9S_VERIFY(theMap.parse(backupJson1));
    theBackup.setProperties(theMap);

    #if 0
    S9S_COMPARE(theNode.className(),  "CmonGaleraHost");
    #endif

    return true;
}

/**
 * Testing the S9sBackup::operator=(const S9sVariantMap &) operator.
 */
bool
UtS9sBackup::testAssign()
{
    S9sVariantMap theMap;
    S9sBackup     theBackup;

    S9S_VERIFY(theMap.parse(backupJson1));
    theBackup = theMap;

    #if 0
    S9S_COMPARE(theNode.className(),  "CmonGaleraHost");
    #endif

    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sBackup)

