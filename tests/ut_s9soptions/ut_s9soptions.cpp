/*
 * Severalnines Tools
 * Copyright (C) 2018 Severalnines AB
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
#include "ut_s9soptions.h"

#include "s9soptions.h"
#include "s9snode.h"
#include "s9sfile.h"
#include "s9saccount.h"

#include <cstdio>
#include <cstring>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

UtS9sOptions::UtS9sOptions()
{
    S9S_DEBUG("");
}

UtS9sOptions::~UtS9sOptions()
{
}

bool
UtS9sOptions::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testCreate,        retval);
    PERFORM_TEST(testConfigFile01,  retval);
    PERFORM_TEST(testConfigFile02,  retval);
    PERFORM_TEST(testController,    retval);
    PERFORM_TEST(testReadOptions01, retval);
    PERFORM_TEST(testAlarmListHistory, retval);
    PERFORM_TEST(testReadOptions02, retval);
    PERFORM_TEST(testReadOptions03, retval);
    PERFORM_TEST(testReadOptions04, retval);
    PERFORM_TEST(testReadOptions05, retval);
    PERFORM_TEST(testReadOptions06, retval);
    PERFORM_TEST(testReadOptions07, retval);
    PERFORM_TEST(testJobStuck,      retval);
    PERFORM_TEST(testSetNodes,      retval);
    PERFORM_TEST(testPerconaProCluster, retval);
    PERFORM_TEST(testPostgreSqlReplication, retval);
    PERFORM_TEST(testPostgreSqlBackupOptions, retval);
    PERFORM_TEST(testAuditLogEventData, retval);
    PERFORM_TEST(testExternalBackup, retval);
    PERFORM_TEST(testConfigureWalOptions, retval);
    PERFORM_TEST(testAddController, retval);
    PERFORM_TEST(testAddDb, retval);
    PERFORM_TEST(testDeleteDb, retval);
    PERFORM_TEST(testListDb, retval);
    PERFORM_TEST(testAddServicesBundle, retval);
    PERFORM_TEST(testDeleteServicesBundle, retval);
    PERFORM_TEST(testListServicesBundles, retval);
    PERFORM_TEST(testServicesBundleOptionErrors, retval);
    PERFORM_TEST(testAddControllerSite, retval);
    PERFORM_TEST(testAddOpenBao, retval);
    PERFORM_TEST(testListOpenBaoOperations, retval);
    PERFORM_TEST(testPoolModePrerequisites, retval);
    PERFORM_TEST(testVirtualRouterId, retval);
    PERFORM_TEST(testRestoreClusterInfoOptions, retval);
    PERFORM_TEST(testLockAccount, retval);
    PERFORM_TEST(testUnlockAccount, retval);
    PERFORM_TEST(testLockUnlockMutualExclusion, retval);
    PERFORM_TEST(testLockAccountMissingAccount, retval);
    PERFORM_TEST(testUpdateAccount, retval);
    PERFORM_TEST(testUpdateAccountMissingAccount, retval);
    PERFORM_TEST(testUpdateAccountMissingPassword, retval);
    PERFORM_TEST(testUpdateCreateMutualExclusion, retval);
    PERFORM_TEST(testAddShard, retval);
    PERFORM_TEST(testShardId, retval);

    return retval;
}

/**
 * Testing the constructors.
 */
bool
UtS9sOptions::testCreate()
{
    S9sOptions *options = S9sOptions::instance();

    S9S_VERIFY(options != NULL);
    S9sOptions::uninit();
    S9S_VERIFY(S9sOptions::sm_instance == NULL);

    return true;
}

bool
UtS9sOptions::testConfigFile01()
{
    const char  *path1 = "tests/s9s_configs/test_config_03.conf";
    const char  *path2 = "s9s_configs/test_config_03.conf";
    S9sOptions  *options;
    S9sString    configFileName;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options != NULL);

    if (S9sFile::fileExists(path1))
        S9sOptions::sm_defaultUserConfigFileName = path1;
    else if (S9sFile::fileExists(path2))
        S9sOptions::sm_defaultUserConfigFileName = path2;

    S9S_VERIFY(options->loadConfigFiles());
    
    //S9S_COMPARE(options->m_userConfig.fileName(), "");
    S9S_COMPARE(options->controllerHostName(), "test.controller.name");
    S9S_COMPARE(options->controllerPort(),     42);

    S9S_COMPARE(options->userName(),           "test_cmon_user");
    S9S_COMPARE(options->backupDir(),          "/etc/test");
    S9S_COMPARE(options->backupMethod(),       "mysqldump_test");
    S9S_COMPARE(options->briefJobLogFormat(),  "1%M\\n");
    S9S_COMPARE(options->briefLogFormat(),     "2%M\\n");
    
    S9S_COMPARE(options->providerVersion(),    "provider_test");
    S9S_COMPARE(options->vendor(),             "vendor_test");

    S9S_VERIFY(options->onlyAscii());

    // The ssh credentials checked here.
    S9S_COMPARE(options->osUser(),             "osuser_test");
    S9S_COMPARE(options->osKeyFile(),          "oskeyfile_test");

    return true;
}

bool
UtS9sOptions::testConfigFile02()
{
    const char  *path1 = "tests/s9s_configs/test_config_04.conf";
    const char  *path2 = "s9s_configs/test_config_04.conf";
    S9sOptions  *options;
    S9sString    configFileName;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options != NULL);

    if (S9sFile::fileExists(path1))
        S9sOptions::sm_defaultUserConfigFileName = path1;
    else if (S9sFile::fileExists(path2))
        S9sOptions::sm_defaultUserConfigFileName = path2;

    S9S_VERIFY(options->loadConfigFiles());
    
    //S9S_COMPARE(options->m_userConfig.fileName(), "");
    S9S_COMPARE(options->controllerHostName(), "test_controller");
    S9S_COMPARE(options->userName(),           "test_user");

    return true;
}

/**
 * This function tests the S9sOptions::setController() function with various
 * strings.
 */
bool
UtS9sOptions::testController()
{
    S9sOptions *options;

    S9sOptions::sm_defaultUserConfigFileName = "";

    S9sOptions::uninit();
    options = S9sOptions::instance();

    //S9S_COMPARE(options->controllerHostName(), "");
    //S9S_COMPARE(options->controllerPort(),     0);
    
    //options->setController("localhost");
    //S9S_COMPARE(options->controllerHostName(), "localhost");
    //S9S_COMPARE(options->controllerPort(),     0);

    options->setController("localhost:9556");
    S9S_COMPARE(options->controllerHostName(), "localhost");
    S9S_COMPARE(options->controllerPort(),     9556);
    
    options->setController("127.0.0.1");
    S9S_COMPARE(options->controllerHostName(), "127.0.0.1");
    S9S_COMPARE(options->controllerPort(),     9556);

    options->setController("127.0.0.1:9556");
    S9S_COMPARE(options->controllerHostName(), "127.0.0.1");
    S9S_COMPARE(options->controllerPort(),     9556);

    options->setController("http://localhost:80");
    S9S_COMPARE(options->controllerProtocol(), "http");
    S9S_COMPARE(options->controllerHostName(), "localhost");
    S9S_COMPARE(options->controllerPort(),     80);
    
    options->setController("https://127.0.0.1:8080");
    S9S_COMPARE(options->controllerProtocol(), "https");
    S9S_COMPARE(options->controllerHostName(), "127.0.0.1");
    S9S_COMPARE(options->controllerPort(),     8080);

    return true;
}


