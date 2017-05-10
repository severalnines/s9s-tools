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

#include "S9sVariantMap"

class S9sNode;
class S9sCluster;
class S9sFormat;

class S9sRpcReply : public S9sVariantMap
{
    public:
        bool isOk() const;
        bool isAuthRequired() const;
        S9sString errorString() const;
        S9sString uuid() const;


        int jobId() const;
        S9sString jobTitle() const;
        bool isJobFailed() const;

        S9sString clusterName(const int clusterId);
        S9sString clusterStatusText(const int clusterId);

        bool progressLine(S9sString &retval, bool syntaxHighlight);

        void printMessages(const S9sString &defaultMessage);
        void printJobStarted();
        void printJobLog();
        void printClusterList();
        void printConfigList();
        void printLogList();
        void printNodeList();
        void printJobList();
        void printBackupList();
        void printUserList();
        void printMaintenanceList();
        void printMetaTypeList();
        void printMetaTypePropertyList();
        void printPing();
        void printProcessList();
        void printProcessListBrief(const int maxLines = -1);
        void printCpuStat();
        void printCpuStatLine1();
        void printMemoryStatLine1();
        void printMemoryStatLine2();

        void printScriptOutput();
        void printScriptBacktrace();
        void printScriptOutputBrief();

        void printScriptTree();
        void printScriptTreeBrief();
        
        void saveConfig(S9sString outputDir);


        void printScriptTreeBrief(
                S9sVariantMap        entry,
                int                  recursionLevel,
                S9sString            indentString,
                bool                 isLast);


        static S9sString progressBar(double percent, bool syntaxHighlight);
        static S9sString progressBar(bool syntaxHighlight);

        static bool useSyntaxHighLight() ;

        static const char *clusterStateColorBegin(S9sString state);
        static const char *clusterStateColorEnd();
        
        static const char *userColorBegin();
        static const char *userColorEnd();
        
        static const char *groupColorBegin(const S9sString &groupName);
        static const char *groupColorEnd();
        
    protected:
        S9sVariantMap clusterMap(const int clusterId);
        
    private:
        void printJobLogBrief();
        void printJobLogLong();
        
        void printLogBrief();

        void printConfigBrief();
        
        void printConfigBrief(
                S9sVariantMap   map, 
                S9sFormat      &sectionFormat,
                S9sFormat      &nameFormat,
                S9sFormat      &valueFormat,
                int             depth);

        void printConfigBriefWiden(
                S9sVariantMap   map, 
                S9sFormat      &sectionFormat,
                S9sFormat      &nameFormat,
                S9sFormat      &valueFormat,
                int            depth); 

        void printConfigLong();


        void printClusterListBrief();
        void printClusterListLong();
        void printClusterListStat();
        void printHostTable(S9sCluster &cluster);
        void printClusterStat(S9sCluster &cluster);
        
        void printNodeListStat();
        void printNodeListBrief();
        void printNodeListLong();
        void printNodeStat(S9sCluster &cluster, S9sNode &node);
        
        void printJobListBrief();
        void printJobListLong();
        
        void printBackupListBrief();
        void printBackupListLong();
        
        void printUserListBrief();
        void printUserListLong();
        
        void printMaintenanceListBrief();
        void printMaintenanceListLong();

        void printMetaTypeListLong();
        void printMetaTypeListBrief();
        
        void printMetaTypePropertyListLong();
        void printMetaTypePropertyListBrief();

        const char *optNameColorBegin() const;
        const char *optNameColorEnd() const;

        const char *clusterColorBegin() const;
        const char *clusterColorEnd() const;

        const char *headerColorBegin() const;
        const char *headerColorEnd() const;

        const char *typeColorBegin() const;
        const char *typeColorEnd() const;
        
        const char *fileColorBegin(const S9sString &fileName) const;
        const char *fileColorEnd() const;

        const char *greyColorBegin() const;
        const char *greyColorEnd() const;
};

