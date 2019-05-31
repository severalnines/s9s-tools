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

#define EXPERIMENTAL_CODE


#include "S9sString"
#include "S9sVariant"
#include "S9sVariantMap"
#include "S9sConfigFile"

class S9sDateTime;
class S9sSshCredentials;

#define PRINT_VERBOSE(...) \
    S9sOptions::printVerbose(__VA_ARGS__)

#define PRINT_ERROR(...) \
    S9sOptions::printError(__VA_ARGS__)

/**
 * Singleton class to handle s9s command line options and settings.
 */
class S9sOptions
{
    public:
        static S9sOptions *instance();
        static void uninit();

        enum OperationMode 
        {
            NoMode  = 0,
            Account,
            Backup,
            Cluster,
            Container,
            Job,
            Log,
            Maintenance,
            MetaType,
            Node,
            Process,
            Script,
            Sheet,
            Server,
            Controller,
            Tree,
            User,
            Event,
            Alarm
        };

        enum ExitCodes
        {
            ExitOk          = 0,
            JobFailed       = 1,
            Failed          = 2,
            // Authentication failed.
            AccessDenied    = 3,
            NotFound        = 4,
            // Error communicating with the server.
            ConnectionError = 5,
            // Bad command line options or config file values.
            BadOptions      = 6,
        };

        bool readOptions(int *argc, char *argv[]);
        bool executeInfoRequest();

        void createConfigFiles();
        bool loadConfigFiles();

        void setController(const S9sString &url);
        S9sString controllerHostName();
        int controllerPort();
        S9sString controllerProtocol();
        S9sString controllerPath();

        S9sString controllerUrl();


        S9sString configFile() const;
       
        
        void enableEventType(const S9sString &eventTypeName);
        bool eventTypeEnabled(const S9sString &eventTypeName);
        void enableEventName(const S9sString &eventName);
        void disableEventName(const S9sString &eventName);
        bool eventNameEnabled(const S9sString &eventName);

        bool onlyAscii() const;
        int clientConnectionTimeout() const;
        
        bool density() const;
        bool setPropertiesOption(const S9sString &assignments);
        S9sVariantMap propertiesOption() const;

        bool setNodes(const S9sString &value);
        S9sVariantList nodes() const;
        
        bool setServers(const S9sString &value);
        S9sVariantList servers() const;

        bool setContainers(const S9sString &value);
        bool hasContainers() const;
        S9sVariantList containers() const;

        S9sString inputFile() const;
        S9sString outputFile() const;
        S9sString logFile() const;

        S9sString briefJobLogFormat() const;
        S9sString briefLogFormat() const;
        S9sString longLogFormat() const;

        S9sString longJobLogFormat() const;
        S9sString longClusterFormat() const;
        S9sString longNodeFormat() const;
        S9sString shortNodeFormat() const;

        S9sString longBackupFormat() const;
        S9sString longUserFormat() const;
        
        S9sString vendor() const;
        S9sString providerVersion(const S9sString &defaultValue = "") const;
        S9sString osSudoPassword() const;

        bool hasSshCredentials();
        S9sSshCredentials
            sshCredentials(
                    const S9sString &categoryName,
                    const S9sString &hostName);


        S9sString osUser(bool defaultsToCmonUser = true) const;
        S9sString osKeyFile() const;
        S9sString osPassword() const;

        S9sString dbAdminUserName(const S9sString &defaultValue = "") const;
        S9sString dbAdminPassword();
        S9sString clusterType() const;
        S9sString formatDateTime(S9sDateTime value) const;
        S9s::AddressType addressType() const;

        bool fullUuid() const;

        S9sString schedule() const;
        S9sString recurrence() const;
        
        int timeout() const;
        bool hasTimeout() const;

        int limit() const;
        int offset() const;
        
        int clusterId() const;
        bool hasClusterIdOption() const;
        
        bool forceOption() const;
        bool hasForceOption() const;

        int alarmId() const;
        bool hasAlarmIdOption() const;

        bool hasCredentialIdOption() const;
        int credentialId() const;
        
        bool hasBackupId() const;
        int backupId() const;

        S9sString backupUser() const;
        S9sString backupPassword() const;

        bool setBackupRetention(const S9sString &value);
        bool hasBackupRetention() const;
        int backupRetention() const;