/**
 * Checking the readOptions() method with some command line options.
 */
bool
UtS9sOptions::testReadOptions01()
{
    S9sOptions *options = S9sOptions::instance();
    bool  success;
    const char *argv[] = 
    { 
        "/bin/s9s", "node", "--list", "--controller=localhost:9555",
        "--color=always", "--verbose",
        NULL 
    };
    int   argc   = sizeof(argv) / sizeof(char *) - 1;


    success = options->readOptions(&argc, (char**)argv);
    S9S_VERIFY(success);
    
    S9S_COMPARE(options->binaryName(),          "s9s");
    S9S_COMPARE(options->m_operationMode,       S9sOptions::Node);
    S9S_COMPARE(options->controllerHostName(),  "localhost");
    S9S_COMPARE(options->controllerPort(),      9555);
    S9S_VERIFY(options->isListRequested());
    S9S_VERIFY(options->isVerbose());
    S9S_VERIFY(options->useSyntaxHighlight());

    S9sOptions::uninit();
    return true;
}

/**
 * Checks that "alarm --list-history" is accepted as a main option, and that
 * --limit and --offset reach the options the request is built from.
 *
 * An option like this needs wiring in six places -- the mode enum,
 * long_options, the parser case, the accessor, the help text, and
 * checkOptionsAlarm()'s countOptions. The last one was missed once, and it is
 * silent in the worst way: the flag appears in --help and the parser accepts
 * it, then the command refuses to run.
 */
bool
UtS9sOptions::testAlarmListHistory()
{
    S9sOptions *options = S9sOptions::instance();
    bool        success;
    const char *argv[] =
    {
        "/bin/s9s", "alarm", "--list-history", "--cluster-id=3",
        "--limit=25", "--offset=50",
        NULL
    };
    int argc = sizeof(argv) / sizeof(char *) - 1;

    // readOptions() runs checkOptionsAlarm() itself, so this single verify is
    // the guard: without --list-history in that function's countOptions the
    // parse fails with "One of the main options is mandatory", even though the
    // flag is in --help and the parser accepts it.
    success = options->readOptions(&argc, (char**)argv);
    S9S_VERIFY(success);

    S9S_COMPARE(options->m_operationMode, S9sOptions::Alarm);
    S9S_VERIFY(options->isListHistoryRequested());
    S9S_COMPARE(options->clusterId(), 3);
    S9S_COMPARE(options->limit(),     25);
    S9S_COMPARE(options->offset(),    50);

    // The main options are mutually exclusive; --list-history is not --list.
    S9S_VERIFY(!options->isListRequested());

    S9sOptions::uninit();
    return true;
}

/**
 * Checking the readOptions() method with some command line options.
 */
bool
UtS9sOptions::testReadOptions02()
{
    S9sOptions *options = S9sOptions::instance();
    bool  success;
    const char *argv[] = 
    { 
        "/bin/s9s", "job", "--list", "--controller=localhost:9555",
        "--color=always", "--verbose",
        NULL 
    };
    int   argc   = sizeof(argv) / sizeof(char *) - 1;


    success = options->readOptions(&argc, (char**)argv);
    S9S_VERIFY(success);
    
    S9S_COMPARE(options->binaryName(),          "s9s");
    S9S_COMPARE(options->m_operationMode,       S9sOptions::Job);
    S9S_COMPARE(options->controllerHostName(),  "localhost");
    S9S_COMPARE(options->controllerPort(),      9555);
    S9S_VERIFY(options->isListRequested());
    S9S_VERIFY(options->isVerbose());
    S9S_VERIFY(options->useSyntaxHighlight());

    S9sOptions::uninit();
    return true;
}

/**
 * Checking the readOptions() method with some command line options.
 */
bool
UtS9sOptions::testReadOptions03()
{
    S9sOptions *options = S9sOptions::instance();
    bool        success;
    S9sVariantList nodes;
    const char *argv[] = 
    { 
        "/bin/s9s", "cluster", "--create", "--controller=localhost:9555",
        "--cluster-type=Galera", 
        "--nodes=10.10.2.2;10.10.2.3;10.10.2.4;10.10.2.5",
        "--vendor=codership", "--provider-version=5.6", "--os-user=14j",
        "--os-elevation=pbrun", "--access-check-cmd=/bin/true", "--wait", NULL 
    };
    int   argc   = sizeof(argv) / sizeof(char *) - 1;


    success = options->readOptions(&argc, (char**)argv);
    S9S_VERIFY(success);
    
    S9S_COMPARE(options->binaryName(),           "s9s");
    S9S_COMPARE(options->m_operationMode,        S9sOptions::Cluster);
    S9S_COMPARE(options->controllerHostName(),   "localhost");
    S9S_COMPARE(options->controllerPort(),       9555);
    S9S_COMPARE(options->clusterType(),          "galera");
    S9S_COMPARE(options->vendor(),               "codership");
    S9S_COMPARE(options->providerVersion(),      "5.6");
    S9S_COMPARE(options->osUser(),               "14j");
    S9S_COMPARE(options->osElevation(),          "pbrun");
    S9S_COMPARE(options->accessCheckCmd(),       "/bin/true");

    S9S_VERIFY(options->hasOsElevation());
    S9S_VERIFY(options->isWaitRequested());
    S9S_VERIFY(!options->isListRequested());
    S9S_VERIFY(!options->isVerbose());

    nodes = options->nodes();
    S9S_COMPARE(nodes.size(), 4);
    S9S_COMPARE(nodes[0].toNode().hostName(), "10.10.2.2");
    S9S_COMPARE(nodes[1].toNode().hostName(), "10.10.2.3");
    S9S_COMPARE(nodes[2].toNode().hostName(), "10.10.2.4");
    S9S_COMPARE(nodes[3].toNode().hostName(), "10.10.2.5");

    S9sOptions::uninit();
    return true;
}

bool
UtS9sOptions::testReadOptions04()
{
    S9sOptions *options = S9sOptions::instance();
    bool  success;
    const char *argv[] = 
    { 
        "/bin/s9s", "--config-file", "/home/johan/.s9s/s9s.conf", 
        "job", "--list", NULL 
    };
    int   argc   = sizeof(argv) / sizeof(char *) - 1;


    success = options->readOptions(&argc, (char**)argv);
    S9S_VERIFY(success);
    
    S9S_COMPARE(options->binaryName(),     "s9s");
    S9S_COMPARE(options->m_operationMode,  S9sOptions::Job);
    S9S_COMPARE(options->configFile(),     "/home/johan/.s9s/s9s.conf");
    S9S_VERIFY(options->isListRequested());

    S9sOptions::uninit();
    return true;
}

bool
UtS9sOptions::testReadOptions05()
{
    S9sOptions *options = S9sOptions::instance();
    bool  success;
    const char *argv[] = 
    { 
        "/bin/s9s", "node", "--stat", "--graph=load", "--density", NULL 
    };
    int   argc   = sizeof(argv) / sizeof(char *) - 1;


    success = options->readOptions(&argc, (char**)argv);
    S9S_VERIFY(success);
    
    S9S_COMPARE(options->binaryName(),     "s9s");
    S9S_COMPARE(options->m_operationMode,  S9sOptions::Node);
    S9S_COMPARE(options->graph(),          "load");
    S9S_VERIFY(options->isStatRequested());
    S9S_VERIFY(options->density());

    S9sOptions::uninit();
    return true;
}

