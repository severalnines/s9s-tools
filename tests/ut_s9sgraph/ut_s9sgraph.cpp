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
    PERFORM_TEST(testCreate04,      retval);
    PERFORM_TEST(testLabel01,       retval);

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
 
    printf("\n");
    graph.setTitle("y = sin(x) + 1 test");
    graph.realize();
    graph.print();
    printf("\n");

    return true;
}

bool
UtS9sGraph::testCreate02()
{
    S9sGraph graph;

    for (double x = 0.0; x < 5.0; x += .1)
        graph.appendValue(x * x);

    graph.appendValue(10.0);

    printf("\n");
    graph.setAggregateType(S9sGraph::Max);
    graph.realize();
    graph.print();
    printf("\n");
    
    printf("\n");
    graph.setAggregateType(S9sGraph::Min);
    graph.realize();
    graph.print();
    printf("\n");

    return true;
}

bool
UtS9sGraph::testCreate03()
{
    S9sGraph graph;

    graph.appendValue(1.0);
    graph.appendValue(2.0);
    graph.appendValue(4.0);
    
    printf("\n");
    graph.realize();
    graph.print();
    printf("\n");

    return true;
}

bool
UtS9sGraph::testCreate04()
{
    S9sGraph graph;

    for (double value = 0; value < 40; value += 1)
        graph.appendValue(value);
    
    printf("\n");
    graph.realize();
    graph.print();
    printf("\n");

    return true;
}

bool
UtS9sGraph::testLabel01()
{
    S9S_COMPARE(roundMultiple(14.8,  5.0), 15.0);
    S9S_COMPARE(roundMultiple(1.48,  0.5), 1.5);
    S9S_COMPARE(roundMultiple(1.443, 0.05), 1.45);
    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sGraph)
