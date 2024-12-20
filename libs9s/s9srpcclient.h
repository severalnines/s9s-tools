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
#pragma once

#include "s9sstring.h"
#include "s9srpcreply.h"

class S9sRpcClientPrivate;
class S9sUser;

typedef void (*S9sJSonHandler)(const S9sVariantMap &jsonMessage, void *userData);

class S9sRpcClient
{
    public:
        S9sRpcClient();

        S9sRpcClient(
                const S9sString &hostName,
                const int        port,
                const S9sString &path,
                const bool       useTls);

        S9sRpcClient(const S9sRpcClient &orig);

        virtual ~S9sRpcClient();

        S9sRpcClient &operator=(const S9sRpcClient &rhs);

        S9sString hostName() const;
        int port() const;
        bool useTls() const;

        bool hasPrivateKey() const;
        bool canAuthenticate(S9sString &reason) const;
        bool needToAuthenticate() const;
        bool isAuthenticated() const;
        bool maybeAuthenticate();

        bool detectVersion();
        S9sString serverVersion() const;

        const S9sRpcReply &reply() const;
        void setExitStatus();

        S9sString errorString() const;
        void printMessages(
                const S9sString &defaultMessage,
                bool             success);

        void printServerRegistered(bool success);

        bool authenticate();
        bool authenticateWithKey();
        bool authenticateWithPassword();

        /*
         * The executers that send an RPC request and receive an RPC reply from
         * the server.
         */
        bool getClusters(
                bool withHosts      = true,
                bool withSheetInfo  = true);

        bool getCluster(const S9sString &clusterName, const int clusterId);
        bool getSqlProcesses();
        bool getTopQueries();

        bool getTree(bool withDotDot = false);
        bool getDatabases();

        
        bool checkHosts();
        bool getSupportedClusterTypes();
        bool checkClusterName();
        bool getSshCredentials();

        bool registerServers();
        bool unregisterServers();
        bool createServer();

        bool renameOrMove();

        bool moveInTree(
                const S9sString &sourcePath, 
                const S9sString &targetPath);

        bool rename(
                const S9sString &sourcePath,
                const S9sString &targetName);

        
        bool deleteFromTree();
        bool deleteFromTree(const S9sString &path);

        bool checkAccess();
        bool cat();
        bool getObject();
        bool getObject(const S9sString &path);
        bool getAcl();
        bool addAcl();
        bool removeAcl();
        bool addTag();
        bool removeTag();
        bool chOwn();

        bool mkdir();
        bool mkdir(const S9sString &fullPath);
        bool rmdir();

        bool mkfile();
        bool mkfile(const S9sString &fullPath);

        bool enableCmonHa();

        bool setContent(const S9sString &fullPath, const S9sString &content);
        bool setContent();

        bool startServers();
        bool stopServers();
        bool startInTree();

        bool unregisterHost();
        bool registerHost();

        bool getContainers();
        bool getServers();
        bool getControllers();
        bool createContainerWithJob();
        bool deleteContainerWithJob();
        bool startContainerWithJob();
        bool stopContainerWithJob();

        bool getConfig(const S9sVariantList &hosts);
        bool setConfig();
        bool unsetConfig();

        bool getLdapConfig();
        bool setLdapConfig();

        bool pingCluster();
        bool pingController();

        bool setHost();
        
        bool setHost(
                const S9sVariantList &hosts,
                const S9sVariantMap  &properties);

        bool getCpuInfo(const int clusterId);
        bool getInfo();
        
        
        bool getStats(
                const int        clusterId,
                const S9sString &statName);

        bool getCpuStats(const int clusterId);
        bool getSqlStats(const int clusterId);
        bool getMemStats(const int clusterId);

        bool getMetaTypes();
        bool getMetaTypeProperties(const S9sString &typeName);
        bool getMemoryStats(const int clusterId);

        bool getRunningProcesses();

        // Methods related to jobs.
        bool getJobInstances(
                const S9sString  &clusterName, 
                const int         clusterId);

        bool deleteJobInstance(const int jobId);
        bool killJobInstance(const int jobId);
        bool cloneJobInstance(const int jobId);
        bool enableJobInstance(const int jobId);
        bool disableJobInstance(const int jobId);
     
        /*
         * Backup related methods.
         */
        bool createBackup();
        bool createBackupSchedule();
        bool deleteBackupSchedule();
        bool createSnapshotRepository();
        bool verifyBackup();
        bool restoreBackup();
        bool deleteOldBackups();
        bool deleteAllBackups();
        
        bool getBackups(const int clusterId);
        bool getBackupSchedules(const int clusterId);
        bool getSnapshotRepositories(const int clusterId);