bool
UtS9sOptions::testReadOptions06()
{
    S9sOptions *options = S9sOptions::instance();
    bool  success;
    const char *argv[] = 
    { 
        "/bin/s9s", "user", "--create", "--group=GROUPNAME", "--create-group",
        "--first-name=FIRSTNAME", "--last-name=LASTNAME", "--title=TITLE",
        "--email-address=EMAIL", "--user-format=FORMAT", "--force-password-update",
        NULL
    };
    int   argc   = sizeof(argv) / sizeof(char *) - 1;


    success = options->readOptions(&argc, (char**)argv);
    S9S_VERIFY(success);

    S9S_COMPARE(options->binaryName(), "s9s");
    S9S_COMPARE(options->m_operationMode, S9sOptions::User);
    S9S_VERIFY(options->isCreateRequested());
    S9S_COMPARE(options->group(), "GROUPNAME");
    S9S_VERIFY(options->createGroup());
    S9S_VERIFY(options->forcePasswordUpdate());
    S9S_COMPARE(options->firstName(), "FIRSTNAME");
    S9S_COMPARE(options->lastName(), "LASTNAME");
    S9S_COMPARE(options->title(), "TITLE");
    S9S_COMPARE(options->emailAddress(), "EMAIL");
    S9S_VERIFY(options->hasUserFormat());
    S9S_COMPARE(options->userFormat(), "FORMAT");

    S9sOptions::uninit();
    return true;
}

bool
UtS9sOptions::testReadOptions07()
{
#if 0
    S9sOptions *options = S9sOptions::instance();
    bool  success;
    const char *argv[] = 
    { 
        "/bin/s9s", "maint", "--create", "--cluster-id=1", "--start=START",
        "--end=END", "--reason=REASON", NULL
    };
    int   argc   = sizeof(argv) / sizeof(char *) - 1;


    success = options->readOptions(&argc, (char**)argv);
    S9S_VERIFY(success);
    
    S9S_COMPARE(options->binaryName(),     "s9s");
    S9S_COMPARE(options->m_operationMode,  S9sOptions::Maintenance);
    S9S_VERIFY(options->isCreateRequested());
    S9S_COMPARE(options->clusterId(), 1);
    S9S_COMPARE(options->start(),     "START");
    S9S_COMPARE(options->end(),       "END");
    S9S_COMPARE(options->reason(),    "REASON");

    S9sOptions::uninit();
#endif
    return true;
}

/**
 * Checking that "job --stuck" is recognized as its own main option (not
 * requiring --list), and that it's mutually exclusive with --list.
 */
bool
UtS9sOptions::testJobStuck()
{
    S9sOptions *options = S9sOptions::instance();
    bool        success;

    {
        const char *argv[] =
        {
            "/bin/s9s", "job", "--stuck", "--controller=localhost:9555",
            "--cluster-id=1",
            NULL
        };
        int argc = sizeof(argv) / sizeof(char *) - 1;

        success = options->readOptions(&argc, (char**)argv);
        S9S_VERIFY(success);

        S9S_COMPARE(options->m_operationMode, S9sOptions::Job);
        S9S_VERIFY(options->isStuckRequested());
        S9S_VERIFY(!options->isListRequested());
        S9S_COMPARE(options->clusterId(), 1);

        S9sOptions::uninit();
    }

    {
        // --stuck and --list are mutually exclusive main options.
        options = S9sOptions::instance();
        const char *argv[] =
        {
            "/bin/s9s", "job", "--stuck", "--list",
            "--controller=localhost:9555",
            NULL
        };
        int argc = sizeof(argv) / sizeof(char *) - 1;

        success = options->readOptions(&argc, (char**)argv);
        S9S_VERIFY(!success);

        S9sOptions::uninit();
    }

    return true;
}

bool
UtS9sOptions::testSetNodes()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  nodes;
    S9sVariantMap   theMap;
    bool            success;

    success = options->setNodes(
            "mongos://192.168.30.10,mongocfg://192.168.30.10,192.168.30.11,"
            "192.168.30.12?rs=replset2");

    S9S_VERIFY(success);
    nodes = options->nodes();
    S9S_COMPARE(nodes.size(), 4);
    
    theMap = nodes[0].toVariantMap();
    S9S_WARNING("-> %s", STR(theMap.toString()));
    S9S_COMPARE(theMap["hostname"], "192.168.30.10");
    S9S_COMPARE(theMap["protocol"], "mongos");
    
    theMap = nodes[1].toVariantMap();
    S9S_WARNING("-> %s", STR(theMap.toString()));
    S9S_COMPARE(theMap["hostname"], "192.168.30.10");
    S9S_COMPARE(theMap["protocol"], "mongocfg");
    
    theMap = nodes[2].toVariantMap();
    S9S_WARNING("-> %s", STR(theMap.toString()));
    S9S_COMPARE(theMap["hostname"], "192.168.30.11");
    
    theMap = nodes[3].toVariantMap();
    S9S_WARNING("-> %s", STR(theMap.toString()));
    S9S_COMPARE(theMap["hostname"], "192.168.30.12");
    S9S_COMPARE(theMap["rs"],       "replset2");

    return true;
}

bool
UtS9sOptions::testPerconaProCluster()
{
    S9sOptions *options = S9sOptions::instance();
    const char *argv[] = { "/bin/s9s",
                           "cluster",
                           "--create",
                           "--cluster-type=mysqlreplication",
                           "--nodes=10.63.201.251",
                           "--vendor=perconapro",
                           "--provider-version=8.0",
                           "--percona-client-id=123",
                           "--percona-pro-token=protoken",
                           NULL };
    int argc = sizeof(argv) / sizeof(char *) - 1;
    S9S_VERIFY(options->readOptions(&argc, (char **)argv));

    S9S_COMPARE(options->m_operationMode, S9sOptions::Cluster);
    S9S_COMPARE(options->vendor(), "perconapro");
    S9S_VERIFY(options->hasPerconaProToken());
    S9S_VERIFY(options->hasPerconaClientId());
    S9S_COMPARE(options->perconaProToken(), "protoken");
    S9S_COMPARE(options->perconaClientId(), "123");
    S9S_VERIFY(!options->hasOsElevation());

    S9sOptions::uninit();
    return true;
}

