/*
 * Severalnines Tools
 * Copyright (C) 2017 Severalnines AB
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
#include "S9sTopUi"

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
            maybeJobRegistered(
                S9sRpcClient &client,
                const int     clusterId,
                bool          success);

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

        void executeUserList(S9sRpcClient &client);
        void executeGroupList(S9sRpcClient &client);
        void executeAccountList(S9sRpcClient &client);

        void executeCreateUser(S9sRpcClient &client);
        void executeCreateUserThroughRpc(S9sRpcClient &client);
        void executeCreateUserThroughPipe(S9sRpcClient &client);

        bool ensureHasAuthKey(
                const S9sString &privateKeyPath,
                S9sString       &publicKey);

        void executeClusterList(S9sRpcClient &client);
        void executeClusterPing(S9sRpcClient &client);
        void executeControllerPing(S9sRpcClient &client);

        void executeNodeList(S9sRpcClient &client);
        void executeNodeGraph(S9sRpcClient &client);
        void executeNodeSet(S9sRpcClient &client);
        void executeConfigList(S9sRpcClient &client);
        void executePullConfig(S9sRpcClient &client);

        void executeExecute(S9sRpcClient &client);
        void executeSystemCommand(S9sRpcClient &client);

        void executeTop(S9sRpcClient &client, S9sTopUi::ViewMode mode);
        void executeProcessList(S9sRpcClient &client);
        void executePrintKeys(S9sRpcClient &client);
        void printBackupSchedules(S9sRpcClient &client);
        void printSnapshotRepositories(S9sRpcClient &client);
        void deleteSnapshotRepository(S9sRpcClient &client);
        void executeBackupList(S9sRpcClient &client);

        void executeJobList(S9sRpcClient &client);
        void executeLogList(S9sRpcClient &client);
        void executeJobLog(S9sRpcClient &client);

        void executeDropCluster(S9sRpcClient &client);

        void executeMaintenanceCreate(S9sRpcClient &client);
        void executeMaintenanceList(S9sRpcClient &client);
        void executeMetaTypeList(S9sRpcClient &client);
        void executeMetaTypePropertyList(S9sRpcClient &client);
        
        void executeScriptTree(S9sRpcClient &client);
};

