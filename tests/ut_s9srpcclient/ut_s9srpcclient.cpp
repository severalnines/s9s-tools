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
#include "ut_s9srpcclient.h"

#include "S9sNode"
#include "S9sOptions"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

#define JOB_DATA "/job/job_spec/job_data/"

/******************************************************************************
 *
 */
bool 
S9sRpcClientTester::doExecuteRequest(
        const S9sString &uri,
        S9sVariantMap &payload)
{
    S9S_DEBUG("*** ");
    S9S_DEBUG("*** uri     : %s", STR(uri));
    S9S_DEBUG("*** payload : \n%s\n", STR(payload.toString()));

    m_urls     << uri;
    m_payloads << payload.toString();
    m_lastPayload = payload;

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
        return S9sString("INDEX OUT OF RANGE");

    return m_payloads[index].toString();
}


S9sVariantMap &
S9sRpcClientTester::lastPayload() 
{
    return m_lastPayload;
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

    PERFORM_TEST(testCreate,              retval);
    PERFORM_TEST(testComposeRequest,      retval);
    PERFORM_TEST(testComposeJob,          retval);
    PERFORM_TEST(testGetNextMaintenance,  retval);
    PERFORM_TEST(testGetSqlProcesses,     retval);
    PERFORM_TEST(testGetTopQueries,       retval);
    PERFORM_TEST(testGetDatabases,        retval);
    PERFORM_TEST(testGetTree,             retval);
    PERFORM_TEST(testGetClusterConfig,    retval);
    PERFORM_TEST(testPingController,      retval);
    PERFORM_TEST(testGetCpuInfo,          retval);
    PERFORM_TEST(testGetCpuStats,         retval);
    PERFORM_TEST(testGetSqlStats,         retval);
    PERFORM_TEST(testGetMemStats,         retval);
    PERFORM_TEST(testGetMemoryStats,      retval);
    PERFORM_TEST(testGetRunningProcesses, retval);
    PERFORM_TEST(testGetJobInstances,     retval);
    PERFORM_TEST(testKillJobInstance,     retval);
    PERFORM_TEST(testCloneJobInstance,    retval);
    PERFORM_TEST(testGetDbGrowth,         retval);

    PERFORM_TEST(testCreateContainer,     retval);
    PERFORM_TEST(testDeleteContainer,     retval);
    PERFORM_TEST(testStartContainer,      retval);
    PERFORM_TEST(testStopContainer,       retval);

    PERFORM_TEST(testCreateCluster01,     retval);
    PERFORM_TEST(testCreateCluster02,     retval);
    PERFORM_TEST(testCreateCluster03,     retval);
    PERFORM_TEST(testCreateCluster04,     retval);
    PERFORM_TEST(testCreateCluster05,     retval);
    PERFORM_TEST(testCreateCluster06,     retval);


    PERFORM_TEST(testGetAllClusterInfo,   retval);
    PERFORM_TEST(testGetCluster,          retval);
    PERFORM_TEST(testPing,                retval);
    PERFORM_TEST(testGetMateTypes,        retval);
    PERFORM_TEST(testGetMetaTypeProps,    retval);
    PERFORM_TEST(testGetJobInstance,      retval);
    PERFORM_TEST(testDeleteJobInstance,   retval);
    PERFORM_TEST(testGetJobLog,           retval);
    PERFORM_TEST(testGetAlarm,            retval);
    PERFORM_TEST(testGetAlarmStatistics,  retval);
    PERFORM_TEST(testCreateFailJob,       retval);
    PERFORM_TEST(testCreateSuccessJob,    retval);
    PERFORM_TEST(testRollingRestart,      retval);
    PERFORM_TEST(testRegisterServers,     retval);
    PERFORM_TEST(testUnregisterServers,   retval);
    PERFORM_TEST(testCreateServer,        retval);
    PERFORM_TEST(testSetHost,             retval);
    PERFORM_TEST(testCreateGalera,        retval);
    PERFORM_TEST(testCreateReplication,   retval);
    PERFORM_TEST(testCreateNdbCluster,    retval);
    PERFORM_TEST(testAddNode,             retval);
    PERFORM_TEST(testCreateHaproxy,       retval);
    PERFORM_TEST(testComposeBackupJob,    retval);
    PERFORM_TEST(testBackup,              retval);
    PERFORM_TEST(testBackupSchedule,      retval);

    return retval;
}

/**
 * \returns true if everything went well.
 *
 * This method is called before each test method.
 */
bool
UtS9sRpcClient::prepareToRunTestCase()
{
    bool retval;
    
    retval = S9sUnitTest::prepareToRunTestCase();
    S9sOptions::uninit();

    return retval;
}

/**
 * \returns true if everything went well.
 *
 * This method is called after each test method.
 */
bool
UtS9sRpcClient::finalizeRunTestCase()
{
    bool retval;
    
    retval = S9sUnitTest::finalizeRunTestCase();
    return retval;
}

bool
UtS9sRpcClient::testCreate()
{
    S9sRpcClient client1("myhost.com", 4242, "/path", true);
    S9sRpcClient client2(client1);
    S9sRpcClient client3;

    client3 = client2;

    S9S_COMPARE(client1.hostName(), "myhost.com");
    S9S_COMPARE(client2.hostName(), client1.hostName());
    S9S_COMPARE(client3.hostName(), client1.hostName());
    
    S9S_COMPARE(client1.useTls(), true);
    S9S_COMPARE(client2.useTls(), client1.useTls());
    S9S_COMPARE(client3.useTls(), client1.useTls());

    S9S_COMPARE(client1.port(), 4242);
    S9S_COMPARE(client2.port(), client1.port());
    S9S_COMPARE(client3.port(), client1.port());
    return true;
}

/**
 * Testing the composeRequest() method.
 */
bool
UtS9sRpcClient::testComposeRequest()
{
    S9sOptions          *options     = S9sOptions::instance();
    S9sRpcClientTester   client;
    S9sVariantMap        request;

    options->m_options["cluster_id"] = 42;

    request = client.composeRequest();
    if (isVerbose())
        printDebug(request);

    S9S_COMPARE(request["class_name"], "CmonRpcRequest");
    S9S_COMPARE(request["cluster_id"], 42);
    return true;
}

