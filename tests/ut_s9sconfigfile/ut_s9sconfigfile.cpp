/*
 * Severalnines Tools
 * Copyright (C) 2018  Severalnines AB
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
#include "ut_s9sconfigfile.h"

#include "S9sConfigFile"
#include "S9sFile"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

UtS9sConfigFile::UtS9sConfigFile()
{
    S9S_DEBUG("");
}

UtS9sConfigFile::~UtS9sConfigFile()
{
}

bool
UtS9sConfigFile::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testParse,        retval);

    return retval;
}

/**
 * Parsing of external files. 
 */
bool
UtS9sConfigFile::testParse()
{
    S9sVariantList fileNames;
    bool           success;

    S9sFile::listFiles("s9s_configs", fileNames, true);
    for (uint idx = 0; idx < fileNames.size(); ++idx)
    {
        const S9sString &fileName = fileNames[idx].toString();

        if (fileName.endsWith(".swp"))
            continue;

        //S9S_WARNING("*** fileName : '%s'", STR(fileName));
        S9sFile       file(fileName);
        S9sConfigFile config;
        S9sString     content;

        success = file.readTxtFile(content);
        S9S_VERIFY(success);

        success = config.parse(STR(content));
        if (!success)
        {
            S9S_WARNING("\nERROR in %s: %s", 
                    STR(fileName),
                    STR(config.errorString()));
        }

        S9S_VERIFY(success);
    }

    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sConfigFile)
