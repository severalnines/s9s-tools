/*
 * Severalnines Tools
 * Copyright (C) 2018 Severalnines AB
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
#include "s9soptions.h"

#include "config.h"
#include "S9sNode"
#include "S9sAccount"
#include "S9sFile"
#include "S9sRegExp"
#include "S9sDir"
#include "s9srsakey.h"
#include "S9sDateTime"
#include "S9sContainer"
#include "S9sSshCredentials"

#include <sys/ioctl.h>
#include <stdio.h>
#include <cstdlib>
#include <getopt.h>
#include <stdarg.h>
#include <unistd.h>
#include <cctype>
#include <fnmatch.h>

// for build/version info
#include "../config.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sOptions *S9sOptions::sm_instance = 0;
S9sString   S9sOptions::sm_defaultUserConfigFileName;
S9sString   S9sOptions::sm_defaultSystemConfigFileName;

enum S9sOptionType
{
    OptionRpcTls     = 1000,
    OptionPrintJson,
    OptionColor,
    OptionConfigFile,
    OptionTop,
    OptionUpdateFreq,
    OptionBatch,
    OptionNoHeader,
    OptionNodes,
    OptionServers,
    OptionContainers,
    OptionAddNode,
    OptionRemoveNode,
    OptionJobId,
    OptionSet,
    OptionChangePassword,
    OptionDrop,
    OptionOsUser,
    OptionOsKeyFile,
    OptionOsPassword,
    OptionOsSudoPassword,
    OptionProviderVersion,
    OptionProperties,
    OptionVendor,
    OptionCreate,
    OptionDelete,
    OptionClone,
    OptionEnable,
    OptionDisable,
    OptionSetGroup,
    OptionAddToGroup,
    OptionRemoveFromGroup,
    OptionDbAdmin,
    OptionDbAdminPassword,
    OptionClusterType,
    OptionStop,
    OptionPromoteSlave,
    OptionDemoteNode,
    OptionHelp,
    OptionTimeStyle,
    OptionWait,
    OptionSaveCluster,
    OptionRestoreCluster,
    OptionSaveController,
    OptionRestoreController,
    OptionRestore,
    OptionVerify,
    OptionDeleteOld,
    OptionBackupId,
    OptionBackupMethod,
    OptionBackupDirectory,
    OptionKeepTempDir,
    OptionTempDirPath,
    OptionSubDirectory,
    OptionBackupEncryption,
    OptionBackupUser,
    OptionBackupPassword,
    OptionSchedule,
    OptionRecurrence,
    OptionTimeout,
    OptionPing,
    OptionGroup,
    OptionCreateGroup,
    OptionFirstName,
    OptionEmailAddress,
    OptionLastName,
    OptionTitle,
    OptionStart,
    OptionRestart,
    OptionEnd,
    OptionReason,
    OptionUuid,
    OptionDateFormat,
    OptionFullUuid,
    OptionWhoAmI,
    OptionListProperties,
    OptionListClusterTypes,
    OptionListContainers,
    OptionType,
    OptionNoCompression,
    OptionPitrCompatible,
    OptionUsePigz,
    OptionOnNode,
    OptionOnController,
    OptionDatabases,
    OptionParallellism,
    OptionBackupRetention,
    OptionCloudRetention,
    OptionSafetyCopies,
    OptionKeep,
    OptionFullPath,
    OptionMemory,
    OptionStat,
    OptionWatch,
    OptionEdit,
    OptionCreateAccount,
    OptionGrant,
    OptionCheckHosts,
    OptionDeleteAccount,
    OptionCreateDatabase,
    OptionListDatabases,
    OptionListFiles,
    OptionAccount,
    OptionWithDatabase,
    OptionObjects,
    OptionPrivileges,
    OptionDbName,
    OptionOptGroup,
    OptionOptName,
    OptionOptValue,
    OptionListConfig,
    OptionChangeConfig,
    OptionPullConfig,
    OptionPushConfig,
    OptionExecute,
    OptionRun,
    OptionKill,
    OptionSystem,
    OptionTree,
    OptionOutputDir,
    OptionLogFormat,
    OptionFrom,
    OptionUntil,
    OptionForce,
    OptionExtended,
    OptionDry,
    OptionDebug,
    OptionClusterFormat,
    OptionNodeFormat,
    OptionBackupFormat,
    OptionUserFormat,
    OptionContainerFormat,
    OptionGraph,
    OptionBegin,
    OptionOnlyAscii,
    OptionDensity,
    OptionRollingRestart,
    OptionCollectLogs,
    OptionImportConfig,
    OptionEnableSsl,
    OptionDisableSsl,
    OptionCreateReport,
    OptionMaskPasswords,
    OptionDeployAgents,
    OptionLimit,
    OptionOffset,
    OptionRegister,
    OptionUnregister,
    OptionMove,
    OptionOldPassword,
    OptionNewPassword,
    OptionListKeys,
    OptionAddKey,
    OptionPublicKeyFile,
    OptionPublicKeyName,
    OptionPrivateKeyFile,
    OptionKeyName,
    OptionListGroups,
    OptionListPartitions,
    OptionListProcessors,
    OptionListMemory,
    OptionGetAcl,
    OptionCat,
    OptionAddAcl,
    OptionRemoveAcl,
    OptionChOwn,
    OptionMkdir,
    OptionRmdir,
    OptionMkfile,
    OptionSave,
    OptionEnableCmonHa,
    OptionAcl,
    OptionOwner,
    OptionListNics,
    OptionListDisks,
    OptionDonor,
    OptionRefresh,
    OptionAll,
    OptionFail,
    OptionSuccess,
    OptionAccess,
    OptionTemplate,
    OptionSubnetId,
    OptionVpcId,
    OptionVolumes,
    OptionCloud,
    OptionImage,
    OptionImageOsUser,
    OptionIndividualFiles,
    OptionTestServer,
    OptionBackupDatadir,
    
    OptionListImages,
    OptionListRegions,
    OptionListSubnets,
    OptionListTemplates,

    OptionSetupAudit,

    OptionEventCluster,
    OptionEventJob,
    OptionEventHost,
    OptionEventMaintenance,
    OptionEventAlarm,
    OptionEventFile,
    OptionEventDebug,
    OptionEventLog,

    OptionWithNoName,
    OptionWithCreated,
    OptionWithDestroyed,
    OptionWithChanged,
    OptionWithStarted,
    OptionWithEnded,
    OptionWithStateChanged,
    OptionWithUserMessage,
    OptionWithLogMessage,
    OptionWithMeasurements,
    
    OptionNoNoName,
    OptionNoCreated,
    OptionNoDestroyed,
    OptionNoChanged,
    OptionNoStarted,
    OptionNoEnded,
    OptionNoStateChanged,
    OptionNoUserMessage,
    OptionNoLogMessage,
    OptionNoMeasurements,

    OptionUseInternalRepos,
    
    OptionJobTags,
    OptionWithTags,
    OptionWithoutTags,

    OptionShowDefined,
    OptionShowRunning,
    OptionShowScheduled,
    OptionShowAborted,
    OptionShowFinished,
    OptionShowFailed,
    OptionFirewalls,
    OptionSortByMemory,
    OptionOutputFile,
    OptionInputFile,
    OptionRegion,
    OptionShellCommand,

    OptionAlarmId,
    OptionLogFile,
    OptionCredentialId,
    OptionCreateSnaphot,
    
    OptionConfigTemplate,
    OptionNoInstall,
};

/**
 * The default constructor, nothing to see here.
 */
S9sOptions::S9sOptions() :
    m_operationMode(NoMode),
    m_exitStatus(EXIT_SUCCESS)
{
    S9sString   theString;
    const char *tmp;

    sm_instance = this;

    /*
     * Setting up some internals that we use later.
     */
    m_modes["backup"]       = Backup;
    m_modes["cluster"]      = Cluster;
    m_modes["container"]    = Container;
    m_modes["job"]          = Job;
    m_modes["log"]          = Log;
    m_modes["maintenance"]  = Maintenance;
    m_modes["metatype"]     = MetaType;
    m_modes["node"]         = Node;
    m_modes["process"]      = Process;
    m_modes["script"]       = Script;
    m_modes["sheet"]        = Sheet;
    m_modes["server"]       = Server;
    m_modes["controller"]   = Controller;
    m_modes["tree"]         = Tree;
    m_modes["user"]         = User;
    m_modes["account"]      = Account;
    m_modes["event"]        = Event;
    m_modes["alarm"]        = Alarm;
    
    // This helps to fix some typos I always had in the command line.
    m_modes["backups"]      = Backup;
    m_modes["clusters"]     = Cluster;
    m_modes["containers"]   = Container;
    m_modes["jobs"]         = Job;
    m_modes["logs"]         = Log;
    m_modes["maintenances"] = Maintenance;
    m_modes["metatypes"]    = MetaType;
    m_modes["nodes"]        = Node;
    m_modes["processes"]    = Process;
    m_modes["scripts"]      = Script;
    m_modes["sheets"]       = Sheet;
    m_modes["servers"]      = Server;
    m_modes["controllers"]  = Server;
    m_modes["users"]        = User;
    m_modes["accounts"]     = Account;
    m_modes["events"]       = Event;
    m_modes["alarms"]       = Alarm;

    /*
     * Reading environment variables and storing them as settings.
     */
    tmp = getenv("CMON_CONTROLLER");
    if (tmp)
    {
        setController(tmp);
    }

    theString = getenv("CMON_CLUSTER_ID");
    if (!theString.empty())
    {
        m_options["cluster_id"] = theString.toInt();
    }
}

/**
 * The destructor of the singleton.
 */
S9sOptions::~S9sOptions()
{
    sm_instance = NULL;
}

/**
 * The usual instance() function or the singleton.
 */
S9sOptions *
S9sOptions::instance()
{
    if (!sm_instance)
        sm_instance = new S9sOptions;

    return sm_instance;
}

/**
 * This method should be called before exiting the application to destroy the
 * singleton instance.
 */
void 
S9sOptions::uninit()
{
    if (sm_instance)
    {
        delete sm_instance;
        sm_instance = 0;
    }
}

/**
 * The idea is that we create a configuration file template if the config file
 * does not exists. The template then can be used to create a config file.
 */
void
S9sOptions::createConfigFiles()
{
    S9sFile  userFile(defaultUserConfigFileName());
    S9sDir   userDir(S9sFile::dirname(userFile.path()));

    if (!userDir.exists())
        userDir.mkdir();

    if (!userDir.exists())
        return;

    if (userFile.exists())
        return;

    userFile.fprintf("[global]\n");
    userFile.fprintf("# controller=https://localhost:9501\n");
    userFile.fprintf("\n");

    userFile.fprintf("#\n");
    userFile.fprintf("# Information about the user for the controller to \n");
    userFile.fprintf("# access the nodes.\n");
    userFile.fprintf("#\n");
    userFile.fprintf("# os_user          = some_user\n");
    userFile.fprintf("# os_sudo_password = some_password\n");
    userFile.fprintf("# os_key_file      = /home/some_user/.ssh/test_ssh_key\n");
    userFile.fprintf("\n");
}

S9sString
S9sOptions::defaultUserConfigFileName() const
{
    // This is for testing.
    if (!sm_defaultUserConfigFileName.empty())
        return sm_defaultUserConfigFileName;

    if (getenv("S9S_USER_CONFIG") != NULL)
        return S9sString(getenv("S9S_USER_CONFIG"));

    return S9sString("~/.s9s/s9s.conf");
}

S9sString
S9sOptions::defaultSystemConfigFileName() const
{
    // This is for testing.
    if (!sm_defaultSystemConfigFileName.empty())
        return sm_defaultSystemConfigFileName;

    if (getenv("S9S_SYSTEM_CONFIG") != NULL)
        return S9sString(getenv("S9S_SYSTEM_CONFIG"));

    return S9sString("/etc/s9s.conf");
}

S9sString
S9sOptions::userStateFilename() const
{
    return S9sString("~/.s9s/s9s.state");
}

bool
S9sOptions::loadStateFile()
{
    S9sString fileName = userStateFilename();
    S9sFile   file(fileName);
    S9sString content;

    s9s_log("Loading state file '%s'.", STR(fileName));
    if (!file.exists())
    {
        s9s_log("File '%s' no exists, ok.", STR(fileName));
        return false;
    }

    if (!file.readTxtFile(content))
    {
        s9s_log("%s.", STR(file.errorString()));
        return false;
    }

    if (!m_state.parse(STR(content)))
    {
        s9s_log("Error parsing state file.");
        return false;
    }

    return true;
}

bool
S9sOptions::writeStateFile()
{
    S9sString fileName = userStateFilename();
    S9sFile   file(fileName);
    S9sString content = m_state.toString();
    bool      success;

    s9s_log("Writing state file '%s'.", STR(fileName));
    success = file.writeTxtFile(content);
    if (!success)
    {
        s9s_log("ERROR: %s", STR(file.errorString()));
    }

    return success;
}

bool
S9sOptions::setState(
        const S9sString    &key,
        const S9sVariant   &value)
{
    m_state[key] = value;
    return writeStateFile();
}

S9sVariant
S9sOptions::getState(
        const S9sString    &key)
{
    if (m_state.contains(key))
        return m_state.at(key);

    return S9sVariant();
}

/**
 * \returns false if there was any error with the configuration file(s), true if
 *   everything went well (even if there are no configuration files).
 */
bool
S9sOptions::loadConfigFiles()
{
    S9sFile userConfig(defaultUserConfigFileName());
    S9sFile systemConfig(defaultSystemConfigFileName());
    bool    success;

    m_userConfig   = S9sConfigFile();
    m_systemConfig = S9sConfigFile();

    /*
     * If the user specified a config file name in the command line we load that
     * config file and nothing else.
     */
    if (!configFile().empty())
    {
        S9sString content;

        userConfig = S9sFile(configFile());

        if (!userConfig.exists())
        {
            PRINT_ERROR(
                    "The file '%s' does not exists.", 
                    STR(userConfig.path()));

            return false;
        }

        S9S_DEBUG("1: Parsing '%s'.", STR(userConfig.path()));
        success = m_userConfig.parse(STR(content));
        if (!success)
        {
            PRINT_ERROR(
                    "Error parsing configuration file '%s': %s",
                    STR(configFile()), STR(m_userConfig.errorString()));

            return false;
        }

        return true;
    }

    /*
     * Loading the user's own config file if it exists.
     */
    if (userConfig.exists())
    {
        S9sString content;

        S9S_DEBUG("2: Parsing '%s'.", STR(userConfig.path()));
        success = userConfig.readTxtFile(content);
        if (!success)
        {
            PRINT_ERROR(
                    "Error reading user configuration file: %s",
                    STR(userConfig.errorString()));

            return false;
        }

        success = m_userConfig.parse(STR(content));
        if (!success)
        {
            printError(
                    "Error parsing user configuration file: %s",
                    STR(m_userConfig.errorString()));

            return false;
        }
    }
    
    /*
     * The system configuration.
     */
    if (systemConfig.exists())
    {
        S9sString content;

        S9S_DEBUG("3: Parsing '%s'.", STR(systemConfig.path()));
        success = systemConfig.readTxtFile(content);
        if (success)
        {
            // If we don't have the rights to read the configuration file we
            // simply ignore it. If we have parse error(s) we complain.
            success = m_systemConfig.parse(STR(content));
            if (!success)
            {
                PRINT_ERROR(
                        "Error parsing system configuration file: %s",
                        STR(m_systemConfig.errorString()));

                return false;
            }
        }
    }

    return true;
}

/**
 * \param url the Cmon Controller host or host:port or protocol://host:port.
 *
 * Sets the controller host name. If the passed string has the format
 * HOSTNAME:PORT sets the controller port too.
 */
void
S9sOptions::setController(
        const S9sString &url)
{
    S9sString myUrl = url;
    S9sRegExp regexp;

    S9S_DEBUG("              myUrl  : '%s'", STR(myUrl));
    regexp = "([a-zA-Z]+):\\/\\/(.+)";
    if (regexp == myUrl)
    {
        /*
        S9S_DEBUG("    MATCH1 '%s', '%s'", 
                STR(regexp[1]), 
                STR(regexp[2]));
                */

        m_options["controller_protocol"] = regexp[1];
        myUrl = regexp[2];
    }
 
    regexp = "(.+):([0-9]+)(\\/.*)?";
    if (regexp == myUrl)
    {
        m_options["controller"]      = regexp[1];
        m_options["controller_port"] = regexp[2].toInt();
        m_options["controller_path"] = regexp[3];
    } else {
        m_options["controller"] = myUrl;
    }
    
    S9S_DEBUG("  controller_protocol : '%s'", 
            STR(m_options["controller_protocol"].toString()));

    S9S_DEBUG("           controller : '%s'", 
            STR(m_options["controller"].toString()));

    S9S_DEBUG("      controller_port : '%s'", 
            STR(m_options["controller_port"].toString()));
    
    S9S_DEBUG("      controller_path : '%s'", 
            STR(m_options["controller_path"].toString()));
}

void
S9sOptions::checkController()
{
    if (m_options.contains("controller"))
        return;

    S9sString tmp;

    tmp = m_userConfig.variableValue("controller");
    if (tmp.empty())
        tmp = m_systemConfig.variableValue("controller");

    if (!tmp.empty())
        setController(tmp);
}

/**
 * \returns the controller hostname.
 */
S9sString
S9sOptions::controllerHostName()
{
    S9sString  retval;

    checkController();
    if (m_options.contains("controller"))
    {
        retval = m_options.at("controller").toString();
    } else {
        retval = m_userConfig.variableValue("controller_host_name");

        if (retval.empty())
            retval = m_systemConfig.variableValue("controller_host_name");
    }

    return retval;
}

/**
 * \returns The protocol part of the url passed as command line option argument
 *   for the --controller command line option.
 */
S9sString
S9sOptions::controllerProtocol()
{
    S9sString  retval;
    
    checkController();
    if (m_options.contains("controller_protocol"))
    {
        retval = m_options.at("controller_protocol").toString();
    } else {
        retval = m_userConfig.variableValue("controller_protocol");

        if (retval.empty())
            retval = m_systemConfig.variableValue("controller_protocol");
    }

    return retval;
}

/**
 * \returns the controller port.
 */
int
S9sOptions::controllerPort()
{
    int retval = 0;

    checkController();
    if (m_options.contains("controller_port"))
    {
        retval = m_options.at("controller_port").toInt();
    } else {
        retval = m_userConfig.variableValue("controller_port").toInt();

        if (retval == 0)
            retval = m_systemConfig.variableValue("controller_port").toInt();
    }

    return retval;
}

S9sString
S9sOptions::controllerPath()
{
    return getString("controller_path");
}

S9sString
S9sOptions::controllerUrl()
{
    S9sString retval;
    S9sString protocol;

    /*
     * The protocol.
     */
    protocol = controllerProtocol();
    if (!protocol.empty())
    {
        retval = protocol;

        if (!retval.endsWith("://"))
            retval += "://";
    } else if (useTls())
    {
        retval = "https://";
    } else {
        retval += "http://";
    }

    /*
     * The hostname.
     */
    retval += controllerHostName();

    /*
     * The port.
     */
    if (controllerPort() != 0)
        retval.sprintf("%s:%d", STR(retval), controllerPort());

    return retval;
}

/**
 * \returns The value of the --config-file command line option or the empty
 *   string if
 */
S9sString
S9sOptions::configFile() const
{
    return getString("config_file");
}

/**
 * \return True if the --only-ascii command line option was provided or the
 *   only_ascii configuration value set to true.
 */
bool
S9sOptions::onlyAscii() const
{
    char *variable;
    const char *key = "only_ascii";
    S9sString   retval;

    variable = getenv("S9S_ONLY_ASCII");
    if (variable != NULL)
    {
        S9sString theString = variable;
        if (theString.toInt() > 0)
            return true;
    }
    
    if (m_options.contains(key))
    {
        retval = m_options.at(key).toString();
    } else {
        retval = m_userConfig.variableValue(key);

        if (retval.empty())
            retval = m_systemConfig.variableValue(key);
    }

    return retval.toBoolean();
}

void
S9sOptions::enableEventType(
        const S9sString &eventTypeName)
{
    S9sVariantMap theMap = getVariantMap("enabled_event_types");

    theMap[eventTypeName] = true;
    m_options["enabled_event_types"] = theMap;
}

/**
 * \returns True if the set event filter says the event type should be
 *   processed.
 */
bool
S9sOptions::eventTypeEnabled(
        const S9sString &eventTypeName)
{
    S9sVariantMap theMap = getVariantMap("enabled_event_types");

    if (!theMap.empty())
        return theMap[eventTypeName].toBoolean();

    return true;
}

/**
 * This method can be used to set the names of the events that will be
 * processed. See the documentation of eventNameEnabled() for further 
 * details.
 */
void
S9sOptions::enableEventName(
        const S9sString &eventName)
{
    S9sVariantMap theMap = getVariantMap("enabled_event_names");

    theMap[eventName] = true;
    m_options["enabled_event_names"] = theMap;
}

/**
 * This method can be used to set the names of the events that will not be
 * processed. See the documentation of eventNameEnabled() for further 
 * details.
 */
void
S9sOptions::disableEventName(
        const S9sString &eventName)
{
    S9sVariantMap theMap = getVariantMap("disabled_event_names");

    theMap[eventName] = true;
    m_options["disabled_event_names"] = theMap;
}

/**
 * Currently the controller supports the following event names: NoSubClass, 
 * Created, Destroyed, Changed, Started, Ended, StateChanged, UserMessage,
 * LogMessage, Measurements.
 */
bool
S9sOptions::eventNameEnabled(
        const S9sString &eventName)
{
    S9sVariantMap enabledMap  = getVariantMap("enabled_event_names");
    S9sVariantMap disabledMap = getVariantMap("disabled_event_names");
    bool          retval = true;

    if (!enabledMap.empty())
        retval = enabledMap[eventName].toBoolean();

    if (disabledMap[eventName].toBoolean())
        retval = false;

    return retval;
}

/**
 * \returns True if the --density command line option was provided.
 */
bool
S9sOptions::density() const
{
    return getBool("density");
}

int
S9sOptions::clientConnectionTimeout() const
{
    S9sString  key = "client_connection_timeout";
    S9sString  stringVal;
    int        intVal = 30;

    // Finding a string value.
    stringVal = getenv("S9S_CONNECTION_TIMEOUT");
    if (stringVal.empty())
        stringVal = m_userConfig.variableValue(key);

    if (stringVal.empty())
        stringVal = m_systemConfig.variableValue(key);

    // Converting to integer.
    if (!stringVal.empty())
        intVal = stringVal.toInt();

    // Lower limit.
    if (intVal < 1)
        intVal = 1;

    //S9S_WARNING("intVal: %d", intVal);
    return intVal;
}

/**
 * \returns The value for the "brief_log_format" config variable that
 *   controls the format of the log lines printed when the --long option is not
 *   provided.
 */
S9sString 
S9sOptions::briefLogFormat() const
{
    const char *key = "brief_log_format";
    S9sString   retval;

    if (m_options.contains("log_format"))
    {
        retval = m_options.at("log_format").toString();
    } else {
        retval = m_userConfig.variableValue(key);

        if (retval.empty())
            retval = m_systemConfig.variableValue(key);
    }

    return retval;
}

S9sString 
S9sOptions::longLogFormat() const
{
    const char *key = "long_log_format";
    S9sString   retval;

    if (m_options.contains("log_format"))
    {
        retval = m_options.at("log_format").toString();
    } else {
        retval = m_userConfig.variableValue(key);

        if (retval.empty())
            retval = m_systemConfig.variableValue(key);
    }

    return retval;
}

/**
 * \returns The value for the "brief_job_log_format" config variable that
 *   controls the format of the job log lines printed when the --long option is
 *   not provided and the --log-format option is not used either.
 */
S9sString 
S9sOptions::briefJobLogFormat() const
{
    const char *key = "brief_job_log_format";
    S9sString   retval;

    if (m_options.contains(key))
    {
        retval = m_options.at(key).toString();
    } else {
        retval = m_userConfig.variableValue(key);

        if (retval.empty())
            retval = m_systemConfig.variableValue(key);
    }

    return retval;
}


/**
 * \returns The value for the "long_job_log_format" config variable that
 *   controls the format of the job log lines printed when the --long option is
 *   provided and the --log-format option is not used.
 */
S9sString 
S9sOptions::longJobLogFormat() const
{
    const char *key = "long_job_log_format";
    S9sString   retval;

    if (m_options.contains(key))
    {
        retval = m_options.at(key).toString();
    } else {
        retval = m_userConfig.variableValue(key);

        if (retval.empty())
            retval = m_systemConfig.variableValue(key);
    }

    return retval;
}

/**
 * \returns The value for the "long_cluster_format" config variable that
 *   controls the format of the cluster lines printed when the --long option is
 *   provided and the --cluster-format option is not used.
 */
S9sString 
S9sOptions::longClusterFormat() const
{
    const char *key = "long_cluster_format";
    S9sString   retval;

    if (m_options.contains(key))
    {
        retval = m_options.at(key).toString();
    } else {
        retval = m_userConfig.variableValue(key);

        if (retval.empty())
            retval = m_systemConfig.variableValue(key);
    }

    return retval;
}

/**
 * \returns The value for the "long_node_format" config variable that
 *   controls the format of the node lines printed when the --long option is
 *   provided and the --node-format option is not used.
 */
S9sString 
S9sOptions::longNodeFormat() const
{
    const char *key = "long_node_format";
    S9sString   retval;

    if (m_options.contains(key))
    {
        retval = m_options.at(key).toString();
    } else {
        retval = m_userConfig.variableValue(key);

        if (retval.empty())
            retval = m_systemConfig.variableValue(key);
    }

    return retval;
}

/**
 * \returns The value for the "short_node_format" config variable that
 *   controls the format of the node lines printed when the --long option is
 *   not provided and the --node-format option is not used.
 */
S9sString 
S9sOptions::shortNodeFormat() const
{
    const char *key = "short_node_format";
    S9sString   retval;

    if (m_options.contains(key))
    {
        retval = m_options.at(key).toString();
    } else {
        retval = m_userConfig.variableValue(key);

        if (retval.empty())
            retval = m_systemConfig.variableValue(key);
    }

    return retval;
}

/**
 * \returns The value for the "long_backup_format" config variable that
 *   controls the format of the backup lines printed when the --long option is
 *   provided and the --backup-format option is not used.
 */
S9sString 
S9sOptions::longBackupFormat() const
{
    const char *key = "long_backup_format";
    S9sString   retval;

    if (m_options.contains(key))
    {
        retval = m_options.at(key).toString();
    } else {
        retval = m_userConfig.variableValue(key);

        if (retval.empty())
            retval = m_systemConfig.variableValue(key);
    }

    return retval;
}

/**
 * \returns The value for the "long_user_format" config variable that controls
 *  the format of the user info lines printed when the --long option is provided
 *  and the --user-format option is not used.
 */
S9sString 
S9sOptions::longUserFormat() const
{
    const char *key = "long_user_format";
    S9sString   retval;

    if (m_options.contains(key))
    {
        retval = m_options.at(key).toString();
    } else {
        retval = m_userConfig.variableValue(key);

        if (retval.empty())
            retval = m_systemConfig.variableValue(key);
    }

    return retval;
}

/**
 * \param assignments The argument of the --properties command line option.
 * \returns true if the format of the optarg is valid.
 *
 * This method is for registering the values received as the argument of the 
 * --properties command line option (e.g. --properties=PROPERTIES). 
 */
bool
S9sOptions::setPropertiesOption(
        const S9sString &assignments)
{
    S9sVariantMap  theMap;
    bool           success;

    success = theMap.parseAssignments(assignments);
    if (!success)
        m_errorMessage.sprintf("failed to parse '%s'.", STR(assignments));
    else
        m_options["properties"] = theMap;

    return success;
}

