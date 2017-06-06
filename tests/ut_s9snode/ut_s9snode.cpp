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
#include "ut_s9snode.h"

#include "S9sNode"
#include "S9sVariantMap"
#include "S9sRpcClient"
#include "S9sOptions"

#define DEBUG
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
    PERFORM_TEST(testVariant02,       retval);
    PERFORM_TEST(testParse,           retval);

    return retval;
}

/**
 * Creating nodes from simple (non JSon) strings and checking if the properties
 * are set.
 */
bool
UtS9sNode::testCreate()
{
    S9sNode   node1("node1.eu:8080");
    S9sNode   node2("http://node2.eu:80");

    S9S_COMPARE(node1.protocol(), "");
    S9S_COMPARE(node1.hostName(), "node1.eu");
    S9S_COMPARE(node1.port(),      8080);

    S9S_COMPARE(node2.protocol(), "http");
    S9S_COMPARE(node2.hostName(), "node2.eu");
    S9S_COMPARE(node2.port(),      80);

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

    S9S_COMPARE(theNode.className(),  "CmonGaleraHost");
    S9S_COMPARE(theNode.id(),         3);
    S9S_COMPARE(theNode.clusterId(),  1);
    S9S_COMPARE(theNode.name(),       "192.168.1.189");
    S9S_COMPARE(theNode.hostName(),   "192.168.1.189");
    S9S_COMPARE(theNode.ipAddress(),  "192.168.1.189");
    S9S_COMPARE(theNode.alias(),      "");
    S9S_COMPARE(theNode.role(),       "none");
    S9S_COMPARE(theNode.isMaster(),   false);
    S9S_COMPARE(theNode.isSlave(),    false);
    S9S_COMPARE(theNode.configFile(), "/etc/mysql/my.cnf");
    S9S_COMPARE(theNode.logFile(),    "/var/log/mysql/mysqld.log");
    S9S_COMPARE(theNode.pidFile(),    "/var/lib/mysql/mysql.pid");
    S9S_COMPARE(theNode.dataDir(),    "/var/lib/mysql/");
    S9S_COMPARE(theNode.pid(),        8272);
    S9S_COMPARE(theNode.port(),       3306);
    S9S_COMPARE(theNode.hostStatus(), "CmonHostOnline");
    S9S_COMPARE(theNode.nodeType(),   "galera");
    S9S_COMPARE(theNode.version(),    "5.6.30-76.3-56");
    S9S_COMPARE(theNode.message(),    "Up and running.");
    S9S_COMPARE(theNode.osVersionString(), "ubuntu 16.04 xenial");
    S9S_COMPARE(theNode.isMaintenanceActive(), true);
    S9S_COMPARE(theNode.hasPort(),     true);
    S9S_COMPARE(theNode.hasError(),    false);
    S9S_COMPARE(theNode.readOnly(),    false);
    S9S_COMPARE(theNode.superReadOnly(), false);
    S9S_COMPARE(theNode.connected(),   false);
    S9S_COMPARE(theNode.managed(),     false);
    S9S_COMPARE(theNode.nodeAutoRecovery(), false);
    S9S_COMPARE(theNode.skipNameResolve(), false);

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
    
    S9S_COMPARE(theNode.className(),  "CmonGaleraHost");
    S9S_COMPARE(theNode.id(),         3);
    S9S_COMPARE(theNode.clusterId(),  1);
    S9S_COMPARE(theNode.name(),       "192.168.1.189");
    S9S_COMPARE(theNode.hostName(),   "192.168.1.189");
    S9S_COMPARE(theNode.ipAddress(),  "192.168.1.189");
    S9S_COMPARE(theNode.alias(),      "");
    S9S_COMPARE(theNode.role(),       "none");
    S9S_COMPARE(theNode.isMaster(),   false);
    S9S_COMPARE(theNode.isSlave(),    false);
    S9S_COMPARE(theNode.configFile(), "/etc/mysql/my.cnf");
    S9S_COMPARE(theNode.logFile(),    "/var/log/mysql/mysqld.log");
    S9S_COMPARE(theNode.pidFile(),    "/var/lib/mysql/mysql.pid");
    S9S_COMPARE(theNode.dataDir(),    "/var/lib/mysql/");
    S9S_COMPARE(theNode.pid(),        8272);
    S9S_COMPARE(theNode.port(),       3306);
    S9S_COMPARE(theNode.hostStatus(), "CmonHostOnline");
    S9S_COMPARE(theNode.nodeType(),   "galera");
    S9S_COMPARE(theNode.version(),    "5.6.30-76.3-56");
    S9S_COMPARE(theNode.message(),    "Up and running.");
    S9S_COMPARE(theNode.osVersionString(), "ubuntu 16.04 xenial");
    S9S_COMPARE(theNode.isMaintenanceActive(), true);
    S9S_COMPARE(theNode.hasPort(),     true);
    S9S_COMPARE(theNode.hasError(),    false);
    S9S_COMPARE(theNode.readOnly(),    false);
    S9S_COMPARE(theNode.superReadOnly(), false);
    S9S_COMPARE(theNode.connected(),   false);
    S9S_COMPARE(theNode.managed(),     false);
    S9S_COMPARE(theNode.nodeAutoRecovery(), false);
    S9S_COMPARE(theNode.skipNameResolve(), false);

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

/**
 * This test will check the S9sRpcClient::nodesField() and
 * S9sRpcClient::topologyField() functions.
 */
bool
UtS9sNode::testVariant02()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariant     theVariant;
    S9sVariant     topology;
    S9sString      nodesString;
    S9sNode        node;

    nodesString = 
        "masterhost1:3306?master;"
        "slavehost1:3306?slave;"
        "masterhost2:3306?master";

    options->setNodes(nodesString);
   
    /*
     * Checking the nodesField() method.
     */
    theVariant = S9sRpcClient::nodesField(options->nodes());
    //S9S_WARNING("nodes    -> \n%s\n", STR(theVariant.toString()));
    S9S_VERIFY(theVariant.isVariantList());
    S9S_COMPARE(theVariant[0].typeName(), "node");
    S9S_COMPARE(theVariant.toVariantList().size(), 3);

    node = theVariant[0].toNode();
    S9S_COMPARE(node.hostName(), "masterhost1");
    S9S_COMPARE(node.isMaster(),  true);
    S9S_COMPARE(node.port(),      3306);
    
    node = theVariant[1].toNode();
    S9S_COMPARE(node.hostName(), "slavehost1");
    S9S_COMPARE(node.isSlave(),   true);
    S9S_COMPARE(node.port(),      3306);
    
    node = theVariant[2].toNode();
    S9S_COMPARE(node.hostName(), "masterhost2");
    S9S_COMPARE(node.isMaster(),  true);
    S9S_COMPARE(node.port(),      3306);


    topology = S9sRpcClient::topologyField(options->nodes());
    //S9S_WARNING("topology -> \n%s\n", STR(topology.toString()));
    S9S_COMPARE(topology.typeName(), "map");
    S9S_COMPARE(topology["master_slave_links"].typeName(), "list");
    S9S_COMPARE(topology["master_slave_links"].size(), 3);
    S9S_COMPARE(topology["master_slave_links"][0].typeName(), "map");
    S9S_COMPARE(topology["master_slave_links"][1].typeName(), "map");
    S9S_COMPARE(topology["master_slave_links"][2].typeName(), "map");
    
    S9S_COMPARE(
            topology["master_slave_links"][0]["masterhost1"], "slavehost1");

    S9S_COMPARE(
            topology["master_slave_links"][1]["masterhost1"], "masterhost2");

    S9S_COMPARE(
            topology["master_slave_links"][2]["masterhost2"], "masterhost1");

    return true;
}

bool
UtS9sNode::testParse()
{
    S9sNode  node1("proxysql://10.10.10.23?db_username=bob&db_password=b0b");
    S9sNode  node2("psql://10.10.10.23?master&password=b0b&big=true");
    S9sNode  node3("10.10.10.23?master");

    S9S_COMPARE(node1.property("db_username"), "bob");
    S9S_COMPARE(node1.property("db_password"), "b0b");

    S9S_COMPARE(node2.property("master"),      true);
    S9S_COMPARE(node2.property("password"),    "b0b");
    S9S_COMPARE(node2.property("big"),         true);

    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sNode)
