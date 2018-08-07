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

        const char *userColorBegin() ;
        const char *userColorEnd();

        const char *groupColorBegin(const S9sString &groupName = "");
        const char *groupColorEnd();

        const char *directoryColorBegin() const;
        const char *directoryColorEnd() const;

        const char *clusterStateColorBegin(const S9sString &state);
        const char *clusterStateColorEnd() const;

        const char *jobStateColorBegin(const S9sString &state);
        const char *jobStateColorEnd() const;

        const char *clusterColorBegin() const;
        const char *clusterColorEnd() const;

        const char *ipColorBegin(const S9sString &ip = "");
        const char *ipColorEnd(const S9sString &ip = "");

        const char *serverColorBegin();
        const char *serverColorEnd();

        const char *containerColorBegin(int stateAsChar);
        const char *containerColorEnd();

        S9sString bytesToHuman(ulonglong bytes) const;
        S9sString mBytesToHuman(ulonglong mBytes) const;
        S9sString kiloBytesToHuman(ulonglong kBytes) const;

        S9sString percent(const ulonglong total, const ulonglong part) const;
};