/**
 * \returns The values from the --properties command line option in a map.
 */
S9sVariantMap
S9sOptions::propertiesOption() const
{
    if (m_options.contains("properties"))
        return m_options.at("properties").toVariantMap();

    return S9sVariantMap();
}

/**
 * \param value the node list as a string using field separators that the
 *   S9sString::split() function can interpret.
 *
 * The node list is usually set in the command line using the --nodes option.
 */
bool
S9sOptions::setNodes(
        const S9sString &value)
{
    S9sVariantList nodeStrings = value.split(";,");
    S9sVariantList nodes;

    for (uint idx = 0; idx < nodeStrings.size(); ++idx)
    {
        S9sString nodeString = nodeStrings[idx].toString();
        S9sNode   node(nodeString.trim());

        //S9S_WARNING("[%2u] %s", idx, STR(nodeStrings[idx].toString()));
        //S9S_WARNING("%s\n", STR(node.toVariantMap().toString()));
        if (node.hasError())
        {
            PRINT_ERROR("%s", STR(node.fullErrorString()));
            m_exitStatus = BadOptions;
            return false;
        }

        nodes << node;
    }

    m_options["nodes"] = nodes;
    return true;
}

/**
 * \returns the node list, one host name in every list item
 */
S9sVariantList
S9sOptions::nodes() const
{
    if (m_options.contains("nodes"))
        return m_options.at("nodes").toVariantList();

    return S9sVariantList();
}

/**
 * \returns The argument of the comman line option --input-file if provided,
 *   the empty string if not.
 */
S9sString
S9sOptions::inputFile() const
{
    return getString("input_file");
}

/**
 * \returns The argument of the comman line option --output-file if provided,
 *   the empty string if not.
 */
S9sString
S9sOptions::outputFile() const
{
    return getString("output_file");
}

S9sString
S9sOptions::logFile() const 
{
    return getString("log_file");
}

/**
 * \param value the node list as a string using field separators that the
 *   S9sString::split() function can interpret.
 *
 * The node list is usually set in the command line using the --servers option.
 */
bool
S9sOptions::setServers(
        const S9sString &value)
{
    S9sVariantList serverStrings = value.split(";,");
    S9sVariantList servers;

    for (uint idx = 0; idx < serverStrings.size(); ++idx)
    {
        S9sString serverString = serverStrings[idx].toString();
        S9sNode   node(serverString.trim());

        if (node.hasError())
        {
            PRINT_ERROR("%s", STR(node.fullErrorString()));
            m_exitStatus = BadOptions;
            return false;
        }

        servers << node;
    }

    m_options["servers"] = servers;
    return true;
}

/**
 * \returns a list of S9sNode objects that are passed as a command line option
 *   argument for the --servers command line option.
 */
S9sVariantList
S9sOptions::servers() const
{
    if (m_options.contains("servers"))
        return m_options.at("servers").toVariantList();

    return S9sVariantList();
}

bool
S9sOptions::setContainers(
        const S9sString &value)
{
    S9sVariantList containerStrings = value.split(";,");
    S9sVariantList containers;

    for (uint idx = 0; idx < containerStrings.size(); ++idx)
    {
        S9sString     containerString = containerStrings[idx].toString();
        S9sContainer  container(containerString.trim());

        #if 0
        if (container.hasError())
        {
            PRINT_ERROR("%s", STR(node.fullErrorString()));
            m_exitStatus = BadOptions;
            return false;
        }
        #endif

        containers << container;
    }

    m_options["containers"] = containers;
    return true;
}

bool
S9sOptions::hasContainers() const
{
    return m_options.contains("containers");
}

S9sVariantList
S9sOptions::containers() const
{
    if (m_options.contains("containers"))
        return m_options.at("containers").toVariantList();

    return S9sVariantList();
}

/**
 * \returns the vendor name as it is set by the --vendor command line option.
 */
S9sString
S9sOptions::vendor() const
{
    S9sString retval;

    if (m_options.contains("vendor"))
    {
        retval = m_options.at("vendor").toString();
    } else {
        retval = m_userConfig.variableValue("vendor");

        if (retval.empty())
            retval = m_systemConfig.variableValue("vendor");
    }


    return retval;
}

/**
 * \returns The option argument passed to the --start command line option.
 */
S9sString
S9sOptions::start() const
{
    return getString("start");
}

/**
 * \returns The option argument passed to the --begin command line option.
 */
S9sString
S9sOptions::begin() const
{
    return getString("begin");
}

/**
 * \returns The option argument passed to the --end command line option.
 */
S9sString
S9sOptions::end() const
{
    return getString("end");
}

/**
 * \returns The option argument passed to the --from command line option.
 */
S9sString
S9sOptions::from() const
{
    return getString("from");
}

/**
 * \returns The option argument passed to the --until command line option.
 */
S9sString
S9sOptions::until() const
{
    return getString("until");
}

S9sString
S9sOptions::reason() const
{
    return getString("reason");
}

S9sString
S9sOptions::uuid() const
{
    return getString("uuid");
}

/**
 * \returns the provider version string as it is set by the --provider-version
 *   command line option.
 */
S9sString
S9sOptions::providerVersion(
        const S9sString &defaultValue) const
{
    S9sString retval = defaultValue;

    if (m_options.contains("provider_version"))
    {
        retval = m_options.at("provider_version").toString();
    } else {
        retval = m_userConfig.variableValue("provider_version");

        if (retval.empty())
            retval = m_systemConfig.variableValue("provider_version");
    }

    return retval;
}

/**
 * \returns the value of the --os-sudo-password command line option or the 
 *   "os_sudo_password" configuration value.
 *
 * The --os-sudo-password is used for executing certain commands with root
 * os privileges.
 */
S9sString
S9sOptions::osSudoPassword() const
{
    S9sString retval;

    if (m_options.contains("os_sudo_password"))
    {
        retval = m_options.at("os_sudo_password").toString();
    } else {
        retval = m_userConfig.variableValue("os_sudo_password");

        if (retval.empty())
            retval = m_systemConfig.variableValue("os_sudo_password");
    }

    return retval;
}

/**
 * I am working on this.
 */
bool
S9sOptions::hasSshCredentials() 
{
    if (m_options.contains("os_user") ||
        m_userConfig.hasVariable("", "os_user") ||
        m_systemConfig.hasVariable("", "os_user"))
    {
        return true;
    }
    
    if (m_options.contains("os_password") ||
        m_userConfig.hasVariable("", "os_password") ||
        m_systemConfig.hasVariable("", "os_password"))
    {
        return true;
    }

    if (m_options.contains("os_key_file") ||
        m_userConfig.hasVariable("", "os_key_file") ||
        m_systemConfig.hasVariable("", "os_key_file"))
    {
        return true;
    }
    
    return false;
}

/**
 * Related command line options are --os-user=, --os-key-file=, --os-password=.
 */
S9sSshCredentials
S9sOptions::sshCredentials(
        const S9sString &categoryName,
        const S9sString &hostName)
{
    S9sSshCredentials retval;
    
    retval.setUserName(osUser());
    retval.setPassword(osPassword());
    retval.setPublicKeyFilePath(osKeyFile());

    return retval;
}

/**
 * \returns the value of the --os-user command line option or the user name as
 *   default.
 *
 * The --os-user controls what user name will be used when authenticating on the
 * nodes. This option defaults to the username, that is the username on the
 * localhost running the application.
 *
 * This value can also be set in the configuration file using the "us_user" 
 * key.
 */
S9sString
S9sOptions::osUser(
        bool defaultsToCmonUser) const
{
    S9sString retval;

    if (m_options.contains("os_user"))
    {
        retval = m_options.at("os_user").toString();
    } else {
        retval = m_userConfig.variableValue("os_user");

        if (retval.empty())
            retval = m_systemConfig.variableValue("os_user");
    }

    if (retval.empty() && defaultsToCmonUser)
        retval = userName();

    return retval;
}

S9sString
S9sOptions::osPassword() const
{
    S9sString retval;

    if (m_options.contains("os_password"))
    {
        retval = m_options.at("os_password").toString();
    } else {
        retval = m_userConfig.variableValue("os_password");

        if (retval.empty())
            retval = m_systemConfig.variableValue("os_password");
    }

    return retval;
}

/**
 * \returns The file (on the controller) that will be used as SSH key while
 *   authenticating on the nodes with SSH.
 *
 * This value can be set in the configuration file using the "os_key_file"
 * coonfiguration key.
 */
S9sString
S9sOptions::osKeyFile() const
{
    S9sString retval;

    if (m_options.contains("os_key_file"))
    {
        retval = m_options.at("os_key_file").toString();
    } else {
        retval = m_userConfig.variableValue("os_key_file");

        if (retval.empty())
            retval = m_systemConfig.variableValue("os_key_file");
    }

    return retval;
}

/**
 * \returns The database administrator user name used when installing new
 *   clusters.
 */
