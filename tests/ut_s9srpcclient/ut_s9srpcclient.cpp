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
#include "ut_s9srpcclient.h"

#include "S9sNode"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

/******************************************************************************
 *
 */
bool 
S9sRpcClientTester::executeRequest(
        const S9sString &uri,
        S9sVariantMap &payload)
{
    //S9S_DEBUG("*** ");
    //S9S_DEBUG("*** uri     : %s", STR(uri));
    //S9S_DEBUG("*** payload : \n%s\n", STR(payload));

    m_urls     << uri;
    m_payloads << payload;

    return true;
}

S9sString 
S9sRpcClientTester::uri(
        const uint index) const
{
    if (index >= m_urls.size())
        return S9sString();

    return m_urls[index].toString();
}

S9sString 
S9sRpcClientTester::payload(
        const uint index) const
{
    if (index >= m_payloads.size())
        return S9sString();

    return m_payloads[index].toString();
}

/******************************************************************************
 *
 */
UtS9sRpcClient::UtS9sRpcClient()
{
}

UtS9sRpcClient::~UtS9sRpcClient()
{
}

bool
UtS9sRpcClient::runTest(
        const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testGetAllClusterInfo,   retval);
    PERFORM_TEST(testSetHost,             retval);
    PERFORM_TEST(testCreateGalera,        retval);
    PERFORM_TEST(testCreateReplication,   retval);
    PERFORM_TEST(testCreateNdbCluster,    retval);
    PERFORM_TEST(testAddNode,             retval);

    return retval;
}

/**
 * Testing the getAllClusterInfo() call.
 */
bool
UtS9sRpcClient::testGetAllClusterInfo()
{
    S9sRpcClientTester client;

    S9S_VERIFY(client.getClusters());
    S9S_COMPARE(client.uri(0u), "/0/clusters/");
    S9S_VERIFY(client.payload(0u).contains(
                "\"operation\": \"getAllClusterInfo\""));

    return true;
}

/**
 * Testing the setHost() call.
 */
bool
UtS9sRpcClient::testSetHost()
{
    S9sRpcClientTester client;
    S9sVariantList     hosts;
    S9sVariantMap      properties;
    int                clusterId = 1;
    S9sString          uri, payload;

    properties["name"] = "value";
    hosts << S9sNode("myserver.eu:80");

    S9S_VERIFY(client.setHost(clusterId, hosts, properties));
    uri     = client.uri(0u);
    payload = client.payload(0u);

    S9S_DEBUG("*** uri     : %s", STR(uri));
    S9S_DEBUG("*** payload : %s", STR(payload));

    S9S_COMPARE(uri, "/1/stat");
    S9S_VERIFY(payload.contains("\"operation\": \"setHost\""));
    S9S_VERIFY(payload.contains("\"hostname\": \"myserver.eu\""));
    S9S_VERIFY(payload.contains("\"port\": 80"));
    S9S_VERIFY(payload.contains("\"name\": \"value\""));

    return true;
}

/**
 * Testing the createGaleraCluster() call.
 */
bool
UtS9sRpcClient::testCreateGalera()
{
    S9sRpcClientTester client;
    S9sVariantList     hosts;
    S9sVariantMap      properties;
    S9sString          uri, payload;

    properties["name"] = "value";
    hosts << S9sNode("192.168.1.191");
    hosts << S9sNode("192.168.1.192");
    hosts << S9sNode("192.168.1.193");

    S9S_VERIFY(client.createGaleraCluster(hosts, "pi", "percona", "5.6", true));
    uri     = client.uri(0u);
    payload = client.payload(0u);

    S9S_DEBUG("*** uri     : %s", STR(uri));
    S9S_DEBUG("*** payload : %s", STR(payload));
    S9S_COMPARE(uri, "/0/job/");
    S9S_VERIFY(payload.contains("\"command\": \"create_cluster\""));
    S9S_VERIFY(payload.contains("\"cluster_type\": \"galera\""));
    S9S_VERIFY(payload.contains("\"ssh_user\": \"pi\""));
    S9S_VERIFY(payload.contains("\"vendor\": \"percona\""));
    S9S_VERIFY(payload.contains("\"mysql_version\": \"5.6\""));
    S9S_VERIFY(payload.contains(
                "\"mysql_hostnames\": "
                "[ \"192.168.1.191\", \"192.168.1.192\", \"192.168.1.193\" ]"));

    return true;
}

/**
 * Testing the createMySqlReplication() call.
 */
