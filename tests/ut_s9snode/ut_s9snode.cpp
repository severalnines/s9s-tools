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
#include "ut_s9snode.h"

#include "S9sNode"
#include "S9sVariantMap"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

static const char *hostJson1 = 
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

static const char *hostJson2 = 
"{\n"
"    \"hostname\": \"192.168.1.189\",\n"
"    \"port\": 3306\n"
"}"
;

UtS9sNode::UtS9sNode()
{
}

UtS9sNode::~UtS9sNode()
{
}

bool
UtS9sNode::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testCreate,          retval);
    PERFORM_TEST(testSetProperties,   retval);
    PERFORM_TEST(testAssign,          retval);
    PERFORM_TEST(testVariant01,       retval);

    return retval;
}

bool
UtS9sNode::testCreate()
{
    S9sNode   node1("node1.eu:8080");

    return true;
}

/**
 * Testing the S9sNode::setProperties() method. 
 */
bool
UtS9sNode::testSetProperties()
{
    S9sVariantMap theMap;
    S9sNode       theNode;

    S9S_VERIFY(theMap.parse(hostJson1));
    theNode.setProperties(theMap);

    S9S_COMPARE(theNode.hostName(),   "192.168.1.189");
    S9S_COMPARE(theNode.port(),        3306);
    S9S_COMPARE(theNode.hostStatus(), "CmonHostOnline");
    S9S_COMPARE(theNode.className(),  "CmonGaleraHost");
    S9S_COMPARE(theNode.nodeType(),   "galera");
    S9S_COMPARE(theNode.version(),    "5.6.30-76.3-56");
    S9S_COMPARE(theNode.message(),    "Up and running.");
    S9S_COMPARE(theNode.isMaintenanceActive(), true);

    return true;
}

/**
 * Testing the S9sNode::operator=(const S9sVariantMap &) operator.
 */
bool
UtS9sNode::testAssign()
{
    S9sVariantMap theMap;
    S9sNode       theNode;

    S9S_VERIFY(theMap.parse(hostJson1));
    theNode = theMap;

    S9S_COMPARE(theNode.hostName(),   "192.168.1.189");
    S9S_COMPARE(theNode.port(),        3306);
    S9S_COMPARE(theNode.hostStatus(), "CmonHostOnline");
    S9S_COMPARE(theNode.className(),  "CmonGaleraHost");
    S9S_COMPARE(theNode.nodeType(),   "galera");
    S9S_COMPARE(theNode.version(),    "5.6.30-76.3-56");
    S9S_COMPARE(theNode.message(),    "Up and running.");
    S9S_COMPARE(theNode.isMaintenanceActive(), true);

    return true;
}

/**
 * Here we put the node into a variant map, then we convert the variant map to a
 * JSon string to see that it is fully integrated into the map.
 */
bool
UtS9sNode::testVariant01()
{
    S9sVariantMap theMap;
    S9sNode       theNode(hostJson2);
    S9sString     jsonString;

    theMap["node"] = theNode;
    jsonString     = theMap.toString();

    //S9S_WARNING("-> \n%s\n", STR(jsonString));
    S9S_VERIFY(jsonString.contains("\"node\":"));
    S9S_VERIFY(jsonString.contains("\"hostname\": \"192.168.1.189\""));
    S9S_VERIFY(jsonString.contains("\"port\": 3306"));

    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sNode)