        bool deleteBackupRecord();
        bool deleteSnapshotRepository();

        /*
         * Account&database handling.
         */
        bool createAccount();
        bool getAccounts();

        bool getClusterConfig();
        bool setClusterConfig();

        bool getSpreadsheets();
        bool getSpreadsheet();
        bool createSpreadsheet();
        
        bool setCell(
                const S9sString &spreadsheetName,
                const int        sheetIndex,
                const int        columnIndex,
                const int        rowIndex,
                const S9sString &content);

        bool getRepositories();
        bool getSupportedSetups();

        bool grantPrivileges(
                const S9sAccount &account,
                const S9sString  &privileges);

        bool revokePrivileges(
                const S9sAccount &account,
                const S9sString  &privileges);

        bool subscribeEvents(
                S9sJSonHandler  callbackFunction,
                void           *userData);

        bool deleteAccount();
        bool createDatabase();
        bool createDeleteDatabaseJob();

        /*
         * Requests related to scripts.
         */
        bool saveScript(
                S9sString remoteFileName, 
                S9sString content);

        bool executeExternalScript(
                S9sString localFileName,
                S9sString content,
                S9sString arguments);

        bool executeCdtEntry(
                const S9sString &cdtPath);
        
        bool executeCdtEntry();

        bool executeScript(
                S9sString remoteFileName,
                S9sString arguments);

        bool removeScript(
                S9sString remoteFileName);

        bool treeScripts();
        bool inspectHost();
        bool executeSystemCommand(const S9sVariant shellCommand);
        bool executeSystemCommand(const S9sVariantList &scriptLines);

        /*
         * Requests related to the cmon users.
         */
        static S9sVariantMap 
            createUserRequest(
                    const S9sUser   &user,
                    const S9sString &newPassword,
                    bool             createGroup,
                    bool             forcePasswordUpdate);

        bool createUser(
                const S9sUser   &user,
                const S9sString &newPassword,
                bool             createGroup,
                bool             forcePasswordUpdate);

        
        bool addToGroup();
        bool addToGroup(
                const S9sUser     &user,
                const S9sString   &groupName,
                bool               replacePrimaryGroup);

        bool setGroup();
        bool resetPassword();

        bool deleteUser();
        bool removeFromGroup();

        bool getUsers();
        bool whoAmI();
        bool canCreateUser();
        bool setUser();
        bool enableUser();
        bool disableUser();
        bool getKeys();
        bool addKey();
        bool setPassword();

        /*
         * Requests related to cmon groups.
         */
        bool getGroups();
        bool createGroup();
        bool deleteGroup();

        /*
         * Requests related to user preferences.
         */
        bool setUserPreferences();
        bool getUserPreferences();
        bool deleteUserPreferences();

        /*
         * Requests related to db versions on clusters
         */
        bool getDbVersions(const S9sString clusterType,
                           const S9sString vendor,
                           const bool      patchNumber,
                           const bool      useVendorApi);
        bool getClusterTypes();
        bool getVendors();

        /*
         * Requests related to cloud credentials operations
         */
        bool listCloudCredentials();
        bool createCloudCredentials(S9sOptions *options);
        bool deleteCloudCredentials(const int & credentialId, const S9sString & provider);

        /*
         * Requests related to logical replication
         */
        bool addPublication();
        bool modifyPublication();
        bool dropPublication();
        bool listPublications();
        bool addSubscription();
        bool modifySubscription();
        bool dropSubscription();
        bool listSubscriptions();

        /*
         *
         */
        bool getJobInstance(const int jobId);
        bool getJobInstanceForWait(const int jobId);
        
        bool getJobLog(
                const int  jobId,
                const int  limit   = 0,
                const int  offset  = 0,
                const bool printRequest = true);

        bool getLog();
        bool getLogStatistics();
        bool getAlarms();
        bool getAlarm();
        bool ignoreAlarm();
        bool getAlarmStatistics();
        bool generateReport();
        bool deleteReport();
        bool getReport();

        bool getReports();
        bool getReportTemplates();

        bool rollingRestart();
        bool importConfig();
        bool collectLogs();
        bool enableSsl();
        bool disableSsl();
        bool saveCluster();
        bool restoreCluster();
        bool saveController();
        bool restoreController();
        bool setupAuditLogging(const int clusterId);
        bool setupLogRotate(const int clusterId);

        bool createReport(const int clusterId);

        bool deployAgents(const int clusterId);
        bool deployCmonAgents(const int clusterId);
        bool uninstallCmonAgents(const int clusterId);