S9sString 
S9sOptions::dbAdminUserName(
        const S9sString &defaultValue) const
{
    S9sString retval;

    if (m_options.contains("db_admin_user_name"))
    {
        retval = m_options.at("db_admin_user_name").toString();
    } else {
        retval = m_userConfig.variableValue("db_admin_user_name");

        if (retval.empty())
            retval = m_systemConfig.variableValue("db_admin_user_name");
    }

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 * \returns The database administrator password used when installing new
 *   clusters.
 */
S9sString 
S9sOptions::dbAdminPassword()
{
    S9sString retval;

    if (m_options.contains("db_admin_password"))
    {
        retval = m_options.at("db_admin_password").toString();
    } else {
        retval = m_userConfig.variableValue("db_admin_password");

        if (retval.empty())
            retval = m_systemConfig.variableValue("db_admin_password");
    }
    
    return retval;
}

/**
 * \returns the cluster type string that is provided by the --cluster-type
 *   command line option
 *
 * This function will convert the cluster type string to lowercase for
 * convenience.
 */
S9sString
S9sOptions::clusterType() const
{
    return getString("cluster_type").toLower();
}

/**
 * \param value The date and time to convert to a string.
 * \returns The string version of the argument.
 *
 * This function will convert a date&time value to a string. The format of the
 * string can be controlled through command line options and configuration
 * settings.
 */
S9sString
S9sOptions::formatDateTime(
        S9sDateTime value) const
{
    S9sString formatString;

    if (m_options.contains("date_format"))
        return value.toString(m_options.at("date_format").toString());

    formatString = m_userConfig.variableValue("date_format");
    if (formatString.empty())
        formatString = m_systemConfig.variableValue("date_format");

    if (!formatString.empty())
        return value.toString(formatString);

    // The default date&time format.
    return value.toString(S9sDateTime::CompactFormat);
}

S9s::AddressType
S9sOptions::addressType() const
{
    return S9s::AnyIpv4Address;
    //return S9s::PrivateIpv4Address;
}

bool
S9sOptions::fullUuid() const
{
    return getBool("full_uuid");
}

/**
 * \returns The option argument for the --schedule command line option.
 */
S9sString
S9sOptions::schedule() const
{
    return getString("schedule");
}

/**
 * \returns The option argument for the --recurrence command line option.
 */

S9sString
S9sOptions::recurrence() const
{
    return getString("recurrence");
}

int
S9sOptions::timeout() const
{
    if (m_options.contains("timeout"))
        return m_options.at("timeout").toInt();

    return 0;
}

bool
S9sOptions::hasTimeout() const
{
    return m_options.contains("timeout");
}

/**
 * \returns The numerical value of the command line option argument for --limit
 *   or -1 if the option was not provided.
 */
int
S9sOptions::limit() const
{
    int retval = -1;

    if (m_options.contains("limit"))
        retval = m_options.at("limit").toInt();

    return retval;
}

/**
 * \returns The numerical value of the command line option argument for --offset
 *   or -1 if the option was not provided.
 */
int
S9sOptions::offset() const
{
    int retval = -1;

    if (m_options.contains("offset"))
        retval = m_options.at("offset").toInt();

    return retval;
}

/**
 * \returns the cluster ID from the command line or the configuration or
 *   environment 0 if the cluster id is not provided in either of these.
 */
int
S9sOptions::clusterId() const
{
    int retval = S9S_INVALID_CLUSTER_ID;

    if (m_options.contains("cluster_id"))
    {
        retval = m_options.at("cluster_id").toInt(S9S_INVALID_CLUSTER_ID);
    } else {
        S9sString stringVal = m_userConfig.variableValue("default_cluster_id");

        if (stringVal.empty())
            stringVal = m_systemConfig.variableValue("default_cluster_id");

        if (!stringVal.empty())
            retval = stringVal.toInt(S9S_INVALID_CLUSTER_ID);
    }

    return retval;
}

/**
 * \returns true if the --cluster-id command line option was provided.
 */
bool
S9sOptions::hasClusterIdOption() const
{
    return m_options.contains("cluster_id");
}

bool
S9sOptions::forceOption() const
{
    return m_options.at("force").toBoolean();
}

bool
S9sOptions::hasForceOption() const
{
    return m_options.contains("force");
}

bool
S9sOptions::extended() const
{
    return getBool("extended");
}

bool
S9sOptions::hasAlarmIdOption() const
{
    return m_options.contains("alarm_id");
}

int
S9sOptions::alarmId() const
{
    return getInt("alarm_id");
}

bool
S9sOptions::hasCredentialIdOption() const
{
    return m_options.contains("credential_id");
}

int
S9sOptions::credentialId() const
{
    return getInt("credential_id");
}


/**
 * \returns True if the --backup-id command line option was provided.
 */
bool
S9sOptions::hasBackupId() const
{
    return m_options.contains("backup_id");
}

/**
 * \returns The command line option argument of the --backup-id option or 0 if
 *   the option is not provided.
 */
int
S9sOptions::backupId() const
{
    int retval = 0;

    if (m_options.contains("backup_id"))
        retval = m_options.at("backup_id").toInt();

    return retval;
}

/**
 * \returns The option argument for the --backup-user option or the empty string
 *   if the option was not provided.
 */
S9sString
S9sOptions::backupUser() const
{
    return getString("backup_user");
}

/**
 * \returns The option argument for the --backup-password option or the empty
 *   string if the option was not provided.
 */
S9sString
S9sOptions::backupPassword() const
{
    return getString("backup_password");
}

/**
 * \returns The value provided using the --type command line option.
 */
S9sString
S9sOptions::type() const
{
    return getString("type");
}

int
S9sOptions::updateFreq() const
{
    S9sString retval;

    if (m_options.contains("update_freq"))
    {
        retval = m_options.at("update_freq").toString();
    } else {
        retval = m_userConfig.variableValue("update_freq");

        if (retval.empty())
            retval = m_systemConfig.variableValue("update_freq");
    }

    if (retval.empty())
        return 10;

    //S9S_DEBUG("-> %d", retval.toInt());
    return retval.toInt();
}

/**
 * \returns the value set by the --cluster-name command line option.
 */
S9sString
S9sOptions::clusterName() const
{
    return getString("cluster_name");
}

bool
S9sOptions::hasClusterNameOption()
{
    return m_options.contains("cluster_name");
}

/**
 * \returns True if the --job-id command line option was provided.
 */
bool
S9sOptions::hasJobId() const
{
    return m_options.contains("job_id");
}

/**
 * \returns The job ID as it is set by the --job-id command line option.
 */
int
S9sOptions::jobId() const
{
    if (m_options.contains("job_id"))
        return m_options.at("job_id").toInt();

    return -1;
}

/**
 * \returns True if the --log-format command line option is provided.
 */
bool
S9sOptions::hasLogFormat() const
{
    return m_options.contains("log_format");
}

/**
 * \returns The "--log-format" command line option argument if the option was
 *   used, the empty string if it was not.
 */
S9sString
S9sOptions::logFormat() const
{
    return getString("log_format");
}

/**
 * \returns True if the --cluster-format command line option was provided.
 */
bool
S9sOptions::hasClusterFormat() const
{
    return m_options.contains("cluster_format");
}

/**
 * \returns The command line option argument for the --cluster-format option or
 *   the empty string if the option was not used.
 */
S9sString
S9sOptions::clusterFormat() const
{
    return getString("cluster_format");
}

/**
 * \returns True if the --node-format command line option was provided.
 */
bool
S9sOptions::hasNodeFormat() const
{
    return m_options.contains("node_format");
}

/**
 * \returns The command line option argument for the --node-format option or
 *   the empty string if the option was not used.
 */
S9sString
S9sOptions::nodeFormat() const
{
    return getString("node_format");
}

/**
 * \returns True if the --backup-format command line option was provided.
 */
bool
S9sOptions::hasBackupFormat() const
{
    return m_options.contains("backup_format");
}

/**
 * \returns The command line option argument for the --backup-format option or
 *   the empty string if the option was not used.
 */
S9sString
S9sOptions::backupFormat() const
{
    return getString("backup_format");
}

/**
 * \returns True if the --memory= command line option was provided.
 */
bool
S9sOptions::hasMemory() const
{
    return m_options.contains("memory");
}

/**
 * \returns The argument of the command line option --memory=
 */
S9sString
S9sOptions::memory() const
{
    return getString("memory");
}

/**
 * \returns True if the --container-format command line option was provided.
 */
bool
S9sOptions::hasContainerFormat() const
{
    return m_options.contains("container_format");
}

/**
 * \returns The command line option argument for the --container-format option
 *   or the empty string if the option was not used.
 */
S9sString
S9sOptions::containerFormat() const
{
    return getString("container_format");
}

/**
 * \returns True if the --user-format command line option was provided.
 */
bool
S9sOptions::hasUserFormat() const
{
    return m_options.contains("user_format");
}

/**
 * \returns The command line option argument for the --user-format option or
 *   the empty string if the option was not used.
 */
S9sString
S9sOptions::userFormat() const
{
    return getString("user_format");
}

/**
 * \returns The command line option argument for the --graph option.
 */
S9sString
S9sOptions::graph() const
{
    return getString("graph");
}

/**
 * \param tryLocalUserToo if the user name could not be determined use the local
 *   OS user (getenv("USER")) instead.
 * \returns The Cmon user name used to authenticate the user on the Cmon system.
 *
 * This method returns the username on the Cmon backend (the Cmon user) that
 * will be used to authenticate the user on the Cmon system and identify
 * everything the user does (like job owner, cluster owner and so on).
 *
 * The command line option that belongs to this property is the --user.
 */
S9sString 
S9sOptions::userName(
        const bool tryLocalUserToo) const
{
    S9sString retval;

    if (m_options.contains("cmon_user"))
    {
        retval = m_options.at("cmon_user").toString();
    } else {
        retval = m_userConfig.variableValue("cmon_user");

        if (retval.empty())
            retval = m_systemConfig.variableValue("cmon_user");
    }

    if (retval.empty() && tryLocalUserToo)
        retval = getenv("USER");

    return retval;
}

/**
 * \returns The command line option argument for the --password option.
 */
S9sString
S9sOptions::password() const
{
    S9sString retval;

    if (m_options.contains("password"))
    {
        retval = m_options.at("password").toString();
    } else if (!m_userConfig.variableValue("cmon_user").empty())
    {
        /*
         * A username is defined in user config the we must
         * use the password from the user config
         */
        retval = m_userConfig.variableValue("cmon_password");
    } else {
        /*
         * Username is not defined in the user config, so
         * we can safely return the system's config password here
         */
        retval = m_systemConfig.variableValue("cmon_password");
    }

    return retval;
}

/**
 * \returns True if the --password command line option was provided.
 */
bool
S9sOptions::hasPassword() const
{
    return m_options.contains("password");
}

/**
 * \returns True if the --old-password command line option was provided.
 */
bool 
S9sOptions::hasOldPassword() const
{
    return m_options.contains("old_password");
}

/**
 * \returns The command line option argument for the --old-password option.
 */
S9sString
S9sOptions::oldPassword() const
{
    return getString("old_password");
}

/**
 * \returns True if the --new-password command line option was provided.
 */
bool 
S9sOptions::hasNewPassword() const
{
    return m_options.contains("new_password");
}

/**
 * \returns The command line option argument for the --new-password option.
 */
S9sString
S9sOptions::newPassword() const
{
    return getString("new_password");
}

/**
 * \returns the account that was provided using the --account command line
 *   options.
 */
S9sAccount
S9sOptions::account() const
{
    S9sAccount retval;

    if (m_options.contains("account"))
        retval = m_options.at("account").toAccount();

    return retval;
}

/**
 * \returns the username property of the account that was presented by the
 *   --account command line option or the empty string if the account was not
 *   provided.
 */
S9sString
S9sOptions::accountName() const
{
    S9sString retval = m_options.at("account").toAccount().userName();

    return retval;

}

/**
 * \returns True if the account description was successfully parsed and the 
 *   account was stored, false if the string is mallformed.
 */
bool
S9sOptions::setAccount(
        const S9sString &value)
{
    S9sAccount account(value);

    m_options["account"] = account;
    return !account.hasError();
}

/**
 * \returns the command line option argument for the --privileges option.
 */
S9sString
S9sOptions::privileges() const
{
    return getString("privileges");
}

/**
 * \returns the argument for the command line option --opt-group.
 */
S9sString
S9sOptions::optGroup() const
{
    return getString("opt_group");
}

/**
 * \returns the argument for the command line option --opt-name.
 */
S9sString
S9sOptions::optName() const
{
    return getString("opt_name");
}

/**
 * \returns the argument for the command line option --opt-value.
 */
S9sString
S9sOptions::optValue() const
{
    return getString("opt_value");
}

/**
 * \returns the argument for the command line option --output-dir.
 */
S9sString
S9sOptions::outputDir() const
{
    return getString("output_dir");
}

/**
 * \returns the presence of the command line option --mask-passwords.
 */
bool
S9sOptions::maskPasswords() const
{
    char *variable;
    const char *key = "mask_passwords";
    S9sString   retval;

    variable = getenv("S9S_MASK_PASSWORDS");
    if (variable != NULL)
    {
        S9sString theString = variable;
        if (theString.toInt() > 0)
            return true;
    }

    /*
     * Return true if any of the config files
     * or command line defines mask_passwords
     */
    if (getBool(key))
        return true;

    if (m_userConfig.variableValue(key).toBoolean())
        return true;

    if (m_systemConfig.variableValue(key).toBoolean())
        return true;

    return false;
}

/**
 * \returns The argument for the --donor= command line option.
 */
S9sString
S9sOptions::donor() const
{
    return getString("donor");
}

/**
 * \returns The value for the --template= command line option.
 */
S9sString
S9sOptions::templateName() const
{
    return getString("template");
}

S9sString
S9sOptions::configTemplate() const
{
    return getString("config_template");
}

bool
S9sOptions::noInstall() const
{
    return getBool("no_install");
}

/**
 * \returns The value for the --cloud= command line option.
 */
S9sString
S9sOptions::cloudName() const
{
    return getString("cloud");
}

S9sString
S9sOptions::subnetId() const
{
    return getString("subnet_id");
}

S9sString
S9sOptions::vpcId() const
{
    return getString("vpc_id");
}

/**
 * \returns The value for the --image= command line option.
 */
S9sString
S9sOptions::imageName() const
{
    return getString("image");
}

/**
 * \returns The value for the --image-os-user= command line option.
 */
S9sString
S9sOptions::imageOsUser() const
{
    return getString("image_os_user");
}

/**
 *
 * \code{.js}
 * "volumes": [ 
 * {
 *     "name": "volume-1",
 *     "size": 10,
 *     "type": "hdd"
 * } ],
 * \endcode
 */
S9sVariantList
S9sOptions::volumes() const
{
    S9sVariantList retval;

    if (m_options.contains("volumes"))
        retval =  m_options.at("volumes").toVariantList();

    return retval;
}

bool
S9sOptions::appendVolumes(
        const S9sString &stringRep)
{
    S9sVariantList volumeStrings = stringRep.split(";");
    S9sVariantList volumesToSet  = volumes();

    for (uint idx = 0u; idx < volumeStrings.size(); ++idx)
    {
        S9sString volumeString = volumeStrings[idx].toString();
        S9sVariantList parts = volumeString.split(":");
        S9sVariantMap  volume;
        S9sString      name;
        S9sString      size;
        S9sString      type;

        if (parts.size() == 1)
        {
            name.sprintf("volume-%u", volumesToSet.size());
            type = "hdd";
            size = parts[0].toString();
        } else if (parts.size() == 2)
        {
            name.sprintf("volume-%u", volumesToSet.size());
            type = parts[1].toString();
            size = parts[0].toString();
        } else if (parts.size() == 3)
        {
            name = parts[0].toString();
            type = parts[2].toString();
            size = parts[1].toString();
        } else {
            return false;
        }

        if (size.toInt() <= 0)
            return false;

        volume["name"] = name;
        volume["type"] = type;
        volume["size"] = size.toInt();

        volumesToSet << volume;
    }

    m_options["volumes"] = volumesToSet;

    return true;
}

/**
 * \return True if the --force command line option is provided.
 */
bool
S9sOptions::force() const
{
    return getBool("force");
}

bool
S9sOptions::dry() const
{
    return getBool("dry");
}

bool
S9sOptions::useInternalRepos() const
{
    bool retval = false;

    if (m_options.contains("use_internal_repos"))
    {
        retval = m_options.at("use_internal_repos").toBoolean();
    } else {
        retval = m_userConfig.variableValue("use_internal_repos").toBoolean();
        if (!retval)
        {
            retval = m_systemConfig.variableValue(
                    "use_internal_repos").toBoolean();
        }
    }

    return retval;
}


/**
 * \returns true if the --with-database command line option was provided.
 */
bool
S9sOptions::withDatabase() const
{
    return getBool("with_database");
}

S9sString
S9sOptions::dbName() const
{
    return getString("db_name");
}

/**
 * \returns The argument for the --backup-directory option or the
 *   backup_directory config value.
 */
S9sString
S9sOptions::backupDir() const
{
    S9sString  retval;

    if (m_options.contains("backup_directory"))
    {
        retval = m_options.at("backup_directory").toString();
    } else {
        retval = m_userConfig.variableValue("backup_directory");
        if (retval.empty())
            retval = m_systemConfig.variableValue("backup_directory");
    }

    return retval;
}

S9sString
S9sOptions::subDirectory() const
{
    return getString("subdirectory");
}

S9sString
S9sOptions::tempDirPath() const
{
    return getString("temp_dir_path");
}

bool
S9sOptions::keepTempDir() const
{
    return getBool("keep_temp_dir");
}

/**
 * \returns true if the --encrypt-backup command line option was provided.
 */
bool
S9sOptions::encryptBackup() const
{
    return getBool("encrypt_backup");
}

/**
 * \returns true if the --no-compression command line option was provided.
 */
bool
S9sOptions::noCompression() const
{
    return getBool("no_compression");
}

bool
S9sOptions::pitrCompatible() const
{
    return getBool("pitr_compatible");
}

/**
 * \returns true if the --use-pigz command line option was provided.
 */
bool
S9sOptions::usePigz() const
{
    return getBool("use_pigz");
}

/**
 * \returns true if the --on-node command line option was provided.
 */
bool
S9sOptions::onNode() const
{
    return getBool("on_node");
}

/**
 * \returns true if the --on-controller command line option was provided.
 */
bool
S9sOptions::onController() const
{
    return getBool("on_controller");
}


/**
 * \returns the value provided with --backup-method or the backup_method
 *   configuration file value or the empty string if none of those are 
 *   provided.
 *
 * The controller currently supports 'ndb', 'mysqldump', 'xtrabackupfull',
 * 'xtrabackupincr', 'mongodump', 'pg_dump' and 'mysqlpump'.
 */
S9sString
S9sOptions::backupMethod() const
{
    S9sString  retval;

    if (m_options.contains("backup_method"))
    {
        retval = m_options.at("backup_method").toString();
    } else {
        retval = m_userConfig.variableValue("backup_method");
        if (retval.empty())
            retval = m_systemConfig.variableValue("backup_method");
    }

    return retval;
}

/**
 * \returns the value received for the --databases command line option.
 */
S9sString
S9sOptions::databases() const
{
    return getString("databases");
}

bool
S9sOptions::setParallellism(
        const S9sString &value)
{
    int integerValue = value.toInt();
    
    if (integerValue < 1)
    {
        m_errorMessage.sprintf(
                "The value '%s' is invalid for parallellism.",
                STR(value));

        m_exitStatus = BadOptions;
        return false;        
    }

    m_options["parallellism"] = integerValue;
    return true;
}

bool
S9sOptions::hasFirewalls() const
{
    return m_options.contains("firewalls");
}

S9sString
S9sOptions::firewalls() const
{
    return getString("firewalls");
}

/**
 * \returns The option argument for the --region option.
 */
S9sString
S9sOptions::region() const
{
    return getString("region");
}

/**
 * \returns The option argument for the --shell-command option.
 */
S9sString
S9sOptions::shellCommand() const
{
    return getString("shell_command");
}

bool 
S9sOptions::hasJobTags() const
{
    return m_options.contains("job_tags");
}

/**
 * \returns The list of tags passed as the argument for the --job-tags command
 *   line option.
 */
S9sVariantList
S9sOptions::jobTags() const
{
    S9sVariantList retval;

    if (m_options.contains("job_tags"))
        retval = m_options.at("job_tags").toVariantList();

    return retval;
}

/**
 * \param value The argument, a list of strings with , or ; as field separator.
 *
 * This is where we store the argument of the command line option --job-tags. 
 */
bool
S9sOptions::setJobTags(
        const S9sString &value)
{
    S9sVariantList tags = value.split(";,");

    m_options["job_tags"] = tags;
    return true;
}

/**
 * \returns The list of tags passed as the argument for the --with-tags command
 *   line option.
 */
S9sVariantList
S9sOptions::withTags() const
{
    S9sVariantList retval;

    if (m_options.contains("with_tags"))
        retval = m_options.at("with_tags").toVariantList();

    return retval;
}

/**
 * \param value The argument, a list of strings with , or ; as field separator.
 *
 * This is where we store the argument of the command line option --with-tags. 
 */
bool
S9sOptions::setWithTags(
        const S9sString &value)
{
    S9sVariantList tags = value.split(";,");

    m_options["with_tags"] = tags;
    return true;
}

/**
 * \returns The list of tags passed as the argument for the --without-tags 
 *   command line option.
 */
S9sVariantList
S9sOptions::withoutTags() const
{
    S9sVariantList retval;

    if (m_options.contains("without_tags"))
        retval = m_options.at("without_tags").toVariantList();

    return retval;
}

/**
 * \param value The argument, a list of strings with , or ; as field separator.
 *
 * This is where we store the argument of the command line option 
 * --without-tags. 
 */
bool
S9sOptions::setWithoutTags(
        const S9sString &value)
{
    S9sVariantList tags = value.split(";,");

    m_options["without_tags"] = tags;
    return true;
}


bool
S9sOptions::hasParallellism() const
{
    return m_options.contains("parallellism");
}

/**
 * \returns the integer value of the command line option argument for
 * --parallellism.
 */
int
S9sOptions::parallellism() const
{
    int retval = 1;

    if (m_options.contains("parallellism"))
        retval = m_options.at("parallellism").toInt();

    return retval;
}

bool
S9sOptions::setBackupRetention(
        const S9sString &value)
{
    if (!value.looksInteger())
    {
        m_errorMessage.sprintf(
                "The value '%s' is invalid for retention.",
                STR(value));

        m_exitStatus = BadOptions;
        return false;
    }

    m_options["backup_retention"] = value.toInt();
    return true;
}

bool
S9sOptions::hasBackupRetention() const
{
    return m_options.contains("backup_retention");
}

int
S9sOptions::backupRetention() const
{
    return getInt("backup_retention");
}

bool
S9sOptions::setCloudRetention(
        const S9sString &value)
{
    if (!value.looksInteger())
    {
        m_errorMessage.sprintf(
                "The value '%s' is invalid for cloud retention.",
                STR(value));

        m_exitStatus = BadOptions;
        return false;
    }

    m_options["cloud_retention"] = value.toInt();
    return true;
}

bool
S9sOptions::hasCloudRetention() const
{
    return m_options.contains("cloud_retention");
}

int
S9sOptions::cloudRetention() const
{
    return getInt("cloud_retention");
}

bool
S9sOptions::setSafetyCopies(
        const S9sString &value)
{
    if (!value.looksInteger())
    {
        m_errorMessage.sprintf(
                "The value '%s' is invalid for safety copies.",
                STR(value));

        m_exitStatus = BadOptions;
        return false;
    }

    m_options["safety_copies"] = value.toInt();
    return true;
}

bool
S9sOptions::hasSafetyCopies() const
{
    return m_options.contains("safety_copies");
}

int
S9sOptions::safetyCopies() const
{
    return getInt("safety_copies");
}


/**
 * \returns true if the --full-path command line option was provided.
 */
bool
S9sOptions::fullPathRequested() const
{
    return getBool("full_path");
}

/**
 * \returns True if the --to-individual-files command line option was provided.
 */
bool
S9sOptions::toIndividualFiles() const
{
    return getBool("to_individual_files");
}

/**
 * \returns True if the --backup-datadir command line option was provided.
 */
bool
S9sOptions::backupDatadir() const
{
    return getBool("backupDatadir");
}



/**
 * \returns true if the main operation is "node".
 */
bool
S9sOptions::isNodeOperation() const
{
    return m_operationMode == Node;
}

/**
 * \returns true if the main operation is "log".
 */
bool
S9sOptions::isLogOperation() const
{
    return m_operationMode == Log;
}

/**
 * \returns true if the main operation is "event".
 */
bool
S9sOptions::isEventOperation() const
{
    return m_operationMode == Event;
}

/**
 * \returns true if the main operation is "alarm".
 */
bool
S9sOptions::isAlarmOperation() const
{
    return m_operationMode == Alarm;
}

/**
 * \returns true if the main operation is "script".
 */
bool
S9sOptions::isScriptOperation() const
{
    return m_operationMode == Script;
}

bool
S9sOptions::isSheetOperation() const
{
    return m_operationMode == Sheet;
}

/**
 * \returns true if the main operation is "server".
 */
bool
S9sOptions::isControllerOperation() const
{
    return m_operationMode == Controller;
}

/**
 * \returns true if the main operation is "server".
 */
bool
S9sOptions::isServerOperation() const
{
    return m_operationMode == Server;
}

/**
 * \returns true if the main operation is "tree".
 */
bool
S9sOptions::isTreeOperation() const
{
    return m_operationMode == Tree;
}

/**
 * \returns true if the main operation is "cluster".
 */
bool
S9sOptions::isClusterOperation() const
{
    return m_operationMode == Cluster;
}

/**
 * \returns true if the main operation is "container".
 */
bool
S9sOptions::isContainerOperation() const
{
    return m_operationMode == Container;
}

/**
 * \returns true if the main operation is "job".
 */
bool
S9sOptions::isJobOperation() const
{
    return m_operationMode == Job;
}

/**
 * \returns true if the main operation is "backup".
 */
bool
S9sOptions::isBackupOperation() const
{
    return m_operationMode == Backup;
}

/**
 * \returns true if the main operation is "process".
 */
bool
S9sOptions::isProcessOperation() const
{
    return m_operationMode == Process;
}

/**
 * \returns true if the main operation is "user".
 */
bool
S9sOptions::isUserOperation() const
{
    return m_operationMode == User;
}

/**
 * \returns true if the main operation is "account".
 */
bool
S9sOptions::isAccountOperation() const
{
    return m_operationMode == Account;
}

/**
 * \returns true if the main operation is "maintenance".
 */
bool
S9sOptions::isMaintenanceOperation() const
{
    return m_operationMode == Maintenance;
}

/**
 * \returns true if the main operation is "metatype".
 */
bool
S9sOptions::isMetaTypeOperation() const
{
    return m_operationMode == MetaType;
}

bool
S9sOptions::isHelpRequested() const
{
    return getBool("help");
}

/**
 * \returns true if the "list" function is requested by providing the --list
 *   command line option.
 */
bool
S9sOptions::isListRequested() const
{
    return getBool("list");
}

bool
S9sOptions::isEnableCmonHaRequested() const
{
    return getBool("enable_cmon_ha");
}

/**
 * \returns True if the --list-partitions command line option is provided.
 */
bool
S9sOptions::isListPartitionsRequested() const
{
    return getBool("list_partitions");
}

/**
 * \returns True if the --list-images command line option is provided.
 */
bool
S9sOptions::isListImagesRequested() const
{
    return getBool("list_images");
}

/**
 * \returns True if the --list-regions command line option is provided.
 */
bool
S9sOptions::isListRegionsRequested() const
{
    return getBool("list_regions");
}

/**
 * \returns True if the --list-subnets command line option is provided.
 */
bool
S9sOptions::isListSubnetsRequested() const
{
    return getBool("list_subnets");
}

/**
 * \returns True if the --list-templates command line option is provided.
 */
bool
S9sOptions::isListTemplatesRequested() const
{
    return getBool("list_templates");
}

/**
 * \returns True if the --list-memory command line option is provided.
 */
bool
S9sOptions::isListMemoryRequested() const
{
    return getBool("list_memory");
}

/**
 * \returns True if the --get-acl command line option is provided.
 */
bool
S9sOptions::isGetAclRequested() const
{
    return getBool("get_acl");
}

bool
S9sOptions::isCatRequested() const
{
    return getBool("cat");
}

/**
 * \returns True if the --chown command line option is provided.
 */
bool
S9sOptions::isChOwnRequested() const
{
    return getBool("chown");
}

/**
 * \returns True if the --mkdir command line option is provided.
 */
bool
S9sOptions::isMkdirRequested() const
{
    return getBool("mkdir");
}

/**
 * \returns True if the --save command line option is provided.
 */
bool
S9sOptions::isSaveRequested() const
{
    return getBool("save");
}


/**
 * \returns True if the --touch command line option is provided.
 */
bool
S9sOptions::isMkfileRequested() const
{
    return getBool("mkfile");
}

/**
 * \returns True if the --rmdir command line option is provided.
 */
bool
S9sOptions::isRmdirRequested() const
{
    return getBool("rmdir");
}

/**
 * \returns True if the --access command line option is provided.
 */
bool
S9sOptions::isAccessRequested() const
{
    return getBool("access");
}

/**
 * \returns True if the --add-acl command line option is provided.
 */
bool
S9sOptions::isAddAclRequested() const
{
    return getBool("add_acl");
}

/**
 * \returns True if the --remove-acl command line option is provided.
 */
bool
S9sOptions::isRemoveAclRequested() const
{
    return getBool("remove_acl");
}

/**
 * \returns The command line option argument for the --acl option or the empty
 *   string if the option was not provided.
 */
S9sString
S9sOptions::acl() const
{
    return getString("acl");
}

bool
S9sOptions::hasOwner() const
{
    return m_options.contains("owner");
}

S9sString
S9sOptions::ownerUserName() const
{
    S9sString retval = getString("owner");

    if (retval.contains(":"))
    {
        S9sVariantList parts = retval.split(":");
        retval = parts[0].toString();
    } 

    return retval;
}

S9sString
S9sOptions::ownerGroupName() const
{
    S9sString retval = getString("owner");

    if (retval.contains(":"))
    {
        S9sVariantList parts = retval.split(":");
        retval = parts[1].toString();
    } 

    return retval;
}

/**
 * \returns True if the --list-processors command line option is provided.
 */
bool
S9sOptions::isListProcessorsRequested() const
{
    return getBool("list_processors");
}

/**
 * \returns True if the --list-nics command line option is provided.
 */
bool
S9sOptions::isListNicsRequested() const
{
    return getBool("list_nics");
}

/**
 * \returns True if the --list-disks command line option is provided.
 */
bool
S9sOptions::isListDisksRequested() const
{
    return getBool("list_disks");
}

bool
S9sOptions::isListGroupsRequested() const
{
    return getBool("list-groups");
}

/**
 * \returns true if the --stat command line option was provided to get a
 *   detailed list of something
 */
bool
S9sOptions::isStatRequested() const
{
    return getBool("stat");
}

bool
S9sOptions::isWatchRequested() const
{
    return getBool("watch");
}

bool
S9sOptions::isEditRequested() const
{
    return getBool("edit");
}

/**
 * \returns true if the "list-config" function is requested by providing the
 *     --list command line option.
 */
bool
S9sOptions::isListConfigRequested() const
{
    return getBool("list_config");
}

/**
 * \returns true if the "change-config" function is requested by providing the
 *     --change-config command line option.
 */
bool
S9sOptions::isChangeConfigRequested() const
{
    return getBool("change_config");
}

/**
 * \returns true if the "pull-config" function is requested by providing the
 *     --pull-config command line option.
 */
bool
S9sOptions::isPullConfigRequested() const
{
    return getBool("pull_config");
}

/**
 * \returns true if the "push-config" function is requested by providing the
 *     --push-config command line option.
 */
bool
S9sOptions::isPushConfigRequested() const
{
    return getBool("push_config");
}

/**
 * \returns true if the --list-properties main option was provided.
 */
bool
S9sOptions::isListPropertiesRequested() const
{
    return getBool("list_properties");
}

/**
 * \returns true if the --list-cluster-types main option was provided.
 */
bool
S9sOptions::isListClusterTypesRequested() const
{
    return getBool("list_cluster_types");
}


/**
 * \returns true if the --list-containers main option was provided.
 */
bool
S9sOptions::isListContainersRequested() const
{
    return getBool("list_containers");
}

/**
 * \returns true if the "whoami" function is requested by providing the 
 * --whoami command line option.
 */
bool
S9sOptions::isWhoAmIRequested() const
{
    return getBool("whoami");
}

/**
 * \returns true if the "list-keys" function is requested by providing the 
 * --list-keys command line option.
 */
bool
S9sOptions::isListKeysRequested() const
{
    return getBool("list_keys");
}

/**
 * \returns true if the "add-key" function is requested by providing the 
 * --add-key command line option.
 */
bool
S9sOptions::isAddKeyRequested() const
{
    return getBool("add_key");
}

/**
 * \returns true if the "set" function is requested using the --set command line
 *   option.
 */
bool
S9sOptions::isSetRequested() const
{
    return getBool("set");
}

/**
 * \returns true if the "change password" function is requested using the 
 *   --change-password command line option.
 */
bool
S9sOptions::isChangePasswordRequested() const
{
    return getBool("change_password");
}


/**
 * \returns true if the --log command line option was provided when the program
 *   was started.
 */
bool
S9sOptions::isLogRequested() const
{
    return getBool("log");
}

/**
 * \returns true if the --create command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isCreateRequested() const
{
    return getBool("create");
}


/**
 * \returns true if the --register command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isRegisterRequested() const
{
    return getBool("register");
}

/**
 * \returns True if the --refresh command line option was provided.
 */
bool
S9sOptions::isRefreshRequested() const
{
    return getBool("refresh");
}

/**
 * \returns True if the --all command line option was provided.
 */
bool
S9sOptions::isAllRequested() const
{
    return getBool("all");
}

/**
 * \returns True if the --recursive command line option was provided.
 */
bool
S9sOptions::isRecursiveRequested() const
{
    return getBool("recursive");
}

bool
S9sOptions::isDirectoryRequested() const
{
    return getBool("directory");
}

/**
 * \returns true if the --unregister command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isUnregisterRequested() const
{
    return getBool("unregister");
}

/**
 * \returns true if the --move command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isMoveRequested() const
{
    return getBool("move");
}

/**
 * \returns true if the --execute command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isExecuteRequested() const
{
    return getBool("execute");
}

/**
 * \returns true if the --run command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isRunRequested() const
{
    return getBool("run");
}

/**
 * \returns true if the --kill command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isKillRequested() const
{
    return getBool("kill");
}

/**
 * \returns true if the --system command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isSystemRequested() const
{
    return getBool("system");
}

/**
 * \returns true if the --tree command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isTreeRequested() const
{
    return getBool("tree");
}

/**
 * \returns True if the --delete command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isDeleteRequested() const
{
    return getBool("delete");
}

/**
 * \returns True if the --clone command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isCloneRequested() const
{
    return getBool("clone");
}

/**
 * \returns True if the --fail command line option was provided.
 */
bool
S9sOptions::isFailRequested() const
{
    return getBool("fail");
}

/**
 * \returns True if the --success command line option was provided.
 */
bool
S9sOptions::isSuccessRequested() const
{
    return getBool("success");
}

/**
 * \returns True if the --enable command line option was provided.
 */
bool
S9sOptions::isEnableRequested() const
{
    return getBool("enable");
}

/**
 * \returns True if the --set-group command line option is provided at startup.
 */
bool
S9sOptions::isSetGroupRequested() const
{
    return getBool("set_group");
}

/**
 * \returns True if the --add-to-group command line option is provided at 
 *   startup.
 */
bool
S9sOptions::isAddToGroupRequested() const
{
    return getBool("add_to_group");
}

/**
 * \returns True if the --remove-from-group command line option is provided at 
 *   startup.
 */
bool
S9sOptions::isRemoveFromGroupRequested() const
{
    return getBool("remove_from_group");
}

bool
S9sOptions::isDisableRequested() const
{
    return getBool("disable");
}

/**
 * \returns True if the --ping main option was used.
 */
bool
S9sOptions::isPingRequested() const
{
    return getBool("ping");
}

/**
 * \returns True if the --promote-slave main option was used.
 */
bool
S9sOptions::isPromoteSlaveRequested() const
{
    return getBool("promote_slave");
}

/**
 * \returns True if the --demote-node main option was used.
 */
bool
S9sOptions::isDemoteNodeRequested() const
{
    return getBool("demote_node");
}

/**
 * \returns true if the --restore command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isRestoreRequested() const
{
    return getBool("restore");
}

/**
 * \returns true if the --save-cluster-info command line option was provided
 *    when the program was started.
 */
bool
S9sOptions::isSaveClusterRequested() const
{
    return getBool("save_cluster");
}

/**
 * \returns true if the --restore-cluster-info command line option was provided
 *   when the program was started.
 */
bool
S9sOptions::isRestoreClusterRequested() const
{
    return getBool("restore_cluster");
}

/**
 * \returns true if the --save-controller command line option was provided
 *    when the program was started.
 */
bool
S9sOptions::isSaveControllerRequested() const
{
    return getBool("save_controller");
}

/**
 * \returns true if the --restore-controller command line option was provided
 *   when the program was started.
 */
bool
S9sOptions::isRestoreControllerRequested() const
{
    return getBool("restore_controller");
}

/**
 * \returns true if the --verify command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isVerifyRequested() const
{
    return getBool("verify");
}

/**
 * \returns true if the --delete-old command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isDeleteOldRequested() const
{
    return getBool("delete_old");
}

/**
 * \returns true if the --rolling-restart command line option was provided when
 *   the program was started.
 */
bool
S9sOptions::isRollingRestartRequested() const
{
    return getBool("rolling_restart");
}

/**
 * \returns true if the --collect-logs command line option was provided when
 *   the program was started.
 */
bool
S9sOptions::isCollectLogsRequested() const
{
    return getBool("collect_logs");
}

/**
 * \returns true if the --import-config command line option was provided when
 *   the program was started.
 */
bool
S9sOptions::isImportConfigRequested() const
{
    return getBool("import_config");
}

bool
S9sOptions::isEnableSslRequested() const
{
    return getBool("enable_ssl");
}

bool
S9sOptions::isDisableSslRequested() const
{
    return getBool("disable_ssl");
}

/**
 * \returns true if the --setup-audit-logging command line option provided.
 */
bool
S9sOptions::isSetupAuditLoggingRequested() const
{
    return getBool("setup_audit_logging");
}

/**
 * \returns true if the --create-report command line option was provided when
 *   the program was started.
 */
bool
S9sOptions::isCreateReportRequested() const
{
    return getBool("create_report");
}

/**
 * \returns true if the --deploy-agents command line option was provided
 * when the program was started.
 */
bool
S9sOptions::isDeployAgentsRequested() const
{
    return getBool("deploy_agents");
}

/**
 * \returns true if the add node operation was requested using the "--add-node"
 *   command line option.
 */
bool
S9sOptions::isAddNodeRequested() const
{
    return getBool("add_node");
}

/**
 * \returns true if the remove node oparation was requested using the
 *   "--remove-node" command line option.
 */
bool
S9sOptions::isRemoveNodeRequested() const
{
    return getBool("remove_node");
}

/**
 * \returns true if the --drop command line option was provided when the program
 *   was started.
 */
bool
S9sOptions::isDropRequested() const
{
    return getBool("drop");
}

bool
S9sOptions::isCreateSnapshotRequested() const
{
    return getBool("create_snapshot");
}

/**
 * \returns true if the --stop command line option was provided when the program
 *   was started.
 */
bool
S9sOptions::isStopRequested() const
{
    return getBool("stop");
}

/**
 * \returns true if the --start command line option was provided when the
 * program was started.
 */
bool
S9sOptions::isStartRequested() const
{
    return getBool("start");
}

/**
 * \returns true if the --restart command line option was provided when the
 * program was started.
 */
bool
S9sOptions::isRestartRequested() const
{
    return getBool("restart");
}

/**
 * \returns true if the --create-account command line option was provided when
 *   the program was started.
 */
bool
S9sOptions::isCreateAccountRequested() const
{
    return getBool("create_account");
}

/**
 * \returns true if the --grant command line option was provided when
 *   the program was started.
 */
bool
S9sOptions::isGrantRequested() const
{
    return getBool("grant");
}

/**
 * \returns true if the --check-hosts command line option was provided when
 *   the program was started.
 */
bool
S9sOptions::isCheckHostsRequested() const
{
    return getBool("check_hosts");
}


/**
 * \returns true if the --delete-account command line option was provided when
 *   the program was started.
 */
bool
S9sOptions::isDeleteAccountRequested() const
{
    return getBool("delete_account");
}

/**
 * \returns true if the --create-database command line option was provided when
 *   the program was started.
 */
bool
S9sOptions::isCreateDatabaseRequested() const
{
    return getBool("create_database");
}

/**
 * \returns true if the --list-databases command line option was provided when
 *   the program was started.
 */
bool
S9sOptions::isListDatabasesRequested() const
{
    return getBool("list_databases");
}

bool
S9sOptions::isListFilesRequested() const
{
    return getBool("list_files");
}

/**
 * \returns true if the --long command line option was provided.
 */
bool
S9sOptions::isLongRequested() const
{
    return getBool("long");
}

/**
 * \returns true if the --print-json command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isJsonRequested() const
{
    return getBool("print_json");
}

/**
 * \returns true if the --top command line option was provided when starting the
 *   program.
 */
bool
S9sOptions::isTopRequested() const
{
    return getBool("top");
}

/**
 * \returns true if the --wait command line option was used.
 */
bool
S9sOptions::isWaitRequested() const
{
    return getBool("wait");
}

/**
 * \returns true if the --batch command line option was used.
 */
bool
S9sOptions::isBatchRequested() const
{
    return getBool("batch");
}

/**
 * \returns true if the --no-header or --batch command line options are
 *   provided.
 */
bool
S9sOptions::isNoHeaderRequested() const
{
    if (isBatchRequested())
        return true;
    
    return getBool("no_header");
}

/**
 * \returns true if the passed string matches the extra command line arguments
 *   (the command line arguments found after the last command line options).
 *   Also returns true if the program did not receive extra arguments. 
 */
bool
S9sOptions::isStringMatchExtraArguments(
        const S9sString &theString) const
{
    if (m_extraArguments.empty())
        return true;

    for (uint idx = 0u; idx < m_extraArguments.size(); ++idx)
    {
        const S9sString &pattern = m_extraArguments[idx];

        if (fnmatch(STR(pattern), STR(theString), FNM_EXTMATCH) == 0)
            return true;
    }

    return false;
}

/**
 * \returns The number of extra arguments, arguments that are not commands (e.g.
 *   'cluster' or 'user'), not command line options and not command line option
 *   arguments.
 */
uint
S9sOptions::nExtraArguments() const
{
    return m_extraArguments.size();
}

/**
 * \returns One extra argument by its index.
 *
 * Extra arguments that are not commands (e.g.  'cluster' or 'user'), not
 * command line options and not command line option arguments.
 */
S9sString
S9sOptions::extraArgument(
        uint idx)
{
    if (idx < m_extraArguments.size())
        return m_extraArguments[idx];

    return S9sString();
}

/**
 * \returns true if the program should use syntax highlighting in its output.
 */
bool
S9sOptions::useSyntaxHighlight() 
{
    S9sString configValue;

    if (isBatchRequested())
        return false;

    if (m_options.contains("color"))
    {
        configValue = m_options.at("color").toString();
    } else {
        configValue = m_userConfig.variableValue("color");

        if (configValue.empty())
            configValue = m_systemConfig.variableValue("color");
    }

    if (configValue.empty())
        configValue = "auto";

    if (configValue.toLower() == "auto")
    {
        if (isBatchRequested())
            return false;

        return isatty(fileno(stdout)) ? true : false;
    } else if (configValue.toLower() == "always")
    {
        return true;
    }

    return false;
}

bool
S9sOptions::truncate()
{
    S9sString configValue;

    if (m_options.contains("truncate"))
    {
        configValue = m_options.at("truncate").toString();
    } else {
        configValue = m_userConfig.variableValue("truncate");

        if (configValue.empty())
            configValue = m_systemConfig.variableValue("truncate");
    }

    if (configValue.empty())
        configValue = "auto";

    if (configValue.toLower() == "auto")
    {
        if (isBatchRequested())
            return false;

        return isatty(fileno(stdout)) ? true : false;
    } else if (configValue.toLower() == "always")
    {
        return true;
    }

    return false;
}

/**
 * \returns true if the --human-readable or -h option was provided and so the
 *   values should be printed with prefixes (like "1.1GB").
 */
bool
S9sOptions::humanReadable() const
{
    return getBool("human_readable");
}

void
S9sOptions::setHumanReadable(
        const bool value)
{
    m_options["human_readable"] = value;
}

S9sString 
S9sOptions::timeStyle() const
{
    return getString("time_style");
}

/**
 * \returns True if the standard output is connected to a terminal.
 */
bool
S9sOptions::isTerminal() 
{
    return isatty(fileno(stdout));
}

/**
 * \returns How many characters the terminal can show in one line.
 */
int 
S9sOptions::terminalWidth() 
{
    S9sString      theString;
    struct winsize win;
    int            retcode;

    retcode = ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);

    if (retcode == 0)
        return win.ws_col;

    theString = getenv("COLUMNS");
    if (!theString.empty())
        return theString.toInt();

    return 80;
}

/**
 * \returns How many lines the terminal can show in one screen.
 */
int 
S9sOptions::terminalHeight() 
{
    struct winsize win;
    int            retcode;

    retcode = ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
    if (retcode == 0)
        return win.ws_row;

    return 25;
}


/**
 * \returns the binary program name of the running application.
 */
S9sString
S9sOptions::binaryName() const
{
    return m_myName;
}

/**
 * \returns the current exit status of the running application.
 *
 * The application should return this integer when exiting.
 */
int 
S9sOptions::exitStatus() const 
{ 
    return m_exitStatus; 
}

/**
 * \param exitStatus the exit status of the program that will be stored in the
 *   object and shall be returned when the application exits.
 */
void
S9sOptions::setExitStatus(
        const S9sOptions::ExitCodes exitStatus)
{
    m_exitStatus = exitStatus;
}

/**
 * \returns true if the application is in verbose mode (for e.g. the --verbose
 *   command line option is provided when the program was started)
 */
bool
S9sOptions::isVerbose() const
{
    char *variable;
    
    variable = getenv("S9S_VERBOSE");
    if (variable != NULL)
    {
        S9sString theString = variable;
        if (theString.toInt() > 0)
            return true;
    }

    return getBool("verbose");
}