bool
UtS9sRpcClient::testComposeJob()
{
    S9sRpcClientTester  client;
    S9sVariantMap       job;

    job = client.composeJob();
    if (isVerbose())
        printDebug(job);

    return true;
}

/**
 * Testing the getNextMaintenance() method.
 */
bool
UtS9sRpcClient::testGetNextMaintenance()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_id"] = 42;
    S9S_VERIFY(client.getNextMaintenance());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload.size(), 4);
    S9S_COMPARE(payload["cluster_id"].toInt(), 42);
    S9S_COMPARE(payload["operation"].toString(), "getNextMaintenance");

    return true;
}

bool
UtS9sRpcClient::testGetSqlProcesses()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_id"] = 42;
    options->m_options["limit"]      = 43;
    S9S_VERIFY(client.getSqlProcesses());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload.size(), 6);
    S9S_COMPARE(payload["limit"].toInt(), 43);
    S9S_COMPARE(payload["cluster_id"].toInt(), 42);
    S9S_COMPARE(payload["operation"].toString(), "getSqlProcesses");

    return true;
}

bool
UtS9sRpcClient::testGetTopQueries()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_id"] = 42;
    options->m_options["limit"]      = 43;
    options->m_options["offset"]     = 44;
    S9S_VERIFY(client.getTopQueries());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    //S9S_COMPARE(payload.size(), 6);
    S9S_COMPARE(payload["limit"].toInt(), 43);
    S9S_COMPARE(payload["offset"].toInt(), 44);
    S9S_COMPARE(payload["cluster_id"].toInt(), 42);
    S9S_COMPARE(payload["operation"].toString(), "getTopQueries");

    return true;
}

bool
UtS9sRpcClient::testGetDatabases()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_id"] = 42;
    S9S_VERIFY(client.getDatabases());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    //S9S_COMPARE(payload.size(), 6);
    S9S_COMPARE(payload["cluster_id"].toInt(), 42);
    S9S_COMPARE(payload["with_databases"].toBoolean(), true);
    S9S_COMPARE(payload["operation"].toString(), "getClusterInfo");

    return true;
}

bool
UtS9sRpcClient::testGetTree()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    
    options->addExtraArgument("/myPath");
    options->m_options["refresh"] = true;

    S9S_VERIFY(client.getTree(true));

    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    //S9S_COMPARE(payload.size(), 6);
    S9S_COMPARE(payload["with_dot_dot"].toBoolean(), true);
    S9S_COMPARE(payload["refresh_now"].toBoolean(), true);
    S9S_COMPARE(payload["path"].toString(), "/myPath");
    S9S_COMPARE(payload["operation"].toString(), "getTree");

    return true;
}

bool
UtS9sRpcClient::testGetClusterConfig()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_id"] = 42;
    S9S_VERIFY(client.getClusterConfig());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload.size(), 5);
    S9S_COMPARE(payload["cluster_id"].toInt(), 42);
    S9S_COMPARE(payload["operation"].toString(), "getConfig");

    return true;
}

bool
UtS9sRpcClient::testPingController()
{
    //S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    S9S_VERIFY(client.pingController());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload.size(), 3);
    S9S_COMPARE(payload["operation"].toString(), "ping");
    S9S_VERIFY(payload["request_created"].toString().startsWith("20"));

    return true;
}

bool
UtS9sRpcClient::testGetCpuInfo()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_id"] = 42;
    S9S_VERIFY(client.getCpuInfo(42));
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload.size(), 4);
    S9S_COMPARE(payload["cluster_id"].toInt(), 42);
    S9S_COMPARE(payload["operation"].toString(), "getCpuPhysicalInfo");

    return true;
}

bool
UtS9sRpcClient::testGetCpuStats()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_id"] = 42;
    S9S_VERIFY(client.getCpuStats(42));
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload.size(), 8);
    S9S_COMPARE(payload["cluster_id"].toInt(), 42);
    S9S_COMPARE(payload["operation"].toString(), "statByName");
    S9S_COMPARE(payload["name"].toString(), "cpustat");
    S9S_COMPARE(payload["with_hosts"].toBoolean(), true);

    return true;
}

bool
UtS9sRpcClient::testGetSqlStats()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_id"] = 42;
    S9S_VERIFY(client.getSqlStats(42));
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload.size(), 8);
    S9S_COMPARE(payload["cluster_id"].toInt(), 42);
    S9S_COMPARE(payload["operation"].toString(), "statByName");
    S9S_COMPARE(payload["name"].toString(), "sqlstat");
    S9S_COMPARE(payload["with_hosts"].toBoolean(), true);

    return true;
}

bool
UtS9sRpcClient::testGetMemStats()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_id"] = 42;
    S9S_VERIFY(client.getMemStats(42));
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload.size(), 8);
    S9S_COMPARE(payload["cluster_id"].toInt(), 42);
    S9S_COMPARE(payload["operation"].toString(), "statByName");
    S9S_COMPARE(payload["name"].toString(), "memorystat");
    S9S_COMPARE(payload["with_hosts"].toBoolean(), true);

    return true;
}

bool
UtS9sRpcClient::testGetMemoryStats()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_id"] = 42;
    S9S_VERIFY(client.getMemoryStats(42));
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload.size(), 5);
    S9S_COMPARE(payload["cluster_id"].toInt(), 42);
    S9S_COMPARE(payload["operation"].toString(), "statByName");
    S9S_COMPARE(payload["name"].toString(), "memorystat");

    return true;
}

bool
UtS9sRpcClient::testGetRunningProcesses()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_id"] = 42;
    S9S_VERIFY(client.getRunningProcesses());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload.size(), 5);
    S9S_COMPARE(payload["cluster_id"].toInt(), 42);
    S9S_COMPARE(payload["operation"].toString(), "getRunningProcesses");

    return true;
}

bool
UtS9sRpcClient::testGetJobInstances()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_id"] = 42;
    options->m_options["limit"] = 10;
    options->m_options["offset"] = 100;
    S9S_VERIFY(client.getJobInstances("clustername", 42));
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload.size(), 7);
    S9S_COMPARE(payload["cluster_id"].toInt(), 42);
    S9S_COMPARE(payload["operation"].toString(), "getJobInstances");
    S9S_COMPARE(payload["limit"].toInt(), 10);
    S9S_COMPARE(payload["offset"].toInt(), 100);

    return true;
}

