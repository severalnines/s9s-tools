/* 
 * Copyright (C) 2016 severalnines.com
 */
#pragma once

#include "S9sString"

class S9sFormatter
{
    public:
        bool useSyntaxHighLight() const;

        const char *headerColorBegin() const;
        const char *headerColorEnd() const;

        const char *directoryColorBegin() const;
        const char *directoryColorEnd() const;

        const char *clusterStateColorBegin(const S9sString &state);
        const char *clusterStateColorEnd() const;

        const char *clusterColorBegin() const;
        const char *clusterColorEnd() const;
 
        S9sString bytesToHuman(ulonglong bytes) const;
        S9sString mBytesToHuman(ulonglong mBytes) const;
        S9sString kiloBytesToHuman(ulonglong kBytes) const;

        S9sString percent(const ulonglong total, const ulonglong part) const;
};