        bool createLocalRepository(
                const int          clusterId,
                const S9sString   &clusterType,
                const S9sString   &vendor,
                const S9sString   &dbVersion,
                const S9sString   &osRelease);

        bool createSnapshotJob();
        bool createCluster();
        bool registerCluster();
        bool createNode();
        bool reinstallNode();
        bool reconfigureNode();
        bool removeNode();
        bool syncClusters();

        bool stopCluster();
        bool startCluster();
        bool dropCluster();

        bool createMaintenanceWithJob();
        bool disableRecoveryWithJob();
        bool enableRecoveryWithJob();

        bool startNode();
        bool startSlave();
        bool promoteReplicationSlave();
        bool resetSlave();
        
        bool stopNode();
        bool stopSlave();
        bool setNodeReadOnly();
        bool setNodeReadWrite();

        bool enableBinaryLogging();

        bool setClusterReadOnly();
        
        bool failoverMaster();
        bool stageSlave();
        bool toggleSync();

        bool restartNode();
        bool promoteSlave();
        bool demoteNode();

        bool availableUpgrades();
        bool upgradeCluster();
        bool checkPkgUpgrades();

        bool createMaintenance();

        bool createMaintenance(
                const S9sVariantList &hosts,
                const S9sString      &start,
                const S9sString      &end,
                const S9sString      &reason);

        bool createMaintenance(
                const int            &clusterId,
                const S9sString      &start,
                const S9sString      &end,
                const S9sString      &reason);

        bool deleteMaintenance();

        bool deleteMaintenance(
                const S9sString      &uuid);

        bool getMaintenance();
        bool getCurrentMaintenance();
        bool getNextMaintenance();
        bool createFailJob();
        bool createSuccessJob();

        bool getDbGrowth();

    protected:
        virtual S9sVariantMap composeRequest();
        virtual S9sVariantMap composeJob() const;
        virtual S9sVariantMap composeJobData(
                bool argumentsAreContainers = false) const;
        virtual void addCredentialsToJobData(S9sVariantMap & jobData) const;
        virtual S9sVariantMap composeBackupJob();
        virtual S9sVariantMap composeJobDataOneContainer() const;
        
        virtual bool executeRequest(
                const S9sString &uri,
                S9sVariantMap   &request,
                bool             printRequest = true,
                S9s::Redirect    redirect = S9s::AllowRedirect);

        virtual bool doExecuteRequest(
                const S9sString &uri,
                S9sVariantMap   &request,
                S9s::Redirect    redirect = S9s::AllowRedirect);

        void setError(
                const S9sString &errorString,
                const S9sString &errorCode = "ConnectError");

        void printRequestJson(S9sVariantMap &request);

        void saveRequestAndReply(
                    const S9sVariantMap &request,
                    const S9sVariantMap &reply) const;

    private:
        bool startNodeJob(
                const S9sString &command,
                const S9sString &title);
        
        // Low level methods that create/register new clusters.
        bool createMySqlSingleCluster(
                const S9sVariantList &hosts,
                const S9sString      &osUserName,
                const S9sString      &vendor,
                const S9sString      &mySqlVersion);

        bool createGaleraCluster(
                const S9sVariantList &hosts,
                const S9sString      &osUserName,
                const S9sString      &vendor,
                const S9sString      &mySqlVersion);

        bool registerGaleraCluster(
                const S9sVariantList &hosts,
                const S9sString      &osUserName);

        bool createMySqlReplication(
                const S9sVariantList &hosts,
                const S9sString      &osUserName,
                const S9sString      &vendor,
                const S9sString      &mySqlVersion);
        
        bool registerMySqlReplication(
                const S9sVariantList &hosts,
                const S9sString      &osUserName);
        
        bool createGroupReplication(
                const S9sVariantList &hosts,
                const S9sString      &osUserName,
                const S9sString      &vendor,
                const S9sString      &mySqlVersion);

        bool registerGroupReplication(
                const S9sVariantList &hosts,
                const S9sString      &osUserName);

        bool createNdbCluster(
                const S9sVariantList &mySqlHosts,
                const S9sVariantList &mgmdHosts,
                const S9sVariantList &ndbdHosts,
                const S9sString      &osUserName, 
                const S9sString      &vendor,
                const S9sString      &mySqlVersion);
        
        bool registerNdbCluster(
                const S9sVariantList &mySqlHosts,
                const S9sVariantList &mgmdHosts,
                const S9sVariantList &ndbdHosts,
                const S9sString      &osUserName);

        bool createPostgreSql(
                const S9sVariantList &hosts,
                const S9sString      &osUserName,
                const S9sString      &psqlVersion);