bool
UtS9sRpcClient::testKillJobInstance()
{
    //S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    //options->m_options["cluster_id"] = 42;
    //options->m_options["limit"] = 10;
    //options->m_options["offset"] = 100;
    S9S_VERIFY(client.killJobInstance(142));
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload.size(), 5);
    S9S_COMPARE(payload["operation"].toString(), "killJobInstance");
    S9S_COMPARE(payload["signal"].toInt(), 15);
    S9S_COMPARE(payload["job_id"].toInt(), 142);

    return true;
}

bool
UtS9sRpcClient::testCloneJobInstance()
{
    //S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    //options->m_options["cluster_id"] = 42;
    //options->m_options["limit"] = 10;
    //options->m_options["offset"] = 100;
    S9S_VERIFY(client.cloneJobInstance(142));
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload.size(), 5);
    S9S_COMPARE(payload["operation"].toString(), "cloneJobInstance");
    S9S_COMPARE(payload["job_id"].toInt(), 142);

    return true;
}

bool
UtS9sRpcClient::testGetDbGrowth()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;

    options->m_options["cluster_id"] = 42;
    S9S_VERIFY(client.getDbGrowth());

    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload.size(), 5);
    S9S_COMPARE(payload["operation"].toString(), "getdbgrowth");
    S9S_COMPARE(payload["cluster_id"].toInt(), 42);

    return true;
}

bool
UtS9sRpcClient::testCreateContainer()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    S9sVariantList      containerList;
    S9sVariantMap       properties;

    /*
     * Setting up the options.
     */
    options->appendVolumes("vol1:5:hdd;vol2:10:hdd");
    options->setServers("lxc://host01");
    options->setContainers("containername1;containername2");

    options->m_options["template"] = "templatename";
    options->m_options["image"] = "imagename";
    options->m_options["image_os_user"] = "username";
    options->m_options["cloud"] = "cloudname";
    options->m_options["subnet_id"] = "subnetname";
    options->m_options["vpc_id"] = "vpcname";

    /*
     * Calling the test function to emit a request.
     */
    S9S_VERIFY(client.createContainerWithJob());
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    /*
     * Checking the emitted request.
     */
    S9S_COMPARE(client.uri(0), "/v2/jobs/");
    S9S_COMPARE(payload["operation"].toString(), "createJobInstance");
    
    S9S_COMPARE(
            payload.valueByPath("/job/title").toString(),
            "Create Containers");

    S9S_COMPARE(
            payload.valueByPath("/job/job_spec/command").toString(),
            "create_containers");
    
    containerList = payload.valueByPath(JOB_DATA "containers").toVariantList();
    S9S_COMPARE(containerList.size(), 2);

    // Checking the properties of the first container.
    properties = containerList[0].toVariantMap();
    S9S_COMPARE(properties["class_name"], "CmonContainer");
    S9S_COMPARE(properties["alias"], "containername1");
    S9S_COMPARE(properties["image"], "imagename");
    S9S_COMPARE(properties["image_os_user"], "username");
    S9S_COMPARE(properties["parent_server"], "host01");
    S9S_COMPARE(properties["provider"], "cloudname");
    S9S_COMPARE(properties["template"], "templatename");
    S9S_COMPARE(properties["subnet"]["id"], "subnetname");
    S9S_COMPARE(properties["subnet"]["vpc_id"], "vpcname");
    
    S9S_COMPARE(properties["volumes"][0]["name"], "vol1");
    S9S_COMPARE(properties["volumes"][0]["size"], 5);
    S9S_COMPARE(properties["volumes"][0]["type"], "hdd");
    
    S9S_COMPARE(properties["volumes"][1]["name"], "vol2");
    S9S_COMPARE(properties["volumes"][1]["size"], 10);
    S9S_COMPARE(properties["volumes"][1]["type"], "hdd");
    
    // Checking the properties of the second container.
    properties = containerList[1].toVariantMap();
    S9S_COMPARE(properties["class_name"], "CmonContainer");
    S9S_COMPARE(properties["alias"], "containername2");
    S9S_COMPARE(properties["image"], "imagename");
    S9S_COMPARE(properties["image_os_user"], "username");
    S9S_COMPARE(properties["parent_server"], "host01");
    S9S_COMPARE(properties["provider"], "cloudname");
    S9S_COMPARE(properties["template"], "templatename");
    S9S_COMPARE(properties["subnet"]["id"], "subnetname");
    S9S_COMPARE(properties["subnet"]["vpc_id"], "vpcname");
    
    S9S_COMPARE(properties["volumes"][0]["name"], "vol1");
    S9S_COMPARE(properties["volumes"][0]["size"], 5);
    S9S_COMPARE(properties["volumes"][0]["type"], "hdd");
    
    S9S_COMPARE(properties["volumes"][1]["name"], "vol2");
    S9S_COMPARE(properties["volumes"][1]["size"], 10);
    S9S_COMPARE(properties["volumes"][1]["type"], "hdd");

    return true;
}

bool
UtS9sRpcClient::testDeleteContainer()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload, tmpMap;
    
    /*
     * Setting up the options.
     */
    options->setContainers("containername");
    
    /*
     * Calling the test function to emit a request.
     */
    S9S_VERIFY(client.deleteContainerWithJob());
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    /*
     * Checking the emitted request.
     */
    S9S_COMPARE(client.uri(0), "/v2/jobs/");
    S9S_COMPARE(payload["operation"].toString(), "createJobInstance");
    
    S9S_COMPARE(
            payload.valueByPath("/job/title").toString(),
            "Delete Container");

    S9S_COMPARE(
            payload.valueByPath("/job/job_spec/command").toString(),
            "delete_container");
    
    tmpMap =  payload.valueByPath(JOB_DATA "container").toVariantMap();
    S9S_COMPARE(tmpMap["class_name"], "CmonContainer");
    S9S_COMPARE(tmpMap["alias"], "containername");

    return true;
}

