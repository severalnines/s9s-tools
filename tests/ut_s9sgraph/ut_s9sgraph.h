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
#include "s9sunittest.h"

class UtS9sGraph : public S9sUnitTest
{
    public:
        UtS9sGraph();
        virtual ~UtS9sGraph();
        virtual bool runTest(const char *testName = 0);
    
    protected:
        bool testCreate01();
        bool testCreate02();
        bool testCreate03();
        bool testCreate04();
        bool testCreate05();
        bool testLabel01();
};