void
S9sOptions::setVerbose(
        bool value)
{
    m_options["verbose"] = value;
}

bool
S9sOptions::isDebug() const
{
    return getBool("debug");
}

/**
 * \returns true if client must use TLS for controller RPC connections
 */
bool
S9sOptions::useTls()
{
    S9sString retval;

    if (controllerProtocol() == "https")
        return true;

    if (m_options.contains("rpc_tls"))
    {
        retval = m_options.at("rpc_tls").toString();
    } else {
        retval = m_userConfig.variableValue("rpc_tls");

        if (retval.empty())
            retval = m_systemConfig.variableValue("rpc_tls");
    }

    return retval.toBoolean();
}
/**
 * \returns a human readable error description stored inside the object.
 */
S9sString 
S9sOptions::errorString() const
{
    return m_errorMessage;
}

/**
 * \param formatString Standard printf() style format string.
 */
void
S9sOptions::printVerbose(
        const char *formatString,
        ...)
{
    S9sOptions *options = S9sOptions::instance();

    if (!options->isVerbose())
        return;

    S9sString  theString;
    va_list     arguments;
    
    va_start(arguments, formatString);
    theString.vsprintf (formatString, arguments);
    va_end(arguments);

    printf("%s\n", STR(theString));
}

/**
 * \param formatString Standard printf() style format string.
 */
void
S9sOptions::printError(
        const char *formatString,
        ...)
{
    S9sString  theString;
    va_list     arguments;
    
    va_start(arguments, formatString);
    theString.vsprintf(formatString, arguments);
    va_end(arguments);

    fprintf(stderr, "%s\n", STR(theString));
    fflush(stderr);
}

/**
 * \returns true if everything was ok, false if there was some errors with the
 *   command line options.
 */
bool
S9sOptions::readOptions(
        int   *argc,
        char  *argv[])
{
    bool retval = true;

    S9S_DEBUG("");
    if (*argc < 2)
    {
        m_errorMessage = "Missing command line options.";
        m_exitStatus   = BadOptions;
        return false;
    }

    m_myName = S9sFile::basename(argv[0]);
    if (*argc < 2)
    {
        m_errorMessage = "Missing command line options.";
        m_exitStatus   = BadOptions;
        return false;
    }

    /*
     * A heuristics to find the mode name. Should be the first, but anyway, we
     * try fo find it anyway.
     */
    for (int n = 1; n < *argc; ++n)
    {
        if (argv[n] == NULL)
            break;

        if (argv[n][0] == '-')
            continue;
        
        if (argv[n][0] == '/')
            continue;

        retval = setMode(argv[n]);
        if (!retval)
            return retval;

        break;
    }

    switch (m_operationMode)
    {
        case NoMode:
            retval = readOptionsNoMode(*argc, argv);
            break;

        case Cluster:
            retval = readOptionsCluster(*argc, argv);
            
            if (retval)
                retval = checkOptionsCluster();

            break;
        
        case Container:
            retval = readOptionsContainer(*argc, argv);
            
            if (retval)
                retval = checkOptionsContainer();

            break;
        
        case Job:
            retval = readOptionsJob(*argc, argv);
            
            if (retval)
                retval = checkOptionsJob();

            break;

        case Node:
            retval = readOptionsNode(*argc, argv);
            
            if (retval)
                retval = checkOptionsNode();

            break;

        case Backup:
            retval = readOptionsBackup(*argc, argv);
            
            if (retval)
                retval = checkOptionsBackup();

            break;
 
        case Process:
            retval = readOptionsProcess(*argc, argv);
            
            if (retval)
                retval = checkOptionsProcess();

            break;

        case User:
            retval = readOptionsUser(*argc, argv);
            
            if (retval)
                retval = checkOptionsUser();

            break;
        
        case Account:
            retval = readOptionsAccount(*argc, argv);
            
            if (retval)
                retval = checkOptionsAccount();

            break;
        
        case Maintenance:
            retval = readOptionsMaintenance(*argc, argv);
            
            if (retval)
                retval = checkOptionsMaintenance();

            break;
        
        case MetaType:
            retval = readOptionsMetaType(*argc, argv);

            if (retval)
                retval = checkOptionsMetaType();

            break;
        
        case Script:
            retval = readOptionsScript(*argc, argv);
            
            if (retval)
                retval = checkOptionsScript();

            break;
        
        case Sheet:
            retval = readOptionsSheet(*argc, argv);
            
            if (retval)
                retval = checkOptionsSheet();

            break;
        
        case Server:
            retval = readOptionsServer(*argc, argv);
            
            if (retval)
                retval = checkOptionsServer();

            break;
        
        case Controller:
            retval = readOptionsController(*argc, argv);
            
            if (retval)
                retval = checkOptionsController();

            break;
        
        case Tree:
            retval = readOptionsTree(*argc, argv);
            
            if (retval)
                retval = checkOptionsTree();

            break;

        case Log:
            retval = readOptionsLog(*argc, argv);
            
            if (retval)
                retval = checkOptionsLog();

            break;

        case Event:
            retval = readOptionsEvent(*argc, argv);
            
            if (retval)
                retval = checkOptionsEvent();

            break;

        case Alarm:
            retval = readOptionsAlarm(*argc, argv);
            
            if (retval)
                retval = checkOptionsAlarm();

            break;
    }

    return retval;
}

/**
 * \returns true if the operation requested by the command line options is
 *   performed and so the application can exit gracefuly.
 *
 * This method is for executing simple info requests like "--version" or
 * "--help". If the user request an operation like these the S9sOptions can
 * execute them and then the application can exit without further ado.
 */
bool
S9sOptions::executeInfoRequest()
{
    S9S_DEBUG("");

    if (m_options["print-version"].toBoolean())
    {
        printf("      ___            _              _     \n"
               " ___ / _ \\ ___      | |_ ___   ___ | |___ \n"
               "/ __| (_) / __|_____| __/ _ \\ / _ \\| / __|\n"
               "\\__ \\\\__, \\__ \\_____| || (_) | (_) | \\__ \\\n"
               "|___/  /_/|___/      \\__\\___/ \\___/|_|___/\n");
        printf("\n");

        printf("%s Version %s (Solar System)\n",
            PACKAGE_NAME, PACKAGE_VERSION);

        /*
         * Older installer scripts (install-s9s-tools.sh) are grepping
         * for uppercase 'BUILD', so lets keep it like that for a while
         */
        printf("BUILD (%s-%s) %s\n",
            PACKAGE_VERSION, GIT_VERSION, BUILD_DATE);

        printf("Copyright (C) 2016-2018 Severalnines AB\n");
        printf("\n");
        //printf("Written by ...\n");
        return true;
    } else if (m_options.contains("help") && m_options["help"].toBoolean())
    {
        printHelp();
        return true;
    }

    return false;
}

bool 
S9sOptions::setMode(
        const S9sString &modeName)
{
    bool retval = true;
   
    S9S_DEBUG("*** modeName: '%s'", STR(modeName));
    if (m_modes.contains(modeName))
    {
        m_operationMode = m_modes.at(modeName);
    } else if (modeName.startsWith("-"))
    {
        // Ignored.
        // FIXME: maybe not the best way to do this.
    } else if (!modeName.empty())
    {
        // There is a mode, but it is not supported or invalid.
        m_errorMessage.sprintf("The '%s' is not a valid mode.", STR(modeName));
        m_exitStatus = BadOptions;
        retval = false;
    } else {
        m_errorMessage = "The first command line option must be the mode.";
        m_exitStatus = BadOptions;
        retval = false;
    }

    return retval;
}

void 
S9sOptions::printHelp()
{
    switch (m_operationMode)
    {
        case NoMode:
            printHelpGeneric();
            break;

        case Cluster:
            printHelpCluster();
            break;
        
        case Container:
            printHelpContainer();
            break;

        case Node:
            printHelpNode();
            break;

        case Job:
            printHelpJob();
            break;

        case Process:
            printHelpProcess();
            break;

        case Backup:
            printHelpBackup();
            break;

        case Maintenance:
            printHelpMaintenance();
            break;
        
        case MetaType:
            printHelpMetaType();
            break;

        case User:
            printHelpUser();
            break;
        
        case Account:
            printHelpAccount();
            break;
        
        case Script:
            printHelpScript();
            break;
        
        case Sheet:
            printHelpSheet();
            break;
        
        case Server:
            printHelpServer();
            break;
        
        case Controller:
            printHelpController();
            break;
        
        case Tree:
            printHelpTree();
            break;

        case Log:
            printHelpLog();
            break;
        
        case Event:
            printHelpEvent();
            break;
        
        case Alarm:
            printHelpAlarm();
            break;
    }
}

void 
S9sOptions::printHelpGeneric()
{
    printf(
"Usage:\n"
"  %s COMMAND [OPTION...]\n"
"\n"
"Where COMMAND is:\n"
"    account - to manage accounts on clusters.\n"
"      alarm - to manage alarms.\n"
"     backup - to view, create and restore database backups.\n"
"    cluster - to list and manipulate clusters.\n"
" controller - to manage Cmon controllers.\n"
"        job - to view jobs.\n"
"      maint - to view and manipulate maintenance periods.\n"
"   metatype - to print metatype information.\n"
"       node - to handle nodes.\n"
"    process - to view processes running on nodes.\n"
"     script - to manage and execute scripts.\n"
"     server - to manage hardware resources.\n"
"      sheet - to manage spreadsheets.\n"
"       user - to manage users.\n"
"\n"
"Generic options:\n"
"  --help                     Show help message and exit.\n" 
"  -v, --verbose              Print more messages than normally.\n"
"  -V, --version              Print version information and exit.\n"
"  -c, --controller=URL       The URL where the controller is found.\n"
"  -P, --controller-port INT  The port of the controller.\n"
"  --rpc-tls                  Use TLS encryption to controller.\n"
"  -u, --cmon-user=USERNAME   The username on the Cmon system.\n"
"  -p, --password=PASSWORD    The password for the Cmon user.\n"
"  --private-key-file=FILE    The name of the file for authentication.\n"
"\n"
"Formatting:\n"
"  --batch                    No colors, no human readable, pure data.\n"
"  --color=always|auto|never  Sets if colors should be used in the output.\n"
"  --config-file=PATH         Set the configuration file.\n"
"  --date-format=FORMAT       The format of the dates printed.\n"
"  -l, --long                 Print the detailed list.\n"
"  --log-file=PATH            The path where the s9s client puts its logs.\n"
"  --no-header                Do not print headers.\n"
"  --only-ascii               Do not use UTF8 characters.\n"
"  --print-json               Print the sent/received JSon messages.\n"
"\n"
"Job related options:\n"
"  --job-tags=LIST            Set job tags when creating a new job.\n"
"  --log                      Wait and monitor job messages.\n"
"  --recurrence=CRONTABSTRING Timing information for recurring jobs.\n"
"  --schedule=DATE&TIME       Run the job at the specified time.\n"
"  --timeout=SECONDS          Timeout value for the entire job.\n"
"  --wait                     Wait until the job ends.\n"
"\n", STR(m_myName));
}

void
S9sOptions::printHelpJob()
{
    printHelpGeneric();

    printf(
"Options for the \"job\" command:\n"
"  --clone                    Clone and re-run a job.\n"
"  --delete                   Delete the job referenced by the job ID.\n"
"  --fail                     Create a job that does nothing and fails.\n"
"  --kill                     Send a signal to the job.\n"
"  --list                     List the jobs.\n"
"  --log                      Print the job log messages.\n"
"  --success                  Create a job that does nothing and succeeds.\n"
"  --wait                     Wait for the job referenced by the job ID.\n"
"\n"
"  --cluster-id=ID            The ID of the cluster.\n"
"  --cluster-name=NAME        Name of the cluster.\n"
"\n"
"  --from=DATE&TIME           The start of the interval to be printed.\n"
"  --job-id=ID                The ID of the job.\n"
"  --limit=NUMBER             Controls how many jobs are printed max.\n"
"  --offset=NUMBER            Controls the index of the first item printed.\n"
"  --until=DATE&TIME          The end of the interval to be printed.\n"
"\n"
"  --show-aborted             Show aborted jobs while printing job list.\n"
"  --show-defined             Show defined jobs while printing job list.\n"
"  --show-failed              Show failed jobs while printing job list.\n"
"  --show-finished            Show finished jobs while printing job list.\n"
"  --show-running             Show running jobs while printing job list.\n"
"  --show-scheduled           Show scheduled jobs while printing job list.\n"
"  --with-tags=LIST           Show only the jobs that has the tags.\n"
"  --without-tags=LIST        Show only the jobs that does not have the tags.\n"
"\n"
    );
}

void
S9sOptions::printHelpProcess()
{
    printHelpGeneric();

    printf(
"Options for the \"process\" command:\n"
"  --list                     List the processes.\n"
"  --top                      Continuosly print top processes.\n"
"\n"
"  --cluster-id=ID            The ID of the cluster to show.\n"
"  --limit=N                  Limit the number of processes shown.\n"
"  --sort-by-memory           Sort by resident size instead of CPU usage.\n"
"  --update-freq=SECS         The screen update frequency for top.\n"
"\n"
    );
}

/**
 * Prints the help text for the "backup" mode. Accessible like this:
 * s9s backup --help
 *
 */
void
S9sOptions::printHelpBackup()
{
    printHelpGeneric();

    printf(
"Options for the \"backup\" command:\n"
"  --create                   Create a new backup.\n"
"  --delete                   Delete a previously created backup.\n"
"  --delete-old               Delete old backups.\n"
"  --list-databases           List the backups in database format.\n"
"  --list-files               List the backups in backup file format.\n"
"  --list                     List the backups.\n"
"  --restore-cluster-info     Restores a saved cluster object.\n"
"  --restore-controller       Restores the controller from a file.\n"
"  --restore                  Restore an existing backup.\n"
"  --save-cluster-info        Saves the information about one cluster.\n"
"  --save-controller          Saves the entire controller into a file.\n"
"  --verify                   Verify an existing backup on a test server.\n"
"\n"
"  --backup-id=ID             The ID of the backup.\n"
"  --cluster-id=ID            The ID of the cluster.\n"
"  --nodes=NODELIST           The list of nodes involved in the backup.\n"
"\n"
"  --backup-datadir           Backup the SQL data directory before restoring.\n"
"  --backup-directory=DIR     The directory where the backup is placed.\n"
"  --backup-format=STRING     The format string used while printing backups.\n"
"  --backup-method=METHOD     Defines the backup program to be used.\n"
"  --backup-password=PASSWD   The password for the backup user.\n"
"  --backup-retention=DAYS    How many days before the backup is removed.\n"
"  --backup-user=USERNAME     The SQL account name creates the backup.\n"
"  --cloud-retention=DAYS     Retention used when the backup is on a cloud.\n"
"  --databases=LIST           Comma separated list of databases to archive.\n"
"  --encrypt-backup           Encrypt the files using AES-256 encryption.\n"
"  --full-path                Print the full path of the files.\n"
"  --no-compression           Do not compress the archive file.\n"
"  --on-controller            Stream the backup to the controller host.\n"
"  --on-node                  Store the archive file on the node itself.\n"
"  --parallellism=N           Number of threads used while creating backup.\n"
"  --pitr-compatible          Create backup compatible with PITR.\n"
"  --safety-copies=N          How many copies kept even when they are old.\n"
"  --subdirectory=PATTERN     The subdirectory that holds the new backup.\n"
"  --test-server=HOSTNAME     Verify the backup by restoring on this server.\n"
"  --title=STRING             Title for the backup.\n"
"  --to-individual-files      Archive every database into individual files.\n"
"  --use-pigz                 Use the pigz program to compress archive.\n"
"\n"
    );
}

void
S9sOptions::printHelpMaintenance()
{
    printHelpGeneric();

    printf(
"Options for the \"maintenance\" command:\n"
"  --create                   Create a new maintenance period.\n"
"  --delete                   Delete a maintenance period.\n"
"  --list                     List the maintenance periods.\n"
"\n"
"  --cluster-id=ID            The cluster for cluster maintenances.\n"
"  --end=DATE&TIME            The end of the maintenance period.\n"
"  --full-uuid                Print the full UUID.\n"
"  --nodes=NODELIST           The nodes for the node maintenances.\n"
"  --reason=STRING            The reason for the maintenance.\n"
"  --start=DATE&TIME          The start of the maintenance period.\n"
"  --uuid=UUID                The UUID to identify the maintenance period.\n"
"\n"
    );
}

void
S9sOptions::printHelpMetaType()
{
    printHelpGeneric();

    printf(
"Options for the \"metatype\" command:\n"
"  --list                     List all the metatypes.\n"
"  --list-properties          List the properties of a certain type.\n"
"\n"
"  --type=NAME                The name of the type.\n"
"\n"
    );
}

/**
 * Prints the help text for the 'user' mode. This is called like this:
 *
 * s9s user --help
 */
void
S9sOptions::printHelpUser()
{
    printHelpGeneric();

    printf(
"Options for the \"user\" command:\n"
"  --add-key                  Register a new public key for a user.\n"
"  --add-to-group             Add the user to a group.\n"
"  --change-password          Change the password for an existing user.\n"
"  --create                   Create a new Cmon user.\n"
"  --delete                   Delete existing user.\n"
"  --disable                  Preventing users to log in.\n"
"  --enable                   Enable disabled/suspended users.\n"
"  --list-groups              List user groups.\n"
"  --list-keys                List the public keys of a user.\n"
"  --list                     List the users.\n"
"  --remove-from-group        Remove the user from a group.\n"
"  --set                      Change the properties of a user.\n"
"  --set-group                Set the primary group for an existing user.\n"
"  --stat                     Print the details of a user.\n"
"  --whoami                   List the current user only.\n"
"\n"
"  --create-group             Create the group if it doesn't exist.\n"
"  --email-address=ADDRESS    The email address for the user.\n"
"  --first-name=NAME          The first name of the user.\n"
"  -g, --generate-key         Generate an RSA keypair for the user.\n"
"  --group=GROUP_NAME         The primary group for the new user.\n"
"  --last-name=NAME           The last name of the user.\n"
"  --new-password=PASSWORD    The new password to set.\n"
"  --public-key-file=FILE     The name of the file where the public key is.\n"
"  --public-key-name=NAME     The name of the public key.\n"
"  --title=TITLE              The prefix title for the user.\n"
"  --user-format=FORMAT       The format string used to print users.\n"
"\n");
}

/**
 * Prints the help text for the 'account' mode. This is called like this:
 *
 * s9s account --help
 */
void
S9sOptions::printHelpAccount()
{
    printHelpGeneric();

    printf(
"Options for the \"account\" command:\n"
"  --create                   Create a new account on the cluster.\n"
"  --delete                   Remove the account from the cluster.\n"
"  --grant                    Grant privileges for the account.\n"
"  --list                     List the accounts on the cluster.\n"
"\n"
"  --privileges=PRIVILEGES    The privileges for the account.\n"
"  --account=ACCOUNT          The account itself.\n"
"\n");
}

void 
S9sOptions::printHelpCluster()
{
    printHelpGeneric();

    printf(
"Options for the \"cluster\" command:\n"
"  --add-node                 Add a new node to the cluster.\n"
"  --check-hosts              Check the hosts before installing a cluster.\n"
"  --collect-logs             Collects logs from the nodes.\n"
"  --create-account           Create a user account on the cluster.\n"
"  --create                   Create and install a new cluster.\n"
"  --create-database          Create a database on the cluster.\n"
"  --create-report            Starts a job that will create an error report.\n"
"  --delete-account           Delete a user account on the cluster.\n"
"  --demote-node              Demote a node to slave.\n"
"  --deploy-agents            Starts a job to deploy agents to the nodes.\n"
"  --disable-ssl              Disable SSL connections on the nodes.\n"
"  --drop                     Drop cluster from the controller.\n"
"  --enable-ssl               Enable SSL connections on the nodes.\n"
"  --import-config            Collects configuration files from the nodes.\n"
"  --list-databases           List the databases found on the cluster.\n"
"  --list                     List the clusters.\n"
"  --ping                     Check the connection to the controller.\n"
"  --promote-slave            Promote a slave to become a master.\n"
"  --register                 Register a pre-existing cluster.\n"
"  --remove-node              Remove a node from the cluster.\n"
"  --rolling-restart          Restart the nodes without stopping the cluster.\n"
"  --setup-audit-logging      Set up the audit logging on the nodes.\n"
"  --start                    Start the cluster.\n"
"  --stat                     Print the details of a cluster.\n"
"  --stop                     Stop the cluster.\n"
"\n"
"  --account=NAME[:PASSWD][@HOST] Account to be created on the cluster.\n"
"  --cloud=PROVIDER           The name of the cloud provider.\n"
"  --cluster-format=FORMAT    The format string used to print clusters.\n"
"  --cluster-id=ID            The ID of the cluster to manipulate.\n"
"  --cluster-name=NAME        Name of the cluster to manipulate or create.\n"
"  --cluster-type=TYPE        The type of the cluster to install. Currently\n"
"  --config-template=FILE     Use the given file as configuration template.\n"
"  --containers=LIST          List of containers to be created.\n"
"  --credential-id=ID         The optional cloud credential ID.\n"
"  --db-admin-passwd=PASSWD   The password for the database admin.\n"
"  --db-admin=USERNAME        The database admin user name.\n"
"  --db-name=NAME             The name of the database.\n"
"  --donor=ADDRESS            The address of the donor node when starting.\n"
"  --firewalls=LIST           ID of the firewalls of the new container.\n"
"  --generate-key             Generate an SSH key when creating containers.\n"
"  --image=NAME               The name of the image for the container.\n"
"  --image-os-user=NAME       The name of the initial user on image.\n"
"  --job-tags=LIST            Tags for the job if a job is created.\n"
"  --nodes=NODE_LIST          List of nodes to work with.\n"
"  --no-install               Do not install the cluster software.\n"
"  --opt-group=NAME           The option group for configuration.\n"
"  --opt-name=NAME            The name of the configuration item.\n"
"  --opt-value=VALUE          The value for the configuration item.\n"
"  --os-key-file=PATH         The key file to register on the container.\n"
"  --os-password=PASSWORD     The password to set on the container.\n"
"  --os-user=USERNAME         The name of the user for the SSH commands.\n"
"  --output-dir=DIR           The directory where the files are created.\n"
"  --provider-version=VER     The version of the software.\n" 
"  --subnet-id=ID             The ID of the subnet for the new container(s).\n"
"  --template=NAME            The name of the template for new container(s).\n"
"  --use-internal-repos       Use local repos when installing software.\n"
"  --vendor=VENDOR            The name of the software vendor.\n"
"  --volumes=LIST             List the volumes for the new container(s).\n"
"  --vpc-id=ID                The ID of the virtual private cloud.\n"
"  --with-database            Create a database for the user too.\n"
"\n");
}

void 
S9sOptions::printHelpContainer()
{
    printHelpGeneric();

    printf(
"Options for the \"container\" command:\n"
"  --create                   Create and start a new container.\n"
"  --delete                   Stop and delete the container.\n"
"  --list                     List the containers.\n"
"  --start                    Start an existing container.\n"
"  --stat                     Print the details of a container.\n"
"  --stop                     Stop the container.\n"
"\n"
"  --cloud=PROVIDER           The name of the cloud provider.\n"
"  --container-format=FORMAT  Format string to print containers.\n"
"  --containers=LIST          List of containers to be created.\n"
"  --credential-id=ID         The optional cloud credential ID.\n"
"  --firewalls=LIST           ID of the firewalls of the new container.\n"
"  --generate-key             Generate an SSH key when creating containers.\n"
"  --image=NAME               The name of the image for the container.\n"
"  --image-os-user=NAME       The name of the initial user on image.\n"
"  --os-key-file=PATH         The key file to register on the container.\n"
"  --os-password=PASSWORD     The password to set on the container.\n"
"  --os-user=USERNAME         The username to create on the container.\n"
"  --region=NAME              The regionin which the container is created.\n"
"  --servers=LIST             A list of servers to work with.\n"
"  --subnet-id=ID             The ID of the subnet.\n"
"  --template=NAME            The name of the container template.\n"
"  --volumes=LIST             List the volumes for the new container.\n"
"  --vpc-id=ID                The ID of the virtual private cloud.\n"
"\n");
}

void 
S9sOptions::printHelpNode()
{
    printHelpGeneric();

    printf(
"Options for the \"node\" command:\n"
"  --change-config            Change the configuration for a node.\n"
"  --list-config              Print the configuration for a node.\n"
"  --list                     List the jobs found on the controller.\n"
"  --pull-config              Copy configuration files from a node.\n"
"  --push-config              Copy configuration files to a node.\n"
"  --set                      Change the properties of a node.\n"
"  --stat                     Print detailed node information.\n"
"  --unregister               Drop the node without touching it.\n"
"\n"
"  --cluster-id=ID            The ID of the cluster in which the node is.\n"
"  --cluster-name=NAME        Name of the cluster to list.\n"
"  --nodes=NODE_LIST          The nodes to list or manipulate.\n"
"\n"
"  --begin=TIMESTAMP          The start of the graph interval.\n"
"  --end=TIMESTAMP            The end of the graph interval.\n"
"  --force                    Force to execute dangerous operations.\n"
"  --graph=NAME               The name of the graph to show.\n"
"  --node-format=FORMAT       The format string used to print nodes.\n"
"  --opt-group=GROUP          The configuration option group.\n"
"  --opt-name=NAME            The name of the configuration option.\n"
"  --opt-value=VALUE          The value of the configuration option.\n"
"  --output-dir=DIR           The directory where the files are created.\n"
"  --properties=ASSIGNMENTS   Names and values of the properties to change.\n"
"\n");
}

void
S9sOptions::printHelpScript()
{
    printHelpGeneric();

    printf(
"Options for the \"script\" command:\n"
"  --execute                  Execute a CJS imparetive program.\n"
"  --run                      Run a CDT entry as a job.\n"
"  --system                   Execute commands on nodes.\n"
"  --tree                     Print the available programs on the controller.\n"
"\n"
"  --cluster-id=ID            The cluster ID.\n"
"\n"
    );
}

void
S9sOptions::printHelpSheet()
{
    printHelpGeneric();

    printf(
"Options for the \"sheet\" command:\n"
"  --create                   Create a new spreadsheet.\n"
"  --edit                     Edit a spreadsheet.\n"
"  --list                     List the spreadsheets on the controller.\n"
"  --stat                     Print the details of a spreadsheet.\n"
"\n"
"  --cluster-id=ID            The cluster ID.\n"
"\n"
    );
}

void
S9sOptions::printHelpServer()
{
    printHelpGeneric();

    printf(
"Options for the \"server\" command:\n"
"  --add-acl                  Adds a new ACL entry to the object.\n"
"  --create                   Creates a new server.\n"
"  --get-acl                  List the ACL of an object.\n"
"  --list-disks               List disks from multiple servers.\n"
"  --list-images              List the supported images.\n"
"  --list                     List the registered servers.\n"
"  --list-memory              List memory modules from multiple servers.\n"
"  --list-nics                List network controllers from multiple servers.\n"
"  --list-partitions          List partitions from multiple servers.\n"
"  --list-processors          List processors from multiple servers.\n"
"  --list-regions             List the regions the server(s) support.\n"
"  --list-subnets             List the supported subnets.\n"
"  --list-templates           List the supported templates.\n"
"  --register                 Register an existint container server.\n"
"  --start                    Boot up a server.\n"
"  --stat                     List details about the server.\n"
"  --stop                     Shut down and power off a server.\n"
"  --unregister               Unregister a container server.\n"
"\n"
"  --acl=ACLSTRING            The ACL entry to set.\n"
"  --os-key-file=PATH         The key file to authenticate on the server.\n"
"  --os-password=PASSWORD     The password to authenticate on the server.\n"
"  --os-user=USERNAME         The username to authenticate on the server.\n"
"  --refresh                  Do not use cached data, collect information.\n"
"  --servers=LIST             List of servers.\n"
"\n"
    );
}