bool
UtS9sRpcClient::testStartContainer()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload, tmpMap;
    
    /*
     * Setting up the options.
     */
    options->setContainers("containername");
    S9S_VERIFY(client.startContainerWithJob());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(client.uri(0), "/v2/jobs/");
    S9S_COMPARE(payload["operation"].toString(), "createJobInstance");
    
    S9S_COMPARE(
            payload.valueByPath("/job/title").toString(),
            "Start Container");

    S9S_COMPARE(
            payload.valueByPath("/job/job_spec/command").toString(),
            "start_container");
    
    tmpMap =  payload.valueByPath(JOB_DATA "container").toVariantMap();
    S9S_COMPARE(tmpMap["class_name"], "CmonContainer");
    S9S_COMPARE(tmpMap["alias"], "containername");

    return true;
}

bool
UtS9sRpcClient::testStopContainer()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload, tmpMap;
    
    /*
     * Setting up the options.
     */
    options->setContainers("containername");
    S9S_VERIFY(client.stopContainerWithJob());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(client.uri(0), "/v2/jobs/");
    S9S_COMPARE(payload["operation"].toString(), "createJobInstance");
    
    S9S_COMPARE(
            payload.valueByPath("/job/title").toString(),
            "Stop Container");

    S9S_COMPARE(
            payload.valueByPath("/job/job_spec/command").toString(),
            "stop_container");
    
    tmpMap =  payload.valueByPath(JOB_DATA "container").toVariantMap();
    S9S_COMPARE(tmpMap["class_name"], "CmonContainer");
    S9S_COMPARE(tmpMap["alias"], "containername");

    return true;
}

bool
UtS9sRpcClient::testCreateCluster01()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_type"]      = "galera";
    options->m_options["vendor"]            = "myvendor";
    options->m_options["provider_version"]  = "myversion";

    options->setNodes("NODE1:42;NODE2:42;NODE3:42");
    options->setContainers("NODE1;NODE2;NODE3");
    S9S_VERIFY(client.createCluster());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload["operation"].toString(), "createJobInstance");

    S9S_COMPARE(
            payload.valueByPath("/job/title").toString(),
            "Create Galera Cluster");
    
    S9S_COMPARE(
            payload.valueByPath("/job/job_spec/command").toString(),
            "create_cluster");
   
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "cluster_type").toString(),
            "galera");
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "enable_uninstall").toBoolean(),
            true);
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "install_software").toBoolean(),
            true);
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "vendor").toString(),
            "myvendor");
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "version").toString(),
            "myversion");

    return true;
}

bool
UtS9sRpcClient::testCreateCluster02()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_type"]      = "mysqlreplication";
    options->m_options["vendor"]            = "myvendor";
    options->m_options["provider_version"]  = "myversion";

    options->setNodes("NODE1:42;NODE2:42;NODE3:42");
    options->setContainers("NODE1;NODE2;NODE3");
    S9S_VERIFY(client.createCluster());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload["operation"].toString(), "createJobInstance");

    S9S_COMPARE(
            payload.valueByPath("/job/title").toString(),
            "Create MySQL Replication Cluster");
    
    S9S_COMPARE(
            payload.valueByPath("/job/job_spec/command").toString(),
            "create_cluster");
   
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "cluster_type").toString(),
            "replication");
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "enable_uninstall").toBoolean(),
            true);
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "install_software").toBoolean(),
            true);
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "vendor").toString(),
            "myvendor");
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "version").toString(),
            "myversion");

    return true;
}

bool
UtS9sRpcClient::testCreateCluster03()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_type"]      = "groupreplication";
    options->m_options["vendor"]            = "myvendor";
    options->m_options["provider_version"]  = "myversion";

    options->setNodes("NODE1:42;NODE2:42;NODE3:42");
    options->setContainers("NODE1;NODE2;NODE3");
    S9S_VERIFY(client.createCluster());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload["operation"].toString(), "createJobInstance");

    S9S_COMPARE(
            payload.valueByPath("/job/title").toString(),
            "Create MySQL Replication Cluster");
    
    S9S_COMPARE(
            payload.valueByPath("/job/job_spec/command").toString(),
            "create_cluster");
   
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "cluster_type").toString(),
            "group_replication");
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "enable_uninstall").toBoolean(),
            true);
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "install_software").toBoolean(),
            true);
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "vendor").toString(),
            "myvendor");
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "version").toString(),
            "myversion");

    return true;
}

bool
UtS9sRpcClient::testCreateCluster04()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_type"]      = "postgresql";
    //options->m_options["vendor"]            = "myvendor";
    options->m_options["provider_version"]  = "myversion";

    options->setNodes("NODE1:42;NODE2:42;NODE3:42");
    options->setContainers("NODE1;NODE2;NODE3");
    S9S_VERIFY(client.createCluster());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload["operation"].toString(), "createJobInstance");

    S9S_COMPARE(
            payload.valueByPath("/job/title").toString(),
            "Creating PostgreSQL Cluster");
    
    S9S_COMPARE(
            payload.valueByPath("/job/job_spec/command").toString(),
            "create_cluster");
   
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "cluster_type").toString(),
            "postgresql_single");
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "enable_uninstall").toBoolean(),
            true);
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "install_software").toBoolean(),
            true);
    
    //S9S_COMPARE(
    //        payload.valueByPath(JOB_DATA "vendor").toString(),
    //        "myvendor");
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "version").toString(),
            "myversion");

    return true;
}

bool
UtS9sRpcClient::testCreateCluster05()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_type"]      = "mongodb";
    options->m_options["vendor"]            = "myvendor";
    options->m_options["provider_version"]  = "myversion";

    options->setNodes("NODE1:42;NODE2:42;NODE3:42");
    options->setContainers("NODE1;NODE2;NODE3");
    S9S_VERIFY(client.createCluster());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload["operation"].toString(), "createJobInstance");

    S9S_COMPARE(
            payload.valueByPath("/job/title").toString(),
            "Create Mongo Cluster");
    
    S9S_COMPARE(
            payload.valueByPath("/job/job_spec/command").toString(),
            "create_cluster");
   
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "cluster_type").toString(),
            "mongodb");
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "enable_uninstall").toBoolean(),
            true);
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "install_software").toBoolean(),
            true);
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "vendor").toString(),
            "myvendor");
    
    // FIXME: MongoDb has a different "version" name.
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "mongodb_version").toString(),
            "myversion");

    return true;
}

