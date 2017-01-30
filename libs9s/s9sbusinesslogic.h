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

#include "S9sRpcClient"

/**
 * A class that is able to execute whatever the user requested through the
 * command line options.
 */
class S9sBusinessLogic
{
    public:
        void execute();

        void waitForJob(
                const int     clusterId,
                const int     jobId, 
                S9sRpcClient &client);

    protected:
        virtual void 
            jobRegistered(
                    S9sRpcClient &client,
                    const int     clusterId);


    private:
        void waitForJobWithProgress(
                const int     clusterId,
                const int     jobId, 
                S9sRpcClient &client);

        void waitForJobWithLog(
                const int     clusterId,
                const int     jobId, 
                S9sRpcClient &client);

        void executeUser(S9sRpcClient &client);

        void executeClusterCreate(S9sRpcClient &client);
        void executeClusterList(S9sRpcClient &client);
        void executePing(S9sRpcClient &client);

        void executeNodeList(S9sRpcClient &client);
        void executeNodeSet(S9sRpcClient &client);

        void executeTop(S9sRpcClient &client);
        void executeProcessList(S9sRpcClient &client);
        void executeBackupList(S9sRpcClient &client);
        void executeUserList(S9sRpcClient &client);

        void executeJobList(S9sRpcClient &client);
        void executeJobLog(S9sRpcClient &client);

        void executeRollingRestart(S9sRpcClient &client);
        void executeAddNode(S9sRpcClient &client);
        void executeRemoveNode(S9sRpcClient &client);
        void executeStopCluster(S9sRpcClient &client);
        void executeStartCluster(S9sRpcClient &client);
        void executeDropCluster(S9sRpcClient &client);
        void executeCreateAccount(S9sRpcClient &client);
        void executeDeleteAccount(S9sRpcClient &client);
        void executeCreateDatabase(S9sRpcClient &client);
        void doExecuteCreateCluster(S9sRpcClient &client);

        void executeCreateBackup(S9sRpcClient &client);
        void executeRestoreBackup(S9sRpcClient &client);
        void executeDeleteBackup(S9sRpcClient &client);
        
        void executeMaintenanceCreate(S9sRpcClient &client);
        void executeMaintenanceDelete(S9sRpcClient &client);
        void executeMaintenanceList(S9sRpcClient &client);
        void executeMetaTypeList(S9sRpcClient &client);
        void executeMetaTypePropertyList(S9sRpcClient &client);
};