        bool setCloudRetention(const S9sString &value);
        bool hasCloudRetention() const;
        int cloudRetention() const;

        bool setSafetyCopies(const S9sString &value);
        bool hasSafetyCopies() const;
        int safetyCopies() const;

        bool encryptBackup() const;
        int updateFreq() const;
        S9sString type() const;

        S9sString clusterName() const;
        bool hasClusterNameOption();

        bool hasJobId() const;
        int jobId() const;

        bool hasLogFormat() const;
        S9sString logFormat() const;
        
        bool hasClusterFormat() const;
        S9sString clusterFormat() const;

        bool hasNodeFormat() const;
        S9sString nodeFormat() const;

        bool hasBackupFormat() const;
        S9sString backupFormat() const;

        bool hasMemory() const;
        S9sString memory() const;

        bool hasContainerFormat() const;
        S9sString containerFormat() const;
        
        bool hasUserFormat() const;
        S9sString userFormat() const;

        S9sString graph() const;

        S9sString userName( const bool tryLocalUserToo = false) const;
        bool hasPassword() const;
        S9sString password() const;

        bool hasOldPassword() const;
        S9sString oldPassword() const;
        
        bool hasNewPassword() const;
        S9sString newPassword() const;

        S9sString privateKeyPath() const;
        S9sString publicKeyPath() const;
        S9sString publicKeyName() const;

        S9sAccount account() const;
        bool setAccount(const S9sString &value);
        S9sString accountName() const;

        bool withDatabase() const;
        bool withTimescaleDb() const;

        S9sString dbName() const;
        S9sString privileges() const;
        
        S9sString optGroup() const;
        S9sString optName() const;
        S9sString optValue() const;
        S9sString outputDir() const;
        bool maskPasswords() const;
        S9sString donor() const;
        S9sString templateName() const;
        S9sString configTemplate() const;
        S9sString token() const;
        bool noInstall() const;
        S9sString cloudName() const;
        S9sString subnetId() const;
        S9sString vpcId() const;
        S9sString imageName() const;
        S9sString imageOsUser() const;
        S9sVariantList volumes() const;
        bool appendVolumes(const S9sString &stringRep);

        bool force() const;
        bool dry() const;
        bool extended() const;

        bool useInternalRepos() const;

        S9sString tempDirPath() const;
        bool keepTempDir() const;
        
        S9sString subDirectory() const;
        S9sString backupDir() const;
        S9sString backupMethod() const;
        bool noCompression() const;
        bool pitrCompatible() const;
        bool usePigz() const;
        bool onNode() const;
        bool onController() const;
        S9sString databases() const;
      
        bool hasFirewalls() const;
        S9sString firewalls() const;

        S9sString region() const;
        S9sString shellCommand() const;
        
        bool hasJobTags() const;
        S9sVariantList jobTags() const;
        bool setJobTags(const S9sString &value);

        S9sVariantList withTags() const;
        bool setWithTags(const S9sString &value);
        S9sVariantList withoutTags() const;
        bool setWithoutTags(const S9sString &value);

        bool setParallellism(const S9sString &value);
        bool hasParallellism() const;
        int parallellism() const;

        bool setRetention(const S9sString &value);
        bool hasRetention() const;
        int retention() const;

        bool fullPathRequested() const;
        bool toIndividualFiles() const;
        bool backupDatadir() const;

        bool useTls();

        bool isNodeOperation() const;
        bool isLogOperation() const;
        bool isEventOperation() const;
        bool isAlarmOperation() const;
        bool isScriptOperation() const;
        bool isSheetOperation() const;
        bool isServerOperation() const;
        bool isControllerOperation() const;
        bool isTreeOperation() const;
        bool isClusterOperation() const;
        bool isContainerOperation() const;
        bool isJobOperation() const;
        bool isBackupOperation() const;
        bool isProcessOperation() const;
        bool isUserOperation() const;
        bool isAccountOperation() const;
        bool isMaintenanceOperation() const;
        bool isMetaTypeOperation() const;

        bool isGenerateKeyRequested() const;
        S9sString group() const;
        bool createGroup() const;
        S9sString lastName() const;
        S9sString firstName() const;
        S9sString title() const;
        S9sString testServer() const;

        S9sString emailAddress() const;