void
S9sOptions::printHelpController()
{
    printHelpGeneric();

    printf(
"Options for the \"controller\" command:\n"
"  --create-snapshot          Creates a controller to controller snapshot.\n"
"  --enable-cmon-ha           Enables the Cmon HA mode.\n"
"  --list                     List the registered controllers.\n"
"  --stat                     Prints details about the controllers.\n"
""
"\n"
    );
}

void
S9sOptions::printHelpTree()
{
    printHelpGeneric();

    printf(
"Options for the \"tree\" command:\n"
"  --access                   Check access rights for a CDT entry.\n"
"  --add-acl                  Adds a new ACL entry to the object.\n"
"  --cat                      Print the content of a CDT file.\n"
"  --chown                    Change the ownership of an object.\n"
"  --delete                   Remove a CDT entry.\n"
"  --get-acl                  List the ACL of an object.\n"
"  --list                     Print the Cmon Directory Tree in list format.\n"
"  --mkdir                    Create a directory in the Cmon Directory Tree.\n"
"  --move                     Move an object inside the tree.\n"
"  --remove-acl               Removes an ACL entry from the object.\n"
"  --rmdir                    Removes a directory in the Cmon Directory Tree.\n"
"  --save                     Save a file in the CDT with content.\n"
"  --touch                    Create a file in the Cmon Directory Tree.\n"
"  --tree                     Print the object tree.\n"
"  --watch                    Opens an interactive UI.\n"
"\n"
"  --acl=ACL                  One ACL entry to be added or removed.\n"
"  --all                      Print also the hidden entries.\n"
"  --owner=USER[:GROUP]       Owner and group of the CDT entry.\n"
"  --recursive                Print/process also the tree sub-entries.\n"
"  --refresh                  Recollect the data.\n"
"\n"
    );
}

void
S9sOptions::printHelpLog()
{
    printHelpGeneric();

    printf(
"Options for the \"log\" command:\n"
"  --list                     List the log messages.\n"
"\n"
"  --from=DATE&TIME           The start of the interval to be printed.\n"
"  --limit=NUMBER             Controls how many jobs are printed max.\n"
"  --log-format=FORMATSTRING  The format of log messages printed.\n"
"  --offset=NUMBER            Controls the index of the first item printed.\n"
"  --until=DATE&TIME          The end of the interval to be printed.\n"
"\n"
    );
}

void
S9sOptions::printHelpEvent()
{
    printHelpGeneric();

    printf(
"Options for the \"event\" command:\n"
"  --list                     List the events as they are detected.\n"
"  --watch                    Open an interactive UI to monitor events.\n"
"\n"
"  --output-file=FILENAME     Save the events into the output file.\n"
"\n"
"  --with-event-alarm         Process alarm events.\n"
"  --with-event-cluster       Process cluster events.\n"
"  --with-event-debug         Process debug events.\n"
"  --with-event-file          Process file events.\n"
"  --with-event-host          Process host events.\n"
"  --with-event-job           Process job events.\n"
"  --with-event-log           Process log events.\n"
"  --with-event-maintenance   Process maintenance events.\n"
"\n"
"  --with-no-name-events      Process event with no event name.\n"
"  --with-created-events      Process events about creation of objects.\n"
"  --with-destroyed-events    Process events about destructing objects.\n"
"  --with-changed-events      Process events about changes in objects.\n"
"  --with-started-events      Process events about things started.\n"
"  --with-ended-events        Process events about things ended.\n"
"  --with-state-changed-events Process events about state changes.\n"
"  --with-user-message-events Process events about user messages.\n"
"  --with-log-message-events  Process events about log messages.\n"
"  --with-measurements-events Process events about measurements.\n"
"\n"
"  --no-no-name-events        Do not process event with no event name.\n"
"  --no-created-events        Do not process events about object creation.\n"
"  --no-destroyed-events      Do not process events destructing objects.\n"
"  --no-changed-events        Do not process events about changes.\n"
"  --no-started-events        Do not process events about things started.\n"
"  --no-ended-events          Do not process events about things ended.\n"
"  --no-state-changed-events  Do not process events about state changes.\n"
"  --no-user-message-events   Do not process events about user messages.\n"
"  --no-log-message-events    Do not process events about log messages.\n"
"  --no-measurements-events   Do not process events about measurements.\n"
"\n"
    );
}

void
S9sOptions::printHelpAlarm()
{
    printHelpGeneric();

    printf(
"Options for the \"alarm\" command:\n"
"  --list                     List the alarms.\n"
"  --delete                   Set the alarm to be ignored.\n"
"\n"
    );
}

/**
 * Reads the command line options in "node" mode.
 */
bool
S9sOptions::readOptionsNode(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0,  4                    },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        { "only-ascii",       no_argument,       0, OptionOnlyAscii       },
        { "density",          no_argument,       0, OptionDensity         },
        { "no-header",        no_argument,       0, OptionNoHeader        },

        // Main Option
        { "change-config",    no_argument,       0, OptionChangeConfig    },
        { "list-config",      no_argument,       0, OptionListConfig      },
        { "list",             no_argument,       0, 'L'                   },
        { "pull-config",      no_argument,       0, OptionPullConfig      },
        { "push-config",      no_argument,       0, OptionPushConfig      },
        { "restart",          no_argument,       0, OptionRestart         },
        { "set",              no_argument,       0, OptionSet             },
        { "start",            no_argument,       0, OptionStart           },
        { "stat",             no_argument,       0, OptionStat            },
        { "stop",             no_argument,       0, OptionStop            },
        { "unregister",       no_argument,       0, OptionUnregister      },

        // Cluster information
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "cluster-name",     required_argument, 0, 'n'                   },
        { "nodes",            required_argument, 0, OptionNodes           },
        { "batch",            no_argument,       0, OptionBatch           },
        
        // Job Related Options
        { "force",            no_argument,       0, OptionForce           },
        { "job-tags",         required_argument, 0, OptionJobTags         },
        { "log",              no_argument,       0, 'G'                   },
        { "recurrence",       required_argument, 0, OptionRecurrence      },
        { "schedule",         required_argument, 0, OptionSchedule        },
        { "timeout",          required_argument, 0, OptionTimeout         },
        { "wait",             no_argument,       0, OptionWait            },

        // Node options. 
        { "properties",       required_argument, 0, OptionProperties      },
        { "opt-group",        required_argument, 0, OptionOptGroup        },
        { "opt-name",         required_argument, 0, OptionOptName         },
        { "opt-value",        required_argument, 0, OptionOptValue        }, 
        { "output-dir",       required_argument, 0, OptionOutputDir       },
        { "node-format",      required_argument, 0, OptionNodeFormat      }, 

        // Graphs...
        { "graph",            required_argument, 0, OptionGraph           }, 
        { "begin",            required_argument, 0, OptionBegin           },
        { "end",              required_argument, 0, OptionEnd             },

        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:V", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;

            case 'c':
                // -c, --controller
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;
            
            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case 'L': 
                // --list
                m_options["list"] = true;
                break;

            case OptionStat:
                // --stat
                m_options["stat"] = true;
                break;

            case OptionSet:
                // --set
                m_options["set"]  = true;
                break;

            case OptionStart:
                // --start
                m_options["start"] = true;
                break;

            case OptionStop:
                // --stop
                m_options["stop"] = true;
                break;

            case OptionRestart:
                // --restart
                m_options["restart"] = true;
                break;

            case OptionListConfig:
                // --list-config
                m_options["list_config"] = true;
                break;

            case OptionChangeConfig:
                // --change-config
                m_options["change_config"] = true;
                break;

            case OptionPullConfig:
                // --pull-config
                m_options["pull_config"] = true;
                break;

            case OptionPushConfig:
                // --push-config
                m_options["push_config"] = true;
                break;
            
            case OptionUnregister:
                // --unregister
                m_options["unregister"] = true;
                break;

            case 4:
                // --config-file=FILE
                m_options["config-file"] = optarg;
                break;
            
            case OptionWait:
                // --wait
                m_options["wait"] = true;
                break;

            case 'G':
                // -G, --log
                m_options["log"] = true;
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;
            
            case OptionOnlyAscii:
                // --only-ascii
                m_options["only_ascii"] = true;
                break;
            
            case OptionDensity:
                // --density
                m_options["density"] = true;
                break;
           
            case OptionSchedule:
                // --schedule=DATETIME
                m_options["schedule"] = optarg;
                break;
            
            case OptionRecurrence:
                // --recurrence=CRONTABSTRING
                m_options["recurrence"] = optarg;
                break;
            
            case OptionJobTags:
                // --job-tags=LIST
                setJobTags(optarg);
                break;

            case OptionTimeout:
                // --timeout=SECONDS
                m_options["timeout"] = optarg;
                break;
            
            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;

            case OptionRpcTls:
                // --rpc-tls
                m_options["rpc_tls"] = true;
                break;
            
            case 'i':
                // -i, --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;
            
            case 'n':
                // -n, --cluster-name=NAME
                m_options["cluster_name"] = optarg;
                break;

            case OptionProperties:
                // --properties=STRING
                setPropertiesOption(optarg);
                break;
            
            case OptionNodes:
                // --nodes=LIST
                if (!setNodes(optarg))
                    return false;
                break;
            
            case OptionOptGroup:
                // --opt-group=NAME
                m_options["opt_group"] = optarg;
                break;

            case OptionOptName:
                // --opt-name=NAME
                m_options["opt_name"] = optarg;
                break;

            case OptionOptValue:
                // --opt-value=VALUE
                m_options["opt_value"] = optarg;
                break;
           
            case OptionOutputDir:
                // --output-dir=DIRECTORY
                m_options["output_dir"] = optarg;
                break;

            case OptionForce:
                // --force
                m_options["force"] = true;
                break;
            
            case OptionNodeFormat:
                // --node-format=VALUE
                m_options["node_format"] = optarg;
                break;

            case OptionGraph:
                // --graph=GRAPH
                m_options["graph"] = optarg;
                break;
            
            case OptionBegin:
                // --begin=DATE
                m_options["begin"] = optarg;
                break;
            
            case OptionEnd:
                // --end=DATE
                m_options["end"] = optarg;
                break;

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }
    
    // 
    // The first extra argument is 'node', so we leave that out. We are
    // interested in the others.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        m_extraArguments << argv[idx];
    }

    return true;
}

/**
 * Reads the command line options in "node" mode.
 */
bool
S9sOptions::readOptionsBackup(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0, OptionConfigFile      },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "controller",       required_argument, 0, 'c'                   },
        { "date-format",      required_argument, 0, OptionDateFormat      },
        { "debug",            no_argument,       0, OptionDebug           },
        { "dry",              no_argument,       0, OptionDry             },
        { "help",             no_argument,       0, OptionHelp            },
        { "human-readable",   no_argument,       0, 'h'                   },
        { "long",             no_argument,       0, 'l'                   },
        { "password",         required_argument, 0, 'p'                   }, 
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "time-style",       required_argument, 0, OptionTimeStyle       },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },

        // Main Option
        { "create",           no_argument,       0, OptionCreate          },
        { "delete",           no_argument,       0, OptionDelete          },
        { "delete-old",       no_argument,       0, OptionDeleteOld       },
        { "list-databases",   no_argument,       0, OptionListDatabases   },
        { "list-files",       no_argument,       0, OptionListFiles       },
        { "list",             no_argument,       0, 'L'                   },
        { "restore-cluster-info", no_argument,   0, OptionRestoreCluster  },
        { "restore-controller", no_argument,     0, OptionRestoreController },
        { "restore",          no_argument,       0, OptionRestore         },
        { "save-cluster-info", no_argument,      0, OptionSaveCluster     },
        { "save-controller",  no_argument,       0, OptionSaveController  },
        { "verify",           no_argument,       0, OptionVerify          },
        
        // Job Related Options
        { "wait",             no_argument,       0, OptionWait            },
        { "log",              no_argument,       0, 'G'                   },
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        { "job-tags",         required_argument, 0, OptionJobTags         },

        // Cluster information
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "cluster-name",     required_argument, 0, 'n'                   },
        { "backup-id",        required_argument, 0, OptionBackupId        },
        { "nodes",            required_argument, 0, OptionNodes           },
        { "schedule",         required_argument, 0, OptionSchedule        },
        { "recurrence",       required_argument, 0, OptionRecurrence      },
        { "timeout",          required_argument, 0, OptionTimeout         },

        // Backup info
        { "backup-datadir",   no_argument,       0, OptionBackupDatadir   },
        { "backup-directory", required_argument, 0, OptionBackupDirectory },
        { "backup-format",    required_argument, 0, OptionBackupFormat    }, 
        { "backup-method",    required_argument, 0, OptionBackupMethod    },
        { "backup-password",  required_argument, 0, OptionBackupPassword  },
        { "backup-retention", required_argument, 0, OptionBackupRetention },
        { "backup-user",      required_argument, 0, OptionBackupUser      },
        { "cloud-retention",  required_argument, 0, OptionCloudRetention  },
        { "databases",        required_argument, 0, OptionDatabases       },
        { "encrypt-backup",   no_argument,       0, OptionBackupEncryption },
        { "full-path",        no_argument,       0, OptionFullPath        },
        { "memory",           required_argument, 0, OptionMemory          },
        { "no-compression",   no_argument,       0, OptionNoCompression   },
        { "on-controller",    no_argument,       0, OptionOnController    },
        { "on-node",          no_argument,       0, OptionOnNode          },
        { "parallellism",     required_argument, 0, OptionParallellism    },
        { "pitr-compatible",  no_argument,       0, OptionPitrCompatible  },
        { "safety-copies",    required_argument, 0, OptionSafetyCopies    },
        { "temp-dir-path",    required_argument, 0, OptionTempDirPath     },
        { "keep-temp-dir",    no_argument,       0, OptionKeepTempDir     },
        { "subdirectory",     required_argument, 0, OptionSubDirectory    },
        { "test-server",      required_argument, 0, OptionTestServer      },
        { "title",            required_argument, 0, OptionTitle           },
        { "to-individual-files", no_argument,    0, OptionIndividualFiles },
        { "use-pigz",         no_argument,       0, OptionUsePigz         },

        // For save cluster and restore cluster...
        { "output-file",      required_argument, 0, OptionOutputFile      },
        { "input-file",       required_argument, 0, OptionInputFile       },
        
        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:V", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;
            
            case OptionDry:
                // --dry
                m_options["dry"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;

            case 'c':
                // -c, --controller
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;
            
            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case 'L': 
                // --list
                m_options["list"] = true;
                break;

            case OptionListFiles:
                // --list-files
                m_options["list_files"] = true;
                break;
            
            case OptionListDatabases:
                // --list-databases
                m_options["list_databases"] = true;
                break;
            
            case OptionWait:
                // --wait
                m_options["wait"] = true;
                break;

            case OptionJobTags:
                // --job-tags=LIST
                setJobTags(optarg);
                break;

            case 'G':
                // -G, --log
                m_options["log"] = true;
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;
            
            case OptionCreate:
                // --create
                m_options["create"] = true;
                break;
 
            case OptionRestoreCluster:
                // --restore-cluster-info
                m_options["restore_cluster"] = true;
                break;
            
            case OptionRestoreController:
                // --restore-controller
                m_options["restore_controller"] = true;
                break;

            case OptionRestore:
                // --restore
                m_options["restore"] = true;
                break;
            
            case OptionSaveCluster:
                // --save-cluster-info
                m_options["save_cluster"] = true;
                break;
            
            case OptionSaveController:
                // --save-controller
                m_options["save_controller"] = true;
                break;
            
            case OptionVerify:
                // --verify
                m_options["verify"] = true;
                break;
            
            case OptionDelete:
                // --delete
                m_options["delete"] = true;
                break;
            
            case OptionDeleteOld:
                // --delete-old
                m_options["delete_old"] = true;
                break;

            case OptionConfigFile:
                // --config-file=FILE
                m_options["config-file"] = optarg;
                break;
            
            case OptionDateFormat:
                // --date-format=FORMAT
                m_options["date_format"] = optarg;
                break;
            
            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case 'h':
                // -h, --human-readable
                m_options["human_readable"] = true;
                break;

            case OptionTimeStyle:
                m_options["time_style"] = optarg;
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;

            case OptionRpcTls:
                // --rpc-tls
                m_options["rpc_tls"] = true;
                break;
            
            case 'i':
                // -i, --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;
            
            case 'n':
                // -n, --cluster-name=NAME
                m_options["cluster_name"] = optarg;
                break;
 
            case OptionNodes:
                // --nodes=LIST
                if (!setNodes(optarg))
                    return false;
                break;

            case OptionSchedule:
                // --schedule=DATETIME
                m_options["schedule"] = optarg;
                break;
            
            case OptionRecurrence:
                // --recurrence=CRONTABSTRING
                m_options["recurrence"] = optarg;
                break;
            
            case OptionTimeout:
                // --timeout=SECONDS
                m_options["timeout"] = optarg;
                break;

            case OptionBackupId:
                // --backup-id=BACKUPID
                m_options["backup_id"] = atoi(optarg);
                break;
           
            case OptionBackupDatadir:
                // --backup-datadir
                m_options["backup_datadir"] = true;
                break;

            case OptionBackupMethod:
                // --backup-method=METHOD
                m_options["backup_method"] = optarg;
                break;

            case OptionBackupDirectory:
                // --backup-directory=DIRECTORY
                m_options["backup_directory"] = optarg;
                break;

            case OptionBackupRetention:
                // --backup-retention=DAYS
                setBackupRetention(optarg);
                break;

            case OptionCloudRetention:
                // --cloud-retention=DAYS
                setCloudRetention(optarg);
                break;

            case OptionSafetyCopies:
                // --safety-copies=N
                setSafetyCopies(optarg);
                break;

            case OptionBackupUser:
                // --backup-user=USERNAME
                m_options["backup_user"] = optarg;
                break;

            case OptionBackupPassword:
                // --backup-password=PASSWD
                m_options["backup_password"] = optarg;
                break;

            case OptionBackupEncryption:
                // --encrypt-backup
                m_options["encrypt_backup"] = true;
                break;
                
            case OptionNoCompression:
                // --no-compression
                m_options["no_compression"] = true;
                break;

            case OptionUsePigz:
                // --use-pigz
                m_options["use_pigz"] = true;
                break;

            case OptionOnNode:
                // --on-node
                m_options["on_node"] = true;
                break;

            case OptionOnController:
                // --on-controller
                m_options["on_controller"] = true;
                break;

            case OptionDatabases:
                // --databases=LIST
                m_options["databases"] = optarg;
                break;

            case OptionParallellism:
                // --parallellism=N
                if (!setParallellism(optarg))
                    return false;

                break;

            case OptionPitrCompatible:
                // --pitr-compatible
                m_options["pitr_compatible"] = true;
                break;

            case OptionKeepTempDir:
                // --keep-temp-dir
                m_options["keep_temp_dir"] = true;
                break;

            case OptionTempDirPath:
                // --temp-dir-path
                m_options["temp_dir_path"] = optarg;
                break;

            case OptionSubDirectory:
                // --subdirectory=PATTERN
                m_options["subdirectory"] = optarg;
                break;

            case OptionFullPath:
                // --full-path
                m_options["full_path"] = true;
                break;

            case OptionMemory:
                // --memory=1024
                m_options["memory"] = optarg;
                break;

            case OptionBackupFormat:
                // --backup-format=VALUE
                m_options["backup_format"] = optarg;
                break;
            
            case OptionTitle:
                // --title=TITLE
                m_options["title"] = optarg;
                break;

            case OptionIndividualFiles:
                // --to-individual-files
                m_options["to_individual_files"] = true;
                break;

            case OptionTestServer:
                // --test-server=HOSTNAME
                m_options["test_server"] = optarg;
                break;
            
            case OptionOutputFile:
                // --output-file=FILE
                m_options["output_file"] = optarg;
                break;
            
            case OptionInputFile:
                // --input-file=FILE
                m_options["input_file"] = optarg;
                break;

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }
    
    // 
    // The first extra argument is 'backup', so we leave that out.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        //S9S_WARNING("argv[%3d] = %s", idx, argv[idx]);
        m_extraArguments << argv[idx];
    }

    return true;

}

/**
 * \returns True if the command line options seem to be ok.
 */
bool
S9sOptions::checkOptionsLog()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isListRequested())
        countOptions++;
    
    if (isCreateRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = 
            "The --list and --create "
            "options are mutually exclusive.";

        m_exitStatus = BadOptions;

        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = 
            "One of the --list and --create options is mandatory.";

        m_exitStatus = BadOptions;

        return false;
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok.
 */
bool
S9sOptions::checkOptionsEvent()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isListRequested())
        countOptions++;
    
    if (isWatchRequested())
        countOptions++;
    
    if (countOptions > 1)
    {
        m_errorMessage = 
            "The --list and --create "
            "options are mutually exclusive.";

        m_exitStatus = BadOptions;

        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = 
            "One of the --list and --create options is mandatory.";

        m_exitStatus = BadOptions;

        return false;
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok.
 */
bool
S9sOptions::checkOptionsAlarm()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isListRequested())
        countOptions++;
    
    if (isDeleteRequested())
        countOptions++;
    
    if (countOptions > 1)
    {
        m_errorMessage = "The main options are mutually exclusive.";
        m_exitStatus = BadOptions;
        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = "One of the main options is mandatory.";
        m_exitStatus = BadOptions;
        return false;
    }

    return true;
}

/**
 * Reads the command line options in "log" mode.
 */
bool
S9sOptions::readOptionsLog(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0,  4                    },
        { "no-header",        no_argument,       0, OptionNoHeader        },

        // Main Option
        { "list",             no_argument,       0, 'L'                   },

        // Cluster information
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "cluster-name",     required_argument, 0, 'n'                   },
        { "nodes",            required_argument, 0, OptionNodes           },
        
        // Job Related Options
        { "wait",             no_argument,       0, OptionWait            },
        { "log",              no_argument,       0, 'G'                   },
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },

        // Log Options 
        { "from",             required_argument, 0, OptionFrom            },
        { "until",            required_argument, 0, OptionUntil           },
        { "limit",            required_argument, 0, OptionLimit           },
        { "offset",           required_argument, 0, OptionOffset          },
        { "log-format",       required_argument, 0, OptionLogFormat       },

        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:V", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;

            case 'c':
                // -c, --controller
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;
            
            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case 'L': 
                // --list
                m_options["list"] = true;
                break;

            case 4:
                // --config-file=FILE
                m_options["config-file"] = optarg;
                break;
            
            case OptionWait:
                // --wait
                m_options["wait"] = true;
                break;

            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;
           
            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;

            case OptionRpcTls:
                // --rpc-tls
                m_options["rpc_tls"] = true;
                break;
            
            case 'i':
                // -i, --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;
            
            case 'n':
                // -n, --cluster-name=NAME
                m_options["cluster_name"] = optarg;
                break;

            case OptionFrom:
                // --from=DATE&TIME
                m_options["from"] = optarg;
                break;
            
            case OptionUntil:
                // --until=DATE&TIME
                m_options["until"] = optarg;
                break;
            
            case OptionLimit:
                // --limit=NUMBER
                m_options["limit"] = optarg;
                break;
            
            case OptionOffset:
                // --offset=NUMBER
                m_options["offset"] = optarg;
                break;
            
            case OptionLogFormat:
                // --log-format=FORMAT
                m_options["log_format"] = optarg;
                break;
            
            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }
    
    // 
    // The first extra argument is 'node', so we leave that out. We are
    // interested in the others.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        m_extraArguments << argv[idx];
    }

    return true;
}

/**
 * Reads the command line options in "event" mode.
 */
bool
S9sOptions::readOptionsEvent(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0,  4                    },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        { "human-readable",   no_argument,       0, 'h'                   },

        // Main Option
        { "list",             no_argument,       0, 'L'                   },
        { "watch",            no_argument,       0, OptionWatch           },

        // Cluster information
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "cluster-name",     required_argument, 0, 'n'                   },
        { "nodes",            required_argument, 0, OptionNodes           },
        { "output-file",      required_argument, 0, OptionOutputFile      },
        { "input-file",       required_argument, 0, OptionInputFile       },
        
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },

        { "with-event-cluster",  no_argument,    0, OptionEventCluster     },
        { "with-event-job",   no_argument,       0, OptionEventJob         },
        { "with-event-host",  no_argument,       0, OptionEventHost        },
        { "with-event-maintenance",  no_argument,0, OptionEventMaintenance },
        { "with-event-alarm", no_argument,       0, OptionEventAlarm       },
        { "with-event-file",  no_argument,       0, OptionEventFile        },
        { "with-event-debug", no_argument,       0, OptionEventDebug       },
        { "with-event-log",   no_argument,       0, OptionEventLog         },
        
        { "with-no-name-events", no_argument,     0, OptionWithNoName       },
        { "with-created-events", no_argument,    0, OptionWithCreated      },
        { "with-destroyed-events", no_argument,  0, OptionWithDestroyed    },
        { "with-changed-events", no_argument,    0, OptionWithChanged      },
        { "with-started-events", no_argument,    0, OptionWithStarted      },
        { "with-ended-events", no_argument,      0, OptionWithEnded        },
        { "with-state-changed-events", no_argument, 0, OptionWithStateChanged },
        { "with-user-message-events", no_argument, 0, OptionWithUserMessage },
        { "with-log-message-events", no_argument, 0, OptionWithLogMessage   },
        { "with-measurements-events", no_argument, 0, OptionWithMeasurements },
        
        { "no-no-name-events", no_argument,      0, OptionNoNoName          },
        { "no-created-events", no_argument,      0, OptionNoCreated         },
        { "no-destroyed-events", no_argument,    0, OptionNoDestroyed       },
        { "no-changed-events", no_argument,      0, OptionNoChanged         },
        { "no-started-events", no_argument,      0, OptionNoStarted         },
        { "no-ended-events", no_argument,        0, OptionNoEnded           },
        { "no-state-changed-events", no_argument, 0, OptionNoStateChanged   },
        { "no-user-message-events", no_argument, 0, OptionNoUserMessage     },
        { "no-log-message-events", no_argument,  0, OptionNoLogMessage      },
        { "no-measurements-events", no_argument, 0, OptionNoMeasurements    },

        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:V", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;

            case 'c':
                // -c, --controller
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;
            
            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case 'L': 
                // --list
                m_options["list"] = true;
                break;
            
            case OptionWatch:
                // --watch
                m_options["watch"] = true;
                break;

            case 4:
                // --config-file=FILE
                m_options["config-file"] = optarg;
                break;
            
            case OptionOutputFile:
                // --output-file=FILE
                m_options["output_file"] = optarg;
                break;
            
            case OptionInputFile:
                // --input-file=FILE
                m_options["input_file"] = optarg;
                break;

            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;
           
            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;
            
            case 'h':
                // -h, --human-readable
                m_options["human_readable"] = true;
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;

            case OptionRpcTls:
                // --rpc-tls
                m_options["rpc_tls"] = true;
                break;
            
            case 'i':
                // -i, --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;
            
            case 'n':
                // -n, --cluster-name=NAME
                m_options["cluster_name"] = optarg;
                break;
            
            case OptionEventCluster:
                // --with-event-cluster
                enableEventType("EventCluster");
                break;
            
            case OptionEventJob:
                // --with-event-job
                enableEventType("EventJob");
                break;

            case OptionEventHost:
                // --with-event-host
                enableEventType("EventHost");
                break;
            
            case OptionEventMaintenance:
                // --with-event-maintenance
                enableEventType("EventMaintenance");
                break;
            
            case OptionEventAlarm:
                // --with-event-alarm
                enableEventType("EventAlarm");
                break;
            
            case OptionEventFile:
                // --with-event-file
                enableEventType("EventFile");
                break;
            
            case OptionEventDebug:
                // --with-event-debug
                enableEventType("EventDebug");
                break;
            
            case OptionEventLog:
                // --with-event-log
                enableEventType("EventLog");
                break;
    
            case OptionWithNoName:
                // --with-no-name-events
                enableEventName("NoSubClass");
                break;

            case OptionWithCreated:
                // --with-created-events
                enableEventName("Created");
                break;
                
            case OptionWithDestroyed:
                // --with-destroyed-events
                enableEventName("Destroyed");
                break;

            case OptionWithChanged:
                // --with-changed-events
                enableEventName("Changed");
                break;

            case OptionWithStarted:
                // --with-started-events
                enableEventName("Started");
                break;

            case OptionWithEnded:
                // --with-ended-events
                enableEventName("Ended");
                break;

            case OptionWithStateChanged:
                // --with-state-changed-events
                enableEventName("StateChanged");
                break;

            case OptionWithUserMessage:
                // --with-user-message-events
                enableEventName("UserMessage");
                break;

            case OptionWithLogMessage:
                // --with-log-message-events
                enableEventName("LogMessage");
                break;

            case OptionWithMeasurements:
                // --with-measurements-events
                enableEventName("Measurements");
                break;
            
            case OptionNoNoName:
                // --no-no-name-events
                disableEventName("NoSubClass");
                break;

            case OptionNoCreated:
                // --no-created-events
                disableEventName("Created");
                break;
                
            case OptionNoDestroyed:
                // --no-destroyed-events
                disableEventName("Destroyed");
                break;

            case OptionNoChanged:
                // --no-changed-events
                disableEventName("Changed");
                break;

            case OptionNoStarted:
                // --no-started-events
                disableEventName("Started");
                break;

            case OptionNoEnded:
                // --no-ended-events
                disableEventName("Ended");
                break;

            case OptionNoStateChanged:
                // --no-state-changed-events
                disableEventName("StateChanged");
                break;

            case OptionNoUserMessage:
                // --no-user-message-events
                disableEventName("UserMessage");
                break;

            case OptionNoLogMessage:
                // --no-log-message-events
                disableEventName("LogMessage");
                break;

            case OptionNoMeasurements:
                // --no-measurements-events
                disableEventName("Measurements");
                break;

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }
    
    // 
    // The first extra argument is 'node', so we leave that out. We are
    // interested in the others.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        m_extraArguments << argv[idx];
    }

    return true;
}

