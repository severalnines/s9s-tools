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

#include <sys/ioctl.h>
#include <stdio.h>
#include <cstdlib>
#include <getopt.h>
#include <stdarg.h>
#include <unistd.h>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sOptions *S9sOptions::sm_instance = 0;

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

S9sOptions::~S9sOptions()
{
    sm_instance = NULL;
}

S9sOptions *
S9sOptions::instance()
{
    if (!sm_instance)
        sm_instance = new S9sOptions;

    return sm_instance;
}

void 
S9sOptions::uninit()
{
    if (sm_instance)
    {
        delete sm_instance;
        sm_instance = 0;
    }
}

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
    userFile.fprintf("# controller_port      = 9555\n");
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

bool
S9sOptions::loadConfigFiles()
{
    S9sFile userConfig("~/.s9s/s9s.conf");
    bool    success;

    m_userConfig = S9sConfigFile();

    if (userConfig.exists())
    {
        S9sString content;

        S9S_DEBUG("User config exists.");
        success = userConfig.readTxtFile(content);
        if (!success)
        {
            printError(
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
    } else {
        S9S_DEBUG("User config does not exist.");
    }

    return true;
}

/**
 * \param url the Cmon Controller host or host:port.
 *
 * Sets the controller host name. If the passed string has the format
 * HOSTNAME:PORT sets the controller port too.
 */
void
S9sOptions::setController(
        const S9sString &url)
{
    S9sRegExp regexp;
 
    S9S_DEBUG("*** url: '%s'", STR(url));
    regexp = "(.+):([0-9]+)";
    if (regexp == url)
    {
        m_options["controller"]      = regexp[1];
        m_options["controller_port"] = regexp[2].toInt();
    } else {
        m_options["controller"] = url;
    }
}

/**
 * \returns the controller hostname.
 */
S9sString
S9sOptions::controller() const
{
    S9sString  retval;
    if (m_options.contains("controller"))
    {
        retval = m_options.at("controller").toString();
    } else {
        retval = m_userConfig.variableValue("controller_host_name");
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
 * \returns the controller port.
 */
int
S9sOptions::controllerPort() const
{
    int retval = 0;

    if (m_options.contains("controller_port"))
    {
        retval = m_options.at("controller_port").toInt();
    } else {
        retval = m_userConfig.variableValue("controller_port").toInt();
    }

    return retval;
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
    if (m_options.contains("vendor"))
        return m_options.at("vendor").toString();

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
    if (m_options.contains("provider_version"))
        return m_options.at("provider_version").toString();

    return defaultValue;
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
    }

    if (retval.empty())
        retval = userName();

    return retval;
}

S9sString
S9sOptions::osKeyFile() const
{
    S9sString retval;

    retval = m_userConfig.variableValue("os_key_file");

    return retval;
}

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

        if (!stringVal.empty())
            retval = stringVal.toInt();
    }

    return retval;
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
 * FIXME: there is no command line option for this.
 */
S9sString 
S9sOptions::userName() const
{
    S9sString retval;

    retval = getenv("USER");
    return retval;
}

/**
 * FIXME: there is no command line option for this.
 */
int
S9sOptions::userId() const
{
    int retval;

    retval = (int) getuid();
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

bool
S9sOptions::isAddNodeRequested() const
{
    bool retval = false;

    if (m_options.contains("add_node"))
        retval = m_options.at("add_node").toBoolean();

    return retval;
}

bool
S9sOptions::isRemoveNodeRequested() const
{
    bool retval = false;

    if (m_options.contains("remove_node"))
        retval = m_options.at("remove_node").toBoolean();

    return retval;
}

bool
S9sOptions::isDropRequested() const
{
    bool retval = false;

    if (m_options.contains("drop"))
        retval = m_options.at("drop").toBoolean();

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
        return isatty(fileno(stdout)) ? true : false;
    } else if (configValue.toLower() == "always")
    {
        return true;
    }

    return false;
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

bool
S9sOptions::readOptions(
        int   *argc,
        char  *argv[])
{
    bool retval = true;

    S9S_DEBUG("");
    if (*argc < 1)
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

    retval = setMode(argv[1]);
    if (!retval)
        return retval;

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
        printf("Help text.\n");
        return true;
    }

    return false;
}

bool 
S9sOptions::setMode(
        const S9sString &modeName)
{
    bool retval = true;
    
    if (modeName == "cluster") 
    {
        m_operationMode = Cluster;
    } else if (modeName == "node")
    {
        m_operationMode = Node;
    } else if (modeName == "job")
    {
        m_operationMode = Job;
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

/**
 * Reads the command line options in "node" mode.
 */
bool
S9sOptions::readOptionsNode(
        int    argc,
        char  *argv[])
{
    S9S_DEBUG("");
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, 'h' },
        { "verbose",          no_argument,       0, 'v' },
        { "version",          no_argument,       0, 'V' },
        { "controller",       required_argument, 0, 'c' },
        { "controller-port",  required_argument, 0, 'P' },
        { "rpc-token",        required_argument, 0, 't' },
        { "long",             no_argument,       0, 'l' },
        { "print-json",       no_argument,       0, '3' },
        { "color",            optional_argument, 0, '2' },
        { "config-file",      required_argument, 0, '1' },

        // Main Option
        { "list",             no_argument,       0, 'L' },
        { "set",              no_argument,       0,  1 },

        // Cluster information
        { "cluster-id",       required_argument, 0, 'i' },
        { "nodes",            required_argument, 0,  3  },

        // 
        { "properties",       required_argument, 0,  2  },

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

        //S9S_DEBUG("*** c : '%c'", c);
        switch (c)
        {
            case 'h':
                // -h, --help
                m_options["help"] = true;
                break;

            case 'v':
                m_options["verbose"] = true;
                break;
            
            case 'V':
                m_options["print-version"] = true;
                break;

            case 'c':
                setController(optarg);
                break;

            case 'P':
                m_options["controller_port"] = atoi(optarg);
                break;

            case 't':
                m_options["rpc_token"] = optarg;
                break;
            
            case 'l':
                m_options["long"] = true;
                break;

            case 'L': 
                // --list
                m_options["list"] = true;
                break;

            case 1:
                // --set
                m_options["set"]  = true;
                break;

            case '1':
                m_options["config-file"] = optarg;
                break;
            
            case '2':
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case '3':
                m_options["print_json"] = true;
                break;
            
            case 'i':
                m_options["cluster_id"] = atoi(optarg);
                break;

            case 2:
                // --properties=STRING
                setPropertiesOption(optarg);
                break;
            
            case 3:
                // --nodes=LIST
                setNodes(optarg);
                break;

            default:
                S9S_WARNING("Unrecognized command line option.");
                m_exitStatus = BadOptions;
                return false;
        }
    }

    return true;
}

bool
S9sOptions::readOptionsCluster(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        // Generic Options
        { "help",             no_argument,       0, 'h' },
        { "verbose",          no_argument,       0, 'v' },
        { "version",          no_argument,       0, 'V' },
        { "controller",       required_argument, 0, 'c' },
        { "controller-port",  required_argument, 0, 'P' },
        { "rpc-token",        required_argument, 0, 't' },
        { "long",             no_argument,       0, 'l' },
        { "print-json",       no_argument,       0, '3' },
        { "color",            optional_argument, 0, '2' },
        { "config-file",      required_argument, 0, '1' },

        // Main Option
        { "list",             no_argument,       0, 'L' },
        { "create",           no_argument,       0, '5' },
        { "rolling-restart",  no_argument,       0, '6' },
        { "add-node",         no_argument,       0,  8  },
        { "remove-node",      no_argument,       0,  9  },
        { "drop",             no_argument,       0, 10  },

        // Job Related Options
        { "wait",             no_argument,       0, '4' },
        { "log",              no_argument,       0,  6 },
        { "batch",            no_argument,       0,  7 },

        // Cluster information.
        // http://52.58.107.236/cmon-docs/current/cmonjobs.html#mysql
        // https://docs.google.com/document/d/1hvPtdWJqLeu1bAk-ZiWsILtj5dLXSLmXUyJBiP7wKjk/edit#heading=h.xsnzbjxs2gss
        { "cluster-id",       required_argument, 0, 'i' },
        { "cluster-name",     required_argument, 0, 'n' },
        { "nodes",            required_argument, 0,  1  },
        { "vendor",           required_argument, 0,  2  },
        { "provider-version", required_argument, 0,  3  },
        { "os-user",          required_argument, 0,  4  },
        { "cluster-type",     required_argument, 0,  5  },
        { "db-admin",         required_argument, 0,  6  },
        { "db-admin-passwd",  required_argument, 0,  7  },

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
            case 'h':
                m_options["help"] = true;
                break;

            case 'v':
                m_options["verbose"] = true;
                break;
            
            case 'V':
                m_options["print-version"] = true;
                break;

            case 'c':
                setController(optarg);
                break;

            case 'P':
                m_options["controller_port"] = atoi(optarg);
                break;

            case 't':
                m_options["rpc_token"] = optarg;
                break;

            case 'l':
                m_options["long"] = true;
                break;

            case 'L': 
                m_options["list"] = true;
                break;
            
            case '6':
                // --rolling-restart
                m_options["rolling_restart"] = true;
                break;

            case 8:
                // --add-node
                m_options["add_node"] = true;
                break;
            
            case 9:
                // --remove-node
                m_options["remove_node"] = true;
                break;

            case 10:
                // --drop
                m_options["drop"] = true;
                break;

            case '1':
                m_options["config-file"] = optarg;
                break;

            case '2':
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case '3':
                m_options["print_json"] = true;
                break;
            
            case '4':
                m_options["wait"] = true;
                break;

            case 6:
                // --log
                m_options["log"] = true;
                break;
            
            case 7:
                // --batch
                m_options["batch"] = true;
                break;
            
            case '5':
                m_options["create"] = true;
                break;
            
            case 'i':
                m_options["cluster_id"] = atoi(optarg);
                break;
            
            case 'n':
                m_options["cluster_name"] = optarg;
                break;

            case 1:
                // --nodes=LIST
                setNodes(optarg);
                break;

            case 2:
                // --vendor=STRING
                m_options["vendor"] = optarg;
                break;

            case 3:
                // --provider-version=STRING
                m_options["provider_version"] = optarg;
                break;

            case 4:
                // --os-user
                m_options["os_user"] = optarg;
                break;

            case 5:
                // --cluster-type
                m_options["cluster_type"] = optarg;
                break;
                
            default:
                S9S_WARNING("Unrecognized command line option.");
                m_exitStatus = BadOptions;
                return false;
        }
    }

    return true;
}