        bool isHelpRequested() const;
        bool isListRequested() const;
        bool isEnableCmonHaRequested() const;
        bool isCreateSnapshotRequested() const;
        bool isListPartitionsRequested() const;
        bool isListImagesRequested() const;
        bool isListRegionsRequested() const;
        bool isListSubnetsRequested() const;
        bool isListTemplatesRequested() const;
        bool isListMemoryRequested() const;
        bool isGetAclRequested() const;
        bool isCatRequested() const;
        bool isAccessRequested() const;
        bool isAddAclRequested() const;
        bool isRemoveAclRequested() const;
        bool isChOwnRequested() const;
        bool isMkdirRequested() const;
        bool isRmdirRequested() const;
        bool isMkfileRequested() const;
        bool isSaveRequested() const;

        S9sString acl() const;

        bool hasOwner() const;
        S9sString ownerUserName() const;
        S9sString ownerGroupName() const;

        bool isListProcessorsRequested() const;
        bool isListNicsRequested() const;
        bool isListDisksRequested() const;
        bool isListGroupsRequested() const;
        bool isStatRequested() const;
        bool isWatchRequested() const;
        bool isEditRequested() const;
        bool isListConfigRequested() const;
        bool isChangeConfigRequested() const;
        bool isPullConfigRequested() const;
        bool isPushConfigRequested() const;
        bool isListPropertiesRequested() const;
        bool isListClusterTypesRequested() const;
        bool isListContainersRequested() const;
        bool isWhoAmIRequested() const;
        bool isListKeysRequested() const;
        bool isAddKeyRequested() const;
        bool isSetRequested() const;
        bool isChangePasswordRequested() const;
        bool isLogRequested() const;
        bool isCreateRequested() const;
        bool isRegisterRequested() const;
        bool isRefreshRequested() const;
        bool isAllRequested() const;
        bool isRecursiveRequested() const;
        bool isDirectoryRequested() const;
        bool isUnregisterRequested() const;
        bool isMoveRequested() const;
        bool isDeleteRequested() const;
        bool isCloneRequested() const;
        bool isFailRequested() const;
        bool isSuccessRequested() const;
        bool isEnableRequested() const;
        bool isSetGroupRequested() const;
        bool isAddToGroupRequested() const;
        bool isRemoveFromGroupRequested() const;
        bool isPasswordResetRequested() const;
        bool isDisableRequested() const;
        bool isPingRequested() const;
        bool isPromoteSlaveRequested() const;
        bool isDemoteNodeRequested() const;
        bool isRestoreRequested() const;
        bool isSaveClusterRequested() const;
        bool isRestoreClusterRequested() const;
        bool isSaveControllerRequested() const;
        bool isRestoreControllerRequested() const;
        bool isVerifyRequested() const;
        bool isDeleteOldRequested() const;
        bool isRollingRestartRequested() const;
        bool isImportConfigRequested() const;
        bool isCollectLogsRequested() const;
        bool isEnableSslRequested() const;
        bool isDisableSslRequested() const;
        bool isSetupAuditLoggingRequested() const;
        bool isCreateReportRequested() const;
        bool isDeployAgentsRequested() const;
        bool isAddNodeRequested() const;
        bool isRemoveNodeRequested() const;
        bool isStopRequested() const;
        bool isStartRequested() const;
        bool isRestartRequested() const;
        bool isCreateAccountRequested() const; 
        bool isGrantRequested() const;
        bool isCheckHostsRequested() const;
        bool isDeleteAccountRequested() const; 
        bool isCreateDatabaseRequested() const; 
        bool isListDatabasesRequested() const; 
        bool isListFilesRequested() const; 
        bool isDropRequested() const;
        bool isExecuteRequested() const;
        bool isRunRequested() const;
        bool isKillRequested() const;
        bool isSystemRequested() const;
        bool isTreeRequested() const;

        bool isLongRequested() const;
        bool isJsonRequested() const;
        bool isTopRequested() const;
        bool isWaitRequested() const;
        bool isBatchRequested() const;
        bool isNoHeaderRequested() const;
        bool isStringMatchExtraArguments(const S9sString &theString) const;
        uint nExtraArguments() const;
        S9sString extraArgument(uint idx);

        bool useSyntaxHighlight();
        bool truncate();
        bool humanReadable() const;
        void setHumanReadable(const bool value = true);

        S9sString timeStyle() const;

