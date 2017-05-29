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
 * s9s-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with s9s-tools. If not, see <http://www.gnu.org/licenses/>.
 */
#include "s9sbusinesslogic.h"

#include "S9sStringList"
#include "S9sRpcReply"
#include "S9sOptions"
#include "S9sNode"
#include "S9sAccount"
#include "S9sDateTime"
#include "S9sTopUi"
#include "S9sFile"
#include "S9sRsaKey"
#include "S9sDir"

#include <stdio.h>
#include <unistd.h>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

/**
 * This method will execute whatever is requested by the user in the command
 * line.
 */
void 
S9sBusinessLogic::execute()
{
    S9sOptions  *options    = S9sOptions::instance();
    S9sString    controller = options->controllerHostName();
    int          port       = options->controllerPort();
    S9sString    token      = options->rpcToken();
    int          clusterId  = options->clusterId();
    bool         useTls     = options->useTls();
    S9sRpcClient client(controller, port, token, useTls);
    bool         success;

    /*
     * Here is a fucked up version and a version that I try to clean up.
     */
    //if (!options->isUserOperation() || !options->rpcToken().empty())
    if (options->isUserOperation() && options->isCreateRequested())
    {
        PRINT_VERBOSE("No authentication required");
        // No authentication required.
    } else {
        PRINT_VERBOSE("Authenticating.");
        S9sString userName = options->userName();
        S9sString keyPath  = options->privateKeyPath();

        if (userName.empty())
        {
            PRINT_ERROR("No user name not set.");
            options->setExitStatus(S9sOptions::BadOptions);
            return;
        }

        if (keyPath.empty())
        {
            PRINT_ERROR("No key file name not set.");
            options->setExitStatus(S9sOptions::BadOptions);
            return;
        }

        if (!client.authenticate())
        {

            if (options->isJsonRequested())
            {
                printf("%s\n", STR(client.reply().toString()));
            } else {
                S9sString errorString = client.errorString();

                if (errorString.empty())
                    errorString = client.reply().errorString();

                if (errorString.empty())
                    errorString = "Access denied.";

                PRINT_ERROR("Authentication failed: %s", STR(errorString));
            }

            // continuing, server replies a nice error
            options->setExitStatus(S9sOptions::AccessDenied);
            return;
        }
    }

    if (options->isClusterOperation())
    {
        if (options->isPingRequested())
        {
            executePing(client);
        } else if (options->isListRequested() || options->isStatRequested())
        {
            executeClusterList(client);
        } else if (options->isCreateRequested())
        {
            success = client.createCluster();
            maybeJobRegistered(client, 0, success);
        } else if (options->isRollingRestartRequested())
        {
            success = client.rollingRestart(clusterId);
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isAddNodeRequested())
        {
            success = client.createNode();
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isRemoveNodeRequested())
        {
            success = client.removeNode();
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isStopRequested())
        {
            success = client.stopCluster();
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isStartRequested())
        {
            success = client.startCluster();
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isDropRequested())
        {
            success = client.dropCluster();
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isCreateAccountRequested())
        {
            executeCreateAccount(client);
        } else if (options->isGrantRequested())
        {
            executeGrant(client);
        } else if (options->isDeleteAccountRequested())
        {
            executeDeleteAccount(client);
        } else if (options->isCreateDatabaseRequested())
        {
            executeCreateDatabase(client);
        } else {
            PRINT_ERROR("Operation is not specified.");
        }
    } else if (options->isLogOperation())
    {
        if (options->isListRequested())
        {
            executeLogList(client);
        } else {
            PRINT_ERROR("Operation is not specified.");
        }
    } else if (options->isNodeOperation())
    {
        if (options->isStatRequested() && !options->graph().empty())
        {
            executeNodeStat(client);
        } else if (options->isListRequested() || options->isStatRequested())
        {
            executeNodeList(client);
        } else if (options->isSetRequested())
        {
            executeNodeSet(client);
        } else if (options->isListConfigRequested())
        {
            executeConfigList(client);
        } else if (options->isChangeConfigRequested())
        {
            executeSetConfig(client);
        } else if (options->isPullConfigRequested())
        {
            executePullConfig(client);
        } else if (options->isStartRequested())
        {
            success = client.startNode();
            maybeJobRegistered(client, clusterId, success); 
        } else if (options->isStopRequested())
        {
            success = client.stopNode();
            maybeJobRegistered(client, clusterId, success); 
        } else if (options->isRestartRequested())
        {
            success = client.restartNode();
            maybeJobRegistered(client, clusterId, success); 
        } else {
            PRINT_ERROR("Operation is not specified.");
        }
    } else if (options->isScriptOperation())
    {
        if (options->isTreeRequested())
        {
            executeScriptTree(client);
        } else if (options->isExecuteRequested())
        {
            executeExecute(client);
        } else {
            PRINT_ERROR("Operation is not specified.");
        }
    } else if (options->isJobOperation())
    {
        if (options->isListRequested())
        {
            executeJobList(client);
        } else if (options->isLogRequested())
        {
            executeJobLog(client);
        } else if (options->isWaitRequested())
        {
            waitForJob(clusterId, options->jobId(), client);
        } else {
            PRINT_ERROR("Unknown job operation.");
        }
    } else if (options->isBackupOperation())
    {
        if (options->isListRequested())
        {
            executeBackupList(client);
        } else if (options->isCreateRequested())
        {
            success = client.createBackup();
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isRestoreRequested())
        {
            success = client.restoreBackup();
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isDeleteRequested())
        {
            executeDeleteBackup(client);
        } else {
            PRINT_ERROR("Unknown backup operation.");
        }
    } else if (options->isProcessOperation())
    {
        if (options->isListRequested())
        {
            executeProcessList(client);
        } else if (options->isTopRequested())
        {
            executeTop(client);
        } else {
            PRINT_ERROR("Unknown process operation.");
        }
    } else if (options->isUserOperation())
    {
        if (options->isListRequested() || options->isWhoAmIRequested())
        {
            executeUserList(client);
        } else {
            executeUser(client);
        }
    } else if (options->isMaintenanceOperation())
    {
        if (options->isListRequested())
        {
            executeMaintenanceList(client);
        } else if (options->isCreateRequested())
        {
            executeMaintenanceCreate(client);
        } else if (options->isDeleteRequested())
        {
            executeMaintenanceDelete(client);
        } else {
            PRINT_ERROR("Unknown maintenance operation.");
        }
    } else if (options->isMetaTypeOperation())
    {
        if (options->isListRequested())
        {
            executeMetaTypeList(client);
        } else if (options->isListPropertiesRequested())
        {
            executeMetaTypePropertyList(client);
        } else {
            PRINT_ERROR("Unknown metatype operation.");
        }
    } else {
        PRINT_ERROR("Unknown operation.");
    }
}