        bool createLogicalPostgreSql(
                const S9sVariantList &hosts,
                const S9sString      &osUserName,
                const S9sString      &psqlVersion);

        bool createRedisOrValkeySharded(
                const S9sVariantList &hosts,
                const S9sString      &osUserName,
                const S9sString      &redisVersion,
                const S9sString      &clusterType);

        bool redisSpecificJobData(S9sOptions *options,
                                  S9sVariantMap &jobData,
                                  const S9sString &version);
        bool valkeySpecificJobData(S9sOptions *options,
                                   S9sVariantMap &jobData,
                                   const S9sString &version);

        bool createRedisOrValkeySentinel(
                const S9sVariantList &hosts,
                const S9sString      &osUserName,
                const S9sString      &providerVersion,
                const S9sString      &clusterType);

        bool registerRedisOrValkeyCluster(
                const S9sVariantList &hosts,
                const S9sString      &osUserName,
                const S9sString      &providerVersion,
                const S9sString      &clusterType);

        bool registerRedisOrValkeyShardedCluster(
                const S9sVariantList &hosts,
                const S9sString      &osUserName,
                const S9sString      &providerVersion,
                const S9sString      &clusterType);

        bool createElasticsearch(
                const S9sVariantList &hosts,
                const S9sString      &osUserName,
                const S9sString      &elasticVersion);

        bool registerElasticsearchCluster(
                const S9sVariantList &hosts,
                const S9sString      &osUserName);

        bool createMsSqlSingle(
                const S9sVariantList &hosts,
                const S9sString      &osUserName,
                const S9sString      &psqlVersion);

        bool createMongoCluster(
                const S9sVariantList &hosts,
                const S9sString      &osUserName,
                const S9sString      &vendor,
                const S9sString      &mongoVersion);

        bool registerPostgreSql(
                const S9sVariantList &hosts,
                const S9sString      &osUserName);

        bool registerMongoDbCluster(
                const S9sVariantList &hosts,
                const S9sString      &osUserName);

        // Low level methods that validate logical replication requests.
        bool validateSubClusterRequestParams(
                S9sVariantMap        &request,
                S9sOptions           *options) const;
        bool validatePublicationRequestParams(
                S9sVariantMap        &request,
                S9sOptions           *options,
                bool                 includePubName = true) const;
        bool validateSubscriptionRequestParams(
                S9sVariantMap        &request,
                S9sOptions           *options,
                bool                 includePubName = true) const;
        
        // Low level methods that create/install a new node and add it to a
        // cluster.
        bool addNode(
                const S9sVariantList &hosts);
        
        bool addReplicationSlave(
                const S9sVariantList &hosts);

        bool addHaProxy(
                const S9sVariantList &hosts);
        
        bool addKeepalived(
                const S9sVariantList &hosts);

        bool addPgBouncer(
                const S9sVariantList &hosts);

        bool addPgBackRest(
                const S9sVariantList &hosts);

        bool addPBMAgent(
                const S9sVariantList &hosts);

        bool addNFSClient(
                const S9sVariantList &hosts);

        bool addNFSServer(
                const S9sVariantList &hosts);
        
        bool addProxySql(
                const S9sVariantList &hosts);

        bool addMaxScale(
                const S9sVariantList &hosts);

        bool addMongoNode(
                const S9sVariantList &hosts);

        bool addElasticNode(
                const S9sVariantList &hosts);

        bool addRedisShardedNode(
                const S9sVariantList &hosts);

        // Low level methods that reinstalls software on a node as an attempt to fix it.

        bool reinstallPBMAgent(
                const S9sVariantList &hosts);

        bool reinstallNFSClient(
                const S9sVariantList &hosts);

        bool reinstallPgBackRest(
                const S9sVariantList &hosts);

        // Low level methods that reconfigures a node as an attempt to fix it.

        bool reconfigurePBMAgent(
                const S9sVariantList &hosts);

        bool reconfigureNFSClient(
                const S9sVariantList &hosts);

        bool reconfigurePgBackRest(
                const S9sVariantList &hosts);

        static S9sVariant 
            topologyField(
                const S9sVariantList &nodes);

        static S9sVariant
            nodesField() ;

        static S9sVariant
            nodesField(
                const S9sVariantList &nodes) ;
        
        static S9sVariant
            serversField(
                const S9sVariantList &servers,
                bool mustHaveSshCredentials = false);

        static S9sString timeStampString();

    private:
        S9sRpcClientPrivate *m_priv;

        friend class UtS9sRpcClient;
        friend class UtS9sNode;
};

bool isValidDateTimeFormat(const std::string& str);
