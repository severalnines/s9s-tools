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

    return retval;
}

/**
 * Testing the constructor that parse the string representation.
 */
bool
UtS9sGraph::testCreate01()
{
    S9sGraph graph;

    for (double y = 0.0; y < 2.3; y += .75)
        graph.appendValue(y);
    
    for (double y = 2.3; y > 0.0; y -= .75)
        graph.appendValue(y);

    graph.appendValue(1.2);
    graph.appendValue(1.3);
    graph.appendValue(1.4);
    graph.appendValue(1.5);
    graph.appendValue(1.6);
    graph.appendValue(1.7);
    graph.appendValue(1.0);
    graph.appendValue(1.2);
    graph.appendValue(1.3);
    graph.appendValue(1.4);
    graph.appendValue(1.5);

    graph.transformWidth(12);

    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sGraph)
