/* 
 * Copyright (C) 2016 severalnines.com
 */
#pragma once

#include "s9sstring.h"

class S9sObject;
class S9sUser;
class S9sCluster;
class S9sNode;
class S9sServer;
class S9sContainer;

/**
 * An enum that controls the format while we print things.
 */
enum S9sFormatFlags
{
    /** Pure strings, e.g. strings we send on the network. */
    S9sFormatNormal   = 0,
    /** Indented srings for readability. */
    S9sFormatIndent   = 1,
    /** Colored text for readability. */
    S9sFormatColor    = 2,
    /** In the debug messages we always print colors and indent. */ 
    S9sFormatForDebug = 3,
};

inline S9sFormatFlags 
operator|(
        S9sFormatFlags a, 
        S9sFormatFlags b)
{
    return static_cast<S9sFormatFlags>(
            static_cast<int>(a) | 
            static_cast<int>(b));
};

/**
 * A class that helps printing formatted strings.
 */
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

        const char *clusterStateColorBegin(const S9sString &state) const;
        const char *clusterStateColorEnd() const;

        const char *jobStateColorBegin(const S9sString &state);
        const char *jobStateColorEnd() const;

        const char *clusterColorBegin() const;
        const char *clusterColorEnd() const;

        const char *ipColorBegin(const S9sString &ip = "") const;
        const char *ipColorEnd(const S9sString &ip = "") const;

        const char *fileColorBegin(const S9sString &fileName) const;
        const char *fileColorEnd() const;

        const char *serverColorBegin(int stateAsChar = '\0') const;
        const char *serverColorEnd() const;

        const char *hostStateColorBegin(const S9sString status) const;
        const char *hostStateColorEnd() const;

        const char *containerColorBegin(int stateAsChar = '\0') const;
        const char *containerColorEnd() const;

        const char *typeColorBegin() const;
        const char *typeColorEnd() const;

        const char *greyColorBegin() const;
        const char *greyColorEnd() const;

        const char *objectColorBegin(const S9sObject &object) const;
        const char *objectColorEnd() const;

        static S9sString bytesToHuman(ulonglong bytes);
        static S9sString mBytesToHuman(ulonglong mBytes);
        static S9sString kiloBytesToHuman(ulonglong kBytes);

        S9sString percent(const ulonglong total, const ulonglong part) const;

        void printObjectStat(const S9sObject &object) const;
        void printUserStat(const S9sUser &user) const;
        
        void printNodeStat(
                const S9sCluster &cluster, 
                const S9sNode    &node) const;

        void printServerStat(const S9sServer &server) const;
        void printControllerStat(const S9sServer &server) const;
        void printClusterStat(const S9sCluster &cluster) const;
        void printContainerStat(const S9sContainer &container) const;

        void printBackendServersSubList(const S9sNode &node) const;
        void printContainersCompact(const S9sVariantList &containers) const;
        void printHostTable(const S9sCluster &cluster) const;
        void printReplicationTable(const S9sCluster &cluster) const;
};