bool
UtS9sOptions::testPostgreSqlReplication()
{
    S9sOptions *options = S9sOptions::instance();

    // --add-publication
    const char *argv1[] = { "/bin/s9s",
                            "cluster",
                            "--add-publication",
                            "--cluster-id=42",
                            "--db-name=mydb",
                            "--include-all-tables",
                            "--pub-name=mypub",
                            "--subcluster-name=psql2",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isAddPublicationRequested());
    S9S_VERIFY(options->hasClusterIdOption());
    S9S_COMPARE(options->clusterId(), 42);
    S9S_COMPARE(options->dbName(), "mydb");
    S9S_COMPARE(options->publicationName(), "mypub");
    S9S_COMPARE(options->subClusterName(), "psql2");
    S9S_COMPARE(options->includeAllTables(), true);

    // --drop-publication
    const char *argv2[] = { "/bin/s9s",
                            "cluster",
                            "--drop-publication",
                            "--cluster-id=42",
                            "--pub-name=mypub",
                            "--db-name=mydb",
                            "--subcluster-id=44",
                            nullptr };
    int         argc2   = sizeof(argv2) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc2, (char **)argv2));
    S9S_VERIFY(options->isDropPublicationRequested());
    S9S_VERIFY(options->hasClusterIdOption());
    S9S_COMPARE(options->clusterId(), 42);
    S9S_COMPARE(options->dbName(), "mydb");
    S9S_COMPARE(options->publicationName(), "mypub");
    S9S_COMPARE(options->subClusterId(), 44);

    // --list-publications
    const char *argv3[] = { "/bin/s9s",
                            "cluster",
                            "--list-publications",
                            nullptr };
    int argc3 = sizeof(argv3) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc3, (char **)argv3));
    S9S_VERIFY(options->isListPublicationsRequested());

    // --add-subscription
    const char *argv4[] = { "/bin/s9s",
                            "cluster",
                            "--add-subscription",
                            "--cluster-id=43",
                            "--db-name=mydb",
                            "--pub-name=pub1",
                            "--sub-name=sub1",
                            "--subcluster-id=99",
                            nullptr };
    int         argc4   = sizeof(argv4) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc4, (char **)argv4));
    S9S_VERIFY(options->isAddSubscriptionRequested());
    S9S_VERIFY(options->hasClusterIdOption());
    S9S_COMPARE(options->clusterId(), 43);
    S9S_COMPARE(options->dbName(), "mydb");
    S9S_COMPARE(options->publicationName(), "pub1");
    S9S_COMPARE(options->subscriptionName(), "sub1");
    S9S_COMPARE(options->subClusterId(), 99);

    // --drop-subscription
    const char *argv5[] = { "/bin/s9s",
                            "cluster",
                            "--drop-subscription",
                            "--cluster-id=46",
                            "--db-name=mydb",
                            "--sub-name=mysub",
                            nullptr };
    int         argc5   = sizeof(argv5) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc5, (char **)argv5));
    S9S_VERIFY(options->isDropSubscriptionRequested());
    S9S_VERIFY(options->hasClusterIdOption());
    S9S_COMPARE(options->clusterId(), 46);
    S9S_COMPARE(options->dbName(), "mydb");
    S9S_COMPARE(options->subscriptionName(), "mysub");

    // --subcluster-id validation
    const char *argv6[] = { "/bin/s9s",
                            "cluster",
                            "--drop-subscription",
                            "--subcluster-id=xxx",
                            nullptr };
    int         argc6   = sizeof(argv6) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc6, (char **)argv6));
    S9S_COMPARE(options->subClusterId(), S9S_INVALID_CLUSTER_ID);

    S9sOptions::uninit();
    return true;
}

bool
UtS9sOptions::testAuditLogEventData()
{
    S9sOptions *options = S9sOptions::instance();
    
    // Test with audit log event data
    const char *argv1[] = { "/bin/s9s",
                            "cluster",
                            "--setup-audit-logging",
                            "--audit-log-events-data=SELECT,INSERT,UPDATE,DELETE",
                            "--cluster-id=1",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isSetupAuditLoggingRequested());
    S9S_COMPARE(options->auditLogEventData(), "SELECT,INSERT,UPDATE,DELETE");

    // Test without audit log event data
    const char *argv2[] = { "/bin/s9s",
                            "cluster",
                            "--setup-audit-logging",
                            "--cluster-id=1",
                            nullptr };
    int         argc2   = sizeof(argv2) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc2, (char **)argv2));
    S9S_VERIFY(options->isSetupAuditLoggingRequested());
    S9S_COMPARE(options->auditLogEventData(), "");

    // Test with empty audit log event data
    const char *argv3[] = { "/bin/s9s",
                            "cluster",
                            "--setup-audit-logging",
                            "--audit-log-events-data=",
                            "--cluster-id=1",
                            nullptr };
    int         argc3   = sizeof(argv3) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc3, (char **)argv3));
    S9S_VERIFY(options->isSetupAuditLoggingRequested());
    S9S_COMPARE(options->auditLogEventData(), "");

    S9sOptions::uninit();
    return true;
}

bool
UtS9sOptions::testPostgreSqlBackupOptions()
{
    S9sOptions *options = S9sOptions::instance();

    // Test all PostgreSQL backup options
    const char *argv1[] = { "/bin/s9s",
                            "backup",
                            "--create",
                            "--cluster-id=1",
                            "--backup-method=pgdump",
                            "--databases=app_db",
                            "--schemas=public,app_data",
                            "--exclude-schemas=temp,audit",
                            "--schema-only",
                            "--data-only",
                            "--no-owner",
                            "--no-privileges",
                            "--backup-format=custom",
                            nullptr };
    int argc1 = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    
    // Check basic operation
    S9S_COMPARE(options->m_operationMode, S9sOptions::Backup);
    S9S_VERIFY(options->isCreateRequested());
    
    // Check PostgreSQL-specific options
    S9S_COMPARE(options->backupMethod(), "pgdump");
    S9S_COMPARE(options->databases(), "app_db");
    S9S_COMPARE(options->schemas(), "public,app_data");
    S9S_COMPARE(options->excludeSchemas(), "temp,audit");
    S9S_VERIFY(options->schemaOnly());
    S9S_VERIFY(options->dataOnly());
    S9S_VERIFY(options->noOwner());
    S9S_VERIFY(options->noPrivileges());
    S9S_COMPARE(options->backupFormat(), "custom");

    // Test with only schema inclusion
    const char *argv2[] = { "/bin/s9s",
                            "backup",
                            "--create",
                            "--cluster-id=1",
                            "--backup-method=pgdump",
                            "--databases=test_db",
                            "--schemas=schema1,schema2",
                            nullptr };
    int argc2 = sizeof(argv2) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc2, (char **)argv2));
    
    S9S_COMPARE(options->schemas(), "schema1,schema2");
    S9S_COMPARE(options->excludeSchemas(), "");
    S9S_VERIFY(!options->schemaOnly());
    S9S_VERIFY(!options->dataOnly());
    
    // Test with only schema exclusion
    const char *argv3[] = { "/bin/s9s",
                            "backup",
                            "--create",
                            "--cluster-id=1",
                            "--backup-method=pgdump",
                            "--databases=prod_db",
                            "--exclude-schemas=temp,logs",
                            "--no-owner",
                            nullptr };
    int argc3 = sizeof(argv3) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc3, (char **)argv3));
    
    S9S_COMPARE(options->schemas(), "");
    S9S_COMPARE(options->excludeSchemas(), "temp,logs");
    S9S_VERIFY(options->noOwner());
    S9S_VERIFY(!options->noPrivileges());
    
    // Test structure-only backup
    const char *argv4[] = { "/bin/s9s",
                            "backup",
                            "--create",
                            "--cluster-id=1",
                            "--backup-method=pgdump",
                            "--databases=dev_db",
                            "--schema-only",
                            "--backup-format=plain",
                            nullptr };
    int argc4 = sizeof(argv4) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc4, (char **)argv4));
    
    S9S_VERIFY(options->schemaOnly());
    S9S_VERIFY(!options->dataOnly());
    S9S_COMPARE(options->backupFormat(), "plain");
    
    // Test data-only backup
    const char *argv5[] = { "/bin/s9s",
                            "backup",
                            "--create",
                            "--cluster-id=1",
                            "--backup-method=pgdump",
                            "--databases=staging_db",
                            "--data-only",
                            "--backup-format=directory",
                            nullptr };
    int argc5 = sizeof(argv5) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc5, (char **)argv5));
    
    S9S_VERIFY(!options->schemaOnly());
    S9S_VERIFY(options->dataOnly());
    S9S_COMPARE(options->backupFormat(), "directory");

    S9sOptions::uninit();
    return true;
}

