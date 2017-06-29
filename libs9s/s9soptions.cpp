/*
 * Severalnines Tools
 * Copyright (C) 2017  Severalnines AB
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
//#define WARNING
#include "s9sdebug.h"

S9sOptions *S9sOptions::sm_instance = 0;

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
    OptionAddNode,
    OptionRemoveNode,
    OptionJobId,
    OptionSet,
    OptionChangePassword,
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
    OptionRestart,
    OptionEnd,
    OptionReason,
    OptionUuid,
    OptionDateFormat,
    OptionFullUuid,
    OptionWhoAmI,
    OptionListProperties,
    OptionType,
    OptionBackupMethod,
    OptionBackupDirectory,
    OptionNoCompression,
    OptionUsePigz,
    OptionOnNode,
    OptionDatabases,
    OptionParallellism,
    OptionFullPath,
    OptionStat,
    OptionCreateAccount,
    OptionGrant,
    OptionDeleteAccount,
    OptionCreateDatabase,
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
    OptionTree,
    OptionOutputDir,
    OptionLogFormat,
    OptionFrom,
    OptionUntil,
    OptionForce,
    OptionDebug,
    OptionClusterFormat,
    OptionNodeFormat,
    OptionBackupFormat,
    OptionUserFormat,
    OptionGraph,
    OptionBegin,
    OptionOnlyAscii,
    OptionDensity,
    OptionRollingRestart,
    OptionCreateReport,
    OptionLimit,
    OptionOffset,
    OptionRegister,
    OptionOldPassword,
    OptionNewPassword,
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
    m_modes["cluster"]     = Cluster;
    m_modes["node"]        = Node;
    m_modes["job"]         = Job;
    m_modes["backup"]      = Backup;
    m_modes["maintenance"] = Maintenance;
    m_modes["user"]        = User;
    m_modes["metatype"]    = MetaType;
    m_modes["script"]      = Script;

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
 * \returns The value of the --config-file command line option or teh empty
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
    const char *key = "only_ascii";
    S9sString   retval;

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

/**
 * \returns True if the --density command line option was provided.
 */
bool
S9sOptions::density() const
{
    return getBool("density");
}

/**
 * \returns The value for the "brief_job_log_format" config variable that
 *   controls the format of the log lines printed when the --long option is not
 *   provided.
 */
