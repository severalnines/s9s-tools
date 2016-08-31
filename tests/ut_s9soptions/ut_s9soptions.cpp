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
#include <cstdio>
#include <cstring>



//#define DEBUG
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
    
    S9S_COMPARE(options->binaryName(),     "s9s");
    S9S_COMPARE(options->m_operationMode,  S9sOptions::Node);
    S9S_COMPARE(options->controller(),     "localhost");
    S9S_COMPARE(options->controllerPort(), 9555);
    S9S_COMPARE(options->rpcToken(),       "the_token");
    S9S_VERIFY(options->isListRequested());
    S9S_VERIFY(options->isVerbose());
    S9S_VERIFY(options->useSyntaxHighlight());

    S9sOptions::uninit();
    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sOptions)