bool
UtS9sOptions::testExternalBackup()
{
    S9sOptions *options = S9sOptions::instance();

    // Test with external backup path
    const char *argv1[]
            = { "/bin/s9s",
                "backup",
                "--restore",
                "--backup-source-address=10.16.186.1",
                "--backup-path=/backup/backup-full.xbstream.gz",
                "--backup-method=xtrabackupfull",
                "--nodes=10.16.186.175:3306",
                "--cluster-id=1",
                nullptr };
    int argc1 = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_COMPARE(options->backupMethod(), "xtrabackupfull");
    S9S_COMPARE(options->backupSourceAddress(), "10.16.186.1");
    S9S_COMPARE(options->backupPath(), "/backup/backup-full.xbstream.gz");

    S9sOptions::uninit();
    return true;
}

bool
UtS9sOptions::testAddController()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "pool-controllers",
                            "--add-controller",
                            "--nodes=10.16.186.1",
                            "--provider-version=2.3.4-17176",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_COMPARE(options->nodes().size(), 1);
    S9S_COMPARE(options->nodes()[0].toNode().hostName(), ("10.16.186.1"));
    S9S_COMPARE(options->providerVersion(), "2.3.4-17176");

    S9sOptions::uninit();
    return true;
}

/**
 * Testing the "pool-controllers --add-db" option (joins a bare host into
 * the pool's cmon DB HA InnoDB Cluster via the addCmonDbInstance job).
 */
bool
UtS9sOptions::testAddDb()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "pool-controllers",
                            "--add-db",
                            "--nodes=10.16.186.1:3306",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isAddDb());
    S9S_COMPARE(options->nodes().size(), 1);
    S9S_COMPARE(options->nodes()[0].toNode().hostName(), ("10.16.186.1"));
    S9S_COMPARE(options->nodes()[0].toNode().port(), 3306);
    S9S_VERIFY(!options->getBool("force"));

    S9sOptions::uninit();

    // --force must also be accepted and readable via the generic
    // getBool("force") accessor, the same way addNewCmonDbInstance() reads
    // it when building the job's job_data.
    const char *argv2[] = { "/bin/s9s",
                            "pool-controllers",
                            "--add-db",
                            "--nodes=10.16.186.1:3306",
                            "--force",
                            nullptr };
    int         argc2   = sizeof(argv2) / sizeof(char *) - 1;

    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc2, (char **)argv2));
    S9S_VERIFY(options->isAddDb());
    S9S_VERIFY(options->getBool("force"));

    S9sOptions::uninit();
    return true;
}

bool
UtS9sOptions::testDeleteDb()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "pool-controllers",
                            "--delete-db",
                            "--nodes=10.16.186.1:3306",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isDeleteDb());
    S9S_COMPARE(options->nodes().size(), 1);
    S9S_COMPARE(options->nodes()[0].toNode().hostName(), ("10.16.186.1"));
    S9S_COMPARE(options->nodes()[0].toNode().port(), 3306);
    S9S_VERIFY(!options->getBool("force"));

    S9sOptions::uninit();

    // --force must also be accepted and readable via the generic
    // getBool("force") accessor, the same way deleteCmonDbInstance() reads
    // it when building the job's job_data.
    const char *argv2[] = { "/bin/s9s",
                            "pool-controllers",
                            "--delete-db",
                            "--nodes=10.16.186.1:3306",
                            "--force",
                            nullptr };
    int         argc2   = sizeof(argv2) / sizeof(char *) - 1;

    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc2, (char **)argv2));
    S9S_VERIFY(options->isDeleteDb());
    S9S_VERIFY(options->getBool("force"));

    S9sOptions::uninit();

    // --node is an alternative to --nodes, identifying the pool DB HA node
    // by hostname/IP alone, without needing a port.
    const char *argv3[] = { "/bin/s9s",
                            "pool-controllers",
                            "--delete-db",
                            "--node=10.16.186.1",
                            nullptr };
    int         argc3   = sizeof(argv3) / sizeof(char *) - 1;

    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc3, (char **)argv3));
    S9S_VERIFY(options->isDeleteDb());
    S9S_VERIFY(options->hasNodeOption());
    S9S_COMPARE(options->node(), "10.16.186.1");
    S9S_VERIFY(options->nodes().empty());

    S9sOptions::uninit();
    return true;
}

bool
UtS9sOptions::testListDb()
{
    S9sOptions *options = S9sOptions::instance();

    // --list-db is a read-only query: no --nodes required, unlike
    // --add-db/--delete-db.
    const char *argv1[] = { "/bin/s9s",
                            "pool-controllers",
                            "--list-db",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isListDb());

    S9sOptions::uninit();
    return true;
}

/**
 * Testing "pool-controllers --add-services-bundle" (the addServicesBundle job):
 * one node with an optional web port, --site, --force, --no-install.
 */
bool
UtS9sOptions::testAddServicesBundle()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "pool-controllers",
                            "--add-services-bundle",
                            "--nodes=10.0.0.12:8443",
                            "--site=site-b",
                            "--force",
                            "--no-install",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isAddServicesBundle());
    S9S_VERIFY(!options->isDeleteServicesBundle());
    S9S_COMPARE(options->nodes().size(), 1);
    S9S_COMPARE(options->nodes()[0].toNode().hostName(), "10.0.0.12");
    S9S_COMPARE(options->nodes()[0].toNode().port(), 8443);
    S9S_COMPARE(options->site(), "site-b");
    S9S_VERIFY(options->getBool("force"));
    S9S_VERIFY(options->noInstall());

    S9sOptions::uninit();
    return true;
}

bool
UtS9sOptions::testDeleteServicesBundle()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "pool-controllers",
                            "--delete-services-bundle",
                            "--nodes=10.0.0.12",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isDeleteServicesBundle());
    S9S_COMPARE(options->nodes()[0].toNode().hostName(), "10.0.0.12");
    S9S_VERIFY(!options->getBool("force"));

    S9sOptions::uninit();
    return true;
}

/**
 * --list-services-bundles is read-only: no --nodes, --long and --print-json apply.
 */
bool
UtS9sOptions::testListServicesBundles()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "pool-controllers",
                            "--list-services-bundles",
                            "--long",
                            "--print-json",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isListServicesBundles());
    S9S_VERIFY(options->isLongRequested());
    S9S_VERIFY(options->isJsonRequested());

    S9sOptions::uninit();
    return true;
}

/**
 * The invalid combinations: --site without an add option, a missing or a
 * second --nodes host, --os-* with a services bundle job, two main options.
 */