bool
UtS9sRpcClient::testCreateCluster06()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    
    options->m_options["cluster_type"]      = "ndbcluster";
    options->m_options["vendor"]            = "myvendor";
    options->m_options["provider_version"]  = "myversion";

    options->setNodes("NODE1:42;NODE2:42;NODE3:42");
    options->setContainers("NODE1;NODE2;NODE3");
    S9S_VERIFY(client.createCluster());
    
    payload = client.lastPayload();
    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(payload["operation"].toString(), "createJobInstance");

    S9S_COMPARE(
            payload.valueByPath("/job/title").toString(),
            "Create NDB Cluster");
    
    S9S_COMPARE(
            payload.valueByPath("/job/job_spec/command").toString(),
            "create_cluster");
   
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "cluster_type").toString(),
            "mysqlcluster");
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "enable_uninstall").toBoolean(),
            true);
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "install_software").toBoolean(),
            true);
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "vendor").toString(),
            "myvendor");
    
    S9S_COMPARE(
            payload.valueByPath(JOB_DATA "version").toString(),
            "myversion");

    return true;
}

/**
 * Testing the getAllClusterInfo() call.
 */
bool
UtS9sRpcClient::testGetAllClusterInfo()
{
    S9sRpcClientTester client;
    S9sVariantMap      payload;

    S9S_VERIFY(client.getClusters());
    S9S_COMPARE(client.uri(0u), "/v2/clusters/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"], "getAllClusterInfo");

    return true;
}

bool
UtS9sRpcClient::testGetCluster()
{
    S9sRpcClientTester client;
    S9sVariantMap      payload;

    S9S_VERIFY(client.getCluster("", 42));
    S9S_COMPARE(client.uri(0u), "/v2/clusters/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"],  "getClusterInfo");
    S9S_COMPARE(payload["cluster_id"], 42);
    S9S_COMPARE(payload["with_hosts"], true);

    return true;
}

bool
UtS9sRpcClient::testPing()
{
    S9sRpcClientTester client;
    S9sVariantMap      payload;

    S9S_VERIFY(client.pingCluster());
    S9S_COMPARE(client.uri(0u), "/v2/clusters/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"],  "ping");
    S9S_VERIFY(payload["request_created"].toString().startsWith("202"));

    return true;
}

bool
UtS9sRpcClient::testGetMateTypes()
{
    S9sRpcClientTester client;
    S9sVariantMap      payload;

    S9S_VERIFY(client.getMetaTypes());
    S9S_COMPARE(client.uri(0u), "/v2/metatype/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"],  "getMetaTypes");
    S9S_VERIFY(payload["request_created"].toString().startsWith("202"));

    return true;
}

bool
UtS9sRpcClient::testGetMetaTypeProps()
{
    S9sRpcClientTester client;
    S9sVariantMap      payload;

    S9S_VERIFY(client.getMetaTypeProperties("typename"));
    S9S_COMPARE(client.uri(0u), "/v2/metatype/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"],  "getMetaTypeInfo");
    S9S_COMPARE(payload["type-name"],  "typename");
    S9S_VERIFY(payload["request_created"].toString().startsWith("202"));

    return true;
}

bool
UtS9sRpcClient::testGetJobInstance()
{
    S9sRpcClientTester client;
    S9sVariantMap      payload;

    S9S_VERIFY(client.getJobInstance(42));
    S9S_COMPARE(client.uri(0u), "/v2/jobs/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"],  "getJobInstance");
    S9S_COMPARE(payload["job_id"],     42);
    S9S_VERIFY(payload["request_created"].toString().startsWith("202"));

    return true;
}

bool
UtS9sRpcClient::testDeleteJobInstance()
{
    S9sRpcClientTester client;
    S9sVariantMap      payload;

    S9S_VERIFY(client.deleteJobInstance(42));
    S9S_COMPARE(client.uri(0u), "/v2/jobs/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"],  "deleteJobInstance");
    S9S_COMPARE(payload["job_id"],     42);
    S9S_VERIFY(payload["request_created"].toString().startsWith("202"));

    return true;
}

bool
UtS9sRpcClient::testGetJobLog()
{
    S9sRpcClientTester client;
    S9sVariantMap      payload;

    S9S_VERIFY(client.getJobLog(42, 10, 25));
    S9S_COMPARE(client.uri(0u), "/v2/jobs/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"],  "getJobLog");
    S9S_COMPARE(payload["job_id"],     42);
    S9S_COMPARE(payload["ascending"],  true);
    S9S_COMPARE(payload["limit"],      10);
    S9S_COMPARE(payload["offset"],     25);

    S9S_VERIFY(payload["request_created"].toString().startsWith("202"));

    return true;
}

bool
UtS9sRpcClient::testGetAlarm()
{
    S9sRpcClientTester client;
    S9sVariantMap      payload;

    S9S_VERIFY(client.getAlarm());
    S9S_COMPARE(client.uri(0u), "/v2/alarm/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"],  "getAlarm");

    S9S_VERIFY(payload["request_created"].toString().startsWith("202"));

    return true;
}

bool
UtS9sRpcClient::testGetAlarmStatistics()
{
    S9sRpcClientTester client;
    S9sVariantMap      payload;

    S9S_VERIFY(client.getAlarmStatistics());
    S9S_COMPARE(client.uri(0u), "/v2/alarm/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"],  "getStatistics");

    S9S_VERIFY(payload["request_created"].toString().startsWith("202"));

    return true;
}

bool
UtS9sRpcClient::testCreateFailJob()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;

    options->m_options.clear();
    options->m_options["timeout"] = 100;
    options->m_options["cluster_id"]     = 42;

    S9S_VERIFY(client.createFailJob());
    S9S_COMPARE(client.uri(0u), "/v2/jobs/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"], "createJobInstance");
    S9S_COMPARE(payload["cluster_id"], 42);
    S9S_COMPARE(payload["job"]["class_name"], "CmonJobInstance");
    S9S_COMPARE(payload["job"]["title"], "Simulated Failure");
    S9S_COMPARE(payload["job"]["job_spec"]["command"],  "fail");
    S9S_COMPARE(payload["job"]["job_spec"]["job_data"]["timeout"],  100);

    S9S_VERIFY(payload["request_created"].toString().startsWith("202"));

    return true;
}

