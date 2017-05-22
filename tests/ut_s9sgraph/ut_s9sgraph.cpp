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
#include "ut_s9sgraph.h"

#include "S9sGraph"

#include <math.h>

//#define DEBUG
#include "s9sdebug.h"

UtS9sGraph::UtS9sGraph()
{
}

UtS9sGraph::~UtS9sGraph()
{
}

bool
UtS9sGraph::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testCreate01,      retval);
    PERFORM_TEST(testCreate02,      retval);
    PERFORM_TEST(testCreate03,      retval);

    return retval;
}

/**
 * Testing the constructor that parse the string representation.
 */
bool
UtS9sGraph::testCreate01()
{
    S9sGraph graph;

    for (double x = 0.0; x < 6.28; x += .1)
        graph.appendValue(sin(x) + 1.0);
   
    graph.transform(40, 10);

    return true;
}

bool
UtS9sGraph::testCreate02()
{
    S9sGraph graph;

    for (double x = 0.0; x < 5.0; x += .1)
        graph.appendValue(x * x);

    graph.appendValue(10.0);

    graph.transform(40, 10);

    return true;
}

bool
UtS9sGraph::testCreate03()
{
    S9sGraph graph;

    graph.appendValue(3.0);
    graph.appendValue(4.0);
    graph.appendValue(5.0);
    
    graph.transform(40, 10);

    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sGraph)
