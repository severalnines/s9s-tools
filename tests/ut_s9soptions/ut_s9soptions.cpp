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
    PERFORM_TEST(testReadOptions02, retval);
    PERFORM_TEST(testReadOptions03, retval);
    PERFORM_TEST(testReadOptions04, retval);
    PERFORM_TEST(testReadOptions05, retval);
    PERFORM_TEST(testReadOptions06, retval);
    PERFORM_TEST(testReadOptions07, retval);
    PERFORM_TEST(testSetNodes,      retval);
    PERFORM_TEST(testPerconaProCluster, retval);
    PERFORM_TEST(testPostgreSqlReplication, retval);
    PERFORM_TEST(testPostgreSqlBackupOptions, retval);
    PERFORM_TEST(testAuditLogEventData, retval);
    PERFORM_TEST(testExternalBackup, retval);

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

S9S_UNIT_TEST_MAIN(UtS9sOptions)
