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
#include "s9soptions.h"

#include "config.h"
#include "S9sNode"
#include "S9sFile"
#include "S9sRegExp"
#include "S9sFile"
#include "S9sDir"
#include "s9srsakey.h"
#include "S9sDateTime"

#include <sys/ioctl.h>
#include <stdio.h>
#include <cstdlib>
#include <getopt.h>
#include <stdarg.h>
#include <unistd.h>
#include <cctype>
#include <fnmatch.h>

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sOptions *S9sOptions::sm_instance = 0;

enum S9sOptionType
{
    OptionRpcTls     = 1,
    OptionPrintJson,
    OptionColor,
    OptionConfigFile,
    OptionTop,
    OptionUpdateFreq,
    OptionBatch,
    OptionNoHeader,
    OptionNodes,
    OptionAddNode,
    OptionRemoveNode,
    OptionJobId,
    OptionSet,
    OptionDrop,
    OptionOsUser,
    OptionProviderVersion,
    OptionProperties,
    OptionVendor,
    OptionCreate,
    OptionDelete,
    OptionDbAdmin,
    OptionDbAdminPassword,
    OptionClusterType,
    OptionStop,
    OptionHelp,
    OptionTimeStyle,
    OptionWait,
    OptionRestore,
    OptionBackupId,
    OptionSchedule,
    OptionPing,
    OptionGroup,
    OptionCreateGroup,
    OptionFirstName,
    OptionEmailAddress,
    OptionLastName,
    OptionTitle,
    OptionStart,
    OptionEnd,
    OptionReason,
    OptionUuid,
    OptionDateFormat,
    OptionFullUuid,
    OptionWhoAmI,
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
    S9sDir   userDir("~/.s9s");
    S9sFile  userFile("~/.s9s/s9s.conf");

    if (!userDir.exists())
        userDir.mkdir();

    if (!userDir.exists())
        return;

    if (userFile.exists())
        return;

    userFile.fprintf("[global]\n");
    userFile.fprintf("# controller_host_name = localhost\n");
    userFile.fprintf("# controller_port      = 9500\n");
    userFile.fprintf("# rpc_tls              = false\n");
    userFile.fprintf("\n");

    userFile.fprintf("\n");
    userFile.fprintf("#\n");
    userFile.fprintf("# Information about the user for the controller to \n");
    userFile.fprintf("# access the nodes.\n");
    userFile.fprintf("#\n");
    userFile.fprintf("# os_user     = some_user\n");
    userFile.fprintf("# os_key_file = /home/some_user/.ssh/test_ssh_key\n");
    userFile.fprintf("\n");
}

/**
 * \returns false if there was any error with the configuration file(s), true if
 *   everything went well (even if there are no configuration files).
 */