/**
 * Reads the command line options in "alarm" mode.
 */
bool
S9sOptions::readOptionsAlarm(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0,  4                    },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        { "human-readable",   no_argument,       0, 'h'                   },

        // Main Option
        { "list",             no_argument,       0, 'L'                   },
        { "delete",           no_argument,       0, OptionDelete          },
        
        /*
         * Alarm related options.
         */
        { "alarm-id",         required_argument, 0, OptionAlarmId         },

        // Cluster information
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "cluster-name",     required_argument, 0, 'n'                   },
        { "nodes",            required_argument, 0, OptionNodes           },
        { "output-file",      required_argument, 0, OptionOutputFile      },
        { "input-file",       required_argument, 0, OptionInputFile       },
        
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },

        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:V", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;

            case 'c':
                // -c, --controller
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;
            
            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case 4:
                // --config-file=FILE
                m_options["config-file"] = optarg;
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;
           
            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;
            
            case 'h':
                // -h, --human-readable
                m_options["human_readable"] = true;
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;

            case OptionRpcTls:
                // --rpc-tls
                m_options["rpc_tls"] = true;
                break;
            
            case 'i':
                // -i, --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;
            
            case 'n':
                // -n, --cluster-name=NAME
                m_options["cluster_name"] = optarg;
                break;

            /*
             * Main options.
             */
            case 'L': 
                // --list
                m_options["list"] = true;
                break;
            
            case OptionDelete:
                // --delete
                m_options["delete"] = true;
                break;
           
            /*
             * Options related to alarms.
             */
            case OptionAlarmId:
                // --alarm-id=ID
                m_options["alarm_id"] = atoi(optarg);
                break;

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }
    
    // 
    // The first extra argument is 'node', so we leave that out. We are
    // interested in the others.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        m_extraArguments << argv[idx];
    }

    return true;
}

bool
S9sOptions::checkOptionsBackup()
{
    int countOptions = 0;
    
    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isListRequested())
        countOptions++;
    
    if (isListDatabasesRequested())
        countOptions++;
    
    if (isListFilesRequested())
        countOptions++;

    if (isCreateRequested())
        countOptions++;

    if (isRestoreRequested())
        countOptions++;
    
    if (isVerifyRequested())
        countOptions++;
    
    if (isDeleteOldRequested())
        countOptions++;
    
    if (isDeleteRequested())
        countOptions++;
    
    if (isSaveClusterRequested())
        countOptions++;
    
    if (isRestoreClusterRequested())
        countOptions++;
    
    if (isSaveControllerRequested())
        countOptions++;
    
    if (isRestoreControllerRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = "The main options are mutually exclusive.";
        m_exitStatus = BadOptions;
        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = "One of the main options is mandatory.";
        m_exitStatus = BadOptions;
        return false;
    }

    /*
     * Using the --databases is missleading when not creating new backup: the
     * user might think it is possible to restore one database of an archive.
     */
    if (!databases().empty())
    {
        if (isListRequested() && isRestoreRequested())
        {
            m_errorMessage = 
                "The --databases option can only be used while creating "
                "backups.";
        
            m_exitStatus = BadOptions;
            return false;
        }
    }

    /*
     * The --memory= should has an integer argument other than 0.
     */
    if (hasMemory())
    {
        if (memory().toInt() <= 0)
        {
            m_errorMessage = 
                "The argument for the --memory option should be "
                "a positive integer.";

            m_exitStatus = BadOptions;
            return false;
        }
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok.
 */
bool
S9sOptions::checkOptionsJob()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isListRequested())
        countOptions++;
    
    if (isKillRequested())
        countOptions++;
    
    if (isFailRequested())
    {
        countOptions++;
    } else if (isSuccessRequested())
    {
        countOptions++;
    } else if (isCloneRequested())
    {
        if (!hasJobId())
        {
            PRINT_ERROR("The --clone option requires the --job-id=ID option.");
            return false;
        }

        countOptions++;
    } else { 
        if (isLogRequested())
            countOptions++;

        if (isWaitRequested())
            countOptions++;
    }

    if (isDeleteRequested())
    {
        if (!hasJobId())
        {
            PRINT_ERROR("The --delete option requires the --job-id=ID option.");
            return false;
        }

        countOptions++;
    }

    if (countOptions > 1)
    {
        m_errorMessage = "The main options are mutually exclusive.";
        m_exitStatus = BadOptions;
        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = "One of the main options is mandatory.";
        m_exitStatus = BadOptions;
        return false;
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok for "cluster" mode.
 */
bool
S9sOptions::checkOptionsCluster()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isListRequested())
        countOptions++;
    
    if (isStatRequested())
        countOptions++;

    if (isCreateRequested())
        countOptions++;

    if (isPingRequested())
        countOptions++;
    
    if (isPromoteSlaveRequested())
        countOptions++;
    
    if (isDemoteNodeRequested())
        countOptions++;

    if (isRollingRestartRequested())
        countOptions++;
    
    if (isCollectLogsRequested())
        countOptions++;
    
    if (isImportConfigRequested())
        countOptions++;
    
    if (isEnableSslRequested())
        countOptions++;
    
    if (isDisableSslRequested())
        countOptions++;
    
    if (isSetupAuditLoggingRequested())
        countOptions++;
    
    if (isCreateReportRequested())
        countOptions++;

    if (isDeployAgentsRequested())
        countOptions++;

    if (isAddNodeRequested())
        countOptions++;

    if (isRemoveNodeRequested())
        countOptions++;

    if (isDropRequested())
        countOptions++;

    if (isStopRequested())
        countOptions++;

    if (isStartRequested())
        countOptions++;

    if (isCreateAccountRequested())
        countOptions++;
    
    if (isGrantRequested())
        countOptions++;

    if (isCheckHostsRequested())
        countOptions++;

    if (isDeleteAccountRequested())
        countOptions++;

    if (isCreateDatabaseRequested())
        countOptions++;
    
    if (isListDatabasesRequested())
        countOptions++;
    
    if (isRegisterRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = "The main options are mutually exclusive.";
        m_exitStatus = BadOptions;
        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = "One of the main options is mandatory.";
        m_exitStatus = BadOptions;
        return false;
    }

    /*
     * Using the --databases is missleading when not creating new backup: the
     * user might think it is possible to restore one database of an archive.
     */
    if (!databases().empty())
    {
        if (isListRequested() && isRestoreRequested())
        {
            m_errorMessage = 
                "The --databases option can only be used while creating "
                "backups.";
        
            m_exitStatus = BadOptions;
            return false;
        }
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok for "cluster" mode.
 */
bool
S9sOptions::checkOptionsContainer()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isListRequested())
        countOptions++;
    
    if (isStatRequested())
        countOptions++;

    if (isCreateRequested())
        countOptions++;

    if (isDeleteRequested())
        countOptions++;

    if (isStopRequested())
        countOptions++;

    if (isStartRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = "The main options are mutually exclusive.";
        m_exitStatus = BadOptions;
        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = "One of the main options is mandatory.";
        m_exitStatus = BadOptions;
        return false;
    }

    /*
     * Using the --databases is missleading when not creating new backup: the
     * user might think it is possible to restore one database of an archive.
     */
    if (!databases().empty())
    {
        if (isListRequested() && isRestoreRequested())
        {
            m_errorMessage = 
                "The --databases option can only be used while creating "
                "backups.";
        
            m_exitStatus = BadOptions;
            return false;
        }
    }

    return true;
}
/**
 * \returns True if the command line options seem to be ok.
 */
bool
S9sOptions::checkOptionsNode()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isListRequested())
        countOptions++;
    
    if (isPullConfigRequested())
        countOptions++;
    
    if (isPushConfigRequested())
        countOptions++;
    
    if (isSetRequested())
        countOptions++;
    
    if (isStatRequested())
        countOptions++;
    
    if (isWatchRequested())
        countOptions++;

    if (isListConfigRequested())
        countOptions++;
    
    if (isChangeConfigRequested())
        countOptions++;
    
    if (isStartRequested())
        countOptions++;
    
    if (isStopRequested())
        countOptions++;
    
    if (isRestartRequested())
        countOptions++;
    
    if (isUnregisterRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = "Main command line options are mutually exclusive.";
        m_exitStatus = BadOptions;

        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = "One main option is required.";
        m_exitStatus = BadOptions;

        return false;
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok.
 */
bool
S9sOptions::checkOptionsMaintenance()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isListRequested())
        countOptions++;
    
    if (isCreateRequested())
        countOptions++;
    
    if (isDeleteRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = 
            "The --list, --create and --delete options are mutually"
            " exclusive.";

        m_exitStatus = BadOptions;

        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = 
            "One of the --list, --create and --delete options is mandatory.";

        m_exitStatus = BadOptions;

        return false;
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok.
 */
bool
S9sOptions::checkOptionsUser()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isListRequested())
        countOptions++;
    
    if (isListGroupsRequested())
        countOptions++;
    
    if (isCreateRequested())
        countOptions++;
    
    if (isDeleteRequested())
        countOptions++;
    
    if (isSetRequested())
        countOptions++;
    
    if (isChangePasswordRequested())
        countOptions++;
    
    if (isWhoAmIRequested())
        countOptions++;

    if (isListKeysRequested())
        countOptions++;
    
    if (isAddKeyRequested())
        countOptions++;
    
    if (isStatRequested())
        countOptions++;
    
    if (isEnableRequested())
        countOptions++;
    
    if (isSetGroupRequested())
        countOptions++;
    
    if (isAddToGroupRequested())
        countOptions++;
    
    if (isRemoveFromGroupRequested())
        countOptions++;

    if (isDisableRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = "The main options are mutually exclusive.";
        m_exitStatus = BadOptions;
        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = "One of the main options is mandatory.";
        m_exitStatus = BadOptions;
        return false;
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok.
 */
bool
S9sOptions::checkOptionsAccount()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isListRequested())
        countOptions++;
    
    if (isListGroupsRequested())
        countOptions++;
    
    if (isCreateRequested())
        countOptions++;
    
    if (isGrantRequested())
        countOptions++;
    
    if (isDeleteRequested())
        countOptions++;
    
    if (isSetRequested())
        countOptions++;
    
    if (isChangePasswordRequested())
        countOptions++;
    
    if (isWhoAmIRequested())
        countOptions++;

    if (isListKeysRequested())
        countOptions++;
    
    if (isAddKeyRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = "The main options are mutually exclusive.";
        m_exitStatus = BadOptions;

        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = "One of the main options is mandatory.";
        m_exitStatus = BadOptions;

        return false;
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok.
 */
bool
S9sOptions::checkOptionsProcess()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isListRequested())
        countOptions++;
    
    if (isTopRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = 
            "The --list and --top options are mutually"
            " exclusive.";

        m_exitStatus = BadOptions;

        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = 
            "One of the --list and --top options is mandatory.";

        m_exitStatus = BadOptions;

        return false;
    }

    return true;
}

/**
 * Reads the command line options in the "process" mode.
 */
bool
S9sOptions::readOptionsProcess(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0,  OptionPrintJson      },
        { "color",            optional_argument, 0,  OptionColor          },
        { "config-file",      required_argument, 0,  OptionConfigFile     },
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        { "limit",            required_argument, 0, OptionLimit           },
        { "human-readable",   no_argument,       0, 'h'                   },
        { "sort-by-memory",   no_argument,       0, OptionSortByMemory    },

        // Main Option
        { "list",             no_argument,       0, 'L'                   },
        { "top",              no_argument,       0,  OptionTop            },

        // Cluster information
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "update-freq",      required_argument, 0,  OptionUpdateFreq     },

        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:V", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;

            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;

            case 'c':
                // -c, --controller=URL
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;
            
            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case 'L': 
                // --list
                m_options["list"] = true;
                break;
            
            case 'h':
                // -h, --human-readable
                m_options["human_readable"] = true;
                break;

            case OptionSortByMemory:
                // --sort-by-memory
                m_options["sort_by_memory"] = true;
                break;

            case OptionConfigFile:
                // --config-file=CONFIG
                m_options["config-file"] = optarg;
                break;
            
            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;
            
            case OptionTop:
                // --top
                m_options["top"] = true;
                break;

            case OptionRpcTls:
                // --rpc-tls
                m_options["rpc_tls"] = true;
                break;

            case 'i':
                // -i, --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;

            case OptionUpdateFreq:
                // --update-freq
                m_options["update_freq"] = atoi(optarg);
                if (m_options["update_freq"].toInt() < 1)
                {
                    m_errorMessage = 
                        "Invalid value for the --update-freq option.";
                
                    m_exitStatus = BadOptions;
                    return false;
                }
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;

            case OptionLimit:
                // --limit=NUMBER
                m_options["limit"] = optarg;
                break;

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }
    
    // 
    // The first extra argument is 'process', so we leave that out.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        //S9S_WARNING("argv[%3d] = %s", idx, argv[idx]);
        m_extraArguments << argv[idx];
    }

    return true;
}

/**
 * Reads the command line options in the "user" mode.
 */
bool
S9sOptions::readOptionsUser(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0, OptionConfigFile      },
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        { "cmon-user",        required_argument, 0, 'u'                   }, 

        // Main Options
        { "add-key",          no_argument,       0, OptionAddKey          },
        { "add-to-group",     no_argument,       0, OptionAddToGroup      },
        { "change-password",  no_argument,       0, OptionChangePassword  },
        { "create",           no_argument,       0, OptionCreate          },
        { "delete",           no_argument,       0, OptionDelete          },
        { "disable",          no_argument,       0, OptionDisable         },
        { "enable",           no_argument,       0, OptionEnable          },
        { "list-groups",      no_argument,       0, OptionListGroups      },
        { "list-keys",        no_argument,       0, OptionListKeys        },
        { "list",             no_argument,       0, 'L'                   },
        { "remove-from-group", no_argument,      0, OptionRemoveFromGroup },
        { "set-group",        no_argument,       0, OptionSetGroup        },
        { "set",              no_argument,       0, OptionSet             },
        { "stat",             no_argument,       0, OptionStat            },
        { "whoami",           no_argument,       0, OptionWhoAmI          },
       
        // Options about the user.
        { "create-group",     no_argument,       0, OptionCreateGroup     },
        { "email-address",    required_argument, 0, OptionEmailAddress    },
        { "first-name",       required_argument, 0, OptionFirstName       },
        { "generate-key",     no_argument,       0, 'g'                   }, 
        { "group",            required_argument, 0, OptionGroup           },
        { "last-name",        required_argument, 0, OptionLastName        },
        { "new-password",     required_argument, 0, OptionNewPassword     }, 
        { "old-password",     required_argument, 0, OptionOldPassword     }, 
        { "public-key-file",  required_argument, 0, OptionPublicKeyFile   }, 
        { "public-key-name",  required_argument, 0, OptionPublicKeyName   }, 
        { "title",            required_argument, 0, OptionTitle           },
        { "user-format",      required_argument, 0, OptionUserFormat      }, 

        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:Vgu:", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;

            case 'c':
                // -c, --controller=URL
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;
            
            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case OptionConfigFile:
                // --config-file=CONFIG
                m_options["config-file"] = optarg;
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;
            
            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;
            
            case OptionRpcTls:
                // --rpc-tls
                m_options["rpc_tls"] = true;
                break;

            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;

            case 'g':
                // --generate-key
                m_options["generate_key"] = true;
                break;

            case OptionCreate:
                // --create
                m_options["create"] = true;
                break;
            
            case OptionDelete:
                // --delete
                m_options["delete"] = true;
                break;
 
            case 'L': 
                // --list
                m_options["list"] = true;
                break;

            case OptionListGroups:
                // --list-groups
                m_options["list-groups"] = true;
                break;

            case OptionListKeys:
                // --list-keys
                m_options["list_keys"] = true;
                break;
            
            case OptionAddKey:
                // --add-keys
                m_options["add_key"] = true;
                break;
            
            case OptionAddToGroup:
                // --add-to-group
                m_options["add_to_group"] = true;
                break;

            case OptionWhoAmI:
                // --whoami
                m_options["whoami"] = true;
                break;
            
            case OptionStat:
                // --stat
                m_options["stat"] = true;
                break;
            
            case OptionEnable:
                // --enable
                m_options["enable"] = true;
                break;
            
            case OptionDisable:
                // --disable
                m_options["disable"] = true;
                break;

            case OptionSet:
                // --set
                m_options["set"]  = true;
                break;
            
            case OptionSetGroup:
                // --set-group
                m_options["set_group"]  = true;
                break;
           
            case OptionChangePassword:
                // --change-password
                m_options["change_password"] = true;
                break;
            
            case OptionRemoveFromGroup:
                // --remove-from-group
                m_options["remove_from_group"] = true;
                break;

            /*
             * Other options.
             */
            case OptionGroup:
                // --group=GROUPNAME
                m_options["group"] = optarg;
                break;
            
            case OptionCreateGroup:
                // --create-group
                m_options["create_group"] = true;
                break;
            
            case OptionFirstName:
                // --first-name=FIRSTNAME
                m_options["first_name"] = optarg;
                break;
            
            case OptionLastName:
                // --last-name=FIRSTNAME
                m_options["last_name"] = optarg;
                break;
            
            case OptionTitle:
                // --title=TITLE
                m_options["title"] = optarg;
                break;
            
            case OptionEmailAddress:
                // --email-address=ADDRESSS
                m_options["email_address"] = optarg;
                break;
            
            case OptionUserFormat:
                // --user-format=VALUE
                m_options["user_format"] = optarg;
                break;
           
            case OptionOldPassword:
                // --old-password=PASSWORD
                m_options["old_password"] = optarg;
                break;
            
            case OptionNewPassword:
                // --new-password=PASSWORD
                m_options["new_password"] = optarg;
                break;

            case OptionPublicKeyFile:
                // --public-key-file=FILE
                m_options["public_key_file"] = optarg;
                break;
            
            case OptionPublicKeyName:
                // --public-key-name=FILE
                m_options["public_key_name"] = optarg;
                break;
            
            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }

    // 
    // The first extra argument is 'user', so we leave that out.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        //S9S_WARNING("argv[%3d] = %s", idx, argv[idx]);
        m_extraArguments << argv[idx];
    }

    return true;
}

/**
 * Reads the command line options in the "user" mode.
 */
bool
S9sOptions::readOptionsAccount(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0, OptionConfigFile      },
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        { "cmon-user",        required_argument, 0, 'u'                   }, 

        // Main Option
        { "create",           no_argument,       0, OptionCreate          },
        { "list",             no_argument,       0, 'L'                   },
        { "delete",           no_argument,       0, OptionDelete          },
        { "grant",            no_argument,       0, OptionGrant           },
        
        // Cluster information
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "cluster-name",     required_argument, 0, 'n'                   },
       
        // Options about the user.
//        { "user-format",      required_argument, 0, OptionUserFormat      }, 
//        { "old-password",     required_argument, 0, OptionOldPassword     }, 
//        { "new-password",     required_argument, 0, OptionNewPassword     }, 
//        { "public-key-file",  required_argument, 0, OptionPublicKeyFile   }, 
//        { "public-key-name",  required_argument, 0, OptionPublicKeyName   }, 
        
        { "with-database",    no_argument,       0, OptionWithDatabase    },
        { "db-name",          required_argument, 0, OptionDbName          },
        { "privileges",       required_argument, 0, OptionPrivileges      },
        { "account",          required_argument, 0, OptionAccount,        },
        { "limit",            required_argument, 0, OptionLimit          },
//        { "offset",           required_argument, 0, OptionOffset         },

        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:Vgu:", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;

            case 'c':
                // -c, --controller=URL
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;
            
            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case OptionConfigFile:
                // --config-file=CONFIG
                m_options["config-file"] = optarg;
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;
            
            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;
            
            case OptionRpcTls:
                // --rpc-tls
                m_options["rpc_tls"] = true;
                break;

            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;
            
            case 'i':
                // -i, --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;
            
            case 'n':
                // -n, --cluster-name=NAME
                m_options["cluster_name"] = optarg;
                break;

            case OptionCreate:
                // --create
                m_options["create"] = true;
                break;
 
            case 'L': 
                // --list
                m_options["list"] = true;
                break;

            case OptionListKeys:
                // --list-keys
                m_options["list_keys"] = true;
                break;
            
            case OptionAddKey:
                // --list-keys
                m_options["add_key"] = true;
                break;
            
            case OptionGrant:
                // --grant
                m_options["grant"] = true;
                break;

            case OptionDelete:
                // --delete
                m_options["delete"] = true;
                break;

            case OptionSet:
                // --set
                m_options["set"]  = true;
                break;
           
            case OptionChangePassword:
                // --change-password
                m_options["change_password"] = true;
                break;

            case OptionUserFormat:
                // --user-format=VALUE
                m_options["user_format"] = optarg;
                break;
           
            case OptionOldPassword:
                // --old-password=PASSWORD
                m_options["old_password"] = optarg;
                break;
            
            case OptionNewPassword:
                // --new-password=PASSWORD
                m_options["new_password"] = optarg;
                break;

            case OptionPublicKeyFile:
                // --public-key-file=FILE
                m_options["public_key_file"] = optarg;
                break;
            
            case OptionPublicKeyName:
                // --public-key-name=FILE
                m_options["public_key_name"] = optarg;
                break;
            
            case OptionWithDatabase:
                // --with-database
                m_options["with_database"] = true;
                break;
            
            case OptionDbName:
                // --db-name=NAME
                m_options["db_name"] = optarg;
                break;
            
            case OptionAccount:
                // --account=USERNAME
                if (!setAccount(optarg))
                    return false;

                break;
            
            case OptionLimit:
                // --limit=NUMBER
                m_options["limit"] = optarg;
                break;
            
            case OptionOffset:
                // --offset=NUMBER
                m_options["offset"] = optarg;
                break;

            case OptionPrivileges:
                // --privileges=PRIVILEGES
                m_options["privileges"] = optarg;
                break;
            
            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }

    // 
    // The first extra argument is 'user', so we leave that out.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        //S9S_WARNING("argv[%3d] = %s", idx, argv[idx]);
        m_extraArguments << argv[idx];
    }

    return true;
}

bool
S9sOptions::readOptionsMaintenance(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0, OptionConfigFile      },
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        { "date-format",      required_argument, 0, OptionDateFormat      },
        { "full-uuid",        no_argument,       0, OptionFullUuid        },

        // Main Option
        { "list",             no_argument,       0, 'L'                   },
        { "create",           no_argument,       0, OptionCreate          },
        { "delete",           no_argument,       0, OptionDelete          },
       
        // Options about the maintenance period.
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "nodes",            required_argument, 0, OptionNodes           },
        { "start",            required_argument, 0, OptionStart           },
        { "end",              required_argument, 0, OptionEnd             },
        { "reason",           required_argument, 0, OptionReason          },
        { "uuid",             required_argument, 0, OptionUuid            },

        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:VgGu:", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;

            case 'c':
                // -c, --controller=URL
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;
            
            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;
            
            case OptionConfigFile:
                // --config-file=CONFIG
                m_options["config-file"] = optarg;
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;

            case OptionDateFormat:
                // --date-format=FORMAT
                m_options["date_format"] = optarg;
                break;

            case OptionFullUuid:
                // --full-uuid
                m_options["full_uuid"] = true;
                break;
            
            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;
            
            case 'L': 
                // --list
                m_options["list"] = true;
                break;
            
            case OptionCreate:
                // --create
                m_options["create"] = true;
                break;
            
            case OptionDelete:
                // --delete
                m_options["delete"] = true;
                break;
            
            case OptionNodes:
                // --nodes=LIST
                if (!setNodes(optarg))
                    return false;
                break;
            
            case 'i':
                // -i, --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;

            case OptionStart:
                // --start=DATE
                m_options["start"] = optarg;
                break;

            case OptionEnd:
                // --end=DATE
                m_options["end"] = optarg;
                break;
            
            case OptionReason:
                // --reason=DATE
                m_options["reason"] = optarg;
                break;

            case OptionUuid:
                // --uuid=UUID
                m_options["uuid"] = optarg;
                break;
            
            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }

    return true;
}

bool
S9sOptions::readOptionsMetaType(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Main Option
        { "help",             no_argument,       0, OptionHelp            },
        { "list",             no_argument,       0, 'L'                   },
        { "list-properties",  no_argument,       0, OptionListProperties  },
        { "list-cluster-types", no_argument,     0, OptionListClusterTypes },

        // Generic Options
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0, OptionConfigFile      },
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        { "date-format",      required_argument, 0, OptionDateFormat      },
        { "full-uuid",        no_argument,       0, OptionFullUuid        },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        
        // Type/property related options
        { "type",             required_argument, 0, OptionType            },

        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:VgGu:", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;

            case 'c':
                // -c, --controller=URL
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;
            
            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;
            
            case OptionConfigFile:
                // --config-file=CONFIG
                m_options["config-file"] = optarg;
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;

            case OptionDateFormat:
                // --date-format=FORMAT
                m_options["date_format"] = optarg;
                break;

            case OptionFullUuid:
                // --full-uuid
                m_options["full_uuid"] = true;
                break;
            
            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;
            
            case 'L': 
                // --list
                m_options["list"] = true;
                break;
           
            case OptionListProperties:
                // --list-properties
                m_options["list_properties"] = true;
                break;

            case OptionListClusterTypes:
                // --list-cluster-types
                m_options["list_cluster_types"] = true;
                break;

            case OptionType:
                // --type=NAME
                m_options["type"] = optarg;
                break;

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }
    
    // 
    // The first extra argument is 'metatype', so we leave that out.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        m_extraArguments << argv[idx];
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok.
 */