bool
UtS9sOptions::testServicesBundleOptionErrors()
{
    S9sOptions *options = S9sOptions::instance();

    const char *siteAlone[] = { "/bin/s9s", "pool-controllers", "--list-services-bundles",
                                "--site=site-b", nullptr };
    const char *noNodes[] = { "/bin/s9s", "pool-controllers", "--add-services-bundle", nullptr };
    const char *twoNodes[] = { "/bin/s9s", "pool-controllers", "--delete-services-bundle",
                               "--nodes=10.0.0.12;10.0.0.13", nullptr };
    const char *withKey[] = { "/bin/s9s", "pool-controllers", "--add-services-bundle",
                              "--nodes=10.0.0.12", "--os-key-file=/root/.ssh/id_rsa", nullptr };
    const char *withUser[] = { "/bin/s9s", "pool-controllers", "--delete-services-bundle",
                               "--nodes=10.0.0.12", "--os-user=root", nullptr };
    const char *twoMains[] = { "/bin/s9s", "pool-controllers", "--add-services-bundle",
                               "--list-services-bundles", "--nodes=10.0.0.12", nullptr };
    const char *badSite[] = { "/bin/s9s", "pool-controllers", "--add-services-bundle",
                              "--nodes=10.0.0.12", "--site=b;id", nullptr };
    const char *spaceSite[] = { "/bin/s9s", "pool-controllers", "--add-controller",
                                "--nodes=10.0.0.13", "--site=site b", nullptr };
    const char *emptySite[] = { "/bin/s9s", "pool-controllers", "--add-services-bundle",
                                "--nodes=10.0.0.12", "--site=", nullptr };

    for (const char **argv : { siteAlone, noNodes, twoNodes, withKey, withUser, twoMains,
                               badSite, spaceSite, emptySite })
    {
        int argc = 0;
        while (argv[argc] != nullptr)
            ++argc;

        S9sOptions::uninit();
        options = S9sOptions::instance();
        S9S_VERIFY(!options->readOptions(&argc, (char **)argv));
        S9S_COMPARE(options->exitStatus(), S9sOptions::BadOptions);
    }

    S9sOptions::uninit();
    options = S9sOptions::instance();
    int argc = 5;
    S9S_VERIFY(!options->readOptions(&argc, (char **)withKey));
    S9S_VERIFY(options->errorString().contains("stored credentials"));

    S9sOptions::uninit();
    return true;
}

/**
 * --site is accepted with --add-controller too.
 */
bool
UtS9sOptions::testAddControllerSite()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "pool-controllers",
                            "--add-controller",
                            "--nodes=10.16.186.1",
                            "--site=site-b",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isAddController());
    S9S_COMPARE(options->site(), "site-b");

    S9sOptions::uninit();
    return true;
}

/**
 * Testing the --add-openbao option and its parameters on the pool-controllers
 * subcommand.
 *
 * The version and the listener port are deliberately not openbao-specific
 * options: they ride --provider-version and the node specification, the same way
 * --add-controller takes them.
 */
bool
UtS9sOptions::testAddOpenBao()
{
    S9sOptions *options;

    // Every parameter given.
    const char *argv1[] = { "/bin/s9s",
                            "pool-controllers",
                            "--add-openbao",
                            "--nodes=10.16.186.1:8300",
                            "--provider-version=2.5.4",
                            "--openbao-mount=clustercontrol",
                            "--openbao-namespace=tenant1",
                            "--openbao-force-reinit",
                            "--no-install",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isAddOpenBao());
    S9S_COMPARE(options->nodes().size(), 1);
    S9S_COMPARE(options->nodes()[0].toNode().hostName(), "10.16.186.1");
    S9S_COMPARE(options->nodes()[0].toNode().port(), 8300);
    S9S_COMPARE(options->providerVersion(), "2.5.4");
    S9S_COMPARE(options->openBaoMount(), "clustercontrol");
    S9S_COMPARE(options->openBaoNamespace(), "tenant1");
    S9S_VERIFY(options->openBaoForceReinit());
    S9S_VERIFY(options->noInstall());

    // No parameter given: the controller's own defaults must be used, so
    // nothing is set here.
    const char *argv2[] = { "/bin/s9s",
                            "pool-controllers",
                            "--add-openbao",
                            "--nodes=10.16.186.1",
                            nullptr };
    int         argc2   = sizeof(argv2) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc2, (char **)argv2));
    S9S_VERIFY(options->isAddOpenBao());
    S9S_VERIFY(!options->hasOpenBaoOption());
    S9S_COMPARE(options->providerVersion(""), "");
    S9S_COMPARE(options->openBaoMount(), "");
    S9S_VERIFY(!options->openBaoForceReinit());
    S9S_VERIFY(!options->noInstall());

    // The host is mandatory.
    const char *argv3[] = { "/bin/s9s",
                            "pool-controllers",
                            "--add-openbao",
                            nullptr };
    int         argc3   = sizeof(argv3) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc3, (char **)argv3));

    // The parameters are meaningless without --add-openbao.
    const char *argv4[] = { "/bin/s9s",
                            "pool-controllers",
                            "--list",
                            "--openbao-mount=clustercontrol",
                            nullptr };
    int         argc4   = sizeof(argv4) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc4, (char **)argv4));

    S9sOptions::uninit();
    return true;
}

/**
 * Testing the two read-only OpenBao options on the pool-controllers
 * subcommand: --list-config-storage and --list-openbao-versions.
 *
 * Both take no arguments and neither needs --nodes: they ask the controller
 * what it already knows, which is the point of having them before a host is
 * chosen.
 */
bool
UtS9sOptions::testListOpenBaoOperations()
{
    S9sOptions *options;

    const char *argv1[] = { "/bin/s9s",
                            "pool-controllers",
                            "--list-config-storage",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isListConfigStorage());
    S9S_VERIFY(!options->isListOpenBaoVersions());
    S9S_VERIFY(!options->isAddOpenBao());

    const char *argv2[] = { "/bin/s9s",
                            "pool-controllers",
                            "--list-openbao-versions",
                            nullptr };
    int         argc2   = sizeof(argv2) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc2, (char **)argv2));
    S9S_VERIFY(options->isListOpenBaoVersions());
    S9S_VERIFY(!options->isListConfigStorage());
    S9S_VERIFY(!options->isAddOpenBao());

    // Neither is set when another operation was asked for, so the dispatch
    // cannot fall into a listing by accident.
    const char *argv3[] = { "/bin/s9s",
                            "pool-controllers",
                            "--add-openbao",
                            "--nodes=10.16.186.1",
                            nullptr };
    int         argc3   = sizeof(argv3) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc3, (char **)argv3));
    S9S_VERIFY(!options->isListConfigStorage());
    S9S_VERIFY(!options->isListOpenBaoVersions());

    // They are operations in their own right, so asking for two at once is a
    // bad command line rather than a silent precedence.
    const char *argv4[] = { "/bin/s9s",
                            "pool-controllers",
                            "--list-config-storage",
                            "--list-openbao-versions",
                            nullptr };
    int         argc4   = sizeof(argv4) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc4, (char **)argv4));

    return true;
}

/**
 * Testing the staged pool mode options on the pool-controllers subcommand:
 * --bootstrap-db and --pool-readiness, plus the
 * --no-require-db-cluster/--no-require-config-storage opt-outs of
 * --set-pool-mode.
 */