bool
UtS9sRpcClient::testCreateReplication()
{
    S9sRpcClientTester client;
    S9sVariantList     hosts;
    S9sString          uri, payload;

    hosts << S9sNode("192.168.1.191");
    hosts << S9sNode("192.168.1.192");
    hosts << S9sNode("192.168.1.193");

    S9S_VERIFY(client.createMySqlReplication(
                hosts, "pi", "percona", "5.6", true));

    uri     = client.uri(0u);
    payload = client.payload(0u);

    S9S_DEBUG("*** uri     : %s", STR(uri));
    S9S_DEBUG("*** payload : %s", STR(payload));
    S9S_COMPARE(uri, "/0/job/");
    S9S_VERIFY(payload.contains("\"command\": \"create_cluster\""));
    S9S_VERIFY(payload.contains("\"cluster_type\": \"replication\""));
    S9S_VERIFY(payload.contains("\"ssh_user\": \"pi\""));
    S9S_VERIFY(payload.contains("\"vendor\": \"percona\""));
    S9S_VERIFY(payload.contains("\"mysql_version\": \"5.6\""));
    S9S_VERIFY(payload.contains(
                "\"mysql_hostnames\": "
                "[ \"192.168.1.191\", \"192.168.1.192\", \"192.168.1.193\" ]"));

    return true;
}

/**
 * Testing the createNdbCluster() method.
 */
bool
UtS9sRpcClient::testCreateNdbCluster()
{
    S9sRpcClientTester client;
    S9sVariantList     mySqlHosts, mgmdHosts, ndbdHosts;
    S9sString          uri, payload;

    mySqlHosts << 
        S9sNode("192.168.1.100") << 
        S9sNode("192.168.1.101") <<
        S9sNode("192.168.1.102");
    
    mgmdHosts << 
        S9sNode("192.168.1.110") << 
        S9sNode("192.168.1.111") <<
        S9sNode("192.168.1.112");
    
    ndbdHosts << 
        S9sNode("192.168.1.120") << 
        S9sNode("192.168.1.121") <<
        S9sNode("192.168.1.122");

    S9S_VERIFY(client.createNdbCluster(
                mySqlHosts, mgmdHosts, ndbdHosts,
                "pip", "oracle", "5.6", true));

    uri     = client.uri(0u);
    payload = client.payload(0u);

    //S9S_DEBUG("*** uri     : %s", STR(uri));
    //S9S_DEBUG("*** payload : %s", STR(payload));
    S9S_COMPARE(uri, "/0/job/");
    S9S_VERIFY(payload.contains("\"command\": \"create_cluster\""));
    S9S_VERIFY(payload.contains("\"cluster_type\": \"mysqlcluster\""));
    S9S_VERIFY(payload.contains("\"type\": \"mysql\""));
    S9S_VERIFY(payload.contains("\"ssh_user\": \"pip\""));
    S9S_VERIFY(payload.contains("\"vendor\": \"oracle\""));
    S9S_VERIFY(payload.contains("\"mysql_version\": \"5.6\""));

    S9S_VERIFY(payload.contains(
                "\"mgmd_hostnames\": [ \"192.168.1.110\", \"192.168.1.111\","));

    S9S_VERIFY(payload.contains(
                "\"mysql_hostnames\": [ \"192.168.1.100\", \"192.168.1.101\""));

    S9S_VERIFY(payload.contains(
                "\"ndbd_hostnames\": [ \"192.168.1.120\", \"192.168.1.121\""));

    return true;
}

/**
 * This function tests the addNode() method of the RPC client.
 */
bool
UtS9sRpcClient::testAddNode()
{
    S9sRpcClientTester client;
    S9sVariantList     hosts;
    S9sString          uri, payload;
    int                clusterId = 1;

    hosts << S9sNode("192.168.1.191");
    S9S_VERIFY(client.addNode(clusterId, hosts));

    uri     = client.uri(0u);
    payload = client.payload(0u);

    S9S_DEBUG("*** uri     : %s", STR(uri));
    S9S_DEBUG("*** payload : %s", STR(payload));
    
    S9S_COMPARE(uri, "/1/job/");
    S9S_VERIFY(payload.contains("\"command\": \"addnode\""));
    S9S_VERIFY(payload.contains("\"disable_firewall\": true"));
    S9S_VERIFY(payload.contains("\"disable_selinux\": true"));
    S9S_VERIFY(payload.contains("\"hostname\": \"192.168.1.191\""));
    S9S_VERIFY(payload.contains("\"install_software\": true"));
    //S9S_VERIFY(payload.contains("\"user_name\": \"pipas\""));

    return true;
}


S9S_UNIT_TEST_MAIN(UtS9sRpcClient)