bool
UtS9sRpcClient::testCreateSuccessJob()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;

    options->m_options.clear();
    options->m_options["timeout"] = 100;
    options->m_options["cluster_id"]     = 42;

    S9S_VERIFY(client.createSuccessJob());
    S9S_COMPARE(client.uri(0u), "/v2/jobs/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"], "createJobInstance");
    S9S_COMPARE(payload["cluster_id"], 42);
    S9S_COMPARE(payload["job"]["class_name"], "CmonJobInstance");
    S9S_COMPARE(payload["job"]["title"], "Simulated Success");
    S9S_COMPARE(payload["job"]["job_spec"]["command"],  "success");
    S9S_COMPARE(payload["job"]["job_spec"]["job_data"]["timeout"],  100);

    S9S_VERIFY(payload["request_created"].toString().startsWith("202"));

    return true;
}

bool
UtS9sRpcClient::testRollingRestart()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;

    options->m_options.clear();
    options->m_options["cluster_id"]     = 42;

    S9S_VERIFY(client.rollingRestart());
    S9S_COMPARE(client.uri(0u), "/v2/jobs/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"], "createJobInstance");
    S9S_COMPARE(payload["cluster_id"], 42);
    S9S_COMPARE(payload["job"]["class_name"], "CmonJobInstance");
    S9S_COMPARE(payload["job"]["title"], "Rolling Restart");
    S9S_COMPARE(payload["job"]["job_spec"]["command"],  "rolling_restart");

    S9S_VERIFY(payload["request_created"].toString().startsWith("202"));

    return true;
}

bool
UtS9sRpcClient::testRegisterServers()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;

    options->m_options.clear();
    options->setServers("lxc://10.10.2.3;cmon-cloud://10.10.2.4");

    S9S_VERIFY(client.registerServers());
    S9S_COMPARE(client.uri(0u), "/v2/host/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"], "registerServers");
    S9S_COMPARE(payload["servers"][0]["class_name"], "CmonLxcServer");
    S9S_COMPARE(payload["servers"][0]["hostname"],   "10.10.2.3");
    //S9S_COMPARE(payload["servers"][0]["protocol"],   "lxc");
    
    S9S_COMPARE(payload["servers"][1]["class_name"], "CmonCloudServer");
    S9S_COMPARE(payload["servers"][1]["hostname"],   "10.10.2.4");
    //S9S_COMPARE(payload["servers"][1]["protocol"],   "cmon-cloud");

    S9S_VERIFY(payload["request_created"].toString().startsWith("202"));

    return true;
}

bool
UtS9sRpcClient::testUnregisterServers()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;

    options->m_options.clear();
    options->setServers("lxc://10.10.2.3;cmon-cloud://10.10.2.4");

    S9S_VERIFY(client.unregisterServers());
    S9S_COMPARE(client.uri(0u), "/v2/host/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"], "unregisterServers");
    S9S_COMPARE(payload["servers"][0]["class_name"], "CmonLxcServer");
    S9S_COMPARE(payload["servers"][0]["hostname"],   "10.10.2.3");
    //S9S_COMPARE(payload["servers"][0]["protocol"],   "lxc");
    
    S9S_COMPARE(payload["servers"][1]["class_name"], "CmonCloudServer");
    S9S_COMPARE(payload["servers"][1]["hostname"],   "10.10.2.4");
    //S9S_COMPARE(payload["servers"][1]["protocol"],   "cmon-cloud");

    S9S_VERIFY(payload["request_created"].toString().startsWith("202"));

    return true;
}

bool
UtS9sRpcClient::testCreateServer()
{
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
    S9sVariantMap       jobData;

    options->m_options.clear();
    options->setServers("lxc://10.10.2.3");

    S9S_VERIFY(client.createServer());
    S9S_COMPARE(client.uri(0u), "/v2/jobs/");
    
    payload = client.lastPayload();
    S9S_COMPARE(payload["operation"], "createJobInstance");
    
    S9S_COMPARE(payload["job"]["class_name"], "CmonJobInstance");
    S9S_COMPARE(payload["job"]["title"], "Create Container Server");
    S9S_COMPARE(payload["job"]["job_spec"]["command"], "create_container_server");

    jobData = payload["job"]["job_spec"]["job_data"].toVariantMap();
    S9S_COMPARE(jobData["disable_firewall"], true);
    S9S_COMPARE(jobData["disable_selinux"], true);
    S9S_COMPARE(jobData["install_software"], true);

    S9S_COMPARE(jobData["server"]["class_name"], "CmonLxcServer");
    S9S_COMPARE(jobData["server"]["hostname"],   "10.10.2.3");
    //S9S_COMPARE(jobData["server"]["protocol"],   "lxc");
    
    S9S_VERIFY(payload["request_created"].toString().startsWith("202"));

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
    S9sString          uri, payload;

    properties["name"] = "value";
    hosts << S9sNode("myserver.eu:80");

    S9S_VERIFY(client.setHost(hosts, properties));
    uri     = client.uri(0u);
    payload = client.payload(0u);

    S9S_DEBUG("*** uri     : %s", STR(uri));
    S9S_DEBUG("*** payload : %s", STR(payload));

    S9S_COMPARE(uri, "/v2/host");
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

    S9S_VERIFY(client.createGaleraCluster(hosts, "pi", "percona", "5.6"));
    uri     = client.uri(0u);
    payload = client.payload(0u);

    if (isVerbose())
        printDebug(payload);

    S9S_COMPARE(uri, "/v2/jobs/");
    S9S_VERIFY(payload.contains("\"command\": \"create_cluster\""));
    S9S_VERIFY(payload.contains("\"cluster_type\": \"galera\""));
    S9S_VERIFY(payload.contains("\"vendor\": \"percona\""));
    S9S_VERIFY(payload.contains("\"version\": \"5.6\""));
    S9S_VERIFY(payload.contains("\"hostname\": \"192.168.1.193\""));

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
                hosts, "pi", "percona", "5.6"));

    uri     = client.uri(0u);
    payload = client.payload(0u);

    S9S_DEBUG("*** uri     : %s", STR(uri));
    S9S_DEBUG("*** payload : %s", STR(payload));
    S9S_COMPARE(uri, "/v2/jobs/");
    S9S_VERIFY(payload.contains("\"command\": \"create_cluster\""));
    S9S_VERIFY(payload.contains("\"cluster_type\": \"replication\""));
    S9S_VERIFY(payload.contains("\"vendor\": \"percona\""));
    S9S_VERIFY(payload.contains("\"version\": \"5.6\""));

    #if 0
    S9S_VERIFY(payload.contains(
                "\"mysql_hostnames\": "
                "[ \"192.168.1.191\", \"192.168.1.192\", \"192.168.1.193\" ]"));
    #endif

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
                "pip", "oracle", "5.6"));

    uri     = client.uri(0u);
    payload = client.payload(0u);

    //S9S_DEBUG("*** uri     : %s", STR(uri));
    //S9S_DEBUG("*** payload : %s", STR(payload));
    S9S_COMPARE(uri, "/v2/jobs/");
    S9S_VERIFY(payload.contains("\"command\": \"create_cluster\""));
    S9S_VERIFY(payload.contains("\"cluster_type\": \"mysqlcluster\""));
    S9S_VERIFY(payload.contains("\"type\": \"mysql\""));
    S9S_VERIFY(payload.contains("\"vendor\": \"oracle\""));
    S9S_VERIFY(payload.contains("\"version\": \"5.6\""));

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
    S9sOptions         *options = S9sOptions::instance();
    S9sRpcClientTester  client;
    S9sVariantList      hosts;
    S9sString           uri, payload;
    int                 clusterId = 1;

    hosts << S9sNode("192.168.1.191");
    options->m_options["cluster_id"] = clusterId;
    S9S_VERIFY(client.addNode(hosts));

    uri     = client.uri(0u);
    payload = client.payload(0u);

    S9S_COMPARE(uri, "/v2/jobs/");
    S9S_VERIFY(payload.contains("\"command\": \"addnode\""));
    S9S_VERIFY(payload.contains("\"disable_firewall\": true"));
    S9S_VERIFY(payload.contains("\"disable_selinux\": true"));
    S9S_VERIFY(payload.contains("\"hostname\": \"192.168.1.191\""));
    S9S_VERIFY(payload.contains("\"install_software\": true"));
    //S9S_VERIFY(payload.contains("\"user_name\": \"pipas\""));

    return true;
}