bool
S9sOptions::checkOptionsMetaType()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isListRequested())
        countOptions++;
    
    if (isListPropertiesRequested())
        countOptions++;
    
    if (isListClusterTypesRequested())
        countOptions++;
    
    if (countOptions > 1)
    {
        m_errorMessage = 
            "The --list and --list-properties options are mutually"
            " exclusive.";

        m_exitStatus = BadOptions;

        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = 
            "One of the --list, and --list-properties options is mandatory.";

        m_exitStatus = BadOptions;

        return false;
    }

    return true;
}

/**
 * Reads the command line options in "cluster" mode.
 */
bool
S9sOptions::readOptionsCluster(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0,  OptionRpcTls         },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "human-readable",   no_argument,       0, 'h'                   },
        { "config-file",      required_argument, 0, OptionConfigFile      },
        { "force",            no_argument,       0, OptionForce           },
        { "extended",         no_argument,       0, OptionExtended        },

        // Main Option
        { "add-node",         no_argument,       0, OptionAddNode         },
        { "check-hosts",      no_argument,       0, OptionCheckHosts      },
        { "create-account",   no_argument,       0, OptionCreateAccount   },
        { "create-database",  no_argument,       0, OptionCreateDatabase  },
        { "create",           no_argument,       0, OptionCreate          },
        { "create-report",    no_argument,       0, OptionCreateReport    },
        { "deploy-agents",    no_argument,       0, OptionDeployAgents    },
        { "delete-account",   no_argument,       0, OptionDeleteAccount   },
        { "drop",             no_argument,       0, OptionDrop            },
        { "grant",            no_argument,       0, OptionGrant           },
        { "list-databases",   no_argument,       0, OptionListDatabases   },
        { "list",             no_argument,       0, 'L'                   },
        { "ping",             no_argument,       0, OptionPing            },
        { "promote-slave",    no_argument,       0, OptionPromoteSlave    },
        { "demote-node",      no_argument,       0, OptionDemoteNode      },
        { "register",         no_argument,       0, OptionRegister        },
        { "remove-node",      no_argument,       0, OptionRemoveNode      },
        { "rolling-restart",  no_argument,       0, OptionRollingRestart  },
        { "collect-logs",     no_argument,       0, OptionCollectLogs     },
        { "import-config",    no_argument,       0, OptionImportConfig    },
        { "enable-ssl",       no_argument,       0, OptionEnableSsl       },
        { "disable-ssl",      no_argument,       0, OptionDisableSsl      },
        { "setup-audit-logging", no_argument,    0, OptionSetupAudit      },
        { "start",            no_argument,       0, OptionStart           },
        { "stat",             no_argument,       0, OptionStat            },
        { "stop",             no_argument,       0, OptionStop            },

        // Option(s) for error-report generation
        { "mask-passwords",   no_argument,       0, OptionMaskPasswords   },
        { "output-dir",       required_argument, 0, OptionOutputDir       },

        // Job Related Options
        { "batch",            no_argument,       0, OptionBatch           },
        { "job-tags",         required_argument, 0, OptionJobTags         },
        { "log",              no_argument,       0, 'G'                   },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        { "recurrence",       required_argument, 0, OptionRecurrence      },
        { "refresh",          no_argument,       0, OptionRefresh         },
        { "schedule",         required_argument, 0, OptionSchedule        },
        { "timeout",          required_argument, 0, OptionTimeout         },
        { "wait",             no_argument,       0, OptionWait            },
        { "backup-id",        required_argument, 0, OptionBackupId        },


        // Cluster information.
        // http://52.58.107.236/cmon-docs/current/cmonjobs.html#mysql
        // https://docs.google.com/document/d/1hvPtdWJqLeu1bAk-ZiWsILtj5dLXSLmXUyJBiP7wKjk/edit#heading=h.xsnzbjxs2gss
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "cluster-name",     required_argument, 0, 'n'                   },
        { "nodes",            required_argument, 0, OptionNodes           },
        { "vendor",           required_argument, 0, OptionVendor          },
        { "provider-version", required_argument, 0, OptionProviderVersion },
        { "os-user",          required_argument, 0, OptionOsUser          },
        { "os-sudo-password", required_argument, 0, OptionOsSudoPassword  },
        { "cluster-type",     required_argument, 0, OptionClusterType     },
        { "db-admin",         required_argument, 0, OptionDbAdmin         },
        { "db-admin-passwd",  required_argument, 0, OptionDbAdminPassword },
        { "account",          required_argument, 0, OptionAccount,        },
        { "with-database",    no_argument,       0, OptionWithDatabase    },
        { "db-name",          required_argument, 0, OptionDbName          },
        { "objects",          required_argument, 0, OptionObjects         },
        { "privileges",       required_argument, 0, OptionPrivileges      },
        { "opt-group",        required_argument, 0, OptionOptGroup        },
        { "opt-name",         required_argument, 0, OptionOptName         },
        { "opt-value",        required_argument, 0, OptionOptValue        }, 
        { "cluster-format",   required_argument, 0, OptionClusterFormat   }, 
        { "donor",            required_argument, 0, OptionDonor           },
        { "config-template",  required_argument, 0, OptionConfigTemplate   },
        { "no-install",       no_argument,       0, OptionNoInstall       },
       
        // Options for containers.
        { "cloud",            required_argument, 0, OptionCloud           },
        { "containers",       required_argument, 0, OptionContainers      },
        { "credential-id",    required_argument, 0, OptionCredentialId    },
        { "firewalls",        required_argument, 0, OptionFirewalls       },
        { "generate-key",     no_argument,       0, 'g'                   }, 
        { "image",            required_argument, 0, OptionImage           },
        { "image-os-user",    required_argument, 0, OptionImageOsUser     },
        { "os-key-file",      required_argument, 0, OptionOsKeyFile       },
        { "os-password",      required_argument, 0, OptionOsPassword      },
        { "os-user",          required_argument, 0, OptionOsUser          },
        { "region",           required_argument, 0, OptionRegion          },
        { "servers",          required_argument, 0, OptionServers         },
        { "subnet-id",        required_argument, 0, OptionSubnetId        },
        { "use-internal-repos", no_argument,     0, OptionUseInternalRepos },
        { "volumes",          required_argument, 0, OptionVolumes          },
        { "vpc-id",           required_argument, 0, OptionVpcId            },
        { "template",         required_argument, 0, OptionTemplate        },
        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:VLli:", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;

            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;
            
            case 'c':
                // -c, --controller=URL
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;

            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case 'L': 
                // -L, --list
                m_options["list"] = true;
                break;
            
            case OptionStat:
                // --stat
                m_options["stat"] = true;
                break;
            
            case OptionRollingRestart:
                // --rolling-restart
                m_options["rolling_restart"] = true;
                break;

            case OptionImportConfig:
                // --import-config
                m_options["import_config"] = true;
                break;
            
            case OptionCollectLogs:
                // --collect-logs
                m_options["collect_logs"] = true;
                break;
            
            case OptionEnableSsl:
                // --enable-ssl
                m_options["enable_ssl"] = true;
                break;
            
            case OptionDisableSsl:
                // --disable-ssl
                m_options["disable_ssl"] = true;
                break;
            
            case OptionSetupAudit:
                // --setup-audit-logging
                m_options["setup_audit_logging"] = true;
                break;
            
            case OptionCreateReport:
                // --create-report
                m_options["create_report"] = true;
                break;

            case OptionMaskPasswords:
                // --mask-passwords
                m_options["mask_passwords"] = true;
                break;

            case OptionDeployAgents:
                // --deploy-agents
                m_options["deploy_agents"] = true;
                break;

            case OptionAddNode:
                // --add-node
                m_options["add_node"] = true;
                break;
            
            case OptionRemoveNode:
                // --remove-node
                m_options["remove_node"] = true;
                break;

            case OptionDrop:
                // --drop
                m_options["drop"] = true;
                break;
            
            case OptionStop:
                // --stop
                m_options["stop"] = true;
                break;
            
            case OptionStart:
                // --start
                m_options["start"] = true;
                break;
            
            case OptionCreateAccount:
                // --create-account
                m_options["create_account"] = true;
                break;
            
            case OptionDeleteAccount:
                // --delete-account
                m_options["delete_account"] = true;
                break;
            
            case OptionCreateDatabase:
                // --create-database
                m_options["create_database"] = true;
                break;
            
            case OptionListDatabases:
                // --list-databases
                m_options["list_databases"] = true;
                break;
            
            case OptionGrant:
                // --grant
                m_options["grant"] = true;
                break;
            
            case OptionCheckHosts:
                // --check-hosts
                m_options["check_hosts"] = true;
                break;

            case 'h':
                // -h, --human-readable
                m_options["human_readable"] = true;
                break;

            case OptionConfigFile:
                // --config-file=FILE
                m_options["config_file"] = optarg;
                break;

            case OptionBackupId:
                // --backup-id=NUMBER
                m_options["backup_id"] = optarg;
                break;

            case OptionForce:
                // --force
                m_options["force"] = true;
                break;

            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;
            
            case OptionWait:
                // --wait
                m_options["wait"] = true;
                break;

            case 'G':
                // -G, --log
                m_options["log"] = true;
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionJobTags:
                // --job-tags=LIST
                setJobTags(optarg);
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;
           
            case OptionSchedule:
                // --schedule=DATETIME
                m_options["schedule"] = optarg;
                break;
            
            case OptionRecurrence:
                // --recurrence=CRONTABSTRING
                m_options["recurrence"] = optarg;
                break;
            
            case OptionTimeout:
                // --timeout=SECONDS
                m_options["timeout"] = optarg;
                break;

            case OptionRefresh:
                // --refresh
                m_options["refresh"] = true;
                break;

            case OptionPing:
                // --ping
                m_options["ping"] = true;
                break;
            
            case OptionPromoteSlave:
                // --promote-slave
                m_options["promote_slave"] = true;
                break;
            
            case OptionDemoteNode:
                // --demote-node
                m_options["demote_node"] = true;
                break;
            
            case OptionCreate:
                // --create
                m_options["create"] = true;
                break;
            
            case OptionRegister:
                // --register
                m_options["register"] = true;
                break;

            case 'i':
                // -i, --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;
            
            case 'n':
                // -n, --cluster-name=NAME
                m_options["cluster_name"] = optarg;
                break;

            case OptionNodes:
                // --nodes=LIST
                if (!setNodes(optarg))
                    return false;
                break;

            case OptionVendor:
                // --vendor=STRING
                m_options["vendor"] = optarg;
                break;

            case OptionProviderVersion:
                // --provider-version=STRING
                m_options["provider_version"] = optarg;
                break;

            case OptionOsSudoPassword:
                // --os-sudo-password
                m_options["os_sudo_password"] = optarg;
                break;

            case OptionClusterType:
                // --cluster-type
                m_options["cluster_type"] = optarg;
                break;
                
            case OptionRpcTls:
                // --rpc-tls
                m_options["rpc_tls"] = true;
                break;

            case OptionDbAdmin:
                // --db-admin=USERNAME
                m_options["db_admin_user_name"] = optarg;
                break;

            case OptionDbAdminPassword:
                // --db-admin-passwd=PASSWD
                m_options["db_admin_password"]  = optarg;
                break;
            
            case OptionAccount:
                // --account=USERNAME
                if (!setAccount(optarg))
                    return false;

                break;
            
            case OptionWithDatabase:
                // --with-database
                m_options["with_database"] = true;
                break;

            case OptionDbName:
                // --db-name=NAME
                m_options["db_name"] = optarg;
                break;

            case OptionObjects:
                // --objects=OBJECTS
                m_options["objects"] = optarg;
                break;
            
            case OptionPrivileges:
                // --privileges=PRIVILEGES
                m_options["privileges"] = optarg;
                break;

            case OptionOptGroup:
                // --opt-group=NAME
                m_options["opt_group"] = optarg;
                break;

            case OptionOptName:
                // --opt-name=NAME
                m_options["opt_name"] = optarg;
                break;

            case OptionOptValue:
                // --opt-value=VALUE
                m_options["opt_value"] = optarg;
                break;
            
            case OptionClusterFormat:
                // --cluster-format=VALUE
                m_options["cluster_format"] = optarg;
                break;
            
            case OptionOutputDir:
                // --output-dir=DIRECTORY
                m_options["output_dir"] = optarg;
                break;

            case OptionDonor:
                // --donor=ADDRESS
                m_options["donor"] = optarg;
                break;

            case OptionConfigTemplate:
                // --config-template=FILE
                m_options["config_template"] = optarg;
                break;

            case OptionNoInstall:
                // --no-install
                m_options["no_install"] = true;
                break;

            /*
             * Options for clouds.
             */
            case OptionCloud:
                // --cloud=NAME
                m_options["cloud"] = optarg;
                break;
                
            case OptionContainers:
                // --containers=LIST
                setContainers(optarg);
                break;
            
            case OptionCredentialId:
                // --credential-id=ID
                m_options["credential_id"] = optarg;
                break;

            case OptionFirewalls:
                // --firewalls=LIST
                m_options["firewalls"] = optarg;
                break;

            case 'g':
                // --generate-key
                m_options["generate_key"] = true;
                break;
            
            case OptionImage:
                // --image=image
                m_options["image"] = optarg;
                break;
            
            case OptionImageOsUser:
                // --image-os-user=user
                m_options["image_os_user"] = optarg;
                break;

            case OptionOsKeyFile:
                // --os-key-file=PATH
                m_options["os_key_file"] = optarg;
                break;
            
            case OptionOsPassword:
                // --os-password=PASSWORD
                m_options["os_password"] = optarg;
                break;
            
            case OptionOsUser:
                // --os-user=USERNAME
                m_options["os_user"] = optarg;
                break;
                
            case OptionRegion:
                // --region=REGION
                m_options["region"] = optarg;
                break;

            case OptionServers:
                // --servers=LIST
                if (!setServers(optarg))
                    return false;

                break;

            case OptionSubnetId:
                // --subnet-id=ID
                m_options["subnet_id"] = optarg;
                break;
            
            case OptionTemplate:
                // --template=NAME
                m_options["template"] = optarg;
                break;
          
            case OptionUseInternalRepos:
                // --use-internal-repos
                m_options["use_internal_repos"] = true;
                break;

            case OptionVolumes:
                // --volumes=STRING
                if (!appendVolumes(optarg))
                {
                    PRINT_ERROR("Invalid argument for --volumes.");
                    return false;
                }
                break;

            case OptionVpcId:
                // --vpc-id=ID
                m_options["vpc_id"] = optarg;
                break;

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }
    
    // 
    // The first extra argument is 'cluster', so we leave that out. We are
    // interested in the others.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        m_extraArguments << argv[idx];
    }

    return true;
}

/**
 * Reads the command line options in "container" mode.
 */
bool
S9sOptions::readOptionsContainer(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0,  OptionRpcTls         },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "human-readable",   no_argument,       0, 'h'                   },
        { "config-file",      required_argument, 0, OptionConfigFile      },

        // Main Option
        { "create",           no_argument,       0, OptionCreate          },
        { "delete",           no_argument,       0, OptionDelete          },
        { "list",             no_argument,       0, 'L'                   },
        { "start",            no_argument,       0, OptionStart           },
        { "stat",             no_argument,       0, OptionStat            },
        { "stop",             no_argument,       0, OptionStop            },

        // Job Related Options
        { "batch",            no_argument,       0, OptionBatch           },
        { "job-tags",         required_argument, 0, OptionJobTags         },
        { "log",              no_argument,       0, 'G'                   },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        { "recurrence",       required_argument, 0, OptionRecurrence      },
        { "refresh",          no_argument,       0, OptionRefresh         },
        { "schedule",         required_argument, 0, OptionSchedule        },
        { "timeout",          required_argument, 0, OptionTimeout         },
        { "wait",             no_argument,       0, OptionWait            },

        // Other options.
        { "cloud",            required_argument, 0, OptionCloud           },
        { "container-format", required_argument, 0, OptionContainerFormat }, 
        { "containers",       required_argument, 0, OptionContainers      },
        { "credential-id",    required_argument, 0, OptionCredentialId    },
        { "firewalls",        required_argument, 0, OptionFirewalls       },
        { "generate-key",     no_argument,       0, 'g'                   }, 
        { "image-os-user",    required_argument, 0, OptionImageOsUser     },
        { "image",            required_argument, 0, OptionImage           },
        { "os-key-file",      required_argument, 0, OptionOsKeyFile       },
        { "os-password",      required_argument, 0, OptionOsPassword      },
        { "os-user",          required_argument, 0, OptionOsUser          },
        { "region",           required_argument, 0, OptionRegion          },
        { "servers",          required_argument, 0, OptionServers         },
        { "subnet-id",        required_argument, 0, OptionSubnetId        },
        { "template",         required_argument, 0, OptionTemplate        },
        { "volumes",          required_argument, 0, OptionVolumes         },
        { "vpc-id",           required_argument, 0, OptionVpcId           },
         
        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:VLli:", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;

            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;
            
            case 'c':
                // -c, --controller=URL
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;

            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case 'h':
                // -h, --human-readable
                m_options["human_readable"] = true;
                break;

            case OptionConfigFile:
                // --config-file=FILE
                m_options["config_file"] = optarg;
                break;

            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;
            
            case OptionWait:
                // --wait
                m_options["wait"] = true;
                break;

            case 'G':
                // -G, --log
                m_options["log"] = true;
                break;
            
            case OptionJobTags:
                // --job-tags=LIST
                setJobTags(optarg);
                break;

            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;
           
            case OptionSchedule:
                // --schedule=DATETIME
                m_options["schedule"] = optarg;
                break;
            
            case OptionRecurrence:
                // --recurrence=CRONTABSTRING
                m_options["recurrence"] = optarg;
                break;
            
            case OptionTimeout:
                // --timeout=SECONDS
                m_options["timeout"] = optarg;
                break;

            /*
             * The main options.
             */
            case OptionCreate:
                // --create
                m_options["create"] = true;
                break;
            
            case OptionDelete:
                // --delete
                m_options["delete"]  = true;
                break;

            case 'L': 
                // -L, --list
                m_options["list"] = true;
                break;
            
            case OptionStat:
                // --stat
                m_options["stat"] = true;
                break;
            
            case OptionStop:
                // --stop
                m_options["stop"] = true;
                break;
            
            case OptionStart:
                // --start
                m_options["start"] = true;
                break;

            case OptionPing:
                // --ping
                m_options["ping"] = true;
                break;
            
            case OptionRegister:
                // --register
                m_options["register"] = true;
                break;

            /*
             * Other options.
             */
            case OptionCloud:
                // --cloud=NAME
                m_options["cloud"] = optarg;
                break;

            case OptionContainerFormat:
                // --container-format=FORMAT
                m_options["container_format"] = optarg;
                break;

            case OptionContainers:
                // --containers=LIST
                setContainers(optarg);
                break;

            case OptionCredentialId:
                // --credential-id=ID
                m_options["credential_id"] = optarg;
                break;

            case OptionFirewalls:
                // --firewalls=LIST
                m_options["firewalls"] = optarg;
                break;

            case 'g':
                // --generate-key
                m_options["generate_key"] = true;
                break;

            case OptionImage:
                // --image=image
                m_options["image"] = optarg;
                break;

            case OptionImageOsUser:
                // --image-os-user=user
                m_options["image_os_user"] = optarg;
                break;

            case OptionOsKeyFile:
                // --os-key-file=PATH
                m_options["os_key_file"] = optarg;
                break;
            
            case OptionOsPassword:
                // --os-password=PASSWORD
                m_options["os_password"] = optarg;
                break;
            
            case OptionOsUser:
                // --os-user=USERNAME
                m_options["os_user"] = optarg;
                break;

            case OptionRegion:
                // --region=REGION
                m_options["region"] = optarg;
                break;
                
            case OptionServers:
                // --servers=LIST
                if (!setServers(optarg))
                    return false;

                break;

            case OptionSubnetId:
                // --subnet-id=ID
                m_options["subnet_id"] = optarg;
                break;
            
            case OptionTemplate:
                // --template=NAME
                m_options["template"] = optarg;
                break;
            
            case OptionVolumes:
                // --volumes=STRING
                if (!appendVolumes(optarg))
                {
                    PRINT_ERROR("Invalid argument for --volumes.");
                    return false;
                }
                break;
            
            case OptionVpcId:
                // --vpc-id=ID
                m_options["vpc_id"] = optarg;
                break;

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }
    
    // 
    // The first extra argument is 'cluster', so we leave that out. We are
    // interested in the others.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        m_extraArguments << argv[idx];
    }

    return true;
}

/**
 * Reads the command line options for the "job" command.
 */
bool
S9sOptions::readOptionsJob(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0,  6                    },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0,  OptionPrintJson      },
        { "config-file",      required_argument, 0,  OptionConfigFile     },
        { "color",            optional_argument, 0,  OptionColor          },
        { "date-format",      required_argument, 0,  OptionDateFormat     },

        // Main Option
        { "batch",            no_argument,       0, OptionBatch           },
        { "clone",            no_argument,       0,  OptionClone          },
        { "delete",           no_argument,       0,  OptionDelete         },
        { "fail",             no_argument,       0,  OptionFail           },
        { "kill",             no_argument,       0,  OptionKill           },
        { "list",             no_argument,       0, 'L'                   },
        { "log",              no_argument,       0, 'G'                   },
        { "success",          no_argument,       0,  OptionSuccess        },
        { "wait",             no_argument,       0,  5                    },

        // Job Related Options
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "cluster-name",     required_argument, 0, 'n'                   },
        { "job-id",           required_argument, 0, OptionJobId           },
        { "job-tags",         required_argument, 0, OptionJobTags         },
        { "limit",            required_argument, 0, OptionLimit           },
        { "log-format",       required_argument, 0, OptionLogFormat       },
        { "offset",           required_argument, 0, OptionOffset          },
        { "recurrence",       required_argument, 0, OptionRecurrence      },
        { "timeout",          required_argument, 0, OptionTimeout         },
        { "schedule",         required_argument, 0, OptionSchedule        },
        
        { "show-aborted",     no_argument,       0, OptionShowAborted     },
        { "show-defined",     no_argument,       0, OptionShowDefined     },
        { "show-failed",      no_argument,       0, OptionShowFailed      },
        { "show-finished",    no_argument,       0, OptionShowFinished    },
        { "show-running",     no_argument,       0, OptionShowRunning     },
        { "show-scheduled",   no_argument,       0, OptionShowScheduled   },
        { "without-tags",     required_argument, 0, OptionWithoutTags     },
        { "with-tags",        required_argument, 0, OptionWithTags        },

        { 0, 0, 0, 0 }
    };

    S9S_DEBUG("*** argc : %d", argc);
    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:VLlG", 
                long_options, &option_index);

        if (c == -1)
            break;
        
        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;

            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;

            case 'c':
                // -c, --controller=URL
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port
                m_options["controller_port"] = atoi(optarg);
                break;

            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case 'L': 
                // -L, --list
                m_options["list"] = true;
                break;
            
            case 'G': 
                // -G, --log
                m_options["log"] = true;
                break;
            
            case OptionDelete: 
                // --delete
                m_options["delete"] = true;
                break;
            
            case OptionClone: 
                // --clone
                m_options["clone"] = true;
                break;

            case OptionFail:
                // --fail
                m_options["fail"] = true;
                break;
            
            case OptionKill:
                // --kill
                m_options["kill"] = true;
                break;
            
            case OptionSuccess:
                // --success
                m_options["success"] = true;
                break;

            case OptionConfigFile:
                // --config-file=FILE 
                m_options["config_file"] = optarg;
                break;

            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;
            
            case OptionDateFormat:
                // --date-format=FORMAT
                m_options["date_format"] = optarg;
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;

            case OptionJobId:
                // --job-id=ID
                m_options["job_id"] = atoi(optarg);
                break;

            case OptionJobTags:
                // --job-tags=LIST
                setJobTags(optarg);
                break;
            
            case OptionTimeout:
                // --timeout=SECONDS
                m_options["timeout"] = optarg;
                break;

            case 5:
                // --wait
                m_options["wait"] = true;
                break;

            case 6:
                // --rpc-tls
                m_options["rpc_tls"] = true;
                break;

            case 'i':
                // --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;
            
            case 'n':
                // -n, --cluster-name=NAME
                m_options["cluster_name"] = optarg;
                break;

            case OptionLogFormat:
                // --log-format=FORMAT
                m_options["log_format"] = optarg;
                break;

            case OptionLimit:
                // --limit=NUMBER
                m_options["limit"] = optarg;
                break;
            
            case OptionOffset:
                // --offset=NUMBER
                m_options["offset"] = optarg;
                break;
            
            case OptionSchedule:
                // --schedule=DATETIME
                m_options["schedule"] = optarg;
                break;
            
            case OptionRecurrence:
                // --recurrence=CRONTABSTRING
                m_options["recurrence"] = optarg;
                break;

            case OptionShowDefined:
                // --show-defined
                m_options["show_defined"] = true;
                break;
            
            case OptionShowRunning:
                // --show-running
                m_options["show_running"] = true;
                break;
            
            case OptionShowScheduled:
                // --show-scheduled
                m_options["show_scheduled"] = true;
                break;
            
            case OptionShowAborted:
                // --show-aborted
                m_options["show_aborted"] = true;
                break;
            
            case OptionShowFinished:
                // --show-finished
                m_options["show_finished"] = true;
                break;
            
            case OptionShowFailed:
                // --show-failed
                m_options["show_failed"] = true;
                break;
            
            case OptionWithTags:
                // --with-tags=one;two
                setWithTags(optarg);
                break;
            
            case OptionWithoutTags:
                // --without-tags=one;two
                setWithoutTags(optarg);
                break;

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) 
                    {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }

    return true;
}

/**
 * Reads the command line options for the "script" mode.
 */
bool
S9sOptions::readOptionsScript(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0, OptionConfigFile      },
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        
        // Job Related Options
        { "force",            no_argument,       0, OptionForce           },
        { "job-tags",         required_argument, 0, OptionJobTags         },
        { "log-format",       required_argument, 0, OptionLogFormat       },
        { "log",              no_argument,       0, 'G'                   },
        { "recurrence",       required_argument, 0, OptionRecurrence      },
        { "schedule",         required_argument, 0, OptionSchedule        },
        { "timeout",          required_argument, 0, OptionTimeout         },
        { "wait",             no_argument,       0, OptionWait            },

        // Main Option
        { "execute",          no_argument,       0, OptionExecute         },
        { "run",              no_argument,       0, OptionRun             },
        { "system",           no_argument,       0, OptionSystem          },
        { "tree",             no_argument,       0, OptionTree            },
       
        // 
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "nodes",            required_argument, 0, OptionNodes           },
        { "shell-command",    required_argument, 0, OptionShellCommand    },

        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:VgGu:", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;

            case 'c':
                // -c, --controller=URL
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;

            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;
            
            case OptionConfigFile:
                // --config-file=CONFIG
                m_options["config-file"] = optarg;
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;

            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;
           
            /*
             * Options about the cluster.
             */
            case 'i':
                // -i, --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;
            
            case OptionNodes:
                // --nodes=LIST
                if (!setNodes(optarg))
                    return false;
                break;

            case OptionExtended:
                // --extended
                m_options["extended"] = true;
                break;

            /*
             * Job options.
             */
            case OptionForce:
                // --force
                m_options["force"] = true;
                break;
            
            case OptionJobTags:
                // --job-tags=LIST
                setJobTags(optarg);
                break;
            
            case OptionLogFormat:
                // --log-format=FORMAT
                m_options["log_format"] = optarg;
                break;

            case 'G':
                // -G, --log
                m_options["log"] = true;
                break;
            
            case OptionRecurrence:
                // --recurrence=CRONTABSTRING
                m_options["recurrence"] = optarg;
                break;
            
            case OptionSchedule:
                // --schedule=DATETIME
                m_options["schedule"] = optarg;
                break;
            
            case OptionTimeout:
                // --timeout=SECONDS
                m_options["timeout"] = optarg;
                break;
            
            case OptionWait:
                // --wait
                m_options["wait"] = true;
                break;
            
            /*
             * Main options.
             */
            case OptionExecute:
                // --execute
                m_options["execute"] = true;
                break;
            
            case OptionRun:
                // --run
                m_options["run"] = true;
                break;
           
            case OptionSystem:
                // --system
                m_options["system"]  = true;
                break;

            case OptionTree:
                // --tree
                m_options["tree"] = true;
                break;

            /*
             * Options about the execution.
             */
            case OptionShellCommand:
                // --shell-command=COMMAND
                m_options["shell_command"] = optarg;
                break;

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }

    // 
    // The first extra argument is 'cluster', so we leave that out. We are
    // interested in the others.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        m_extraArguments << argv[idx];
    }

    return true;
}

