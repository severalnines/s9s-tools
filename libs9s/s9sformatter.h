/* 
 * Copyright (C) 2016 severalnines.com
 */
#pragma once

#include "S9sString"

class S9sFormatter
{
    public:
        S9sString bytesToHuman(ulonglong mBytes);
        S9sString kiloBytesToHuman(ulonglong kBytes);
};