/**
 * \param client A client for the communication.
 * \param jobId The ID of the job that we wait for.
 * \param client A client for the communication.
 *
 * This method waits for the given job to be finished (or failed, or aborted)
 * and will provide feedback in the form the user requested in the command 
 * line (printing job messages or a progress bar).
 */
void 
S9sBusinessLogic::waitForJob(
        const int       clusterId,
        const int       jobId, 
        S9sRpcClient   &client)
{
    S9sOptions  *options = S9sOptions::instance();
    
    if (options->isLogRequested())
    {
        waitForJobWithLog(clusterId, jobId, client);
    } else {
        waitForJobWithProgress(clusterId, jobId, client);
    }
}

void
S9sBusinessLogic::executePing(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    bool         continuous = options->isWaitRequested();
    S9sRpcReply  reply;
    bool         success;

again:
    success = client.ping();
    if (success)
    {
        reply = client.reply();

        success = reply.isOk();
        if (success)
        {
            // well, nothing now
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                reply.printPing();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));

            options->setExitStatus(S9sOptions::Failed);
        }
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
            
        options->setExitStatus(S9sOptions::Failed);
    }

    if (continuous)
    {
        usleep(500000);
        goto again;
    }
}

/**
 * \param client A client for the communication.
 *
 * Execute the "script --tree" operation.
 */
