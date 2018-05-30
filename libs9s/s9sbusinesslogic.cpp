/*
 * Severalnines Tools
 * Copyright (C) 2016-2018 Severalnines AB
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
#include "S9sUser"
#include "S9sNode"
#include "S9sAccount"
#include "S9sDateTime"
#include "S9sTopUi"
#include "S9sFile"
#include "S9sRsaKey"
#include "S9sDir"
#include "S9sCmonGraph"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

/**
 * This is an example callback method to process the events live
 * from the JSon stream coming from the controller.
 */
void
s9sEventHandler(const S9sVariantMap &jsonMessage, void *userData)
{
    S9sBusinessLogic *businessLogic = (S9sBusinessLogic*) userData;
    (void) businessLogic; // unused for now

    printf("* Incoming event (JSon):\n%s\n", STR(jsonMessage.toString()));
}

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
    int          clusterId  = options->clusterId();
    bool         useTls     = options->useTls();
    S9sRpcClient client(controller, port, useTls);
    bool         success;

    /*
     *
     */
    success = authenticate(client);
    if (!success)
        return;

    if (options->isClusterOperation())
    {
        /*
         * If the cluster operation requested, like
         * s9s cluster [OPTION]...
         */
        if (options->isPingRequested())
        {
            executePing(client);
        } else if (options->isListRequested() || options->isStatRequested())
        {
            executeClusterList(client);
        } else if (options->isListDatabasesRequested())
        {
            S9sRpcReply reply;

            success = client.getDatabases();
            reply = client.reply();
            reply.printDatabaseList();
        } else if (options->isCheckHostsRequested())
        {
            client.checkHosts();
            #if 0
            client.printMessages("Checked.", success);
            client.setExitStatus();            
            #else
            S9sRpcReply reply;

            reply = client.reply();
            reply.printCheckHostsReply();
            #endif
        } else if (options->isCreateRequested())
        {
            success = client.createCluster();
            maybeJobRegistered(client, 0, success);
        } else if (options->isRegisterRequested())
        {
            success = client.registerCluster();
            maybeJobRegistered(client, 0, success);
        } else if (options->isRollingRestartRequested())
        {
            success = client.rollingRestart(clusterId);
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isSetupAuditLoggingRequested())
        {
            success = client.setupAuditLogging(clusterId);
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isCreateReportRequested())
        {
            success = client.createReport(clusterId);
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
            success = client.createAccount();
            client.printMessages("Created.", success);
            client.setExitStatus();
        } else if (options->isGrantRequested())
        {
            success = client.grantPrivileges(
                    options->account(), 
                    options->privileges());

            client.printMessages("Grant.", success);
            client.setExitStatus();
        } else if (options->isDeleteAccountRequested())
        {
            success = client.deleteAccount();
            client.printMessages("Created.", success);
            client.setExitStatus();
        } else if (options->isCreateDatabaseRequested())
        {
            success = client.createDatabase();
            client.printMessages("Created.", success);
            client.setExitStatus();
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
    } else if (options->isEventOperation())
    {
        if (options->isListRequested())
        {
            client.subscribeEvents(s9sEventHandler, this);
        } else {
            PRINT_ERROR("Operation is not specified.");
        }
    } else if (options->isNodeOperation())
    {
        if (options->isStatRequested() && !options->graph().empty())
        {
            executeNodeGraph(client);
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
        } else if (options->isUnregisterRequested()) 
        {
            success = client.unregisterHost();
            client.printMessages("Unregistered.", success);
            client.setExitStatus();
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
    } else if (options->isTreeOperation())
    {
        if (options->isListRequested())
        {
            S9sRpcReply reply;
            
            client.getTree();
            reply = client.reply();
            reply.printObjectList();
        } else if (options->isTreeRequested())
        {
            S9sRpcReply reply;
            
            success = client.getTree();
            if (success)
            {
                reply = client.reply();
                reply.printObjectTree();
            } else {
                if (options->isJsonRequested())
                    printf("%s\n", STR(reply.toString()));
                else
                    PRINT_ERROR("%s", STR(client.errorString()));
            }
        } else if (options->isAccessRequested())
        {
#if 0
            S9sRpcReply reply;

            success = client.checkAccess();
            reply = client.reply();
            reply.printAcl();
            client.setExitStatus();
#endif
            success = client.checkAccess();
            client.printMessages("Ok", success);
            client.setExitStatus();
        } else if (options->isCatRequested())
        {
            // s9s tree --cat PATH
            S9sRpcReply reply;

            success = client.cat();
            reply = client.reply();
            reply.printCat();
            client.setExitStatus();
        } else if (options->isGetAclRequested())
        {
            S9sRpcReply reply;

            success = client.getAcl();
            reply = client.reply();
            reply.printAcl();
            client.setExitStatus();
        } else if (options->isAddAclRequested())
        {
            success = client.addAcl();
            client.printMessages("Acl is added.", success);
            client.setExitStatus();
        } else if (options->isRemoveAclRequested())
        {
            success = client.removeAcl();
            client.printMessages("Acl is removed.", success);
            client.setExitStatus();
        } else if (options->isChOwnRequested())
        {
            success = client.chOwn();
            client.printMessages("Changed.", success);
            client.setExitStatus();
        } else if (options->isMkdirRequested())
        {
            success = client.mkdir();
            client.printMessages("Created.", success);
            client.setExitStatus();
        } else if (options->isRmdirRequested())
        {
            success = client.rmdir();
            client.printMessages("Removed.", success);
            client.setExitStatus();
        } else if (options->isDeleteRequested())
        {
            success = client.deleteFromTree();
            client.printMessages("Removed.", success);
            client.setExitStatus();
        } else if (options->isMoveRequested())
        {
            /* 
             * s9s tree --move "/Hungary" "/"
             */
            success = client.moveInTree();
            client.printMessages("Moved.", success);
            client.setExitStatus();
        } else {
            PRINT_ERROR("Operation is not specified.");
        }
    } else if (options->isServerOperation())
    {
        if (options->isCreateRequested())
        {
            // s9s server --create  
            success = client.createServer();
            maybeJobRegistered(client, 0, success);
        } else if (options->isGetAclRequested())
        {
            S9sRpcReply reply;

            success = client.getAcl();
            reply = client.reply();
            reply.printAcl();
            client.setExitStatus();
        } else if (options->isAddAclRequested())
        {
            success = client.addAcl();
            client.printMessages("Acl is added.", success);
            client.setExitStatus();
        } else if (options->isStartRequested())
        {
            success = client.startServers();
            client.printMessages("Started.", success);
            client.setExitStatus();
        } else if (options->isStopRequested())
        {
            success = client.stopServers();
            client.printMessages("Stopped.", success);
            client.setExitStatus();
        } else if (options->isRegisterRequested())
        {
            /* 
             * Here is an example of the server registration:
             * s9s server --register --servers="lxc://storage01?cdt_path=myservers" --print-json --verbose
             */
            success = client.registerServers();
            client.printServerRegistered(success);
            client.setExitStatus();
        } else if (options->isUnregisterRequested())
        {
            /* 
             * Here is an example of the server unregister:
             * s9s server --unregister --servers="lxc://storage01" 
             */
            success = client.unregisterServers();
            client.printMessages("Unregistered.", success);
            client.setExitStatus();
        } else if (options->isDeleteRequested())
        {
            /* 
             * s9s server --move "/Hungary" "/"
             */
            success = client.deleteFromTree();
            client.printMessages("Deleted.", success);
            client.setExitStatus();
        } else if (options->isListRequested() || options->isStatRequested())
        {
            /*
             * s9s server --list
             */
            S9sRpcReply reply;

            success = client.getServers();
            reply = client.reply();
            reply.printServers();
            client.setExitStatus();
        } else if (options->isListPartitionsRequested())
        {
            /*
             * s9s server --list-partitions
             */
            S9sRpcReply reply;

            success = client.getServers();
            reply = client.reply();
            reply.printPartitions();
            client.setExitStatus();
        } else if (options->isListMemoryRequested())
        {
            /*
             * s9s server --list-memory
             */
            S9sRpcReply reply;

            success = client.getServers();
            reply = client.reply();
            reply.printMemoryBanks();
            client.setExitStatus();
        } else if (options->isListSubnetsRequested())
        {
            /*
             * s9s server --list-memory
             */
            S9sRpcReply reply;

            success = client.getServers();
            reply = client.reply();
            reply.printSubnets();
            client.setExitStatus();
        } else if (options->isListTemplatesRequested())
        {
            /*
             * s9s server --list-memory
             */
            S9sRpcReply reply;

            success = client.getServers();
            reply = client.reply();
            reply.printTemplates();
            client.setExitStatus();
        } else if (options->isListImagesRequested())
        {
            /*
             * s9s server --list-memory
             */
            S9sRpcReply reply;

            success = client.getServers();
            reply = client.reply();
            reply.printImages();
            client.setExitStatus();
        } else if (options->isListProcessorsRequested())
        {
            /*
             * s9s server --list-processors
             */
            S9sRpcReply reply;

            success = client.getServers();
            reply = client.reply();
            reply.printProcessors();
            client.setExitStatus();
        } else if (options->isListNicsRequested())
        {
            /*
             * s9s server --list-nics
             */
            S9sRpcReply reply;

            success = client.getServers();
            reply = client.reply();
            reply.printNics();
            client.setExitStatus();
        } else if (options->isListDisksRequested())
        {
            /*
             * s9s server --list-nics
             */
            S9sRpcReply reply;

            success = client.getServers();
            reply = client.reply();
            reply.printDisks();
            client.setExitStatus();
        } else if (options->isListContainersRequested())
        {
            /*
             * s9s server --list-containers
             */
            S9sRpcReply reply;

            success = client.getContainers();
            reply = client.reply();
            reply.printContainers();
            client.setExitStatus();
#if 0
        } else if (options->isDeleteRequested())
        {
            // s9s server --delete NAME
            success = client.deleteContainer();
            client.printMessages("Deleted.", success);
            client.setExitStatus();
#endif
        } else {
            PRINT_ERROR("Operation is not specified.");
        }
    } else if (options->isContainerOperation())
    {
        if (options->isListRequested() || options->isStatRequested())
        {
            /*
             * s9s container --list
             * s9s container --stat
             */
            S9sRpcReply reply;

            success = client.getContainers();
            reply = client.reply();
            reply.printContainers();
            client.setExitStatus();
        } else if (options->isCreateRequested())
        {
            success = client.createContainerWithJob();
            maybeJobRegistered(client, 0, success);
        } else if (options->isDeleteRequested())
        {
            success = client.deleteContainerWithJob();
            maybeJobRegistered(client, 0, success);
        } else if (options->isStopRequested())
        {
            success = client.stopContainerWithJob();
            maybeJobRegistered(client, 0, success);
        } else if (options->isStartRequested())
        {
            success = client.startContainerWithJob();
            maybeJobRegistered(client, 0, success);
        } else {
            PRINT_ERROR("Unimplemented main option in 'container' mode.");
        }
    } else if (options->isJobOperation())
    {
        if (options->isListRequested())
        {
            executeJobList(client);
        } else if (options->isFailRequested())
        {
            success = client.createFailJob();
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isSuccessRequested())
        {
            success = client.createSuccessJob();
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isLogRequested())
        {
            executeJobLog(client);
        } else if (options->isWaitRequested())
        {
            waitForJob(clusterId, options->jobId(), client);
        } else if (options->isDeleteRequested())
        {
            success = client.deleteJobInstance(options->jobId());
            client.printMessages("Deleted.", success);
            client.setExitStatus();
        } else {
            PRINT_ERROR("Unknown job operation.");
        }
    } else if (options->isBackupOperation())
    {
        if (options->isListRequested() || 
                options->isListFilesRequested() ||
                options->isListDatabasesRequested())
        {
            executeBackupList(client);
        } else if (options->isCreateRequested())
        {
            success = client.createBackup();
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isVerifyRequested())
        {
            success = client.verifyBackup();
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isRestoreRequested())
        {
            success = client.restoreBackup();
            maybeJobRegistered(client, clusterId, success);
        } else if (options->isDeleteRequested())
        {
            success = client.deleteBackupRecord();
            client.printMessages("Deleted.", success);
            client.setExitStatus();
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
        if (options->isListRequested() || options->isWhoAmIRequested() ||
                options->isStatRequested())
        {
            executeUserList(client);
        } else if (options->isListGroupsRequested())
        {
            executeGroupList(client);
        } else if (options->isSetRequested())
        {
            success = client.setUser();
            client.printMessages("Ok.", success);
            client.setExitStatus();
        } else if (options->isDisableRequested())
        {
            success = client.disableUser();
            client.printMessages("Ok.", success);
            client.setExitStatus();
        } else if (options->isEnableRequested())
        {
            success = client.enableUser();
            client.printMessages("Ok.", success);
            client.setExitStatus();
        } else if (options->isChangePasswordRequested())
        {
            success = client.setPassword();
            client.printMessages("Ok.", success);
            client.setExitStatus();
        } else if (options->isListKeysRequested())
        {
            executePrintKeys(client);
        } else if (options->isAddKeyRequested())
        {
            success = client.addKey();
            client.printMessages("Ok.", success);
            client.setExitStatus();
        } else if (options->isSetGroupRequested())
        {
            success = client.setGroup();
            client.printMessages("Ok.", success);
            client.setExitStatus();
        } else {
            executeCreateUser(client);
        }
    } else if (options->isAccountOperation())
    {
        if (options->isListRequested())
        {
            executeAccountList(client);
        } else if (options->isCreateRequested())
        {
            success = client.createAccount();
            client.printMessages("Created.", success);
            client.setExitStatus();
        } else if (options->isGrantRequested())
        {
            success = client.grantPrivileges(
                    options->account(), 
                    options->privileges());

            client.printMessages("Grant.", success);
            client.setExitStatus();
        } else if (options->isDeleteRequested())
        {
            success = client.deleteAccount();
            client.printMessages("Created.", success);
            client.setExitStatus();
        } else {
            PRINT_ERROR("Operation is not specified.");
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
            success = client.deleteMaintenance();
            client.printMessages("Created.", success);
            client.setExitStatus();
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

bool
S9sBusinessLogic::authenticate(
        S9sRpcClient &client)
{
    S9sOptions  *options    = S9sOptions::instance();
    S9sString    errorString;
    bool         canAuthenticate;
    bool         needAuthenticate;

    canAuthenticate  = client.canAuthenticate(errorString);
    needAuthenticate = client.needToAuthenticate();

    // We can authenticate, the user intended to.
    if (canAuthenticate)
    {
        bool success = client.authenticate();
        if (!success)
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

                PRINT_ERROR("%s", STR(errorString));
            }

            // continuing, server replies a nice error
            // The lower levels set a more spcific error code.
            // options->setExitStatus(S9sOptions::AccessDenied);
        }

        S9sString controllerVersion = client.serverVersion();
        if (options->isVerbose())
        {
            printf("Controller version: %s\n", STR(controllerVersion));
        }

        // I am not sure if this is the best place, but this version
        // of the s9s CLI is not compatible with versions <= 1.4.2
        if (controllerVersion.startsWith("1.4.2") ||
            controllerVersion.startsWith("1.4.1"))
        {
            PRINT_ERROR(
                    "\n"
                    "WARNING: clustercontrol-controller <= 1.4.2 is detected.\n"
                    "Some features may be unavailable until the controller "
                    "software is upraded.\n");

            #if 0
            options->setExitStatus(S9sOptions::Failed);
            success = false;
            #endif
        }

        return success;
    }

    // Can't authenticate, but we need.
    if (needAuthenticate)
    {
        PRINT_ERROR("%s", STR(errorString));
        options->setExitStatus(S9sOptions::BadOptions);

        return false;
    }

    // We can't and we don't have to... ok then.
    return true;
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

/**
 *
 */
void 
S9sBusinessLogic::executeNodeGraph(
        S9sRpcClient &client)
{
    S9sOptions  *options   = S9sOptions::instance();
    int          clusterId = options->clusterId();
    S9sString    graphName = options->graph().toLower();
    S9sCmonGraph::GraphTemplate graphTemplate;
    S9sRpcReply  reply;
    bool         success;

    graphTemplate = S9sCmonGraph::stringToGraphTemplate(graphName);
    if (graphTemplate == S9sCmonGraph::Unknown)
    {
        PRINT_ERROR("Graph type '%s' is invalid.", STR(graphName));
        return;
    }

    success = client.getStats(clusterId, S9sCmonGraph::statName(graphTemplate));
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

    client.printMessages("OK.", success);
    client.setExitStatus();            
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
    client.printMessages("Ok.", success);
    client.setExitStatus();
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
S9sBusinessLogic::executePrintKeys(
        S9sRpcClient &client)
{
    S9S_DEBUG("");
    S9sRpcReply reply;
    bool        success;

    success = client.getKeys();
    if (success)
    {
        reply = client.reply();
        reply.printKeys();
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

void 
S9sBusinessLogic::executeAccountList(
        S9sRpcClient &client)
{
    S9sRpcReply reply;
    bool        success;

    success = client.getAccounts();
    if (success)
    {
        reply = client.reply();
        reply.printAccountList();
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}



void 
S9sBusinessLogic::executeGroupList(
        S9sRpcClient &client)
{
    S9sRpcReply reply;
    bool        success;

    success = client.getGroups();
    if (success)
    {
        reply = client.reply();
        reply.printGroupList();
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
    S9sOptions  *options     = S9sOptions::instance();
    int          clusterId   = options->clusterId();
    S9sString    clusterName = options->clusterName();
    S9sRpcReply  reply;
    bool         success;

    success = client.getJobInstances(clusterName, clusterId);
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
    bool           isTerminal      = options->isTerminal();
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

        if (options->isJsonRequested())
        {
            reply     = client.reply();
            printf("%s\n", STR(reply.toString()));
        
            fflush(stdout); 
            finished = reply.progressLine(progressLine, syntaxHighlight);

            if (finished)
                break;

            sleep(1);
        }
        
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
        
        if (!isTerminal && progressLine == previousProgressLine)
            goto end_of_loop;
        
        // This helps debug the progress values the controller send us.
        if (options->isDebug() && 
                !previousProgressLine.empty() &&
                progressLine != previousProgressLine)
        {
            printf("\n");
        }

        printf("%s %s\033[K\r", rotate[rotateCycle], STR(progressLine));

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

/**
 * \param privateKeyPath The path of the private key file.
 * \param publicKey The string where the method returns the public key.
 * \returns True if the keys exists or created.
 *
 */
bool
S9sBusinessLogic::ensureHasAuthKey(
        const S9sString &privateKeyPath,
        S9sString       &publicKey)
{
    S9sOptions    *options  = S9sOptions::instance();
    S9sRsaKey      rsaKey;
    S9sString      errorString;
    S9sString      publicKeyPath;

    if (privateKeyPath.empty())
    {
        PRINT_ERROR("The private key path is empty!");
        return false;
    }

    if (options->isGenerateKeyRequested())
    {
        // check if key exists and is valid, otherwise generate a new key-pair
        S9sRsaKey key;

        if (key.loadKeyFromFile(privateKeyPath) && key.isValid())
        {
            PRINT_VERBOSE(
                    "Keyfile '%s' exists and valid.", 
                    STR(privateKeyPath));
        } else {
            PRINT_VERBOSE(
                    "Generating an RSA key-pair (%s).", 
                    STR(privateKeyPath));

            publicKeyPath = privateKeyPath;
            publicKeyPath.replace(".key", "");
            publicKeyPath += ".pub";

            if (!key.generateKeyPair() ||
                !key.saveKeys(privateKeyPath, publicKeyPath, errorString))
            {
                if (errorString.empty())
                    errorString = "RSA key generation failure.";

                PRINT_ERROR("Key generation failed: %s", STR(errorString));
            }
        }
    }

    if (!rsaKey.loadKeyFromFile(privateKeyPath) || !rsaKey.isValid())
    {
        if (options->isGenerateKeyRequested())
        {
            PRINT_ERROR(
                    "User keyfile couldn't be loaded: %s", 
                    STR(privateKeyPath));
        }

        return false;
    }
        
    publicKeyPath = privateKeyPath;
    publicKeyPath.replace(".key", "");
    publicKeyPath += ".pub";

    S9sFile pubKeyFile(publicKeyPath);
    if (!pubKeyFile.readTxtFile(publicKey) || publicKey.empty())
    {
        // Private key exists and valid, but
        PRINT_ERROR("Could not load public key (%s): %s",
                STR(publicKeyPath), 
                STR(pubKeyFile.errorString()));

        return false;
    }

    return true;
}

void
S9sBusinessLogic::executeCreateUser(
        S9sRpcClient        &client)
{
    S9sString errorString;
   
    if (client.canAuthenticate(errorString))
    {
        executeCreateUserThroughRpc(client);
    } else {
        executeCreateUserThroughPipe(client);
    }
}

/**
 * This is the method that can "bootstrap" the Cmon User management: it can
 * create a new user through a named pipe without password or a registered RSA
 * key.
 */
void
S9sBusinessLogic::executeCreateUserThroughPipe(
        S9sRpcClient        &client)
{
    S9sString      errorString;
    S9sOptions    *options  = S9sOptions::instance();
    S9sString      userName;
    S9sString      privateKeyPath;
    S9sString      pubKeyStr;
    S9sConfigFile  config;
    bool           success;
    S9sUser        user;
    S9sVariantMap  request;
    bool           useNewRpc  = true;

    if (client.detectVersion() &&
        (client.serverVersion().startsWith("1.4.1") ||
         client.serverVersion().startsWith("1.4.2")))
    {
        // cmon <= 1.4.2 requires different user creation request
        useNewRpc = false;
    }

    PRINT_VERBOSE("Detected controller '%s', using new API: %s",
        STR(client.serverVersion()), STR_BOOL(useNewRpc));

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "One username should be passed as command line argument "
                "when creating new user.");

        options->setExitStatus(S9sOptions::BadOptions);
        return;
    }

    userName = options->extraArgument(0);

    /*
     * Now make sure that we save the specified username/controller into the 
     * user config file if there is no username/controller.
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

    if (!config.hasVariable("global", "cmon_user") &&
            !config.hasVariable("", "cmon_user"))
    {
        config.setVariable("global", "cmon_user",  userName);
    }

    if (!config.hasVariable("global", "controller") &&
            !config.hasVariable("", "controller"))
    {
        config.setVariable("global", "controller", options->controllerUrl());
    }

    if (!config.save(errorString))
    {
        PRINT_ERROR(
                "Could not update user configuration file: %s", 
                STR(errorString));
        
        return;
    }

    /*
     * Making sure we have an RSA key for the user.
     */
    privateKeyPath.sprintf("~/.s9s/%s.key", STR(userName));
    success = ensureHasAuthKey(privateKeyPath, pubKeyStr);
    if (!success)
    {
        if (useNewRpc)
        {
            if (!options->hasNewPassword())
            {
                PRINT_ERROR("No RSA key and no password for the new user.");
                options->setExitStatus(S9sOptions::Failed);
            }
        }
        else
        {
            // The old released API only supports keys.
            options->setExitStatus(S9sOptions::Failed);
        }

        return;
    }
    
    /*
     * Creating user through the pipe. 
     */
    if (options->isCreateRequested())
    {
        bool          oneSucceed = false;
        int           exitCode = 0;
        S9sString     sshCommand;
        S9sString     sudoCommand = "sudo ";
        S9sStringList fifos;

        S9sString controller = options->controllerHostName();
        if (controller.empty())
            controller = "127.0.0.1";

        // this one is used by unit/functional tests
        if (S9sFile("/tmp/cmon_test/usermgmt.fifo").exists())
            fifos << "/tmp/cmon_test/usermgmt.fifo";

        // and in real cmon daemon
        fifos << "/var/lib/cmon/usermgmt.fifo";

        if (useNewRpc)
        {
            /*
             * We create a user object, then we create a createUser request.
             */
            user.setProperty("user_name",     userName);
            user.setProperty("title",         options->title());
            user.setProperty("first_name",    options->firstName());
            user.setProperty("last_name",     options->lastName());
            user.setProperty("email_address", options->emailAddress());
            user.setGroup(options->group());
            user.setPublicKey("No Name", pubKeyStr);

            request = S9sRpcClient::createUserRequest(
                    user, 
                    options->newPassword(),
                    options->createGroup());
            S9S_WARNING("-> \n%s", STR(request.toString()));
        }
        else
        {
            /*
             * For controller <= 1.4.2
             */
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
        }

        for (uint idx = 0; idx < fifos.size(); ++idx)
        {
            S9sString     escapedJson;
            S9sString     path = fifos.at(idx);
            S9sFile       file(path);

            // 
            // Building up a request for the user creation.
            //
            escapedJson = request.toString().escape();

            if (options->isJsonRequested())
                printf("Request: %s\n", STR(request.toString()));
            
            PRINT_VERBOSE("escapedJson: \n%s", STR(escapedJson));

            if (controller == "localhost" || controller == "127.0.0.1")
            {
                if (geteuid() == 0)
                {
                    // The effective user is root, no need to use sudo
                    sudoCommand = "";
                }

                sshCommand.sprintf(
                    "echo \"%s\" "
                    "| %s tee %s >/dev/null",
                    STR(escapedJson),
                    STR(sudoCommand),
                    STR(path));
            } else {
                if (S9sString(getenv("USER")) == "root")
                {
                    // The username for SSH is root, sudo not needed
                    sudoCommand = "";
                }

                sshCommand.sprintf(
                    "ssh -tt "
                    "-oUserKnownHostsFile=/dev/null "
                    "-oStrictHostKeyChecking=no "
                    "-oBatchMode=yes "
                    "-oPasswordAuthentication=no "
                    "-oConnectTimeout=30 "
                    " '%s' "
                    "'echo \"%s\" "
                    "| %s tee %s >/dev/null'",
                    STR(controller),
                    STR(escapedJson),
                    STR(sudoCommand),
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
S9sBusinessLogic::executeCreateUserThroughRpc(
        S9sRpcClient        &client)
{
    S9sString      errorString;
    S9sOptions    *options  = S9sOptions::instance();
    S9sString      userName;
    S9sString      privateKeyPath;
    S9sString      pubKeyStr;
    S9sConfigFile  config;
    S9sUser        user;
    S9sVariantMap  request;
    bool           success;

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "One username should be passed as command line argument "
                "when creating new user.");

        options->setExitStatus(S9sOptions::BadOptions);
        return;
    }

    userName = options->extraArgument(0);
    
    /*
     * Now make sure that we save the specified username/controller into the 
     * user config file if there is no username/controller.
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

    if (!config.hasVariable("global", "cmon_user") &&
            !config.hasVariable("", "cmon_user"))
    {
        config.setVariable("global", "cmon_user",  userName);
    }

    if (!config.hasVariable("global", "controller") &&
            !config.hasVariable("", "controller"))
    {
        config.setVariable("global", "controller", options->controllerUrl());
    }

    if (!config.save(errorString))
    {
        PRINT_ERROR(
                "Could not update user configuration file: %s", 
                STR(errorString));
        
        return;
    }
    
    /*
     * Making sure we have an RSA key for the user.
     */
    privateKeyPath.sprintf("~/.s9s/%s.key", STR(userName));
    success = ensureHasAuthKey(privateKeyPath, pubKeyStr);
    if (!success)
    {
        if (options->isGenerateKeyRequested())
        {
            options->setExitStatus(S9sOptions::Failed);
            return;
        }
    }

        
    /*
     * We create a user object, then we create a createUser request.
     */
    user.setProperty("user_name",     userName);
    user.setProperty("title",         options->title());
    user.setProperty("first_name",    options->firstName());
    user.setProperty("last_name",     options->lastName());
    user.setProperty("email_address", options->emailAddress());
    user.setGroup(options->group());
    user.setPublicKey("No Name", pubKeyStr);

    success = client.createUser(
            user, options->newPassword(), options->createGroup());

    client.printMessages("User created.", success);
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

