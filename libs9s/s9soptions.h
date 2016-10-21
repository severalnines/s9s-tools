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

#define EXPERIMENTAL_CODE


#include "S9sString"
#include "S9sVariant"
#include "S9sVariantMap"
#include "S9sConfigFile"

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
            Cluster,
            Node,
            Job,
            Backup,
            Process,
        };

        enum ExitCodes
        {
            ExitOk       = 0,
            JobFailed    = 1,
            BadOptions   = 6,
        };

        bool readOptions(int *argc, char *argv[]);
        bool executeInfoRequest();

        void createConfigFiles();
        bool loadConfigFiles();

        void setController(const S9sString &url);
        S9sString controller() const;

        S9sString configFile() const;
        
        bool setPropertiesOption(const S9sString &assignments);
        S9sVariantMap propertiesOption() const;

        int controllerPort() const;

        void setNodes(const S9sString &value);
        S9sVariantList nodes() const;

        S9sString vendor() const;
        S9sString providerVersion(const S9sString &defaultValue = "") const;
        S9sString osUser() const;
        S9sString osKeyFile() const;
        S9sString dbAdminUserName(const S9sString &defaultValue = "") const;
        S9sString dbAdminPassword();
        S9sString clusterType() const;

        S9sString rpcToken() const;
        int clusterId() const;
        int updateFreq() const;

        S9sString clusterName() const;
        int jobId() const;

        S9sString userName() const;

        S9sString authUsername() const;
        S9sString privateKeyPath() const;

        S9sString backupDir() const;
        S9sString backupMethod() const;

        bool useTls() const;

        bool isNodeOperation() const;
        bool isClusterOperation() const;
        bool isJobOperation() const;
        bool isBackupOperation() const;
        bool isProcessOperation() const;

        bool isListRequested() const;
        bool isSetRequested() const;
        bool isLogRequested() const;
        bool isCreateRequested() const;
        bool isRollingRestartRequested() const;
        bool isAddNodeRequested() const;
        bool isRemoveNodeRequested() const;
        bool isStopRequested() const;
        bool isDropRequested() const;

        bool isLongRequested() const;
        bool isJsonRequested() const;
        bool isTopRequested() const;
        bool isWaitRequested() const;
        bool isBatchRequested() const;

        bool useSyntaxHighlight() const;
        bool humanReadable() const;
        void setHumanReadable(const bool value = true);

        int terminalWidth() const;
        int terminalHeight() const;
        S9sString binaryName() const;
        S9sString errorString() const;
        
        int exitStatus() const;
        void setExitStatus(const S9sOptions::ExitCodes exitStatus);

        bool isVerbose() const;

        static void printVerbose(const char *formatString, ...);
        static void printError(const char *formatString, ...);

        void printHelp();

    private:
        void printHelpGeneric();
        void printHelpCluster();
        void printHelpNode();

        bool readOptionsNoMode(int argc, char *argv[]);
        bool readOptionsNode(int argc, char *argv[]);
        bool readOptionsBackup(int argc, char *argv[]);
        bool readOptionsCluster(int argc, char *argv[]);
        bool readOptionsJob(int argc, char *argv[]);
        bool readOptionsProcess(int argc, char  *argv[]);

        bool setMode(const S9sString &modeName);

        S9sOptions();
        ~S9sOptions();

        static S9sOptions *sm_instance;

    private:
        S9sFileName        m_myName;
        OperationMode      m_operationMode;
        int                m_exitStatus;
        S9sString          m_errorMessage;
        S9sVariantMap      m_options;
        S9sConfigFile      m_userConfig;
        S9sConfigFile      m_systemConfig;

    friend class UtS9sOptions;
};
