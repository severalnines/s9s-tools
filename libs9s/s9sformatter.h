/* 
 * Copyright (C) 2016 severalnines.com
 */
#pragma once

#include "S9sString"

class S9sObject;
class S9sUser;
class S9sCluster;
class S9sNode;

class S9sFormatter
{
    public:
        bool useSyntaxHighLight() const;

        const char *headerColorBegin() const;
        const char *headerColorEnd() const;

        const char *userColorBegin() const;
        const char *userColorEnd() const;

        const char *groupColorBegin(const S9sString &groupName = "") const;
        const char *groupColorEnd() const;

        const char *directoryColorBegin() const;
        const char *directoryColorEnd() const;
        
        const char *folderColorBegin() const;
        const char *folderColorEnd() const;

        const char *clusterStateColorBegin(const S9sString &state);
        const char *clusterStateColorEnd() const;

        const char *jobStateColorBegin(const S9sString &state);
        const char *jobStateColorEnd() const;

        const char *clusterColorBegin() const;
        const char *clusterColorEnd() const;

        const char *ipColorBegin(const S9sString &ip = "") const;
        const char *ipColorEnd(const S9sString &ip = "") const;

        const char *fileColorBegin(const S9sString &fileName) const;
        const char *fileColorEnd() const;

        const char *serverColorBegin();
        const char *serverColorEnd();

        const char *containerColorBegin(int stateAsChar);
        const char *containerColorEnd();

        const char *typeColorBegin() const;
        const char *typeColorEnd() const;

        const char *greyColorBegin() const;
        const char *greyColorEnd() const;

        S9sString bytesToHuman(ulonglong bytes) const;
        S9sString mBytesToHuman(ulonglong mBytes) const;
        S9sString kiloBytesToHuman(ulonglong kBytes) const;

        S9sString percent(const ulonglong total, const ulonglong part) const;

        void printObjectStat(S9sObject &object) const;
        void printUserStat(S9sUser &user) const;
        void printNodeStat(S9sCluster &cluster, S9sNode &node) const;

        void printBackendServersSubList(const S9sNode &node) const;
};