        S9sString start() const;
        S9sString begin() const;
        S9sString end() const;
        S9sString reason() const;
        S9sString uuid() const;

        S9sString from() const;
        S9sString until() const;

        static bool isTerminal();
        static int terminalWidth();
        static int terminalHeight();
        S9sString binaryName() const;
        S9sString errorString() const;
        
        int exitStatus() const;
        void setExitStatus(const S9sOptions::ExitCodes exitStatus);

        bool isVerbose() const;
        void setVerbose(bool value);

        bool isDebug() const;

        static void printVerbose(const char *formatString, ...);
        static void printError(const char *formatString, ...);

        void printHelp();
        
        bool getBool(const char *key) const;
        S9sString getString(const char *key) const;        
        int getInt(const char *key) const;        
        S9sVariantMap getVariantMap(const char *key) const;

        S9sString defaultUserConfigFileName() const;
        S9sString defaultSystemConfigFileName() const;

        S9sString userStateFilename() const;
        bool loadStateFile();
        bool writeStateFile();

        bool setState(
                const S9sString    &key,
                const S9sVariant   &value);
        
        S9sVariant getState(const S9sString    &key);

    private:
        void checkController();
        void printHelpGeneric();
        void printHelpCluster();
        void printHelpContainer();
        void printHelpNode();
        void printHelpUser();
        void printHelpAccount();
        void printHelpJob();
        void printHelpLog();
        void printHelpEvent();
        void printHelpAlarm();
        void printHelpProcess();
        void printHelpBackup();
        void printHelpMaintenance();
        void printHelpMetaType();
        void printHelpScript();
        void printHelpSheet();
        void printHelpServer();
        void printHelpController();
        void printHelpTree();

        bool readOptionsNoMode(int argc, char *argv[]);
        
        bool readOptionsNode(int argc, char *argv[]);
        bool checkOptionsNode();

        bool readOptionsBackup(int argc, char *argv[]);
        bool checkOptionsBackup();

        bool readOptionsCluster(int argc, char *argv[]);
        bool checkOptionsCluster();
        
        bool readOptionsContainer(int argc, char *argv[]);
        bool checkOptionsContainer();

        bool readOptionsJob(int argc, char *argv[]);
        bool checkOptionsJob();

        bool readOptionsProcess(int argc, char  *argv[]);
        bool checkOptionsProcess();
        
        bool readOptionsUser(int argc, char *argv[]);
        bool checkOptionsUser();
        
        bool readOptionsAccount(int argc, char *argv[]);
        bool checkOptionsAccount();

        bool readOptionsMaintenance(int argc, char *argv[]);
        bool checkOptionsMaintenance();

        bool readOptionsScript(int argc, char *argv[]);
        bool checkOptionsScript();
        
        bool readOptionsSheet(int argc, char *argv[]);
        bool checkOptionsSheet();
        
        bool readOptionsServer(int argc, char *argv[]);
        bool checkOptionsServer();
        
        bool readOptionsController(int argc, char *argv[]);
        bool checkOptionsController();
        
        bool readOptionsTree(int argc, char *argv[]);
        bool checkOptionsTree();

        bool readOptionsMetaType(int argc, char *argv[]);
        bool checkOptionsMetaType();
        
        bool readOptionsLog(int argc, char *argv[]);
        bool checkOptionsLog();
        
        bool readOptionsEvent(int argc, char *argv[]);
        bool checkOptionsEvent();
        
        bool readOptionsAlarm(int argc, char *argv[]);
        bool checkOptionsAlarm();

        bool setMode(const S9sString &modeName);

        S9sOptions();
        ~S9sOptions();

        static S9sString   sm_defaultUserConfigFileName;
        static S9sString   sm_defaultSystemConfigFileName;
        static S9sOptions *sm_instance;

    private:
        S9sMap<S9sString, OperationMode> m_modes;
        S9sFileName          m_myName;
        OperationMode        m_operationMode;
        int                  m_exitStatus;
        S9sString            m_errorMessage;
        S9sVariantMap        m_options;
        S9sConfigFile        m_userConfig;
        S9sConfigFile        m_systemConfig;
        S9sVector<S9sString> m_extraArguments;
        S9sVariantMap        m_state;

    friend class UtS9sOptions;
    friend class UtS9sRpcClient;
};