/**
 * Reads the command line options for the "sheet" mode.
 */
bool
S9sOptions::readOptionsSheet(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0, OptionConfigFile      },
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        
        // Job Related Options
        { "force",            no_argument,       0, OptionForce           },
        { "job-tags",         required_argument, 0, OptionJobTags         },
        { "log-format",       required_argument, 0, OptionLogFormat       },
        { "log",              no_argument,       0, 'G'                   },
        { "recurrence",       required_argument, 0, OptionRecurrence      },
        { "schedule",         required_argument, 0, OptionSchedule        },
        { "timeout",          required_argument, 0, OptionTimeout         },
        { "wait",             no_argument,       0, OptionWait            },

        // Main Option
        { "create",           no_argument,       0,  OptionCreate         },
        { "edit",             no_argument,       0, OptionEdit            },
        { "list",             no_argument,       0, 'L'                   },
        { "stat",             no_argument,       0, OptionStat            },
       
        // Other options.
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "input-file",       required_argument, 0, OptionInputFile       },

        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:VgGu:", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;

            case 'c':
                // -c, --controller=URL
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;

            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;
            
            case OptionConfigFile:
                // --config-file=CONFIG
                m_options["config-file"] = optarg;
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;

            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;
           
            /*
             * Options about the cluster.
             */
            case 'i':
                // -i, --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;
            
            /*
             * Job options.
             */
            case OptionForce:
                // --force
                m_options["force"] = true;
                break;
            
            case OptionJobTags:
                // --job-tags=LIST
                setJobTags(optarg);
                break;
            
            case OptionLogFormat:
                // --log-format=FORMAT
                m_options["log_format"] = optarg;
                break;

            case 'G':
                // -G, --log
                m_options["log"] = true;
                break;
            
            case OptionRecurrence:
                // --recurrence=CRONTABSTRING
                m_options["recurrence"] = optarg;
                break;
            
            case OptionSchedule:
                // --schedule=DATETIME
                m_options["schedule"] = optarg;
                break;
            
            case OptionTimeout:
                // --timeout=SECONDS
                m_options["timeout"] = optarg;
                break;
            
            case OptionWait:
                // --wait
                m_options["wait"] = true;
                break;
            
            /*
             * Main options.
             */
            case OptionCreate:
                // --create
                m_options["create"] = true;
                break;
            
            case OptionEdit:
                // --edit
                m_options["edit"] = true;
                break;

            case 'L': 
                // --list
                m_options["list"] = true;
                break;
            
            case OptionStat:
                // --stat
                m_options["stat"] = true;
                break;

            /*
             *
             */
            case OptionInputFile:
                // --input-file=FILE
                m_options["input_file"] = optarg;
                break;

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }

    // 
    // The first extra argument is 'cluster', so we leave that out. We are
    // interested in the others.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        m_extraArguments << argv[idx];
    }

    return true;
}

/**
 * Reads the command line options for the "server" mode.
 */
bool
S9sOptions::readOptionsServer(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "human-readable",   no_argument,       0, 'h'                   },
        { "config-file",      required_argument, 0, OptionConfigFile      },
        { "batch",            no_argument,       0, OptionBatch           },
        { "only-ascii",       no_argument,       0, OptionOnlyAscii       },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        
        // Job Related Options
        { "wait",             no_argument,       0, OptionWait            },
        { "job-tags",         required_argument, 0, OptionJobTags         },
        { "log",              no_argument,       0, 'G'                   },
        { "schedule",         required_argument, 0, OptionSchedule        },
        { "recurrence",       required_argument, 0, OptionRecurrence      },
        { "timeout",          required_argument, 0, OptionTimeout         },

        // Main Option
        { "add-acl",          no_argument,       0, OptionAddAcl          },
        { "create",           no_argument,       0, OptionCreate          },
        { "delete",           no_argument,       0, OptionDelete          },
        { "get-acl",          no_argument,       0, OptionGetAcl          },
        { "list-containers",  no_argument,       0, OptionListContainers  },
        { "list-disks",       no_argument,       0, OptionListDisks       },
        { "list-images",      no_argument,       0, OptionListImages      },
        { "list-regions",     no_argument,       0, OptionListRegions     },
        { "list-memory",      no_argument,       0, OptionListMemory      },
        { "list-nics",        no_argument,       0, OptionListNics        },
        { "list",             no_argument,       0, 'L'                   },
        { "list-partitions",  no_argument,       0, OptionListPartitions  },
        { "list-processors",  no_argument,       0, OptionListProcessors  },
        { "list-subnets",     no_argument,       0, OptionListSubnets     },
        { "list-templates",   no_argument,       0, OptionListTemplates   },
        { "move",             no_argument,       0, OptionMove            },
        { "register",         no_argument,       0, OptionRegister        },
        { "start",            no_argument,       0, OptionStart           },
        { "stat",             no_argument,       0, OptionStat            },
        { "stop",             no_argument,       0, OptionStop            },
        { "unregister",       no_argument,       0, OptionUnregister      },
       
        // FIXME: remove this.
        //{ "cluster-id",       required_argument, 0, 'i'                   },
        
        { "acl",              required_argument, 0, OptionAcl             },
        { "cloud",            required_argument, 0, OptionCloud           },
        { "os-key-file",      required_argument, 0, OptionOsKeyFile       },
        { "os-password",      required_argument, 0, OptionOsPassword      },
        { "os-user",          required_argument, 0, OptionOsUser          },
        { "refresh",          no_argument,       0, OptionRefresh         },
        { "region",           required_argument, 0, OptionRegion          },
        { "servers",          required_argument, 0, OptionServers         },

        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:VgGu:", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // -h, --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;

            case 'c':
                // -c, --controller=URL
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;

            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;
            
            case OptionConfigFile:
                // --config-file=CONFIG
                m_options["config-file"] = optarg;
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;
            
            case OptionOnlyAscii:
                // --only-ascii
                m_options["only_ascii"] = true;
                break;

            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case 'h':
                // -h, --human-readable
                m_options["human_readable"] = true;
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;
            
            case 'i':
                // -i, --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;
            
            case OptionStat:
                // --stat
                m_options["stat"] = true;
                break;

            case OptionCreate:
                // --create
                m_options["create"] = true;
                break;
            
            case OptionMove:
                // --move
                m_options["move"] = true;
                break;
            
            case OptionRegister:
                // --register
                m_options["register"] = true;
                break;
            
            case OptionUnregister:
                // --unregister
                m_options["unregister"] = true;
                break;
            
            case 'L': 
                // --list
                m_options["list"] = true;
                break;

            case OptionListContainers:
                // --list-containers
                m_options["list_containers"] = true;
                break;
            
            case OptionListPartitions:
                // --list-partitions
                m_options["list_partitions"] = true;
                break;
            
            case OptionListImages:
                // --list-images
                m_options["list_images"] = true;
                break;
            
            case OptionListRegions:
                // --list-regions
                m_options["list_regions"] = true;
                break;
            
            case OptionListMemory:
                // --list-memory
                m_options["list_memory"] = true;
                break;
            
            case OptionGetAcl:
                // --get-acl
                m_options["get_acl"] = true;
                break;

            case OptionAddAcl:
                // --add-acl
                m_options["add_acl"] = true;
                break;
            
            case OptionListProcessors:
                // --list-processors
                m_options["list_processors"] = true;
                break;
            
            case OptionListSubnets:
                // --list-subnets
                m_options["list_subnets"] = true;
                break;
            
            case OptionListTemplates:
                // --list-templates
                m_options["list_templates"] = true;
                break;
            
            case OptionListNics:
                // --list-nics
                m_options["list_nics"] = true;
                break;
            
            case OptionListDisks:
                // --list-nics
                m_options["list_disks"] = true;
                break;
            
            case OptionDelete:
                // --delete
                m_options["delete"] = true;
                break;
            
            case OptionStart:
                // --start
                m_options["start"] = true;
                break;
            
            case OptionStop:
                // --stop
                m_options["stop"] = true;
                break;
            

            /*
             * Job related options.
             */
            case OptionWait:
                // --wait
                m_options["wait"] = true;
                break;
            
            case OptionJobTags:
                // --job-tags=LIST
                setJobTags(optarg);
                break;

            case 'G':
                // -G, --log
                m_options["log"] = true;
                break;
            
            case OptionSchedule:
                // --schedule=DATETIME
                m_options["schedule"] = optarg;
                break;
            
            case OptionRecurrence:
                // --recurrence=CRONTABSTRING
                m_options["recurrence"] = optarg;
                break;
            
            case OptionTimeout:
                // --timeout=SECONDS
                m_options["timeout"] = optarg;
                break;
            
            /*
             * Other command line options.
             */
            case OptionAcl:
                // --acl=ACLSTRING
                m_options["acl"] = optarg;
                break;
            
            case OptionOsKeyFile:
                // --os-key-file=PATH
                m_options["os_key_file"] = optarg;
                break;
            
            case OptionOsPassword:
                // --os-password=PASSWORD
                m_options["os_password"] = optarg;
                break;
            
            case OptionOsUser:
                // --os-user=USERNAME
                m_options["os_user"] = optarg;
                break;
            
            case OptionRefresh:
                // --refresh
                m_options["refresh"] = true;
                break;
            
            case OptionCloud:
                // --cloud=NAME
                m_options["cloud"] = optarg;
                break;
            
            case OptionRegion:
                // --region=REGION
                m_options["region"] = optarg;
                break;
            
            case OptionServers:
                // --servers=LIST
                if (!setServers(optarg))
                    return false;
                break;
            

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }

    // 
    // The first extra argument is 'cluster', so we leave that out. We are
    // interested in the others.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        m_extraArguments << argv[idx];
    }

    return true;
}

/**
 * Reads the command line options for the "controller" mode.
 */
bool
S9sOptions::readOptionsController(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "human-readable",   no_argument,       0, 'h'                   },
        { "config-file",      required_argument, 0, OptionConfigFile      },
        { "batch",            no_argument,       0, OptionBatch           },
        { "only-ascii",       no_argument,       0, OptionOnlyAscii       },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        
        // Job Related Options
        { "wait",             no_argument,       0, OptionWait            },
        { "job-tags",         required_argument, 0, OptionJobTags         },
        { "log",              no_argument,       0, 'G'                   },
        { "schedule",         required_argument, 0, OptionSchedule        },
        { "recurrence",       required_argument, 0, OptionRecurrence      },
        { "timeout",          required_argument, 0, OptionTimeout         },

        // Main Option
        { "create-snapshot",  no_argument,       0, OptionCreateSnaphot   },
        { "enable-cmon-ha",   no_argument,       0, OptionEnableCmonHa    },
        { "list",             no_argument,       0, 'L'                   },
        { "stat",             no_argument,       0, OptionStat            },
       
        // FIXME: remove this.
        //{ "cluster-id",       required_argument, 0, 'i'                   },
        
        { "acl",              required_argument, 0, OptionAcl             },
        { "cloud",            required_argument, 0, OptionCloud           },
        { "os-key-file",      required_argument, 0, OptionOsKeyFile       },
        { "os-password",      required_argument, 0, OptionOsPassword      },
        { "os-user",          required_argument, 0, OptionOsUser          },
        { "refresh",          no_argument,       0, OptionRefresh         },
        { "region",           required_argument, 0, OptionRegion          },
        { "servers",          required_argument, 0, OptionServers         },
        
        { "log-file",         required_argument, 0, OptionLogFile         },

        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:VgGu:", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // -h, --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;

            case 'c':
                // -c, --controller=URL
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;

            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;
            
            case OptionConfigFile:
                // --config-file=CONFIG
                m_options["config-file"] = optarg;
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;
            
            case OptionOnlyAscii:
                // --only-ascii
                m_options["only_ascii"] = true;
                break;

            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case 'h':
                // -h, --human-readable
                m_options["human_readable"] = true;
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;
            
            /*
             * Main options.
             */
            case OptionCreateSnaphot:
                // --create-snapshot
                m_options["create_snapshot"] = true;
                break;

            case OptionEnableCmonHa:
                // --enable-cmon-ha
                m_options["enable_cmon_ha"] = true;
                break;

            case 'L': 
                // --list
                m_options["list"] = true;
                break;

            case OptionStat:
                // --stat
                m_options["stat"] = true;
                break;

            /*
             * Job related options.
             */
            case OptionWait:
                // --wait
                m_options["wait"] = true;
                break;
            
            case OptionJobTags:
                // --job-tags=LIST
                setJobTags(optarg);
                break;

            case 'G':
                // -G, --log
                m_options["log"] = true;
                break;
            
            case OptionSchedule:
                // --schedule=DATETIME
                m_options["schedule"] = optarg;
                break;
            
            case OptionRecurrence:
                // --recurrence=CRONTABSTRING
                m_options["recurrence"] = optarg;
                break;
            
            case OptionTimeout:
                // --timeout=SECONDS
                m_options["timeout"] = optarg;
                break;
            
            /*
             * Other command line options.
             */ 
            case OptionLogFile:
                // --log-file=FILE
                m_options["log_file"] = optarg;
                break;            

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }

    // 
    // The first extra argument is 'cluster', so we leave that out. We are
    // interested in the others.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        m_extraArguments << argv[idx];
    }

    return true;
}

/**
 * Reads the command line options for the "tree" mode.
 */
bool
S9sOptions::readOptionsTree(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "private-key-file", required_argument, 0, OptionPrivateKeyFile  }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "human-readable",   no_argument,       0, 'h'                   },
        { "config-file",      required_argument, 0, OptionConfigFile      },
        { "batch",            no_argument,       0, OptionBatch           },
        { "only-ascii",       no_argument,       0, OptionOnlyAscii       },
        { "no-header",        no_argument,       0, OptionNoHeader        },

        // Main Option
        { "access",           no_argument,       0, OptionAccess          },
        { "add-acl",          no_argument,       0, OptionAddAcl          },
        { "cat",              no_argument,       0, OptionCat             },
        { "chown",            no_argument,       0, OptionChOwn           },
        { "delete",           no_argument,       0, OptionDelete          },
        { "get-acl",          no_argument,       0, OptionGetAcl          },
        { "list",             no_argument,       0, 'L'                   },
        { "mkdir",            no_argument,       0, OptionMkdir           },
        { "move",             no_argument,       0, OptionMove            },
        { "remove-acl",       no_argument,       0, OptionRemoveAcl       },
        { "rmdir",            no_argument,       0, OptionRmdir           },
        { "save",             no_argument,       0, OptionSave            },
        { "stat",             no_argument,       0, OptionStat            },
        { "touch",            no_argument,       0, OptionMkfile          }, 
        { "tree",             no_argument,       0, OptionTree            },
        { "watch",            no_argument,       0, OptionWatch           },
        
        { "acl",              required_argument, 0, OptionAcl             },
        { "all",              no_argument,       0, OptionAll             },
        { "directory",        no_argument,       0, 'd'                   },
        { "owner",            required_argument, 0, OptionOwner           },
        { "privileges",       required_argument, 0, OptionPrivileges      },
        { "recursive",        no_argument,       0, 'R'                   },
        { "refresh",          no_argument,       0, OptionRefresh         },
        { "full-path",        no_argument,       0, OptionFullPath        },

        { "log-file",         required_argument, 0, OptionLogFile         },

        { 0, 0, 0, 0 }
    };

    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:VgGu:Rd", 
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                // -h, --help
                m_options["help"] = true;
                break;
            
            case OptionDebug:
                // --debug
                m_options["debug"] = true;
                break;

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;
            
            case 'u':
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
                break;
            
            case OptionPrivateKeyFile:
                // --private-key-file=FILE
                m_options["private_key_file"] = optarg;
                break;

            case 'c':
                // -c, --controller=URL
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;

            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;
            
            case OptionConfigFile:
                // --config-file=CONFIG
                m_options["config-file"] = optarg;
                break;
            
            case OptionBatch:
                // --batch
                m_options["batch"] = true;
                break;
            
            case OptionNoHeader:
                // --no-header
                m_options["no_header"] = true;
                break;
            
            case OptionOnlyAscii:
                // --only-ascii
                m_options["only_ascii"] = true;
                break;

            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case 'h':
                // -h, --human-readable
                m_options["human_readable"] = true;
                break;

            case OptionPrintJson:
                // --print-json
                m_options["print_json"] = true;
                break;
            
            case 'i':
                // -i, --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;

            /*
             * Main options.
             */
            case OptionStat:
                // --stat
                m_options["stat"] = true;
                break;
            
            case OptionMkfile:
                // --touch
                m_options["mkfile"] = true;
                break;

            case OptionTree:
                // --tree
                m_options["tree"] = true;
                break;

             case OptionWatch:
                // --watch
                m_options["watch"] = true;
                break;

            case OptionMove:
                // --move
                m_options["move"] = true;
                break;
            
            case 'L': 
                // --list
                m_options["list"] = true;
                break;

            case OptionMkdir:
                // --mkdir
                m_options["mkdir"] = true;
                break;
            
            case OptionRmdir:
                // --rmdir
                m_options["rmdir"] = true;
                break;
            
            case OptionSave:
                // --save
                m_options["save"] = true;
                break;

            case OptionGetAcl:
                // --get-acl
                m_options["get_acl"] = true;
                break;
            
            case OptionCat:
                // --cat
                m_options["cat"] = true;
                break;

            case OptionAccess:
                // --access
                m_options["access"] = true;
                break;

            case OptionAddAcl:
                // --add-acl
                m_options["add_acl"] = true;
                break;

            case OptionRemoveAcl:
                // --remove-acl
                m_options["remove_acl"] = true;
                break;

            case OptionChOwn:
                // --chown
                m_options["chown"] = true;
                break;
            
            case OptionDelete:
                // --delete
                m_options["delete"]  = true;
                break;
            
            /*
             * Other command line options.
             */
            case OptionAcl:
                // --acl=ACLSTRING
                m_options["acl"] = optarg;
                break;
            
            case OptionAll:
                // --all
                m_options["all"] = true;
                break;
            
            case 'd':
                // -d, --directory
                m_options["directory"] = true;
                break;


            case OptionOwner:
                // --owner=USER | --owner=USER.GROUP
                m_options["owner"] = optarg;
                break;
            
            case OptionPrivileges:
                // --privileges=PRIVILEGES
                m_options["privileges"] = optarg;
                break;

            case 'R':
                // --recursive
                m_options["recursive"] = true;
                break;

            case OptionRefresh:
                // --refresh
                m_options["refresh"] = true;
                break;
            
            case OptionFullPath:
                // --full-path
                m_options["full_path"] = true;
                break;

            case OptionLogFile:
                // --log-file=FILE
                m_options["log_file"] = optarg;
                break;

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }

    // 
    // The first extra argument is 'cluster', so we leave that out. We are
    // interested in the others.
    //
    for (int idx = optind + 1; idx < argc; ++idx)
    {
        m_extraArguments << argv[idx];
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok in "script" mode.
 */
bool
S9sOptions::checkOptionsScript()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isTreeRequested())
        countOptions++;
    
    if (isExecuteRequested())
        countOptions++;
    
    if (isRunRequested())
        countOptions++;
    
    if (isSystemRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = 
            "The main options are mutually exclusive.";

        m_exitStatus = BadOptions;

        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = "One of the main options is mandatory.";
        m_exitStatus = BadOptions;

        return false;
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok in "script" mode.
 */
bool
S9sOptions::checkOptionsSheet()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isListRequested())
        countOptions++;
    else if (isStatRequested())
        countOptions++;
    else if (isCreateRequested())
        countOptions++;
    else if (isEditRequested())
        countOptions++;
    
    if (countOptions > 1)
    {
        m_errorMessage = "The main options are mutually exclusive.";
        m_exitStatus = BadOptions;

        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = "One of the main options is mandatory.";
        m_exitStatus = BadOptions;

        return false;
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok in "server" mode.
 */
bool
S9sOptions::checkOptionsServer()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isTreeRequested())
    {
        countOptions++;
        if (nExtraArguments() > 1) 
        {
            m_errorMessage = 
                "The --tree option enables only one command line argument: "
                "the path to print.";
            m_exitStatus = BadOptions;
            return false;
        }
    }

    if (isDeleteRequested())
        countOptions++;
    
    if (isCreateRequested())
        countOptions++;
    
    if (isMoveRequested())
        countOptions++;

    if (isRegisterRequested())
        countOptions++;
    
    if (isUnregisterRequested())
        countOptions++;
    
    if (isListContainersRequested())
        countOptions++;
    
    if (isListPartitionsRequested())
        countOptions++;
    
    if (isListImagesRequested())
        countOptions++;
    
    if (isListRegionsRequested())
        countOptions++;
    
    if (isListMemoryRequested())
        countOptions++;
    
    if (isGetAclRequested())
        countOptions++;
    
    if (isAddAclRequested())
        countOptions++;
    
    if (isListProcessorsRequested())
        countOptions++;
    
    if (isListSubnetsRequested())
        countOptions++;
    
    if (isListTemplatesRequested())
        countOptions++;
    
    if (isListNicsRequested())
        countOptions++;
    
    if (isListDisksRequested())
        countOptions++;
    
    if (isListRequested())
        countOptions++;
    
    if (isStatRequested())
        countOptions++;
    
    if (isStartRequested())
        countOptions++;
    
    if (isStopRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = "Main options are mutually exclusive.";
        m_exitStatus = BadOptions;
        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = "Main option is required.";
        m_exitStatus = BadOptions;

        return false;
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok in "controller" mode.
 */
bool
S9sOptions::checkOptionsController()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isCreateSnapshotRequested())
        countOptions++;

    if (isEnableCmonHaRequested())
        countOptions++;

    if (isListRequested())
        countOptions++;
    
    if (isStatRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = "Main options are mutually exclusive.";
        m_exitStatus = BadOptions;
        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = "Main option is required.";
        m_exitStatus = BadOptions;

        return false;
    }

    return true;
}

/**
 * \returns True if the command line options seem to be ok in "tree" mode.
 */
bool
S9sOptions::checkOptionsTree()
{
    int countOptions = 0;

    if (isHelpRequested())
        return true;

    /*
     * Checking if multiple operations are requested.
     */
    if (isTreeRequested())
    {
        countOptions++;
        if (nExtraArguments() > 1) 
        {
            m_errorMessage = 
                "The --tree option enables only one command line argument: "
                "the path to print.";
            m_exitStatus = BadOptions;
            return false;
        }
    }

    if (isMoveRequested())
        countOptions++;

    if (isGetAclRequested())
        countOptions++;
    
    if (isCatRequested())
        countOptions++;
    
    if (isAccessRequested())
        countOptions++;

    if (isAddAclRequested())
        countOptions++;

    if (isChOwnRequested())
        countOptions++;
    
    if (isMkdirRequested())
        countOptions++;
    
    if (isMkfileRequested())
        countOptions++;
    
    if (isRmdirRequested())
        countOptions++;
    
    if (isSaveRequested())
        countOptions++;
 
    if (isRemoveAclRequested())
        countOptions++;
    
    if (isListRequested())
        countOptions++;
    
    if (isDeleteRequested())
        countOptions++;
    
    if (isWatchRequested())
        countOptions++;
    
    if (isStatRequested())
        countOptions++;
    
    if (countOptions > 1)
    {
        m_errorMessage = "Only one of the main options are allowed.";
        m_exitStatus = BadOptions;
        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = "One of the main options is mandatory.";
        m_exitStatus = BadOptions;
        return false;
    }

    return true;
}

bool
S9sOptions::readOptionsNoMode(
        int    argc,
        char  *argv[])
{
    S9S_DEBUG("");
    int           c;
    struct option long_options[] =
    {
        { "help",          no_argument,       0, OptionHelp },
        { "verbose",       no_argument,       0, 'v' },
        { "version",       no_argument,       0, 'V' },
        { 0, 0, 0, 0 }
    };

    for (;;)
    {
        int option_index = 0;
        c = getopt_long(argc, argv, "hvc:V", long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case OptionHelp:
                m_options["help"] = true;
                break;

            case 'v':
                m_options["verbose"] = true;
                break;
            
            case 'V':
                m_options["print-version"] = true;
                break;

            case '?':
            default:
                S9S_WARNING("Unrecognized command line option.");
                {
                    if (isascii(c)) {
                        m_errorMessage.sprintf("Unknown option '%c'.", c);
                    } else {
                        m_errorMessage.sprintf("Unkown option %d.", c);
                    }
                }
                m_exitStatus = BadOptions;
                return false;
        }
    }

    return true;
}

/**
 * \returns The path of the private key of the user. There is a default path
 *   here, so this method will always return a non-empty string.
 *
 * Please note that this is a path based on the --auth-key or --cmon-user
 * command line options. When creating a new user this is probably not what you
 * want.
 */
S9sString
S9sOptions::privateKeyPath() const
{
    if (m_options.contains("private_key_file"))
        return m_options.at("private_key_file").toString();

    S9sString authKey;
    
    authKey = m_userConfig.variableValue("auth_key");

    if (authKey.empty())
        authKey =  m_systemConfig.variableValue("auth_key");

    if (authKey.empty() && !userName().empty())
        authKey.sprintf("~/.s9s/%s.key", STR(userName()));

    return authKey;
}

S9sString
S9sOptions::publicKeyPath() const
{
    return getString("public_key_file");
}

S9sString
S9sOptions::publicKeyName() const
{
    return getString("public_key_name");
}

/**
 * \returns True if the --generate-key command line option was used.
 */
bool
S9sOptions::isGenerateKeyRequested() const
{
    return getBool("generate_key");
}

/**
 * \returns The value of provided by the --group command line option or the
 *   empty string if no such an option is used.
 */
S9sString
S9sOptions::group() const
{
    return getString("group");
}

/**
 * \returns True if the --create-group command line option is used.
 */
bool
S9sOptions::createGroup() const
{
    return getBool("create_group");
}

/**
 * \returns The argument of the --title command line option.
 */
S9sString
S9sOptions::title() const
{
    return getString("title");
}

/**
 * \returns The argument of the --test-server command line option.
 */
S9sString
S9sOptions::testServer() const
{
    return getString("test_server");
}


/**
 * \returns The argument of the --last-name command line option.
 */
S9sString
S9sOptions::lastName() const
{
    return getString("last_name");
}

/**
 * \returns The argument of the --first-name command line option.
 */
S9sString
S9sOptions::firstName() const
{
    return getString("first_name");
}

/**
 * \returns The argument of the --email-address command line option.
 */
S9sString
S9sOptions::emailAddress() const
{
    return getString("email_address");
}

bool 
S9sOptions::getBool(
        const char *key) const
{
    bool retval = false;

    if (m_options.contains(key))
        retval = m_options.at(key).toBoolean();

    return retval;
}

S9sString
S9sOptions::getString(
        const char *key) const
{
    S9sString retval;

    if (m_options.contains(key))
        retval = m_options.at(key).toString();

    return retval;
}

int
S9sOptions::getInt(
        const char *key) const
{
    int retval = 0;

    if (m_options.contains(key))
        retval = m_options.at(key).toInt();

    return retval;
}

S9sVariantMap
S9sOptions::getVariantMap(
        const char *key) const
{
    S9sVariantMap retval;
        
    if (m_options.contains(key))
        retval = m_options.at(key).toVariantMap();

    return retval;
}
