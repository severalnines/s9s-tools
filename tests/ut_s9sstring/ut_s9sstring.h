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
#include "s9sunittest.h"

class UtS9sString : public S9sUnitTest
{
    public:
        UtS9sString();
        virtual ~UtS9sString();
        virtual bool runTest(const char *testName = 0);
    
    protected:
        bool testCreate();
        bool testAssign();
        bool testUnQuote();
        bool testToUpper();
        bool testToInt();
        bool testReplace();
        bool testConcat();
        bool testStartsWith();
        bool testPrintf();
        bool testContains();
        bool testEscape();
        bool testSplit();
        bool testSizeString();
        bool testMilliseconds();
};