S9sString 
S9sOptions::briefLogFormat() const
{
    const char *key = "brief_log_format";
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
    S9sVariantList nodeStrings = value.split(";");
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
    return getString("db_admin_password");
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

bool
S9sOptions::fullUuid() const
{
    return getBool("full_uuid");
}

/**
 * \returns the RPC token to be used while communicating with the controller.
 */
S9sString
S9sOptions::rpcToken() const
{
    return getString("rpc_token");
}

S9sString
S9sOptions::schedule() const
{
    return getString("schedule");
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

/**
 * \returns true if the --cluster-id command line option was provided.
 */
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

S9sString
S9sOptions::password() const
{
    return getString("password");
}

bool 
S9sOptions::hasOldPassword() const
{
    return m_options.contains("old_password");
}

S9sString
S9sOptions::oldPassword() const
{
    return getString("old_password");
}

bool 
S9sOptions::hasNewPassword() const
{
    return m_options.contains("new_password");
}

S9sString
S9sOptions::newPassword() const
{
    return getString("new_password");
}

bool
S9sOptions::hasPassword() const
{
    return m_options.contains("password");
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
 * \return True if the --force command line option is provided.
 */
bool
S9sOptions::force() const
{
    return getBool("force");
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

/**
 * \returns true if the --no-compression command line option was provided.
 */
bool
S9sOptions::noCompression() const
{
    return getBool("no_compression");
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

/**
 * \returns true if the --full-path command line option was provided.
 */
bool
S9sOptions::fullPathRequested() const
{
    return getBool("full_path");
}

/**
 * \returns true if the main operation is "node".
 */
bool
S9sOptions::isNodeOperation() const
{
    return m_operationMode == Node;
}

bool
S9sOptions::isLogOperation() const
{
    return m_operationMode == Log;
}

/**
 * \returns true if the main operation is "script".
 */
bool
S9sOptions::isScriptOperation() const
{
    return m_operationMode == Script;
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

/**
 * \returns true if the --stat command line option was provided to get a
 *   detailed list of something
 */
bool
S9sOptions::isStatRequested() const
{
    return getBool("stat");
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
 * \returns true if the "whoami" function is requested by providing the 
 * --whoami command line option.
 */
bool
S9sOptions::isWhoAmIRequested() const
{
    return getBool("whoami");
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
 * \returns true if the --execute command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isExecuteRequested() const
{
    return getBool("execute");
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
 * \returns true if the --delete command line option was provided when the
 *   program was started.
 */
bool
S9sOptions::isDeleteRequested() const
{
    return getBool("delete");
}

bool
S9sOptions::isPingRequested() const
{
    return getBool("ping");
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
 * \returns true if the --rolling-restart command line option was provided when
 *   the program was started.
 */
bool
S9sOptions::isRollingRestartRequested() const
{
    return getBool("rolling_restart");
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
 * \returns true if the --create-account command line option was provided when
 *   the program was started.
 */
bool
S9sOptions::isGrantRequested() const
{
    return getBool("grant");
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

uint
S9sOptions::nExtraArguments() const
{
    return m_extraArguments.size();
}

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
    return getBool("human_readable");
}


S9sString 
S9sOptions::timeStyle() const
{
    return getString("time_style");
}

void
S9sOptions::setHumanReadable(
        const bool value)
{
    m_options["human_readable"] = value;
}

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
    return getBool("verbose");
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
        
        case Maintenance:
            retval = readOptionsMaintenance(*argc, argv);
            
            if (retval)
                retval = checkOptionsMaintenance();

            break;
        
        case MetaType:
            retval = readOptionsMetaType(*argc, argv);
            break;
        
        case Script:
            retval = readOptionsScript(*argc, argv);
            
            if (retval)
                retval = checkOptionsScript();

            break;

        case Log:
            retval = readOptionsLog(*argc, argv);
            
            if (retval)
                retval = checkOptionsLog();

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
        printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
        printf("Copyright (C) 2016-2017 Severalnines AB\n");
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
    } else if (modeName == "metatype")
    {
        m_operationMode = MetaType;
    } else if (modeName == "script")
    {
        m_operationMode = Script;
    } else if (modeName == "log")
    {
        m_operationMode = Log;
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
        
        case MetaType:
            printHelpMetaType();
            break;

        case User:
            printHelpUser();
            break;
        
        case Script:
            printHelpScript();
            break;

        case Log:
            // Missing help
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
"    maint - to view and manipulate maintenance periods.\n"
" metatype - to print metatype information.\n"
"     node - to handle nodes.\n"
"  process - to view processes running on nodes.\n"
"   script - to manage and execute scripts.\n"
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
"  --date-format=FORMAT       The format of the dates printed.\n"
"  --job-id=ID                The ID of the job.\n"
"  -u, --cmon-user=USERNAME   The username on the Cmon system.\n"
"  -p, --password=PASSWORD    The password for the Cmon user.\n"
"\n"
"  --from=DATE&TIME           The start of the interval to be printed.\n"
"  --limit=NUMBER             Controls how many jobs are printed max.\n"
"  --offset=NUMBER            Controls the index of the first item printed.\n"
"  --until=DATE&TIME          The end of the interval to be printed.\n"
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
"  -u, --cmon-user=USERNAME   The username on the Cmon system.\n"
"  -p, --password=PASSWORD    The password for the Cmon user.\n"
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
"  --create                   Create a new backup.\n"
"  --delete                   Delete a previously created backup.\n"
"  --list                     List the backups.\n"
"  --restore                  Restore an existing backup.\n"
"\n"
"  --backup-id=ID             The ID of the backup.\n"
"  --cluster-id=ID            The ID of the cluster.\n"
"  --nodes=NODELIST           The list of nodes involved in the backup.\n"
"  -u, --cmon-user=USERNAME   The username on the Cmon system.\n"
"  -p, --password=PASSWORD    The password for the Cmon user.\n"
"\n"
"  --backup-directory=DIR     The directory where the backup is placed.\n"
"  --backup-format            The format string used while printing backups.\n"
"  --backup-method=METHOD     Defines the backup program to be used.\n"
"  --databases=LIST           Comma separated list of databases to archive.\n"
"  --date-format=FORMAT       The format of the dates printed.\n"
"  --full-path                Print the full path of the files.\n"
"  --no-compression           Do not compress the archive file.\n"
"  --on-node                  Store the archive file on the node itself.\n"
"  --parallellism=N           Number of threads used while creating backup.\n"
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
"  --list                     List the maintenance period.\n"
"  --create                   Create a new maintenance period.\n"
"  --delete                   Delete a maintenance period.\n"
"\n"
"  --cluster-id=ID            The cluster for cluster maintenances.\n"
"  --date-format=FORMAT       The format of the dates printed.\n"
"  --end=DATE&TIME            The end of the maintenance period.\n"
"  --full-uuid                Print the full UUID.\n"
"  --nodes=NODELIST           The nodes for the node maintenances.\n"
"  --reason=STRING            The reason for the maintenance.\n"
"  --start=DATE&TIME          The start of the maintenance period.\n"
"  -u, --cmon-user=USERNAME   The username on the Cmon system.\n"
"  -p, --password=PASSWORD    The password for the Cmon user.\n"
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
"  --change-password          Change the password for an existing user.\n"
"  --create                   Create a new Cmon user.\n"
"  --list                     List the users.\n"
"  --set                      Change the properties of a user.\n"
"  --whoami                   List the current user only.\n"
""
"\n"
"  -u, --cmon-user=USERNAME   The username on the Cmon system.\n"
"  -p, --password=PASSWORD    The password for the Cmon user.\n"
"  -g, --generate-key         Generate an RSA keypair for the user.\n"
"  --group=GROUP_NAME         The primary group for the new user.\n"
"  --create-group             Create the group if it doesn't exist.\n"
"  --first-name=NAME          The first name of the user.\n"
"  --last-name=NAME           The last name of the user.\n"
"  --title=TITLE              The prefix title for the user.\n"
"  --email-address=ADDRESS    The email address for the user.\n"
"  --user-format=FORMAT       The format string used to print users.\n"
"\n");
}

void 
S9sOptions::printHelpCluster()
{
    printHelpGeneric();

    printf(
"Options for the \"cluster\" command:\n"
"  --add-node                 Add a new node to the cluster.\n"
"  --create-account           Create a user account on the cluster.\n"
"  --create                   Create and install a new cluster.\n"
"  --create-database          Create a database on the cluster.\n"
"  --create-report            Starts a job that will create a report.\n"
"  --delete-account           Delete a user account on the cluster.\n"
"  --drop                     Drop cluster from the controller.\n"
"  --list                     List the clusters.\n"
"  --ping                     Check the connection to the controller.\n"
"  --remove-node              Remove a node from the cluster.\n"
"  --rolling-restart          Restart the nodes without stopping the cluster.\n"
"  --start                    Start the cluster.\n"
"  --stat                     Print the details of a cluster.\n"
"  --stop                     Stop the cluster.\n"
"\n"
"  --account=NAME[:PASSWD][@HOST] Account to be created on the cluster.\n"
"  --cluster-format=FORMAT    The format string used to print clusters.\n"
"  --cluster-id=ID            The ID of the cluster to manipulate.\n"
"  --cluster-name=NAME        Name of the cluster to manipulate or create.\n"
"  --cluster-type=TYPE        The type of the cluster to install. Currently\n"
"  --db-admin-passwd=PASSWD   The password for the database admin.\n"
"  --db-admin=USERNAME        The database admin user name.\n"
"  --db-name=NAME             The name of the database.\n"
"    groupreplication (or group_replication), ndb (or ndbcluster) and\n"
"  --nodes=NODE_LIST          List of nodes to work with.\n"
"  --opt-group=NAME           The option group for configuration.\n"
"  --opt-name=NAME            The name of the configuration item.\n"
"  --opt-value=VALUE          The value for the configuration item.\n"
"  --os-user=USERNAME         The name of the user for the SSH commands.\n"
"  --output-dir=DIR           The directory where the files are created.\n"
"    postgresql.\n"
"  --provider-version=VER     The version of the software.\n"
"    the following types are supported: galera, mysqlreplication,\n"
"  -u, --cmon-user=USERNAME   The username on the Cmon system.\n"
"  -p, --password=PASSWORD    The password for the Cmon user.\n"
"  --vendor=VENDOR            The name of the software vendor.\n"
"  --with-database            Create a database for the user too.\n"
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
"\n"
"  --cluster-id=ID            The ID of the cluster in which the node is.\n"
"  --cluster-name=NAME        Name of the cluster to list.\n"
"  --nodes=NODE_LIST          The nodes to list or manipulate.\n"
"  -u, --cmon-user=USERNAME   The username on the Cmon system.\n"
"  -p, --password=PASSWORD    The password for the Cmon user.\n"
"\n"
"  --begin=TIMESTAMP          The start of the graph interval.\n"
"  --end=TIMESTAMP            The end of teh graph interval.\n"
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
"  --execute                  Execute a script.\n"
"  --tree                     Print the scripts available on the controller.\n"
"\n"
"  -u, --cmon-user=USERNAME   The username on the Cmon system.\n"
"  -p, --password=PASSWORD    The password for the Cmon user.\n"
"  --cluster-id=ID            The cluster for cluster maintenances.\n"
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
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "rpc-token",        required_argument, 0, 't'                   },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0,  4                    },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        { "only-ascii",       no_argument,       0, OptionOnlyAscii       },
        { "density",          no_argument,       0, OptionDensity         },

        // Main Option
        { "list",             no_argument,       0, 'L'                   },
        { "stat",             no_argument,       0,  OptionStat           },
        { "set",              no_argument,       0,  OptionSet            },
        { "start",            no_argument,       0,  OptionStart          },
        { "stop",             no_argument,       0,  OptionStop           },
        { "restart",          no_argument,       0,  OptionRestart        },
        { "list-config",      no_argument,       0,  OptionListConfig     },
        { "change-config",    no_argument,       0,  OptionChangeConfig   },
        { "pull-config",      no_argument,       0,  OptionPullConfig     },
        { "push-config",      no_argument,       0,  OptionPushConfig     },

        // Cluster information
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "cluster-name",     required_argument, 0, 'n'                   },
        { "nodes",            required_argument, 0, OptionNodes           },
        
        // Job Related Options
        { "wait",             no_argument,       0, OptionWait            },
        { "log",              no_argument,       0, 'G'                   },
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        { "schedule",         required_argument, 0, OptionSchedule        },
        { "force",            no_argument,       0, OptionForce           },

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
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "rpc-token",        required_argument, 0, 't'                   },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "human-readable",   no_argument,       0, 'h'                   },
        { "time-style",       required_argument, 0, OptionTimeStyle       },
        { "config-file",      required_argument, 0, OptionConfigFile      },
        { "date-format",      required_argument, 0, OptionDateFormat      },

        // Main Option
        { "list",             no_argument,       0, 'L'                   },
        { "create",           no_argument,       0,  OptionCreate         },
        { "restore",          no_argument,       0,  OptionRestore        },
        { "delete",           no_argument,       0,  OptionDelete         },
        
        // Job Related Options
        { "wait",             no_argument,       0, OptionWait            },
        { "log",              no_argument,       0, 'G'                   },
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },

        // Cluster information
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "cluster-name",     required_argument, 0, 'n'                   },
        { "backup-id",        required_argument, 0, OptionBackupId        },
        { "nodes",            required_argument, 0, OptionNodes           },
        { "schedule",         required_argument, 0, OptionSchedule        },

        // Backup info
        { "backup-method",    required_argument, 0, OptionBackupMethod    },
        { "backup-directory", required_argument, 0, OptionBackupDirectory },
        { "no-compression",   no_argument,       0, OptionNoCompression   },
        { "use-pigz",         no_argument,       0, OptionUsePigz         },
        { "on-node",          no_argument,       0, OptionOnNode          },
        { "databases",        required_argument, 0, OptionDatabases       },
        { "parallellism",     required_argument, 0, OptionParallellism    },
        { "full-path",        no_argument,       0, OptionFullPath        },
        { "backup-format",    required_argument, 0, OptionBackupFormat    }, 
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

            case OptionDelete:
                // --delete
                m_options["delete"]  = true;
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

            case OptionBackupId:
                // --backup-id=BACKUPID
                m_options["backup_id"] = atoi(optarg);
                break;
           
            case OptionBackupMethod:
                // --backup-method=METHOD
                m_options["backup_method"] = optarg;
                break;

            case OptionBackupDirectory:
                // --backup-directory=DIRECTORY
                m_options["backup_directory"] = optarg;
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

            case OptionDatabases:
                // --databases=LIST
                m_options["databases"] = optarg;
                break;

            case OptionParallellism:
                // --parallellism=N
                if (!setParallellism(optarg))
                    return false;

                break;

            case OptionFullPath:
                // full-path
                m_options["full_path"] = true;
                break;

            case OptionBackupFormat:
                // --backup-format=VALUE
                m_options["backup_format"] = optarg;
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
 * Reads the command line options in "node" mode.
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
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "rpc-token",        required_argument, 0, 't'                   },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0,  4                    },
        { "no-header",        no_argument,       0, OptionNoHeader        },

        // Main Option
        { "list",             no_argument,       0, 'L'                   },
        { "stat",             no_argument,       0,  OptionStat           },
        { "set",              no_argument,       0,  OptionSet            },
        { "start",            no_argument,       0,  OptionStart          },
        { "stop",             no_argument,       0,  OptionStop           },
        { "restart",          no_argument,       0,  OptionRestart        },
        { "list-config",      no_argument,       0,  OptionListConfig     },
        { "change-config",    no_argument,       0,  OptionChangeConfig   },
        { "pull-config",      no_argument,       0,  OptionPullConfig     },
        { "push-config",      no_argument,       0,  OptionPushConfig     },

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
        { "from",            required_argument,  0, OptionFrom            },
        { "until",           required_argument,  0, OptionUntil           },

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
            
            case OptionFrom:
                // --from=DATE&TIME
                m_options["from"] = optarg;
                break;
            
            case OptionUntil:
                // --until=DATE&TIME
                m_options["until"] = optarg;
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
    
    if (isCreateRequested())
        countOptions++;

    if (isRestoreRequested())
        countOptions++;
    
    if (isDeleteRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = 
            "The --list, --create, --restore and --delete options are mutually"
            " exclusive.";

        m_exitStatus = BadOptions;
        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = 
            "One of the --list, --create, --restore and --delete options"
            " is mandatory.";

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
    
    if (isLogRequested())
        countOptions++;

    if (isWaitRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = 
            "The --list, --log and --wait options are mutually"
            " exclusive.";

        m_exitStatus = BadOptions;

        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = 
            "One of the --list, --log and --wait options is mandatory.";

        m_exitStatus = BadOptions;

        return false;
    }


    return true;
}

/**
 * \returns True if the command line options seem to be ok.
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

    if (isRollingRestartRequested())
        countOptions++;
    
    if (isCreateReportRequested())
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

    if (isDeleteAccountRequested())
        countOptions++;

    if (isCreateDatabaseRequested())
        countOptions++;
    
    if (isRegisterRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = 
            "The following options are mutually exclusive: "
            "--list, --stat, --create, --ping, --rolling-restart, --add-node,"
            " --remove-node, --drop, --stop, --start, --create-account,"
            " --create-report,"
            " --delete-account, --create-database, --grant, --register"
            ".";

        m_exitStatus = BadOptions;
        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = 
            "One of the following options is mandatory: "
            "--list, --stat, --create, --ping, --rolling-restart, --add-node,"
            " --create-report,"
            " --remove-node, --drop, --stop, --start, --create-account,"
            " --delete-account, --create-database, --grant, --register"
            ".";

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

    if (countOptions > 1)
    {
        m_errorMessage = 
            "The --list, --list-config, --change-config, --stat and --set "
            "options are mutually exclusive.";

        m_exitStatus = BadOptions;

        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = 
            "One of the --list, --list-config, --change-config, --stat and "
            "--set options is mandatory.";

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
    
    if (isCreateRequested())
        countOptions++;
    
    if (isSetRequested())
        countOptions++;
    
    if (isChangePasswordRequested())
        countOptions++;
    
    if (isWhoAmIRequested())
        countOptions++;


    if (countOptions > 1)
    {
        m_errorMessage = 
            "The --list, --whoami, --set, --change-password and --create "
            "options are mutually exclusive.";

        m_exitStatus = BadOptions;

        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = 
            "One of the --list, --whoami, --set, --change-password and "
            "--create options is mandatory.";

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
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "rpc-token",        required_argument, 0, 't'                   },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0,  OptionPrintJson      },
        { "color",            optional_argument, 0,  OptionColor          },
        { "config-file",      required_argument, 0,  OptionConfigFile     },

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
        { "help",             no_argument,       0, 'h'                   },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "rpc-token",        required_argument, 0, 't'                   },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0, OptionConfigFile      },
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },

        // Main Option
        { "generate-key",     no_argument,       0, 'g'                   }, 
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "list",             no_argument,       0, 'L'                   },
        { "whoami",           no_argument,       0, OptionWhoAmI          },
        { "create",           no_argument,       0, OptionCreate          },
        { "set",              no_argument,       0, OptionSet             },
        { "change-password",  no_argument,       0, OptionChangePassword  },
       
        // Options about the user.
        { "group",            required_argument, 0, OptionGroup           },
        { "create-group",     no_argument,       0, OptionCreateGroup     },
        { "first-name",       required_argument, 0, OptionFirstName       },
        { "last-name",        required_argument, 0, OptionLastName        },
        { "title",            required_argument, 0, OptionTitle           },
        { "email-address",    required_argument, 0, OptionEmailAddress    },
        { "user-format",      required_argument, 0, OptionUserFormat      }, 
        { "old-password",     required_argument, 0, OptionOldPassword     }, 
        { "new-password",     required_argument, 0, OptionNewPassword     }, 

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
                // --cmon-user=USERNAME
                m_options["cmon_user"] = optarg;
                break;
            
            case 'p':
                // --password=PASSWORD
                m_options["password"] = optarg;
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
           
            case OptionChangePassword:
                // --change-password
                m_options["change_password"] = true;
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
        { "help",             no_argument,       0, 'h'                   },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0, OptionRpcTls          },
        { "rpc-token",        required_argument, 0, 't'                   },
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
            case 'h':
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
S9sOptions::readOptionsMetaType(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, 'h'               },
        { "debug",            no_argument,       0, OptionDebug       },
        { "verbose",          no_argument,       0, 'v'               },
        { "version",          no_argument,       0, 'V'               },
        { "controller",       required_argument, 0, 'c'               },
        { "controller-port",  required_argument, 0, 'P'               },
        { "rpc-tls",          no_argument,       0, OptionRpcTls      },
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
        { "list-properties",  no_argument,       0, OptionListProperties },
        
        // Type/property related options
        { "type",             required_argument, 0, OptionType        },

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
           
            case OptionListProperties:
                // --list-properties
                m_options["list_properties"] = true;
                break;

            case OptionType:
                // --type=NAME
                m_options["type"] = optarg;
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
        { "help",             no_argument,       0, OptionHelp            },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0,  OptionRpcTls         },
        { "rpc-token",        required_argument, 0, 't'                   },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0, OptionConfigFile      },

        // Main Option
        { "ping",             no_argument,       0, OptionPing            },
        { "list",             no_argument,       0, 'L'                   },
        { "stat",             no_argument,       0, OptionStat            },
        { "create",           no_argument,       0, OptionCreate          },
        { "register",         no_argument,       0, OptionRegister        },
        { "rolling-restart",  no_argument,       0, OptionRollingRestart  },
        { "create-report",    no_argument,       0, OptionCreateReport    },
        { "add-node",         no_argument,       0, OptionAddNode         },
        { "remove-node",      no_argument,       0, OptionRemoveNode      },
        { "drop",             no_argument,       0, OptionDrop            },
        { "stop",             no_argument,       0, OptionStop            },
        { "start",            no_argument,       0, OptionStart           },
        { "create-account",   no_argument,       0, OptionCreateAccount   },
        { "delete-account",   no_argument,       0, OptionDeleteAccount   },
        { "create-database",  no_argument,       0, OptionCreateDatabase  },
        { "grant",            no_argument,       0, OptionGrant           },

        // Job Related Options
        { "wait",             no_argument,       0, OptionWait            },
        { "log",              no_argument,       0, 'G'                   },
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },
        { "schedule",         required_argument, 0, OptionSchedule        },


        // Cluster information.
        // http://52.58.107.236/cmon-docs/current/cmonjobs.html#mysql
        // https://docs.google.com/document/d/1hvPtdWJqLeu1bAk-ZiWsILtj5dLXSLmXUyJBiP7wKjk/edit#heading=h.xsnzbjxs2gss
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "cluster-name",     required_argument, 0, 'n'                   },
        { "nodes",            required_argument, 0,  OptionNodes          },
        { "vendor",           required_argument, 0, OptionVendor          },
        { "provider-version", required_argument, 0, OptionProviderVersion },
        { "os-user",          required_argument, 0, OptionOsUser          },
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
        { "output-dir",       required_argument, 0, OptionOutputDir       },
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
            
            case OptionStat:
                // --stat
                m_options["stat"] = true;
                break;
            
            case OptionRollingRestart:
                // --rolling-restart
                m_options["rolling_restart"] = true;
                break;
            
            case OptionCreateReport:
                // --create-report
                m_options["create_report"] = true;
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
            
            case OptionGrant:
                // --grant
                m_options["grant"] = true;
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
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "rpc-tls",          no_argument,       0,  6                    },
        { "rpc-token",        required_argument, 0, 't'                   },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0,  OptionPrintJson      },
        { "config-file",      required_argument, 0,  OptionConfigFile     },
        { "color",            optional_argument, 0,  OptionColor          },
        { "date-format",      required_argument, 0,  OptionDateFormat     },

        // Main Option
        { "wait",             no_argument,       0,  5                    },
        { "log",              no_argument,       0, 'G'                   },
        { "list",             no_argument,       0, 'L'                   },

        // Job Related Options
        { "cluster-id",       required_argument, 0, 'i'                   },
        { "job-id",           required_argument, 0, OptionJobId           },
        { "log-format",       required_argument, 0, OptionLogFormat       },
        { "limit",            required_argument, 0, OptionLimit           },
        { "offset",           required_argument, 0, OptionOffset          },

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
S9sOptions::readOptionsScript(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, 'h'                   },
        { "debug",            no_argument,       0, OptionDebug           },
        { "verbose",          no_argument,       0, 'v'                   },
        { "version",          no_argument,       0, 'V'                   },
        { "cmon-user",        required_argument, 0, 'u'                   }, 
        { "password",         required_argument, 0, 'p'                   }, 
        { "controller",       required_argument, 0, 'c'                   },
        { "controller-port",  required_argument, 0, 'P'                   },
        { "long",             no_argument,       0, 'l'                   },
        { "print-json",       no_argument,       0, OptionPrintJson       },
        { "color",            optional_argument, 0, OptionColor           },
        { "config-file",      required_argument, 0, OptionConfigFile      },
        { "batch",            no_argument,       0, OptionBatch           },
        { "no-header",        no_argument,       0, OptionNoHeader        },

        // Main Option
        { "execute",          no_argument,       0, OptionExecute         },
        { "tree",             no_argument,       0, OptionTree            },
       
        // Options about the maintenance period.
        { "cluster-id",       required_argument, 0, 'i'                   },

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
            
            case 'i':
                // -i, --cluster-id=ID
                m_options["cluster_id"] = atoi(optarg);
                break;

            case OptionExecute:
                // --execute
                m_options["execute"] = true;
                break;
            
            case OptionTree:
                // --tree
                m_options["tree"] = true;
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
 * \returns True if the command line options seem to be ok.
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

    if (isListRequested())
        countOptions++;
    
    if (isExecuteRequested())
        countOptions++;
    
    if (isDeleteRequested())
        countOptions++;

    if (countOptions > 1)
    {
        m_errorMessage = 
            "The --list, --execute and --delete options are mutually"
            " exclusive.";

        m_exitStatus = BadOptions;

        return false;
    } else if (countOptions == 0)
    {
        m_errorMessage = 
            "One of the --list, --execute and --delete options is mandatory.";

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