void
S9sBusinessLogic::executeScriptTree(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    bool        success;

    success = client.treeScripts();
    if (success)
    {
        reply = client.reply();
        reply.printScriptTree();
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * \param client A client for the communication.
 *
 * Execute the "cluster --list" operation.
 */
void
S9sBusinessLogic::executeClusterList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    bool        success;

    success = client.getClusters();
    if (success)
    {
        reply = client.reply();
        reply.printClusterList();
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}

void 
S9sBusinessLogic::executeNodeStat(
        S9sRpcClient &client)
{
    S9sOptions  *options   = S9sOptions::instance();
    int          clusterId = options->clusterId();
    S9sString    graphName = options->graph().toLower();
    S9sRpcReply  reply;
    bool         success;

    if (graphName.startsWith("cpu") || graphName.startsWith("load"))
    {
        success = client.getCpuStats(clusterId);
    } else if (graphName.startsWith("mem") || graphName.startsWith("swap"))
    {
        success = client.getMemStats(clusterId);
    } else {
        success = client.getSqlStats(clusterId);
    }

    if (success)
    {
        reply = client.reply();
        success = reply.isOk();
        if (success)
        {
            reply.printGraph();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}
 
/**
 * \param client A client for the communication.
 * 
 * Executing the node --list and node --stat if no graph is requested.
 */
void 
S9sBusinessLogic::executeNodeList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    bool        success;


    success = client.getClusters();
    if (success)
    {
        reply = client.reply();

        success = reply.isOk();
        if (success)
        {
            reply.printNodeList();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * \param client A client for the communication.
 *
 */
void 
S9sBusinessLogic::executeConfigList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    bool        success;


    success = client.getConfig(options->nodes());
    if (success)
    {
        reply = client.reply();

        success = reply.isOk();
        if (success)
        {
            reply.printConfigList();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * \param client A client for the communication.
 *
 */
void 
S9sBusinessLogic::executePullConfig(
        S9sRpcClient &client)
{
    S9sOptions  *options   = S9sOptions::instance();
    S9sString    outputDir = options->outputDir();
    S9sRpcReply  reply;
    S9sDir       dir;
    bool         success;

    /*
     * 
     */
    if (outputDir.empty())
    {
        PRINT_ERROR(
                "The output driectory is not set.\n"
                "Use the --output-dir command line option to set it.");
        return;
    }

    dir = S9sDir(outputDir);
    if (!dir.exists())
    {
        success = dir.mkdir();
        if (!success)
        {
            PRINT_ERROR("%s", STR(dir.errorString()));
            return;
        }
    }
    
    if (!dir.exists())
    {
        PRINT_ERROR("Unable to create directory '%s'.", STR(outputDir));
        return;
    }

    /*
     *
     */
    success = client.getConfig(options->nodes());
    if (!success)
    {
        PRINT_ERROR("%s", STR(client.errorString()));
        return;
    }

    reply   = client.reply();
    success = reply.isOk();
    if (!success)
    {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(reply.errorString()));

        return;
    }

    reply.saveConfig(outputDir);
}

void 
S9sBusinessLogic::executeMaintenanceList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    bool        success;

    success = client.getMaintenance();
    if (success)
    {
        reply = client.reply();
        success = reply.isOk();
        if (success)
        {
            if (options->isJsonRequested())
                printf("\n%s\n", STR(reply.toString()));
            else
                reply.printMaintenanceList();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}

void 
S9sBusinessLogic::executeMetaTypeList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    bool        success;

    success = client.getMetaTypes();
    if (success)
    {
        reply = client.reply();
        success = reply.isOk();
        if (success)
        {
            if (options->isJsonRequested())
                printf("\n%s\n", STR(reply.toString()));
            else
                reply.printMetaTypeList();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}

void 
S9sBusinessLogic::executeMetaTypePropertyList(
        S9sRpcClient &client)
{
    S9sOptions  *options  = S9sOptions::instance();
    S9sString    typeName = options->type();
    S9sRpcReply reply;
    bool        success;

    success = client.getMetaTypeProperties(typeName);
    if (success)
    {
        reply = client.reply();
        success = reply.isOk();
        if (success)
        {
            if (options->isJsonRequested())
                printf("\n%s\n", STR(reply.toString()));
            else
                reply.printMetaTypePropertyList();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * \param client A client for the communication.
 */
void 
S9sBusinessLogic::executeSetConfig(
        S9sRpcClient &client)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sRpcReply     reply;
    S9sVariantList  hosts;
    S9sString       optName;
    bool            success;

    hosts = options->nodes();
    if (hosts.empty())
    {
        options->printError(
                "Node list is empty while setting configuration.\n"
                "Use the --nodes command line option to provide the node list."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return;
    }
    
    optName = options->optName();
    if (optName.empty())
    {
        options->printError(
                "Configuration option name is not provided.\n"
                "Use the --opt-name command line option to provide"
                " a configuration option name."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return;
    }

    success = client.setConfig(hosts);
    if (success)
    {
        reply = client.reply();
        reply.printMessages("OK");
    }
}

/**
 * \param client A client for the communication.
 */
void 
S9sBusinessLogic::executeExecute(
        S9sRpcClient &client)
{
    S9sOptions  *options    = S9sOptions::instance();
    uint         nFileNames = options->nExtraArguments();
    S9sString    content;
    S9sString    fileName  = "stdin";
    S9sString    arguments = "";
    S9sString    errorString;
    S9sRpcReply  reply;
    bool         success;
 
    if (nFileNames == 0u)
    {
        content = S9sString::readStdIn();
        success = client.executeExternalScript(fileName, content, arguments);

        if (success)
        {
            reply = client.reply();
            reply.printScriptOutput();
        }
    } else if (nFileNames > 1u)
    {
        PRINT_ERROR("Multiple file names in the command line.");
    } else {
        fileName = options->extraArgument(0u);
        
        success = S9sString::readFile(fileName, content, errorString);
        if (!success)
        {
            PRINT_ERROR("%s\n", STR(errorString));
            return;
        }

        success = client.executeExternalScript(fileName, content, arguments);
        if (success)
        {
            reply = client.reply();
            reply.printScriptOutput();
        }
    }
}

/**
 * \param client A client for the communication.
 */
void 
S9sBusinessLogic::executeNodeSet(
        S9sRpcClient &client)
{
    bool            success;

    success = client.setHost();
    client.printMessages("Ok.\n", success);
}

void
S9sBusinessLogic::executeCreateAccount(
        S9sRpcClient &client)
{
    bool           success;

    /*
     * Running the request on the controller.
     */
    success = client.createAccount();
    client.printMessages("Created.\n", success);
}

void
S9sBusinessLogic::executeGrant(
        S9sRpcClient &client)
{
    S9sOptions    *options = S9sOptions::instance();
    bool           success;

    /*
     * Running the request on the controller.
     */
    success = client.grantPrivileges(options->account(), options->privileges());
    client.printMessages("Grant.\n", success);
}

void
S9sBusinessLogic::executeDeleteAccount(
        S9sRpcClient &client)
{
    bool           success;

    /*
     * Running the request on the controller.
     */
    success = client.deleteAccount();
    client.printMessages("Created.\n", success);
}

void
S9sBusinessLogic::executeCreateDatabase(
        S9sRpcClient &client)
{
    bool           success;

    /*
     * Running the request on the controller.
     */
    success = client.createDatabase();
    client.printMessages("Created.\n", success);
}

void
S9sBusinessLogic::executeDeleteBackup(
        S9sRpcClient &client)
{
    S9sOptions    *options = S9sOptions::instance();
    bool           success;

    /*
     * Running the request on the controller.
     */
    success = client.deleteBackupRecord(options->backupId());
    client.printMessages("Deleted.\n", success);
}

/**
 * This method provides a continuous display of one specific cluster that is
 * similar to the "top" utility.
 */
void 
S9sBusinessLogic::executeTop(
        S9sRpcClient &client)
{
    S9sTopUi ui;

    ui.executeTop(client);
}

/**
 * \param client A client for the communication.
 *
 * Executes the --list operation on the processes thus providing a list of
 * running processes.
 */
void 
S9sBusinessLogic::executeProcessList(
        S9sRpcClient &client)
{
    S9sRpcReply reply;
    bool        success;

    success = client.getRunningProcesses();
    if (success)
    {
        reply = client.reply();
        reply.printProcessListBrief();
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}

void 
S9sBusinessLogic::executeBackupList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    int         clusterId = options->clusterId();
    S9sRpcReply reply;
    bool        success;

    success = client.getBackups(clusterId);
    if (success)
    {
        reply = client.reply();
        reply.printBackupList();
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * This function will execute the listing of the users that can be requested by
 * the --list and --whoami command line options.
 */
void 
S9sBusinessLogic::executeUserList(
        S9sRpcClient &client)
{
    S9sRpcReply reply;
    bool        success;

    success = client.getUsers();
    if (success)
    {
        reply = client.reply();
        reply.printUserList();
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * \param client A client for the communication.
 *
 * Executes the --list operation on the jobs thus providing a list of jobs.
 */
void 
S9sBusinessLogic::executeJobList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    bool        success;

    success = client.getJobInstances();
    if (success)
    {
        reply = client.reply();
        success = reply.isOk();
        if (success)
        {
            reply.printJobList();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    } 
}

void 
S9sBusinessLogic::executeLogList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    bool        success;

    success = client.getLog();
    if (success)
    {
        reply = client.reply();
        success = reply.isOk();
        if (success)
        {
            reply.printLogList();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    } 
}

/**
 * \param client A client for the communication.
 * 
 * This method will get the job messages from the controller and print them to
 * the standard output.
 */
void 
S9sBusinessLogic::executeJobLog(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    int         jobId     = options->jobId();
    bool        success;

    success = client.getJobLog(jobId);
    if (success)
    {
        reply = client.reply();
        
        success = reply.isOk();
        if (success)
        {
            reply.printJobLog();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }

    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
}
}

void
S9sBusinessLogic::maybeJobRegistered(
        S9sRpcClient &client,
        const int     clusterId,
        bool          success)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sRpcReply    reply;

    if (success)
    {
        jobRegistered(client, clusterId);
    } else {
        reply = client.reply();

        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }    
}


/**
 * \param client The client that just created the new job with the reply in it.
 * \param clusterId The ID of the cluster that executes the job.
 *
 * This method can be called when a new job is registered using the RPC client.
 * This function will print the necessary messages to inform the user about the
 * new job, wait for the job to ended if necessary, show the progress or the job
 * messages if the command line options or settings suggests that should be
 * done.
 */
void
S9sBusinessLogic::jobRegistered(
        S9sRpcClient &client,
        const int     clusterId)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sRpcReply    reply;
    bool           success;

    reply = client.reply();
    success = reply.isOk();
    
    if (success)
    {
        if (options->isWaitRequested() || options->isLogRequested())
        {
            waitForJob(clusterId, reply.jobId(), client);
        } else {
            reply.printJobStarted();
        }
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(reply.errorString()));
    }
}

/**
 * \param jobId The ID of the job to monitor with a progress bar.
 * \param client The client for the communication.
 *
 * This method can be used to wait until a job is finished and print a progress
 * report in the meantime.
 */
void 
S9sBusinessLogic::waitForJobWithProgress(
        const int     clusterId,
        const int     jobId, 
        S9sRpcClient &client)
{
    S9sOptions    *options         = S9sOptions::instance();
    bool           syntaxHighlight = options->useSyntaxHighlight();
    const char    *rotate[]        = { "/", "-", "\\", "|" };
    int            rotateCycle     = 0;
    S9sRpcReply    reply;
    bool           success, finished;
    S9sString      progressLine, previousProgressLine;
    bool           titlePrinted = false;
    int            nFailures = 0;
    int            nAuthentications = 0;

    if (syntaxHighlight)
        printf("\033[?25l"); 

    for (;;)
    {
        /*
         * Getting the job instance from the controller.
         */
        success = client.getJobInstance(jobId);
        if (success)
        {
            reply     = client.reply();
            success   = reply.isOk();

            if (reply.isAuthRequired())
            {
                if (nAuthentications > 3)
                    break;

                success = client.authenticate();
                ++nAuthentications;
            } else {
                nAuthentications = 0;
            }
        }
        
        if (success)
        {
            nFailures = 0;
        } else {
            PRINT_ERROR("%s", STR(reply.errorString()));
            printf("\n\n%s\n", STR(reply.toString()));
            ++nFailures;
            if (nFailures > 3)
                break;

            continue;
        }

        nFailures = 0;

        /*
         * Printing the title if it is not yet printed.
         */
        if (!titlePrinted && !reply.jobTitle().empty())
        {
            const char *titleBegin = "";
            const char *titleEnd   = "";

            if (syntaxHighlight)
            {
                titleBegin = TERM_BOLD;
                titleEnd   = TERM_NORMAL;
            }

            printf("%s%s%s\n", 
                    titleBegin,
                    STR(reply.jobTitle()),
                    titleEnd);

            titlePrinted = true;
        }

        /*
         * Printing the progress line.
         */
        finished = reply.progressLine(progressLine, syntaxHighlight);
        if (progressLine.empty())
            goto end_of_loop;
        
        #ifdef DEBUG
        if (progressLine == previousProgressLine)
            goto end_of_loop;
        #endif

        printf("%s %s\033[K\r", rotate[rotateCycle], STR(progressLine));
        #ifdef DEBUG
        printf("\n");
        #endif

        previousProgressLine = progressLine;

        if (reply.isJobFailed())
            options->setExitStatus(S9sOptions::JobFailed);

        //printf("%s", STR(reply.toString()));

        fflush(stdout);
        sleep(1);

        ++rotateCycle;
        rotateCycle %= sizeof(rotate) / sizeof(void *);

end_of_loop:
        if (finished)
            break;
    }

    if (syntaxHighlight)
        printf("\033[?25h");

    printf("\n");
}

/**
 * \param clusterId The ID of the cluster that executes the job.
 * \param jobId The ID of the job to monitor.
 * \param client The client for the communication.
 *
 * This function will wait for the job to be finished and will continuously
 * print the job messages.
 */
void 
S9sBusinessLogic::waitForJobWithLog(
        const int     clusterId,
        const int     jobId, 
        S9sRpcClient &client)
{
    S9sOptions    *options         = S9sOptions::instance();
    S9sVariantMap  job;
    S9sRpcReply    reply;
    bool           success, finished;
    int            nLogsPrinted = 0;
    int            nEntries;
    int            nFailures = 0;
    int            nAuthentications = 0;

    for (;;)
    {
        /*
         * Requested at most 300 log messages. If we have more we will print
         * them later in the next round.
         */
        success = client.getJobLog(jobId, 300, nLogsPrinted);
        if (success)
        {
            reply     = client.reply();
            success   = reply.isOk();

            if (reply.isAuthRequired())
            {
                if (nAuthentications > 3)
                    break;

                success = client.authenticate();
                ++nAuthentications;
            } else {
                nAuthentications = 0;
            }
        }

        /*
         * If we have errors we count them, if we have more errors than we care
         * to abide we exit.
         */
        if (success)
        { 
            nFailures = 0;
        } else {
            PRINT_ERROR("%s", STR(reply.errorString()));
            printf("\n\n%s\n", STR(reply.toString()));
            ++nFailures;
            if (nFailures > 3)
                break;

            continue;
        }

        /*
         * Printing the log messages.
         */
        nEntries = reply["messages"].toVariantList().size();
        if (nEntries > 0)
            reply.printJobLog();

        nLogsPrinted += nEntries;

        job = reply["job"].toVariantMap();
        if (job["status"] == "FAILED")
            options->setExitStatus(S9sOptions::JobFailed);

        finished = 
            job["status"] == "ABORTED"   ||
            job["status"] == "FINISHED"  ||
            job["status"] == "FAILED";
        
        fflush(stdout);
        if (finished)
            break;
        
        sleep(1);
    }

    printf("\n");
}

void
S9sBusinessLogic::executeUser(
        S9sRpcClient        &client)
{
    S9sString    errorString;
    S9sOptions  *options  = S9sOptions::instance();
    S9sString    userName = options->userName();
    S9sString    keyFilePath;
    S9sConfigFile config;

    if (userName.empty())
    {
        PRINT_ERROR(
                 "User name is not set.\n"
                 "Use the --cmon-user command line option to set it.");

        options->setExitStatus(S9sOptions::BadOptions);
        return;
    }

    /*
     * Now make sure that we save the specified username into the user config
     * file
     */
    config.setFileName("~/.s9s/s9s.conf");
    PRINT_VERBOSE(
            "Saving Cmon user '%s' into config file at %s.",
            STR(userName), 
            STR(config.fileName()));

    if (!config.parseSourceFile())
    {
        PRINT_ERROR("Couldn't parse %s: %s",
                STR(config.fileName()), STR(config.errorString()));
        return;
    }

    if (config.hasVariable("global", "cmon_user"))
    {
        config.changeVariable("global", "cmon_user", userName);
    } else if (config.hasVariable("", "cmon_user"))
    {
        config.changeVariable("cmon_user", userName);
    } else {
        config.addVariable("global", "cmon_user", userName);
    }

    if (!config.save(errorString))
    {
        PRINT_ERROR(
                "Could not update user configuration file: %s", 
                STR(errorString));
        
        return;
    }

    /*
     *
     */
    S9sRsaKey rsaKey;

    keyFilePath = options->privateKeyPath();
    if (options->isGenerateKeyRequested())
    {
        // check if key exists and is valid, otherwise generate a new key-pair
        S9sRsaKey key;

        if (key.loadKeyFromFile(keyFilePath) && key.isValid())
        {
            PRINT_VERBOSE("Keyfile '%s' exists and valid.", STR(keyFilePath));
        } else {
            S9sString pubKeyPath;

            PRINT_VERBOSE("Generating an RSA key-pair (%s).", STR(keyFilePath));

            pubKeyPath = keyFilePath;
            pubKeyPath.replace(".key", "");
            pubKeyPath += ".pub";

            if (!key.generateKeyPair() ||
                !key.saveKeys(keyFilePath, pubKeyPath, errorString))
            {
                if (errorString.empty())
                    errorString = "RSA key generation failure.";

                PRINT_ERROR("Key generation failed: %s", STR(errorString));
            }
        }
    }

    if (!rsaKey.loadKeyFromFile(keyFilePath) || !rsaKey.isValid())
    {
        PRINT_ERROR("User keyfile couldn't be loaded: %s", STR(keyFilePath));
        options->setExitStatus(S9sOptions::JobFailed);
        return;
    }

    /*
     * Granting.
     */
    if (options->isCreateRequested())
    {
        bool          oneSucceed = false;
        int           exitCode = 0;
        S9sString     sshCommand;
        S9sStringList fifos;
        S9sString     pubKeyPath;

        // There must be a better way to determine this
        pubKeyPath = keyFilePath;
        pubKeyPath.replace(".key", "");
        pubKeyPath += ".pub";

        S9sFile pubKeyFile (pubKeyPath);
        S9sString pubKeyStr;
        if (!pubKeyFile.readTxtFile(pubKeyStr) || pubKeyStr.empty())
        {
            // Private key exists and valid, but
            PRINT_ERROR("Couldn't load public key (%s): %s",
                STR(pubKeyPath), STR(pubKeyFile.errorString()));

            options->setExitStatus(S9sOptions::JobFailed);
            return;
        }

        S9sString controller = options->controllerHostName();
        if (controller.empty())
            controller = "127.0.0.1";

        // this one is used by unit/functional tests
        if (S9sFile("/tmp/cmon_test/usermgmt.fifo").exists())
            fifos << "/tmp/cmon_test/usermgmt.fifo";

        // and in real cmon daemon
        fifos << "/var/lib/cmon/usermgmt.fifo";


        for (uint idx = 0; idx < fifos.size(); ++idx)
        {
            S9sVariantMap request;
            S9sString     escapedJson;
            S9sString     path = fifos.at(idx);
            S9sFile       file(path);

            // 
            // Building up a request for the user creation.
            //
            request["user_name"] = userName;
            request["pubkey"]    = pubKeyStr;
            
            if (!options->title().empty())
                request["title"] = options->title();
            
            if (!options->firstName().empty())
                request["first_name"] = options->firstName();
            
            if (!options->lastName().empty())
                request["last_name"] = options->lastName();
            
            if (!options->group().empty())
                request["group"] = options->group();

            if (!options->emailAddress().empty())
                request["email_address"] = options->emailAddress();

            if (options->createGroup())
                request["create_group"] = true;
            
            escapedJson = request.toString().escape();
            if (options->isJsonRequested())
                printf("Request: %s\n", STR(request.toString()));
            
            PRINT_VERBOSE("escapedJson: \n%s", STR(escapedJson));

            if (controller == "localhost" || controller == "127.0.0.1")
            {
                sshCommand.sprintf(
                    "echo \"%s\" "
                    "| sudo -n tee %s 2>/dev/null >/dev/null",
                    STR(escapedJson), STR(path));
            } else {
                sshCommand.sprintf(
                    "ssh -tt "
                    "-oUserKnownHostsFile=/dev/null "
                    "-oStrictHostKeyChecking=no "
                    "-oBatchMode=yes "
                    "-oPasswordAuthentication=no "
                    "-oConnectTimeout=30 "
                    " '%s' "
                    "'echo \"%s\" "
                    "| sudo -n tee %s 2>/dev/null >/dev/null'",
                    STR(controller),
                    STR(escapedJson),
                    STR(path));
            }

            PRINT_VERBOSE("Command: \n%s", STR(sshCommand));
            PRINT_VERBOSE(
                    "Tried to grant on %s:%s, exitCode=%d.",
                    STR(controller), STR(path), exitCode);

            exitCode    = ::system(STR(sshCommand));
            oneSucceed |= (exitCode == 0);

            if (exitCode != 0)
            {
                errorString.sprintf (
                    "SSH command exited with non-zero status: %d\n%s",
                    exitCode, STR(sshCommand));
            }
        }

        if (!oneSucceed)
        {
            PRINT_ERROR("%s", STR(errorString));
            options->setExitStatus(S9sOptions::JobFailed);

            return;
        } else {
            if (!options->isBatchRequested())
            {
                fprintf(stderr, 
                        "Grant user '%s' succeeded.\n", 
                        STR(userName));

                fflush(stderr);
            }
        }
    }
}

void
S9sBusinessLogic::executeMaintenanceCreate(
        S9sRpcClient &client)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sRpcReply    reply;
    bool           success;

    /*
     * Running the request on the controller.
     */
    success = client.createMaintenance();
    reply   = client.reply();
    if (success)
    {
        if (options->isJsonRequested())
        {
            printf("%s\n", STR(reply.toString()));
        } else if (!options->isBatchRequested()) 
        {
            printf("%s\n", STR(reply.uuid()));
        }
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}

void
S9sBusinessLogic::executeMaintenanceDelete(
        S9sRpcClient &client)
{
    bool           success;

    /*
     * Running the request on the controller.
     */
    success = client.deleteMaintenance();
    client.printMessages("Created.\n", success);
}

