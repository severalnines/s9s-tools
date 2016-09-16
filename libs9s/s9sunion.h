/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

#include "s9sglobal.h"

class S9sVariantMap;
class S9sVariantList;
class S9sVariantArray;
class S9sString;
class S9sNode;

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
};