bool
S9sOptions::readOptionsJob(
        int    argc,
        char  *argv[])
{
    int           c;
    struct option long_options[] =
    {
        { "help",             no_argument,       0, 'h' },
        { "verbose",          no_argument,       0, 'v' },
        { "version",          no_argument,       0, 'V' },
        { "controller",       required_argument, 0, 'c' },
        { "controller-port",  required_argument, 0, 'P' },
        { "rpc-token",        required_argument, 0, 't' },
        { "list",             no_argument,       0, 'L' },
        { "log",              no_argument,       0, 'G' },
        { "long",             no_argument,       0, 'l' },
        { "print-json",       no_argument,       0, '3' },
        { "wait",             no_argument,       0, '5' },
        { "config-file",      required_argument, 0, '1' },
        { "color",            optional_argument, 0, '2' },
        { "cluster-id",       required_argument, 0, 'i' },
        { "job-id",           required_argument, 0, '4' },

        { 0, 0, 0, 0 }
    };

    S9S_DEBUG("*** argc : %d", argc);
    optind = 0;
    //opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:VLl", 
                long_options, &option_index);

        if (c == -1)
            break;
        
        S9S_DEBUG("*** c : '%c'", c);

        switch (c)
        {
            case 'h':
                m_options["help"] = true;
                break;

            case 'v':
                m_options["verbose"] = true;
                break;
            
            case 'V':
                m_options["print-version"] = true;
                break;

            case 'c':
                setController(optarg);
                break;

            case 'P':
                m_options["controller_port"] = atoi(optarg);
                break;

            case 't':
                m_options["rpc_token"] = optarg;
                break;

            case 'l':
                m_options["long"] = true;
                break;

            case 'L': 
                m_options["list"] = true;
                break;
            
            case 'G': 
                m_options["log"] = true;
                break;

            case '1':
                m_options["config-file"] = optarg;
                break;

            case '2':
                if (optarg)
                    m_options["color"] = optarg;
                else
                    m_options["color"] = "always";
                break;

            case '3':
                m_options["print_json"] = true;
                break;

            case '4':
                m_options["job_id"] = atoi(optarg);
                break;

            case '5':
                m_options["wait"] = true;
                break;

            case 'i':
                m_options["cluster_id"] = atoi(optarg);
                break;

            default:
                S9S_WARNING("Unrecognized command line option.");
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
        { "help",          no_argument,       0, 'h' },
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
            case 'h':
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
                m_exitStatus = BadOptions;
                return false;
        }
    }

    return true;
}
