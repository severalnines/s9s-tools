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
#include "ut_s9soptions.h"

#include "S9sOptions"
#include "S9sNode"

#include <cstdio>
#include <cstring>

#define DEBUG
#define WARNING
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
    PERFORM_TEST(testReadOptions01, retval);
    PERFORM_TEST(testReadOptions02, retval);
    PERFORM_TEST(testReadOptions03, retval);
    PERFORM_TEST(testReadOptions04, retval);

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
        "--rpc-token=the_token", "--color=always", "--verbose",
        NULL 
    };
    int   argc   = sizeof(argv) / sizeof(char *) - 1;


    success = options->readOptions(&argc, (char**)argv);
    S9S_VERIFY(success);
    
    S9S_COMPARE(options->binaryName(),          "s9s");
    S9S_COMPARE(options->m_operationMode,       S9sOptions::Node);
    S9S_COMPARE(options->controllerHostName(),  "localhost");
    S9S_COMPARE(options->controllerPort(),      9555);
    S9S_COMPARE(options->rpcToken(),            "the_token");
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
        "--rpc-token=the_token", "--color=always", "--verbose",
        NULL 
    };
    int   argc   = sizeof(argv) / sizeof(char *) - 1;


    success = options->readOptions(&argc, (char**)argv);
    S9S_VERIFY(success);
    
    S9S_COMPARE(options->binaryName(),          "s9s");
    S9S_COMPARE(options->m_operationMode,       S9sOptions::Job);
    S9S_COMPARE(options->controllerHostName(),  "localhost");
    S9S_COMPARE(options->controllerPort(),      9555);
    S9S_COMPARE(options->rpcToken(),            "the_token");
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
        "--rpc-token=the_token", "--cluster-type=Galera", 
        "--nodes=10.10.2.2;10.10.2.3;10.10.2.4;10.10.2.5",
        "--vendor=codership", "--provider-version=5.6", "--os-user=14j",
        "--wait", NULL 
    };
    int   argc   = sizeof(argv) / sizeof(char *) - 1;


    success = options->readOptions(&argc, (char**)argv);
    S9S_VERIFY(success);
    
    S9S_COMPARE(options->binaryName(),           "s9s");
    S9S_COMPARE(options->m_operationMode,        S9sOptions::Cluster);
    S9S_COMPARE(options->controllerHostName(),   "localhost");
    S9S_COMPARE(options->controllerPort(),       9555);
    S9S_COMPARE(options->rpcToken(),             "the_token");
    S9S_COMPARE(options->clusterType(),          "galera");
    S9S_COMPARE(options->vendor(),               "codership");
    S9S_COMPARE(options->providerVersion(),      "5.6");
    S9S_COMPARE(options->osUser(),               "14j");

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
        "job", "--list", "-t", "THE_TOKEN", NULL 
    };
    int   argc   = sizeof(argv) / sizeof(char *) - 1;


    success = options->readOptions(&argc, (char**)argv);
    S9S_VERIFY(success);
    
    S9S_COMPARE(options->binaryName(),     "s9s");
    S9S_COMPARE(options->m_operationMode,  S9sOptions::Job);
    S9S_COMPARE(options->configFile(),     "/home/johan/.s9s/s9s.conf");
    S9S_COMPARE(options->rpcToken(),       "THE_TOKEN");
    S9S_VERIFY(options->isListRequested());

    S9sOptions::uninit();
    return true;
}


S9S_UNIT_TEST_MAIN(UtS9sOptions)