bool
UtS9sOptions::testPoolModePrerequisites()
{
    S9sOptions *options;

    // --bootstrap-db is a job acting on the local controller only: no
    // --nodes, and the usual job options apply.
    const char *argv1[] = { "/bin/s9s",
                            "pool-controllers",
                            "--bootstrap-db",
                            "--log",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isBootstrapDb());
    S9S_VERIFY(options->isLogRequested());
    S9S_VERIFY(!options->isPoolReadiness());

    const char *argv2[] = { "/bin/s9s",
                            "pool-controllers",
                            "--migrate-db",
                            nullptr };
    int         argc2   = sizeof(argv2) / sizeof(char *) - 1;

    // --migrate-db is gone: cmon's DB is migrated to MySQL manually.
    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc2, (char **)argv2));

    const char *argv3[] = { "/bin/s9s",
                            "pool-controllers",
                            "--pool-readiness",
                            "--print-json",
                            nullptr };
    int         argc3   = sizeof(argv3) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc3, (char **)argv3));
    S9S_VERIFY(options->isPoolReadiness());
    S9S_VERIFY(options->isJsonRequested());
    S9S_VERIFY(!options->isSetPoolModeRequested());

    // They are main options: two at once is a bad command line.
    const char *argv4[] = { "/bin/s9s",
                            "pool-controllers",
                            "--bootstrap-db",
                            "--set-pool-mode",
                            nullptr };
    int         argc4   = sizeof(argv4) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc4, (char **)argv4));

    // The opt-outs belong to --set-pool-mode.
    const char *argv5[] = { "/bin/s9s",
                            "pool-controllers",
                            "--set-pool-mode",
                            "--no-require-db-cluster",
                            "--no-require-config-storage",
                            nullptr };
    int         argc5   = sizeof(argv5) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc5, (char **)argv5));
    S9S_VERIFY(options->isSetPoolModeRequested());
    S9S_VERIFY(options->noRequireDbCluster());
    S9S_VERIFY(options->noRequireConfigStorage());

    const char *argv6[] = { "/bin/s9s",
                            "pool-controllers",
                            "--set-pool-mode",
                            nullptr };
    int         argc6   = sizeof(argv6) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc6, (char **)argv6));
    S9S_VERIFY(!options->noRequireDbCluster());
    S9S_VERIFY(!options->noRequireConfigStorage());

    // ... and are rejected with anything else, even the readiness check.
    const char *argv7[] = { "/bin/s9s",
                            "pool-controllers",
                            "--pool-readiness",
                            "--no-require-db-cluster",
                            nullptr };
    int         argc7   = sizeof(argv7) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc7, (char **)argv7));

    const char *argv8[] = { "/bin/s9s",
                            "pool-controllers",
                            "--unset-pool-mode",
                            "--no-require-config-storage",
                            nullptr };
    int         argc8   = sizeof(argv8) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc8, (char **)argv8));

    S9sOptions::uninit();
    return true;
}

/**
 * Testing the --configure-wal option with --archive-mode and --summarize-wal.
 */
bool
UtS9sOptions::testConfigureWalOptions()
{
    S9sOptions *options;

    // Test --configure-wal with all WAL options
    const char *argv1[] = {
        "/bin/s9s", "node",
        "--configure-wal",
        "--cluster-id=1",
        "--nodes=pghost:5432",
        "--archive-mode=on",
        "--summarize-wal=on",
        nullptr
    };
    int argc1 = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));

    S9S_COMPARE(options->m_operationMode, S9sOptions::Node);
    S9S_VERIFY(options->isConfigureWal());
    S9S_COMPARE(options->clusterId(), 1);
    S9S_COMPARE(options->archiveMode(), "on");
    S9S_COMPARE(options->summarizeWal(), "on");

    S9sVariantList nodes = options->nodes();
    S9S_COMPARE(nodes.size(), 1);
    S9S_COMPARE(nodes[0].toNode().hostName(), "pghost");
    S9S_COMPARE(nodes[0].toNode().port(), 5432);

    // Test --configure-wal with summarize-wal=off
    const char *argv2[] = {
        "/bin/s9s", "node",
        "--configure-wal",
        "--cluster-id=2",
        "--nodes=dbhost:5433",
        "--archive-mode=always",
        "--summarize-wal=off",
        nullptr
    };
    int argc2 = sizeof(argv2) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc2, (char **)argv2));

    S9S_VERIFY(options->isConfigureWal());
    S9S_COMPARE(options->clusterId(), 2);
    S9S_COMPARE(options->archiveMode(), "always");
    S9S_COMPARE(options->summarizeWal(), "off");

    // Test --configure-wal without optional archive-mode/summarize-wal
    // should fail since at least one of them is required.
    const char *argv3[] = {
        "/bin/s9s", "node",
        "--configure-wal",
        "--cluster-id=3",
        "--nodes=host1:5432",
        nullptr
    };
    int argc3 = sizeof(argv3) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc3, (char **)argv3));

    // Test --configure-wal without --cluster-id should fail.
    const char *argv4[] = {
        "/bin/s9s", "node",
        "--configure-wal",
        "--nodes=host1:5432",
        "--summarize-wal=on",
        nullptr
    };
    int argc4 = sizeof(argv4) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc4, (char **)argv4));

    // Test --configure-wal without --nodes should fail.
    const char *argv5[] = {
        "/bin/s9s", "node",
        "--configure-wal",
        "--cluster-id=3",
        "--summarize-wal=on",
        nullptr
    };
    int argc5 = sizeof(argv5) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc5, (char **)argv5));

    S9sOptions::uninit();
    return true;
}

/**
 * Testing that --restore-cluster-info rejects --cluster-id: the cluster ID
 * is stored in the archive, and passing --cluster-id has no state on the
 * controller in which the restore can succeed.
 */
bool
UtS9sOptions::testRestoreClusterInfoOptions()
{
    S9sOptions *options = S9sOptions::instance();

    // --restore-cluster-info without --cluster-id should succeed.
    const char *argv1[] = {
        "/bin/s9s", "backup",
        "--restore-cluster-info",
        "--input-file=/tmp/cluster-1.tar.gz",
        nullptr
    };
    int argc1 = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));

    // --restore-cluster-info with --cluster-id should fail.
    const char *argv2[] = {
        "/bin/s9s", "backup",
        "--restore-cluster-info",
        "--input-file=/tmp/cluster-1.tar.gz",
        "--cluster-id=1",
        nullptr
    };
    int argc2 = sizeof(argv2) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc2, (char **)argv2));

    S9sOptions::uninit();
    return true;
}

/**
 * Testing the --virtual-router-id option parsing and validation.
 */
bool
UtS9sOptions::testVirtualRouterId()
{
    S9sOptions *options;
    bool        success;

    // Test valid value (42)
    const char *argv1[] = {
        "/bin/s9s", "node",
        "--register",
        "--cluster-id=1",
        "--nodes=keepalived://1.2.3.4",
        "--virtual-router-id=42",
        nullptr
    };
    int argc1 = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    success = options->readOptions(&argc1, (char **)argv1);
    S9S_VERIFY(success);
    S9S_COMPARE(options->getInt("virtual_router_id"), 42);

    // Test boundary value: 1 (minimum)
    const char *argv2[] = {
        "/bin/s9s", "node",
        "--register",
        "--cluster-id=1",
        "--nodes=keepalived://1.2.3.4",
        "--virtual-router-id=1",
        nullptr
    };
    int argc2 = sizeof(argv2) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    success = options->readOptions(&argc2, (char **)argv2);
    S9S_VERIFY(success);
    S9S_COMPARE(options->getInt("virtual_router_id"), 1);

    // Test boundary value: 255 (maximum)
    const char *argv3[] = {
        "/bin/s9s", "node",
        "--register",
        "--cluster-id=1",
        "--nodes=keepalived://1.2.3.4",
        "--virtual-router-id=255",
        nullptr
    };
    int argc3 = sizeof(argv3) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    success = options->readOptions(&argc3, (char **)argv3);
    S9S_VERIFY(success);
    S9S_COMPARE(options->getInt("virtual_router_id"), 255);

    // Test out-of-range: 0 (below minimum)
    const char *argv4[] = {
        "/bin/s9s", "node",
        "--register",
        "--cluster-id=1",
        "--nodes=keepalived://1.2.3.4",
        "--virtual-router-id=0",
        nullptr
    };
    int argc4 = sizeof(argv4) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    success = options->readOptions(&argc4, (char **)argv4);
    S9S_VERIFY(!success);

    // Test out-of-range: 256 (above maximum)
    const char *argv5[] = {
        "/bin/s9s", "node",
        "--register",
        "--cluster-id=1",
        "--nodes=keepalived://1.2.3.4",
        "--virtual-router-id=256",
        nullptr
    };
    int argc5 = sizeof(argv5) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    success = options->readOptions(&argc5, (char **)argv5);
    S9S_VERIFY(!success);

    // Test non-integer value
    const char *argv6[] = {
        "/bin/s9s", "node",
        "--register",
        "--cluster-id=1",
        "--nodes=keepalived://1.2.3.4",
        "--virtual-router-id=abc",
        nullptr
    };
    int argc6 = sizeof(argv6) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    success = options->readOptions(&argc6, (char **)argv6);
    S9S_VERIFY(!success);

    S9sOptions::uninit();
    return true;
}