bool
UtS9sRpcClient::testCreateHaproxy()
{
    // FIXME: I will have to write some tests...
#if 0
    S9sOptions         *options = S9sOptions::instance();    
    S9sRpcClientTester  client;
    
    options->setNodes("node1?rw_port=42&ro_port=45");
#endif
    return true;
}

bool
UtS9sRpcClient::testComposeBackupJob()
{
    S9sOptions         *options = S9sOptions::instance();    
    S9sRpcClientTester  client;
    S9sVariantMap       job, jobData;
    S9sString           jsonString;

    options->setNodes("node1:43");
    options->m_options["cluster_id"]          = 42;    
    options->m_options["backup_method"]       = "mybackupmethod";
    options->m_options["backup_directory"]    = "/backupdir";
    options->m_options["subdirectory"]        = "subdir";
    options->m_options["backup_user"]         = "backupuser";
    options->m_options["backup_password"]     = "backuppassword";
    options->m_options["databases"]           = "databases";
    options->m_options["pitr_compatible"]     = true;
    options->m_options["no_compression"]      = false;
    options->m_options["use_pigz"]            = true;
    options->m_options["on_node"]             = true;
    options->m_options["on_controller"]       = false;
    options->m_options["parallellism"]        = 10;
    options->m_options["encrypt_backup"]      = true;
    options->m_options["backup_retention"]    = 8;
    options->m_options["to_individual_files"] = true;
    options->m_options["test_server"]         = "testserver1.com";

    // Composing the backup job.
    job = client.composeBackupJob();
    jobData = job.valueByPath("job_spec/job_data").toVariantMap();
    jsonString = job.toString();
    //::printf("\n%s\n", STR(jsonString));

    // 
    S9S_COMPARE(
            jobData.valueByPath("backup_individual_schemas"),
            true);

    S9S_COMPARE(
            jobData.valueByPath("backup_method"),
            "mybackupmethod");
    
    S9S_COMPARE(
            jobData.valueByPath("backup_retention"),
            8);
    
    S9S_COMPARE(
            jobData.valueByPath("backup_user"),
            "backupuser");
    
    S9S_COMPARE(
            jobData.valueByPath("backup_user_password"),
            "backuppassword");
    
    S9S_COMPARE(
            jobData.valueByPath("backupdir"),
            "/backupdir");
    
    S9S_COMPARE(
            jobData.valueByPath("backupsubdir"),
            "subdir");
    
    S9S_COMPARE(
            jobData.valueByPath("cc_storage"),
            false);
    
    S9S_COMPARE(
            jobData.valueByPath("description"),
            "Backup created by s9s-tools.");
    
    S9S_COMPARE(
            jobData.valueByPath("encrypt_backup"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("hostname"),
            "node1");
    
    S9S_COMPARE(
            jobData.valueByPath("include_databases"),
            "databases");
    
    S9S_COMPARE(
            jobData.valueByPath("pitr_compatible"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("port"),
            43);
    
    S9S_COMPARE(
            jobData.valueByPath("terminate_db_server"),
            true);

    S9S_COMPARE(
            jobData.valueByPath("use_pigz"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("verify_backup/disable_firewall"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("verify_backup/disable_selinux"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("verify_backup/install_software"),
            "true");

    S9S_COMPARE(
            jobData.valueByPath("verify_backup/server_address"),
            "testserver1.com");
    
    S9S_COMPARE(
            jobData.valueByPath("xtrabackup_parallellism"),
            10);

    return true;
}

bool
UtS9sRpcClient::testBackup()
{
    S9sOptions         *options = S9sOptions::instance();    
    S9sRpcClientTester  client;
    S9sVariantMap       payload, jobData;
  
    options->setNodes("node1:43");
    options->m_options["cluster_id"]          = 42;    
    options->m_options["backup_method"]       = "mybackupmethod1";
    options->m_options["backup_directory"]    = "/backupdir1";
    options->m_options["snapshot_repository"] = "snapshotRepo1";
    options->m_options["snapshot_repository_type"] = "fs";
    options->m_options["credential_id"]       = 1;
    options->m_options["s3_bucket"]           = "elastic-s3-test";
    options->m_options["s3_region"]           = "eu-west-3";
    options->m_options["snapshot_location"]   = "/home/vagrant/backups/es-snapshot-repositories";
    options->m_options["subdirectory"]        = "subdir1";
    options->m_options["backup_user"]         = "backupuser1";
    options->m_options["backup_password"]     = "backuppassword1";
    options->m_options["databases"]           = "databases1";
    options->m_options["pitr_compatible"]     = true;
    options->m_options["no_compression"]      = false;
    options->m_options["use_pigz"]            = true;
    options->m_options["on_node"]             = true;
    options->m_options["on_controller"]       = false;
    options->m_options["parallellism"]        = 10;
    options->m_options["encrypt_backup"]      = true;
    options->m_options["backup_retention"]    = 8;
    options->m_options["to_individual_files"] = true;
    options->m_options["test_server"]         = "testserver1.com";

    S9S_VERIFY(client.createBackup());
    payload = client.lastPayload();
    jobData = payload.valueByPath("job/job_spec/job_data").toVariantMap();

    //if (isVerbose())
    //    ::printf("\n%s\n", STR(payload.toString()));
   
    //
    //
    //
    S9S_COMPARE(
            payload.valueByPath("cluster_id"),
            42);
    
    S9S_COMPARE(
            payload.valueByPath("job/class_name"),
            "CmonJobInstance");
    
    S9S_COMPARE(
            payload.valueByPath("job/job_spec/command"),
            "backup");
    
    S9S_COMPARE(
            jobData.valueByPath("backup_individual_schemas"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("backup_method"),
            "mybackupmethod1");
    
    S9S_COMPARE(
            jobData.valueByPath("backup_retention"),
            8);
    
    S9S_COMPARE(
            jobData.valueByPath("backup_user"),
            "backupuser1");
    
    S9S_COMPARE(
            jobData.valueByPath("backup_user_password"),
            "backuppassword1");
    
    S9S_COMPARE(
            jobData.valueByPath("backupdir"),
            "/backupdir1");
    
    S9S_COMPARE(
            jobData.valueByPath("snapshot_repository"),
            "snapshotRepo1");

    S9S_COMPARE(
            jobData.valueByPath("snapshot_repository_type"),
            "fs");

    S9S_COMPARE(
            jobData.valueByPath("credential_id"),
            1);

    S9S_COMPARE(
            jobData.valueByPath("s3_bucket"),
            "elastic-s3-test");

    S9S_COMPARE(
            jobData.valueByPath("s3_region"),
            "eu-west-3");

    S9S_COMPARE(
            jobData.valueByPath("snapshot_location"),
            "/home/vagrant/backups/es-snapshot-repositories");

    S9S_COMPARE(
            jobData.valueByPath("cc_storage"),
            false);
    
    S9S_COMPARE(
            jobData.valueByPath("description"),
            "Backup created by s9s-tools.");
    
    S9S_COMPARE(
            jobData.valueByPath("encrypt_backup"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("hostname"),
            "node1");
    
    S9S_COMPARE(
            jobData.valueByPath("include_databases"),
            "databases1");
    
    S9S_COMPARE(
            jobData.valueByPath("pitr_compatible"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("port"),
            43);
    
    S9S_COMPARE(
            jobData.valueByPath("terminate_db_server"),
            true);

    S9S_COMPARE(
            jobData.valueByPath("use_pigz"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("verify_backup/disable_firewall"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("backupsubdir"),
            "subdir1");
    
    S9S_COMPARE(
            jobData.valueByPath("cc_storage"),
            false);
    
    S9S_COMPARE(
            jobData.valueByPath("description"),
            "Backup created by s9s-tools.");
    
    S9S_COMPARE(
            jobData.valueByPath("encrypt_backup"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("hostname"),
            "node1");
    
    S9S_COMPARE(
            jobData.valueByPath("include_databases"),
            "databases1");
    
    S9S_COMPARE(
            jobData.valueByPath("pitr_compatible"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("port"),
            43);
    
    S9S_COMPARE(
            jobData.valueByPath("terminate_db_server"),
            true);

    S9S_COMPARE(
            jobData.valueByPath("use_pigz"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("verify_backup/disable_firewall"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("verify_backup/disable_selinux"),
            true);
    
    S9S_COMPARE(
            jobData.valueByPath("verify_backup/install_software"),
            "true");

    S9S_COMPARE(
            jobData.valueByPath("verify_backup/server_address"),
            "testserver1.com");
    
    S9S_COMPARE(
            jobData.valueByPath("xtrabackup_parallellism"),
            10);
    
    S9S_COMPARE(
            payload.valueByPath("operation"),
            "createJobInstance");

    return true;
}

bool
UtS9sRpcClient::testBackupSchedule()
{
    S9sOptions         *options = S9sOptions::instance();    
    S9sRpcClientTester  client;
    S9sVariantMap       payload;
  
    options->setNodes("node1:43");
    options->m_options["cluster_id"]          = 42;    
    options->m_options["backup_method"]       = "mybackupmethod1";
    options->m_options["backup_directory"]    = "/backupdir1";
    options->m_options["subdirectory"]        = "subdir1";
    options->m_options["backup_user"]         = "backupuser1";
    options->m_options["backup_password"]     = "backuppassword1";
    options->m_options["databases"]           = "databases1";
    options->m_options["pitr_compatible"]     = true;
    options->m_options["no_compression"]      = false;
    options->m_options["use_pigz"]            = true;
    options->m_options["on_node"]             = true;
    options->m_options["on_controller"]       = false;
    options->m_options["parallellism"]        = 10;
    options->m_options["encrypt_backup"]      = true;
    options->m_options["backup_retention"]    = 8;
    options->m_options["to_individual_files"] = true;
    options->m_options["test_server"]         = "testserver1.com";
    options->m_options["recurrence"]          = "0 12 * * 5";

    S9S_VERIFY(client.createBackupSchedule());
    payload = client.lastPayload();

    if (isVerbose())
        printDebug(payload);

    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sRpcClient)
