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

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

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

    PERFORM_TEST(testCreate,        retval);

    return retval;
}

/**
 * Testing the constructors.
 */
bool
UtS9sNode::testCreate()
{
    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sNode)
