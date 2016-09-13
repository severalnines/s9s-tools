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
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "S9sRpcClient"

class S9sBusinessLogic
{
    public:
        void execute();

        void waitForJob(const int jobId, S9sRpcClient &client);

    protected:
        virtual void jobRegistered(S9sRpcClient &client);


    private:
        void waitForJobWithProgess(const int jobId, S9sRpcClient &client);
        void waitForJobWithLog(const int jobId, S9sRpcClient &client);

        void executeClusterCreate(S9sRpcClient &client);
        void executeClusterList(S9sRpcClient &client);

        void executeNodeList(S9sRpcClient &client);
        void executeJobList(S9sRpcClient &client);
        void executeJobLog(S9sRpcClient &client);

        void executeRollingRestart(S9sRpcClient &client);
        void executeAddNode(S9sRpcClient &client);
        void doExecuteCreateCluster(S9sRpcClient &client);
};