bool
S9sOptions::loadConfigFiles()
{
    S9sFile userConfig("~/.s9s/s9s.conf");
    S9sFile systemConfig("/etc/s9s.conf");
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
            PRINT_ERROR("The file '%s' does not exists.", STR(configFile()));
            return false;
        }

        success = m_userConfig.parse(STR(content));
        if (!success)
        {
            printError(
                    "Error parsing configuration file '%s': %s",
                    STR(configFile()),
                    STR(m_userConfig.errorString()));

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

        S9S_DEBUG("User config exists.");
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

        S9S_DEBUG("System config exists.");
        success = systemConfig.readTxtFile(content);
        if (!success)
        {
            printError(
                    "Error reading system configuration file: %s",
                    STR(systemConfig.errorString()));

            return false;
        }

        success = m_systemConfig.parse(STR(content));
        if (!success)
        {
            printError(
                    "Error parsing system configuration file: %s",
                    STR(m_systemConfig.errorString()));

            return false;
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

    S9S_DEBUG("*** myUrl  : '%s'", STR(myUrl));
    regexp = "([a-zA-Z]+):\\/\\/(.+)";
    if (regexp == myUrl)
    {
        S9S_DEBUG("MATCH1 '%s', '%s'", 
                STR(regexp[1]), 
                STR(regexp[2]));

        m_options["controller_protocol"] = regexp[1];
        myUrl = regexp[2];
    }
 
    regexp = "(.+):([0-9]+)";
    if (regexp == myUrl)
    {
        m_options["controller"]      = regexp[1];
        m_options["controller_port"] = regexp[2].toInt();
    } else {
        m_options["controller"] = myUrl;
    }
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


/**
 * \returns The value of the --config-file command line option or teh empty
 *   string if
 */
S9sString
S9sOptions::configFile() const
{
    S9sString retval;

    if (m_options.contains("config_file"))
        retval = m_options.at("config_file").toString();

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
void
S9sOptions::setNodes(
        const S9sString &value)
{
    S9sVariantList nodeStrings = value.split(";");
    S9sVariantList nodes;

    for (uint idx = 0; idx < nodeStrings.size(); ++idx)
    {
        S9sString nodeString = nodeStrings[idx].toString();
        S9sNode   node(nodeString.trim());

        nodes << node;
    }

    m_options["nodes"] = nodes;
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

S9sString
S9sOptions::start() const
{
    if (m_options.contains("start"))
        return m_options.at("start").toString();

    return S9sString();
}

S9sString
S9sOptions::end() const
{
    if (m_options.contains("end"))
        return m_options.at("end").toString();

    return S9sString();
}


S9sString
S9sOptions::reason() const
{
    if (m_options.contains("reason"))
        return m_options.at("reason").toString();

    return S9sString();
}

S9sString
S9sOptions::uuid() const
{
    if (m_options.contains("uuid"))
        return m_options.at("uuid").toString();

    return S9sString();
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
 * \returns the value of the --os-user command line option or the user name as
 *   default.
 *
 * The --os-user controls what user name will be used when authenticating on the
 * nodes. This option defaults to the username, that is the username on the
 * localhost running the application.
 */
S9sString
S9sOptions::osUser() const
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

    if (retval.empty())
        retval = userName();

    return retval;
}

/**
 * \returns The file (on the controller) that will be used as SSH key while
 *   authenticating on the nodes with SSH.
 */
S9sString
S9sOptions::osKeyFile() const
{
    S9sString retval;

    retval = m_userConfig.variableValue("os_key_file");

    if (retval.empty())
        retval = m_systemConfig.variableValue("os_key_file");

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
        retval = m_options.at("db_admin_user_name").toString();
    else
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
        retval = m_options.at("db_admin_password").toString();

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
    if (m_options.contains("cluster_type"))
        return m_options.at("cluster_type").toString().toLower();

    return S9sString();
}

S9sString
S9sOptions::formatDateTime(
        S9sDateTime value) const
{
    if (m_options.contains("date_format"))
    {
        return value.toString(m_options.at("date_format").toString());
    }

    // The default date&time format.
    return value.toString(S9sDateTime::CompactFormat);
}

bool
S9sOptions::fullUuid() const
{
    if (m_options.contains("full_uuid"))
        return m_options.at("full_uuid").toBoolean();

    return false;
}

/**
 * \returns the RPC token to be used while communicating with the controller.
 */
S9sString
S9sOptions::rpcToken() const
{
    if (m_options.contains("rpc_token"))
        return m_options.at("rpc_token").toString();

    return S9sString();
}

S9sString
S9sOptions::schedule() const
{
    S9sString retval;

    if (m_options.contains("schedule"))
        retval = m_options.at("schedule").toString();

    return retval;
}

#if 0
bool
S9sOptions::setSchedule(
        const S9sString &value)
{
    m_options["schedule"] = value;
    return true;
}
#endif

/**
 * \returns the cluster ID from the command line or the configuration or
 *   environment 0 if the cluster id is not provided in either of these.
 */
int
S9sOptions::clusterId() const
{
    int retval = 0;

    if (m_options.contains("cluster_id"))
    {
        retval = m_options.at("cluster_id").toInt();
    } else {
        S9sString stringVal = m_userConfig.variableValue("default_cluster_id");

        if (stringVal.empty())
            stringVal = m_systemConfig.variableValue("default_cluster_id");

        if (!stringVal.empty())
            retval = stringVal.toInt();
    }

    return retval;
}

bool
S9sOptions::hasClusterIdOption() const
{
    return m_options.contains("cluster_id");
}

int
S9sOptions::backupId() const
{
    int retval = 0;

    if (m_options.contains("backup_id"))
        retval = m_options.at("backup_id").toInt();

    return retval;
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
        return 3;

    //S9S_DEBUG("-> %d", retval.toInt());
    return retval.toInt();
}

/**
 * \returns the value set by the --cluster-name command line option.
 */
S9sString
S9sOptions::clusterName() const
{
    S9sString retval;

    if (m_options.contains("cluster_name"))
        return m_options.at("cluster_name").toString();

    return retval;
}

/**
 * \returns the job ID as it is set by the --job-id command line option.
 */
int
S9sOptions::jobId() const
{
    if (m_options.contains("job_id"))
        return m_options.at("job_id").toInt();

    return -1;
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

S9sString
S9sOptions::backupDir() const
{
    S9sString  retval;

    retval = m_userConfig.variableValue("backup_directory");
    if (retval.empty())
        retval = m_systemConfig.variableValue("backup_directory");

    return retval;
}

S9sString
S9sOptions::backupMethod() const
{
    S9sString  retval;

    retval = m_userConfig.variableValue("backup_method");
    if (retval.empty())
        retval = m_systemConfig.variableValue("backup_method");

    return retval;
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
 * \returns true if the main operation is "cluster".
 */
bool
S9sOptions::isClusterOperation() const
{
    return m_operationMode == Cluster;
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
 * \returns true if the main operation is "maintenance".
 */
bool
S9sOptions::isMaintenanceOperation() const
{
    return m_operationMode == Maintenance;
}

/**
 * \returns true if the "list" function is requested by providing the --list
 *   command line option.
 */
bool
S9sOptions::isListRequested() const
{
    if (m_options.contains("list"))
        return m_options.at("list").toBoolean();

    return false;
}

/**
 * \returns true if the "whoami" function is requested by providing the 
 * --whoami command line option.
 */
bool
S9sOptions::isWhoAmIRequested() const
{
    if (m_options.contains("whoami"))
        return m_options.at("whoami").toBoolean();

    return false;
}

/**
 * \returns true if the "set" function is requested using the --set command line
 *   option.
 */
bool
S9sOptions::isSetRequested() const
{
    if (m_options.contains("set"))
        return m_options.at("set").toBoolean();

    return false;
}

/**
 * \returns true if the --log command line option was provided when the program
 *   was started.
 */
bool
S9sOptions::isLogRequested() const
{
    if (m_options.contains("log"))
        return m_options.at("log").toBoolean();

    return false;
}

/**
 * \returns true if the --create command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isCreateRequested() const
{
    if (m_options.contains("create"))
        return m_options.at("create").toBoolean();

    return false;
}

/**
 * \returns true if the --delete command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isDeleteRequested() const
{
    if (m_options.contains("delete"))
        return m_options.at("delete").toBoolean();

    return false;
}

bool
S9sOptions::isPingRequested() const
{
    if (m_options.contains("ping"))
        return m_options.at("ping").toBoolean();

    return false;
}

/**
 * \returns true if the --restore command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isRestoreRequested() const
{
    if (m_options.contains("restore"))
        return m_options.at("restore").toBoolean();

    return false;
}

/**
 * \returns true if the --rolling-restart command line option was provided when
 *   the program was started.
 */
bool
S9sOptions::isRollingRestartRequested() const
{
    bool retval = false;

    if (m_options.contains("rolling_restart"))
        retval = m_options.at("rolling_restart").toBoolean();

    return retval;
}

/**
 * \returns true if the add node operation was requested using the "--add-node"
 *   command line option.
 */
bool
S9sOptions::isAddNodeRequested() const
{
    bool retval = false;

    if (m_options.contains("add_node"))
        retval = m_options.at("add_node").toBoolean();

    return retval;
}

/**
 * \returns true if the remove node oparation was requested using the
 *   "--remove-node" command line option.
 */
bool
S9sOptions::isRemoveNodeRequested() const
{
    bool retval = false;

    if (m_options.contains("remove_node"))
        retval = m_options.at("remove_node").toBoolean();

    return retval;
}

/**
 * \returns true if the --drop command line option was provided when the program
 *   was started.
 */
bool
S9sOptions::isDropRequested() const
{
    bool retval = false;

    if (m_options.contains("drop"))
        retval = m_options.at("drop").toBoolean();

    return retval;
}

/**
 * \returns true if the --stop command line option was provided when the program
 *   was started.
 */
bool
S9sOptions::isStopRequested() const
{
    bool retval = false;

    if (m_options.contains("stop"))
        retval = m_options.at("stop").toBoolean();

    return retval;
}

/**
 * \returns true if the --start command line option was provided when the
 * program was started.
 */
bool
S9sOptions::isStartRequested() const
{
    bool retval = false;

    if (m_options.contains("start"))
        retval = m_options.at("start").toBoolean();

    return retval;
}

/**
 * \returns true if the --long command line option was provided.
 */
bool
S9sOptions::isLongRequested() const
{
    if (m_options.contains("long"))
        return m_options.at("long").toBoolean();

    return false;
}

/**
 * \returns true if the --print-json command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isJsonRequested() const
{
    if (m_options.contains("print_json"))
        return m_options.at("print_json").toBoolean();

    return false;
}

/**
 * \returns true if the --top command line option was provided when starting the
 *   program.
 */
bool
S9sOptions::isTopRequested() const
{
    if (m_options.contains("top"))
        return m_options.at("top").toBoolean();

    return false;
}

/**
 * \returns true if the --wait command line option was used.
 */
bool
S9sOptions::isWaitRequested() const
{
    if (m_options.contains("wait"))
        return m_options.at("wait").toBoolean();

    return false;
}

/**
 * \returns true if the --batch command line option was used.
 */
bool
S9sOptions::isBatchRequested() const
{
    if (m_options.contains("batch"))
        return m_options.at("batch").toBoolean();

    return false;
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
    
    if (m_options.contains("no_header"))
        return m_options.at("no_header").toBoolean();

    return false;
}

/**
 * \returns true if the passed string matches the extra command line arguments
 *   (the command line arguments found after the last command line options).
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
 * \returns true if the program should use syntax highlighting in its output.
 */
bool
S9sOptions::useSyntaxHighlight() const
{
    S9sString configValue = "auto";

    if (m_options.contains("color"))
        configValue = m_options.at("color").toString();

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
S9sOptions::humanReadable() const
{
    bool retval = false;

    if (m_options.contains("human_readable"))
        retval = m_options.at("human_readable").toBoolean();

    return retval;
}


S9sString 
S9sOptions::timeStyle() const
{
    S9sString retval;

    if (m_options.contains("time_style"))
        retval = m_options.at("time_style").toString();

    return retval;
}

void
S9sOptions::setHumanReadable(
        const bool value)
{
    m_options["human_readable"] = value;
}


/**
 * \returns How many characters the terminal can show in one line.
 */
int 
S9sOptions::terminalWidth() const
{
    struct winsize win;
    int            retcode;

    retcode = ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);

    if (retcode == 0)
        return win.ws_col;

    return 60;
}

/**
 * \returns How many lines the terminal can show in one screen.
 */
int 
S9sOptions::terminalHeight() const
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
    if (!m_options.contains("verbose"))
        return false;

    return m_options.at("verbose").toBoolean();
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
            break;
        
        case Job:
            retval = readOptionsJob(*argc, argv);
            break;

        case Node:
            retval = readOptionsNode(*argc, argv);
            break;

        case Backup:
            retval = readOptionsBackup(*argc, argv);
            break;
 
        case Process:
            retval = readOptionsProcess(*argc, argv);
            break;

        case User:
            retval = readOptionsUser(*argc, argv);
            break;
        
        case Maintenance:
            retval = readOptionsMaintenance(*argc, argv);
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
        printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
        printf("Copyright (C) 2016...\n");
        printf("\n");
        printf("Written by ...\n");
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
    if (modeName == "cluster") 
    {
        m_operationMode = Cluster;
    } else if (modeName == "node")
    {
        m_operationMode = Node;
    } else if (modeName == "job")
    {
        m_operationMode = Job;
    } else if (modeName == "process")
    {
        m_operationMode = Process;
    } else if (modeName == "backup")
    {
        m_operationMode = Backup;
    } else if (modeName == "maintenance" || modeName == "maint")
    {
        m_operationMode = Maintenance;
    } else if (modeName == "user")
    {
        m_operationMode = User;
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

        case User:
            printHelpUser();
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
"   backup - to view, create and restore database backups.\n"
"  cluster - to list and manipulate clusters.\n"
"      job - to view jobs.\n"
"    maint - to view and manupilate maintenance periods.\n"
"     node - to handle nodes.\n"
"  process - to view processes running on nodes.\n"
"     user - to manage users.\n"
"\n"
"Generic options:\n"
"  -h, --help                 Show help message and exit.\n" 
"  -v, --verbose              Print more messages than normally.\n"
"  -V, --version              Print version information and exit.\n"
"  -c, --controller=URL       The URL where the controller is found.\n"
"  -P, --controller-port INT  The port of the controller.\n"
"  --rpc-tls                  Use TLS encryption to controller.\n"
"  -t, --rpc-token=TOKEN      The RPC authentication token (deprecated).\n"
"\n"
"Formatting:\n"
"  -l, --long                 Print the detailed list.\n"
"  --print-json               Print the sent/received JSon messages.\n"
"  --config-file=PATH         Set the configuration file.\n"
"  --color=always|auto|never  Sets if colors should be used in the output.\n"
"  --batch                    No colors, no human readable, pure data.\n"
"  --no-header                Do not print headers.\n"
"\n"
"Job related options:\n"
"  --wait                     Wait until the job ends.\n"
"  --log                      Wait and monitor job messages.\n"
"  --schedule=DATE&TIME       Run the job at the specified time.\n"
"\n", STR(m_myName));
}

void
S9sOptions::printHelpJob()
{
    printHelpGeneric();

    printf(
"Options for the \"job\" command:\n"
"  --list                     List the jobs.\n"
"\n"
"  --job-id=ID                The ID of the job.\n"
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
"  --update-freq=SECS         The screen update frequency.\n"
"\n"
    );
}

void
S9sOptions::printHelpBackup()
{
    printHelpGeneric();

    printf(
"Options for the \"backup\" command:\n"
"  --list                     List the backups.\n"
"  --create                   Create a new backup.\n"
"  --restore                  Restore an existing backup.\n"
"\n"
"  --cluster-id=ID            The ID of the cluster.\n"
"  --backup-id=ID             The ID of the backup.\n"
"  --nodes=NODELIST           The list of nodes inved in the backup.\n"
    );
}

void
S9sOptions::printHelpMaintenance()
{
    printHelpGeneric();

    printf(
"Options for the \"maintenance\" command:\n"
"  --list                     List the maintenance period.\n"
"  --create                   Create a new maintenance period.\n"
"  --delete                   Delete a maintenance period.\n"
"\n"
"  --cluster-id=ID            The cluster for cluster maintenances.\n"
"  --nodes=NODELIST           The nodes for the node maintenances.\n"
"  --full-uuid                Print the full UUID.\n"
"  --start=DATE&TIME          The start of the maintenance period.\n"
"  --end=DATE&TIME            The end of the maintenance period.\n"
"  --reason=STRING            The reason for the maintenance.\n"
"  --uuid=UUID                The UUID to identify the maintenance period.\n"
"\n"
    );
}

void
S9sOptions::printHelpUser()
{
    printHelpGeneric();

    printf(
"Options for the \"user\" command:\n"
"  --list                     List the users.\n"
"  --whoami                   List the current user only.\n"
"  --create                   Create a new Cmon user.\n"
""
"\n"
"  -u, --cmon-user=USERNAME   The username on the Cmon system.\n"
"  -g, --generate-key         Generate an RSA keypair for the user.\n"
"  --group=GROUP_NAME         The primary group for the new user.\n"
"  --create-group             Create the group if it doesn't exist.\n"
"  --first-name=NAME          The first name of the user.\n"
"  --last-name=NAME           The last name of the user.\n"
"  --title=TITLE              The prefix title for the user.\n"
"  --email-address=ADDRESS    The email address for the user.\n"
"\n");
}

void 
S9sOptions::printHelpCluster()
{
    printHelpGeneric();

    printf(
"Options for the \"cluster\" command:\n"
"  --list                     List the users.\n"
"  --create                   Create and install a new cluster.\n"
"  --ping                     Check the connection to the controller.\n"
"  --rolling-restart          Restart the nodes without stopping the cluster.\n"
"  --add-node                 Add a new node to the cluster.\n"
"  --remove-node              Remove a node from the cluster.\n"
"  --drop                     Drop cluster from the controller.\n"
"  --stop                     Stop the cluster.\n"
"  --start                    Start the cluster.\n"
"\n"
"  --cluster-id=ID            The ID of the cluster to manipulate.\n"
"  --cluster-name=NAME        Name of the cluster to manipulate or create.\n"
"  --nodes=NODE_LIST          List of nodes to work with.\n"
"  --vendor=VENDOR            The name of the software vendor.\n"
"  --provider-version=VER     The version of the software.\n"
"  --os-user=USERNAME         The name of the user for the SSH commands.\n"
"  --cluster-type=TYPE        The type of the cluster to install.\n"
"  --db-adnim=USERNAME        The database admin user name.\n"
"  --db-admin-passwd=PASSWD   The pasword for the database admin.\n"
"\n");
}

void 
S9sOptions::printHelpNode()
{
    printHelpGeneric();

    printf(
"Options for the \"node\" command:\n"
"  --list                     List the jobs found on the controller.\n"
"  --set                      Change the properties of a node.\n"
"\n"
"  --cluster-id=ID            The ID of the cluster in which the node is.\n"
"  --cluster-name=NAME        Name of the cluster to list.\n"
"  --nodes=NODE_LIST          The nodes to list or manipulate.\n"
"  --properties=ASSIGNMENTS   The names and values of the properties to change.\n"
"\n");
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
        { "help",             no_argument,       0, OptionHelp        },
        { "verbose",          no_argument,       0, 'v'               },
        { "version",          no_argument,       0, 'V'               },
        { "controller",       required_argument, 0, 'c'               },
        { "controller-port",  required_argument, 0, 'P'               },
        { "rpc-tls",          no_argument,       0, OptionRpcTls      },
        { "rpc-token",        required_argument, 0, 't'               },
        { "long",             no_argument,       0, 'l'               },
        { "print-json",       no_argument,       0, OptionPrintJson   },
        { "color",            optional_argument, 0, OptionColor       },
        { "config-file",      required_argument, 0,  4                },
        { "batch",            no_argument,       0, OptionBatch       },
        { "no-header",        no_argument,       0, OptionNoHeader    },

        // Main Option
        { "list",             no_argument,       0, 'L'               },
        { "set",              no_argument,       0,  OptionSet        },

        // Cluster information
        { "cluster-id",       required_argument, 0, 'i'               },
        { "cluster-name",     required_argument, 0, 'n'               },
        { "nodes",            required_argument, 0, OptionNodes       },

        // Node options. 
        { "properties",       required_argument, 0, OptionProperties  },

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

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;

            case 'c':
                // -c, --controller
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;

            case 't':
                // -t, --token
                m_options["rpc_token"] = optarg;
                break;
            
            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case 'L': 
                // --list
                m_options["list"] = true;
                break;

            case OptionSet:
                // --set
                m_options["set"]  = true;
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
                setNodes(optarg);
                break;

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
        { "help",             no_argument,       0, OptionHelp },
        { "verbose",          no_argument,       0, 'v' },
        { "version",          no_argument,       0, 'V' },
        { "controller",       required_argument, 0, 'c' },
        { "controller-port",  required_argument, 0, 'P' },
        { "rpc-tls",          no_argument,       0, OptionRpcTls  },
        { "rpc-token",        required_argument, 0, 't' },
        { "long",             no_argument,       0, 'l' },
        { "print-json",       no_argument,       0, OptionPrintJson  },
        { "color",            optional_argument, 0, OptionColor      },
        { "human-readable",   no_argument,       0, 'h' },
        { "time-style",       required_argument, 0, OptionTimeStyle   },
        { "config-file",      required_argument, 0, OptionConfigFile },

        // Main Option
        { "list",             no_argument,       0, 'L' },
        { "create",           no_argument,       0,  OptionCreate     },
        { "restore",          no_argument,       0,  OptionRestore    },
        
        // Job Related Options
        { "wait",             no_argument,       0, OptionWait        },
        { "log",              no_argument,       0, 'G'               },
        { "batch",            no_argument,       0, OptionBatch       },
        { "no-header",        no_argument,       0, OptionNoHeader    },

        // Cluster information
        { "cluster-id",       required_argument, 0, 'i' },
        { "backup-id",        required_argument, 0, OptionBackupId    },
        { "nodes",            required_argument, 0, OptionNodes       },
        { "schedule",         required_argument, 0, OptionSchedule    },

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

            case 'v':
                // -v, --verbose
                m_options["verbose"] = true;
                break;
            
            case 'V':
                // -V, --version
                m_options["print-version"] = true;
                break;

            case 'c':
                // -c, --controller
                setController(optarg);
                break;

            case 'P':
                // -P, --controller-port=PORT
                m_options["controller_port"] = atoi(optarg);
                break;

            case 't':
                // -t, --token
                m_options["rpc_token"] = optarg;
                break;
            
            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case 'L': 
                // --list
                m_options["list"] = true;
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
            
            case OptionCreate:
                // --create
                m_options["create"] = true;
                break;

            case OptionRestore:
                // --restore
                m_options["restore"] = true;
                break;

            case OptionConfigFile:
                // --config-file=FILE
                m_options["config-file"] = optarg;
                break;
            
            case OptionColor:
                // --color=COLOR
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case 'h':
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
            
            case OptionNodes:
                // --nodes=LIST
                setNodes(optarg);
                break;

            case OptionSchedule:
                // --schedule=DATETIME
                m_options["schedule"] = optarg;
                break;

            case OptionBackupId:
                // --backup-id=BACKUPID
                m_options["backup_id"] = atoi(optarg);
                break;

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
 * Reads the command line options in the "process" mode.
 */
bool
S9sOptions::readOptionsProcess(
        int    argc,
        char  *argv[])
{
    S9S_DEBUG("");
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, OptionHelp },
        { "verbose",          no_argument,       0, 'v' },
        { "version",          no_argument,       0, 'V' },
        { "controller",       required_argument, 0, 'c' },
        { "controller-port",  required_argument, 0, 'P' },
        { "rpc-tls",          no_argument,       0, OptionRpcTls },
        { "rpc-token",        required_argument, 0, 't' },
        { "long",             no_argument,       0, 'l' },
        { "print-json",       no_argument,       0,  OptionPrintJson },
        { "color",            optional_argument, 0,  OptionColor },
        { "config-file",      required_argument, 0,  OptionConfigFile },

        // Main Option
        { "list",             no_argument,       0, 'L' },
        { "top",              no_argument,       0,  OptionTop },

        // Cluster information
        { "cluster-id",       required_argument, 0, 'i' },
        { "update-freq",      required_argument, 0,  OptionUpdateFreq },

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

            case 't':
                // -t, --token=RPC_TOKEN
                m_options["rpc_token"] = optarg;
                break;
            
            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case 'L': 
                // --list
                m_options["list"] = true;
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
 * Reads the command line options in the "user" mode.
 */
bool
S9sOptions::readOptionsUser(
        int    argc,
        char  *argv[])
{
    S9S_DEBUG("");
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, 'h'                },
        { "verbose",          no_argument,       0, 'v'                },
        { "version",          no_argument,       0, 'V'                },
        { "controller",       required_argument, 0, 'c'                },
        { "controller-port",  required_argument, 0, 'P'                },
        { "rpc-tls",          no_argument,       0, OptionRpcTls       },
        { "rpc-token",        required_argument, 0, 't'                },
        { "long",             no_argument,       0, 'l'                },
        { "print-json",       no_argument,       0, OptionPrintJson    },
        { "color",            optional_argument, 0, OptionColor        },
        { "config-file",      required_argument, 0, OptionConfigFile   },
        { "batch",            no_argument,       0, OptionBatch        },
        { "no-header",        no_argument,       0, OptionNoHeader     },

        // Main Option
        { "generate-key",     no_argument,       0, 'g'                }, 
        { "cmon-user",        required_argument, 0, 'u'                }, 
        { "list",             no_argument,       0, 'L'                },
        { "whoami",           no_argument,       0, OptionWhoAmI       },
        { "create",           no_argument,       0, OptionCreate       },
        { "set",              no_argument,       0, OptionSet          },
       
        // Options about the user.
        { "group",            required_argument, 0, OptionGroup        },
        { "create-group",     no_argument,       0, OptionCreateGroup  },
        { "first-name",       required_argument, 0, OptionFirstName    },
        { "last-name",        required_argument, 0, OptionLastName     },
        { "title",            required_argument, 0, OptionTitle        },
        { "email-address",    required_argument, 0, OptionEmailAddress },

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
            case 'h':
                // -h, --help
                m_options["help"] = true;
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

            case 't':
                // -t, --token=RPC_TOKEN
                m_options["rpc_token"] = optarg;
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
                // --cmon-user
                m_options["cmon_user"] = optarg;
                break;

            case 'g':
                // --generate-key
                m_options["generate_key"] = true;
                break;

            case OptionCreate:
                // --create
                m_options["create"] = true;
                break;
 
            case 'L': 
                // --list
                m_options["list"] = true;
                break;

            case OptionWhoAmI:
                // --whoami
                m_options["whoami"] = true;
                break;

            case OptionSet:
                // --set
                m_options["set"]  = true;
                break;
            
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
        { "help",             no_argument,       0, 'h'               },
        { "verbose",          no_argument,       0, 'v'               },
        { "version",          no_argument,       0, 'V'               },
        { "controller",       required_argument, 0, 'c'               },
        { "controller-port",  required_argument, 0, 'P'               },
        { "rpc-tls",          no_argument,       0, OptionRpcTls      },
        { "rpc-token",        required_argument, 0, 't'               },
        { "long",             no_argument,       0, 'l'               },
        { "print-json",       no_argument,       0, OptionPrintJson   },
        { "color",            optional_argument, 0, OptionColor       },
        { "config-file",      required_argument, 0, OptionConfigFile  },
        { "batch",            no_argument,       0, OptionBatch       },
        { "no-header",        no_argument,       0, OptionNoHeader    },
        { "date-format",      required_argument, 0, OptionDateFormat  },
        { "full-uuid",        no_argument,       0, OptionFullUuid    },

        // Main Option
        { "list",             no_argument,       0, 'L'               },
        { "create",           no_argument,       0, OptionCreate      },
        { "delete",           no_argument,       0, OptionDelete      },
       
        // Options about the maintenance period.
        { "cluster-id",       required_argument, 0, 'i'               },
        { "nodes",            required_argument, 0, OptionNodes       },
        { "start",            required_argument, 0, OptionStart       },
        { "end",              required_argument, 0, OptionEnd         },
        { "reason",           required_argument, 0, OptionReason      },
        { "uuid",             required_argument, 0, OptionUuid        },

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
            case 'h':
                // -h, --help
                m_options["help"] = true;
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

            case 't':
                // -t, --token=RPC_TOKEN
                m_options["rpc_token"] = optarg;
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
                setNodes(optarg);
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
 * Reads the command line options in cluster mode.
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
        { "help",             no_argument,       0, OptionHelp       },
        { "verbose",          no_argument,       0, 'v'              },
        { "version",          no_argument,       0, 'V'              },
        { "controller",       required_argument, 0, 'c'              },
        { "controller-port",  required_argument, 0, 'P'              },
        { "rpc-tls",          no_argument,       0,  OptionRpcTls    },
        { "rpc-token",        required_argument, 0, 't'              },
        { "long",             no_argument,       0, 'l'              },
        { "print-json",       no_argument,       0, OptionPrintJson  },
        { "color",            optional_argument, 0, OptionColor      },
        { "config-file",      required_argument, 0, OptionConfigFile },

        // Main Option
        { "ping",             no_argument,       0, OptionPing       },
        { "list",             no_argument,       0, 'L'              },
        { "create",           no_argument,       0, OptionCreate     },
        { "rolling-restart",  no_argument,       0, 12               },
        { "add-node",         no_argument,       0, OptionAddNode    },
        { "remove-node",      no_argument,       0, OptionRemoveNode },
        { "drop",             no_argument,       0, OptionDrop       },
        { "stop",             no_argument,       0, OptionStop       },
        { "start",            no_argument,       0, OptionStart      },

        // Job Related Options
        { "wait",             no_argument,       0, OptionWait       },
        { "log",              no_argument,       0, 'G'              },
        { "batch",            no_argument,       0, OptionBatch      },
        { "no-header",        no_argument,       0, OptionNoHeader   },
        { "schedule",         required_argument, 0, OptionSchedule   },


        // Cluster information.
        // http://52.58.107.236/cmon-docs/current/cmonjobs.html#mysql
        // https://docs.google.com/document/d/1hvPtdWJqLeu1bAk-ZiWsILtj5dLXSLmXUyJBiP7wKjk/edit#heading=h.xsnzbjxs2gss
        { "cluster-id",       required_argument, 0, 'i' },
        { "cluster-name",     required_argument, 0, 'n' },
        { "nodes",            required_argument, 0,  OptionNodes },
        { "vendor",           required_argument, 0, OptionVendor  },
        { "provider-version", required_argument, 0, OptionProviderVersion },
        { "os-user",          required_argument, 0, OptionOsUser  },
        { "cluster-type",     required_argument, 0, OptionClusterType },
        { "db-admin",         required_argument, 0, OptionDbAdmin },
        { "db-admin-passwd",  required_argument, 0, OptionDbAdminPassword },

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

            case 't':
                // -t, --rpc-token=TOKEN
                m_options["rpc_token"] = optarg;
                break;

            case 'l':
                // -l, --long
                m_options["long"] = true;
                break;

            case 'L': 
                // -L, --list
                m_options["list"] = true;
                break;
            
            case 12:
                // --rolling-restart
                m_options["rolling_restart"] = true;
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

            case OptionPing:
                // --ping
                m_options["ping"] = true;
                break;
            
            case OptionCreate:
                // --create
                m_options["create"] = true;
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
                setNodes(optarg);
                break;

            case OptionVendor:
                // --vendor=STRING
                m_options["vendor"] = optarg;
                break;

            case OptionProviderVersion:
                // --provider-version=STRING
                m_options["provider_version"] = optarg;
                break;

            case OptionOsUser:
                // --os-user
                m_options["os_user"] = optarg;
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
        { "help",             no_argument,       0, OptionHelp },
        { "verbose",          no_argument,       0, 'v' },
        { "version",          no_argument,       0, 'V' },
        { "controller",       required_argument, 0, 'c' },
        { "controller-port",  required_argument, 0, 'P' },
        { "rpc-tls",          no_argument,       0,  6  },
        { "rpc-token",        required_argument, 0, 't' },
        { "long",             no_argument,       0, 'l' },
        { "print-json",       no_argument,       0,  OptionPrintJson  },
        { "config-file",      required_argument, 0,  OptionConfigFile },
        { "color",            optional_argument, 0,  OptionColor      },
        { "date-format",      required_argument, 0,  OptionDateFormat },

        // Main Option
        { "wait",             no_argument,       0,  5  },
        { "log",              no_argument,       0, 'G' },
        { "list",             no_argument,       0, 'L' },

        // Job Related Options
        { "cluster-id",       required_argument, 0, 'i' },
        { "job-id",           required_argument, 0, OptionJobId  },

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
                // -P, --controller-port
                m_options["controller_port"] = atoi(optarg);
                break;

            case 't':
                // -t, --rpc-token=TOKEN
                m_options["rpc_token"] = optarg;
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

            case OptionJobId:
                // --job-id=ID
                m_options["job_id"] = atoi(optarg);
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

            case '?':
                // 
                return false;

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

S9sString
S9sOptions::privateKeyPath() const
{
    if (m_options.contains("auth_key"))
        return m_options.at("auth_key").toString();

    S9sString authKey;
    
    authKey = m_userConfig.variableValue("auth_key");

    if (authKey.empty())
        authKey =  m_systemConfig.variableValue("auth_key");

    if (authKey.empty() && !userName().empty())
        authKey.sprintf("~/.s9s/%s.key", STR(userName()));

    return authKey;
}

bool
S9sOptions::isGenerateKeyRequested() const
{
    if (m_options.contains("generate_key"))
        return m_options.at("generate_key").toBoolean();

    return false;
}

/**
 * \returns The value of provided by the --group command line option or the
 *   empty string if no such an option is used.
 */
S9sString
S9sOptions::group() const
{
    if (m_options.contains("group"))
        return m_options.at("group").toString();

    return S9sString();
}

/**
 * \returns True if the --create-group command line option is used.
 */
bool
S9sOptions::createGroup() const
{
    if (m_options.contains("create_group"))
        return m_options.at("create_group").toBoolean();

    return false;
}

S9sString
S9sOptions::title() const
{
    if (m_options.contains("title"))
        return m_options.at("title").toString();

    return S9sString();
}

S9sString
S9sOptions::lastName() const
{
    if (m_options.contains("last_name"))
        return m_options.at("last_name").toString();

    return S9sString();
}

S9sString
S9sOptions::firstName() const
{
    if (m_options.contains("first_name"))
        return m_options.at("first_name").toString();

    return S9sString();
}

S9sString
S9sOptions::emailAddress() const
{
    if (m_options.contains("email_address"))
        return m_options.at("email_address").toString();

    return S9sString();
}
