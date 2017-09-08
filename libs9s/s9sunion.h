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
 * S9sTools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with S9sTools. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "s9sglobal.h"

class S9sVariantMap;
class S9sVariantList;
class S9sVariantArray;
class S9sString;
class S9sNode;
class S9sAccount;
class S9sContainer;

/** 
 * An enum to identify the basic types for S9s. Well, basic types are types that
 * we consider basic, some of them are quite complex.
 */
enum S9sBasicType 
{
    Invalid   = 0,
    Bool,
    Int,
    Ulonglong,
    Double,
    String,
    Node,
    Container,
    Account,
    Map,
    List
};

union S9sUnion 
{
    int               iVal;
    double            dVal;
    bool              bVal;
    ulonglong         ullVal;
    S9sVariantMap    *mapValue;
    S9sVariantList   *listValue;
    S9sVariantArray  *arrayValue;
    S9sString        *stringValue;
    S9sNode          *nodeValue;
    S9sContainer     *containerValue;
    S9sAccount       *accountValue;
};
