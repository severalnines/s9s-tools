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
#include "S9sFile"
#include "S9sRegExp"

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
    sm_instance = this;
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
    if (m_options.contains("controller"))
        return m_options.at("controller").toString();

    return S9sString();
}

/**
 * \returns the controller port.
 */
int
S9sOptions::controllerPort() const
{
    if (m_options.contains("controller_port"))
        return m_options.at("controller_port").toInt();

    return 0;
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
 * \returns the cluster ID from the command line, 0 if the cluster id is not
 *   provided.
 */
int
S9sOptions::clusterId() const
{
    if (m_options.contains("cluster_id"))
        return m_options.at("cluster_id").toInt();

    return 0;
}

S9sString 
S9sOptions::userName() const
{
    S9sString retval;

    retval = getenv("USER");
    return retval;
}

int
S9sOptions::userId() const
{
    int retval;

    retval = (int) getuid();
    return retval;
}

bool
S9sOptions::isNodeOperationRequested() const
{
    return m_operationMode == Node;
}

bool
S9sOptions::isClusterOperationRequested() const
{
    return m_operationMode == Cluster;
}

bool
S9sOptions::isJobOperationRequested() const
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
S9sOptions::isRollingRestartRequested() const
{
    bool retval = false;
    if (m_options.contains("rolling_restart"))
        retval = m_options.at("rolling_restart").toBoolean();

    S9S_WARNING("*** retval : %s", retval ? "true" : "false");
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

bool
S9sOptions::isJsonRequested() const
{
    if (m_options.contains("print_json"))
        return m_options.at("print_json").toBoolean();

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
 * \returns the binary program name of the running application.
 */
S9sString
S9sOptions::binaryName() const
{
    return m_myName;
}

/**
 * \returns the current exit status of the running application.
 */
int 
S9sOptions::exitStatus() const 
{ 
    return m_exitStatus; 
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

bool
S9sOptions::readOptions(
        int   *argc,
        char  *argv[])
{
    bool retval = true;

    if (*argc < 1)
    {
        m_errorMessage = "Missing command line options.";
        return false;
    }

    m_myName = S9sFile::basename(argv[0]);
    if (*argc < 2)
    {
        m_errorMessage = "Missing command line options.";
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
    if (m_options["print-version"].toBoolean())
    {
        printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
        printf("Copyright (C) 2016...\n");
        printf("\n");
        printf("Written by ...\n");
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
    } else {
        m_errorMessage = "The first command line option must be the mode.";
        retval = false;
    }

    return retval;
}

bool
S9sOptions::readOptionsNode(
        int    argc,
        char  *argv[])
{
    S9S_DEBUG("");
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
        { "long",             no_argument,       0, 'l' },
        { "print-json",       no_argument,       0, '3' },
        { "config-file",      required_argument, 0, '1' },
        { "color",            optional_argument, 0, '2' },
        { 0, 0, 0, 0 }
    };

    //S9S_DEBUG("*** argc: %d", argc);
    optind = 0;
    opterr = 0;
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

            default:
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
    S9S_DEBUG("");
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
        { "rolling-restart",  no_argument,       0, 'R' },
        { "long",             no_argument,       0, 'l' },
        { "print-json",       no_argument,       0, '3' },
        { "config-file",      required_argument, 0, '1' },
        { "color",            optional_argument, 0, '2' },
        { "cluster-id",       required_argument, 0, 'i' },

        { 0, 0, 0, 0 }
    };

    optind = 0;
    opterr = 0;
    for (;;)
    {
        int option_index = 0;
        c = getopt_long(
                argc, argv, "hvc:P:t:VLRli:", 
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
            
            case 'R':
                S9S_WARNING("rolling restart");
                m_options["rolling_restart"] = true;
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

            default:
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
    S9S_DEBUG("");
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
        { "long",             no_argument,       0, 'l' },
        { "print-json",       no_argument,       0, '3' },
        { "config-file",      required_argument, 0, '1' },
        { "color",            optional_argument, 0, '2' },
        { "cluster-id",       required_argument, 0, 'i' },

        { 0, 0, 0, 0 }
    };

    S9S_DEBUG("*** argc : %d", argc);
    optind = 0;
    opterr = 0;
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

            default:
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
                return false;
        }
    }

    return true;
}