/**
 * Testing "s9s account --lock --account=USERNAME" parses correctly
 * (CLUS-7664).
 */
bool
UtS9sOptions::testLockAccount()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "account",
                            "--lock",
                            "--cluster-id=1",
                            "--account=joe",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isLockRequested());
    S9S_VERIFY(!options->isUnlockRequested());
    S9S_COMPARE(options->account().userName(), "joe");

    S9sOptions::uninit();
    return true;
}

/**
 * Testing "s9s account --unlock --account=USERNAME" parses correctly
 * (CLUS-7664).
 */
bool
UtS9sOptions::testUnlockAccount()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "account",
                            "--unlock",
                            "--cluster-id=1",
                            "--account=joe",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isUnlockRequested());
    S9S_VERIFY(!options->isLockRequested());
    S9S_COMPARE(options->account().userName(), "joe");

    S9sOptions::uninit();
    return true;
}

/**
 * Testing that "--lock" and "--unlock" given together are rejected the
 * same way every other pair of the account command's mutually exclusive
 * main options is (checkOptionsAccount() counts all main options and
 * refuses more than one with "The main options are mutually exclusive.")
 * (CLUS-7664).
 */
bool
UtS9sOptions::testLockUnlockMutualExclusion()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "account",
                            "--lock",
                            "--unlock",
                            "--cluster-id=1",
                            "--account=joe",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc1, (char **)argv1));
    S9S_COMPARE(options->errorString(), "The main options are mutually exclusive.");

    S9sOptions::uninit();
    return true;
}

/**
 * Testing that "--lock"/"--unlock" without "--account=" is rejected with
 * the account-name-is-not-provided check in checkOptionsAccount()
 * (CLUS-7664).
 */
bool
UtS9sOptions::testLockAccountMissingAccount()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "account",
                            "--lock",
                            "--cluster-id=1",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc1, (char **)argv1));
    S9S_COMPARE(options->errorString(), "Account name is not provided.");

    S9sOptions::uninit();
    return true;
}

/**
 * Testing "s9s account --update --account=USER:PASSWORD" parses correctly and
 * carries the new password.
 */
bool
UtS9sOptions::testUpdateAccount()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "account",
                            "--update",
                            "--cluster-id=1",
                            "--account=joe:n3wsecret",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isUpdateRequested());
    S9S_VERIFY(!options->isCreateRequested());
    S9S_COMPARE(options->account().userName(), "joe");
    S9S_COMPARE(options->account().password(), "n3wsecret");

    S9sOptions::uninit();
    return true;
}

/**
 * Testing that "--update" without "--account=" is rejected.
 */
bool
UtS9sOptions::testUpdateAccountMissingAccount()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "account",
                            "--update",
                            "--cluster-id=1",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc1, (char **)argv1));
    S9S_COMPARE(options->errorString(), "Account name is not provided.");

    S9sOptions::uninit();
    return true;
}

/**
 * Testing that "--update --account=USER" without a password is rejected rather
 * than sending an updateAccount request with no password in it.
 */
bool
UtS9sOptions::testUpdateAccountMissingPassword()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "account",
                            "--update",
                            "--cluster-id=1",
                            "--account=joe",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc1, (char **)argv1));
    S9S_COMPARE(
            options->errorString(),
            "The new password is not provided (--account=USER:PASSWORD).");

    S9sOptions::uninit();
    return true;
}

/**
 * Testing that "--update" is one of the account command's mutually exclusive
 * main options.
 */
bool
UtS9sOptions::testUpdateCreateMutualExclusion()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "account",
                            "--update",
                            "--create",
                            "--cluster-id=1",
                            "--account=joe:n3wsecret",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc1, (char **)argv1));
    S9S_COMPARE(options->errorString(), "The main options are mutually exclusive.");

    S9sOptions::uninit();
    return true;
}

bool
UtS9sOptions::testAddShard()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "cluster",
                            "--add-shard",
                            "--cluster-id=5",
                            "--nodes=clickhouse://10.0.2.11;clickhouse://10.0.2.12",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_VERIFY(options->isAddShardRequested());
    S9S_VERIFY(!options->isAddNodeRequested());
    S9S_COMPARE(options->nodes().size(), 2);

    const char *argv2[] = { "/bin/s9s",
                            "cluster",
                            "--add-shard",
                            "--add-node",
                            "--cluster-id=5",
                            "--nodes=clickhouse://10.0.2.11",
                            nullptr };
    int         argc2   = sizeof(argv2) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc2, (char **)argv2));
    S9S_COMPARE(options->errorString(), "The main options are mutually exclusive.");

    const char *argv3[] = { "/bin/s9s",
                            "cluster",
                            "--add-shard",
                            "--cluster-id=5",
                            "--nodes=clickhouse://10.0.2.11",
                            "--shard-id=3",
                            nullptr };
    int         argc3   = sizeof(argv3) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(!options->readOptions(&argc3, (char **)argv3));
    S9S_COMPARE(options->errorString(),
            "The --shard-id option can only be used with --add-node.");

    S9sOptions::uninit();
    return true;
}

bool
UtS9sOptions::testShardId()
{
    S9sOptions *options = S9sOptions::instance();

    const char *argv1[] = { "/bin/s9s",
                            "cluster",
                            "--add-node",
                            "--cluster-id=5",
                            "--nodes=clickhouse://10.0.2.13",
                            "--shard-id=2",
                            nullptr };
    int         argc1   = sizeof(argv1) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc1, (char **)argv1));
    S9S_COMPARE(options->shardId(), 2);

    const char *argv2[] = { "/bin/s9s",
                            "cluster",
                            "--add-node",
                            "--cluster-id=5",
                            "--nodes=clickhouse://10.0.2.13",
                            nullptr };
    int         argc2   = sizeof(argv2) / sizeof(char *) - 1;

    S9sOptions::uninit();
    options = S9sOptions::instance();
    S9S_VERIFY(options->readOptions(&argc2, (char **)argv2));
    S9S_COMPARE(options->shardId(), 0);

    const char *invalidValues[] = { "0", "-1", "abc", "2x", "" };
    for (const char *value : invalidValues)
    {
        S9sString   shardOption = S9sString("--shard-id=") + value;
        const char *argv3[] = { "/bin/s9s",
                                "cluster",
                                "--add-node",
                                "--cluster-id=5",
                                "--nodes=clickhouse://10.0.2.13",
                                STR(shardOption),
                                nullptr };
        int         argc3   = sizeof(argv3) / sizeof(char *) - 1;
        S9sString   expected;

        expected.sprintf(
                "The value '%s' is invalid for --shard-id, "
                "shards are numbered from 1.",
                value);

        S9sOptions::uninit();
        options = S9sOptions::instance();
        S9S_VERIFY(!options->readOptions(&argc3, (char **)argv3));
        S9S_COMPARE(options->errorString(), expected);
        S9S_COMPARE(options->exitStatus(), S9sOptions::BadOptions);
    }

    S9sOptions::uninit();
    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sOptions)
