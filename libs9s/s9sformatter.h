/* 
 * Copyright (C) 2016 severalnines.com
 */
#pragma once

#include "S9sString"

class S9sFormatter
{
    public:
        S9sString bytesToHuman(ulonglong bytes) const;
        S9sString mBytesToHuman(ulonglong mBytes) const;
        S9sString kiloBytesToHuman(ulonglong kBytes) const;
};
