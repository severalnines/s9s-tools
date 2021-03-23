/*
 * Severalnines Tools
 * Copyright (C) 2018-present  Severalnines AB
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
#include "s9srpcclient.h"
#include "s9srpcclient_p.h"

#include "S9sOptions"
#include "S9sUser"
#include "S9sNode"
#include "S9sAccount"
#include "S9sRsaKey"
#include "S9sDateTime"
#include "S9sFile"
#include "S9sSshCredentials"
#include "S9sContainer"

#include <cstring>
#include <cstdio>
#include <iostream> 

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

#define READ_SIZE 10240
//#define SEND_NODES

/**
 * Default constructor.
 */
S9sRpcClient::S9sRpcClient() :
    m_priv(new S9sRpcClientPrivate)
{
}

/**
 * \param hostName the name if the host where the Cmon controller accepts
 *   requests.
 * \param port the port where the Cmon controller accepts requests.
 * \param useTls if client must initiate TLS encryption to the server.
 *
 */
S9sRpcClient::S9sRpcClient(
        const S9sString &hostName,
        const int        port,
        const S9sString &path,
        const bool       useTls) :
    m_priv(new S9sRpcClientPrivate)
{
    m_priv->m_hostName = hostName;
    m_priv->m_port     = port;
    m_priv->m_path     = path;
    m_priv->m_useTls   = useTls;
}


/**
 * Copy constructor. Nothing to see here.
 */
S9sRpcClient::S9sRpcClient (
        const S9sRpcClient &orig)
{
    m_priv = orig.m_priv;

    if (m_priv) 
        m_priv->ref();
}

/**
 * Normal destructor. 
 */
S9sRpcClient::~S9sRpcClient()
{
    if (m_priv && m_priv->unRef() == 0)
    {
        delete m_priv;
        m_priv = 0;
    }
}

/**
 * Assignment operator to utilize the implicit sharing.
 */
S9sRpcClient &
S9sRpcClient::operator= (
        const S9sRpcClient &rhs)
{
    if (this == &rhs)
        return *this;

    if (m_priv && m_priv->unRef() == 0)
    {
        delete m_priv;
        m_priv = 0;
    }

    m_priv = rhs.m_priv;
    if (m_priv) 
    {
        m_priv->ref ();
    }

    return *this;
}

S9sString
S9sRpcClient::hostName() const
{
    return m_priv->m_hostName;
}

bool
S9sRpcClient::useTls() const
{
    return m_priv->m_useTls;
}

int
S9sRpcClient::port() const
{
    return m_priv->m_port;
}

bool
S9sRpcClient::hasPrivateKey() const
{
    S9sOptions    *options = S9sOptions::instance();
    S9sFile        keyFile(options->privateKeyPath());

    if (options->userName().empty())
        return false;
 
    return keyFile.exists();
}

bool
S9sRpcClient::canAuthenticate(
        S9sString &reason) const
{
    S9sOptions    *options = S9sOptions::instance();

    // No authentication without a username.
    if (options->userName().empty())
    {
        reason = "No user name set.";
        return false;
    }

    // It is possible with a password...
    if (!options->password().empty())
        return true;

    // or a key.
    if (hasPrivateKey())
        return true;

    reason.sprintf(
            "No password and no RSA key for user %s.", 
            STR(options->userName()));

    return false;
}

bool
S9sRpcClient::needToAuthenticate() const
{
    S9sOptions    *options = S9sOptions::instance();

    // Creating a new user is possible without authentication through the pipe.
    if (options->isUserOperation() && options->isCreateRequested())
        return false;

    return true;
}

/**
 * This method checks if it is possible to authenticate (e.g. because the
 * username and the password is provided) and if it is needed to authenticate
 * (because the requested operation needs authentication). Then if it is
 * possible theauthentication will be performed. If it is needed, but not
 * possible an error will be returned. If it is nor needed nor possible nothing
 * will be done but the return value will be true.
 */
bool
S9sRpcClient::maybeAuthenticate()
{
    S9sOptions  *options    = S9sOptions::instance();
    bool         canDoAuthentication;
    bool         needAuthenticate;

    canDoAuthentication  = canAuthenticate(m_priv->m_errorString);
    needAuthenticate = needToAuthenticate();

    // We can authenticate, the user intended to.
    if (canDoAuthentication)
    {
        bool success = authenticate();
        if (!success)
        {
            if (options->isJsonRequested())
            {
                printf("%s\n", STR(reply().toString()));
            } else {
                if (m_priv->m_errorString.empty())
                    m_priv->m_errorString = reply().errorString();

                if (m_priv->m_errorString.empty())
                    m_priv->m_errorString = "Access denied.";
            }

            // continuing, server replies a nice error
            // The lower levels set a more spcific error code.
            // options->setExitStatus(S9sOptions::AccessDenied);
        }

        S9sString controllerVersion = serverVersion();
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
        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    // We can't and we don't have to... ok then.
    return true;
}


/**
 * \returns the reply that received from the controller.
 *
 * The reply the controller sends is a JSON string which is parsed by the
 * S9sRpcClient and presented here as an S9sVariantMap (S9sRpcReply that
 * inherits S9sVariantMap to be more precise).
 */
const S9sRpcReply &
S9sRpcClient::reply() const
{
    return m_priv->m_reply;
}

/**
 * Takes the request status from the reply and sets the exit code of the
 * program accordingly. If the request status is "ok" the exit code is not going
 * to be changed.
 */
void
S9sRpcClient::setExitStatus() 
{
    S9sRpcReply::ErrorCode errorCode = reply().requestStatus();

    if (errorCode != S9sRpcReply::Ok)
    {
        S9sOptions *options = S9sOptions::instance();

        switch (errorCode)
        {
            case S9sRpcReply::Ok:
                break;

            case S9sRpcReply::InvalidRequest:
                options->setExitStatus(S9sOptions::Failed);
                break;

            case S9sRpcReply::ObjectNotFound:
                options->setExitStatus(S9sOptions::NotFound);
                break;

            case S9sRpcReply::TryAgain:
                options->setExitStatus(S9sOptions::Failed);
                break;

            case S9sRpcReply::ClusterNotFound:
                options->setExitStatus(S9sOptions::NotFound);
                break;

            case S9sRpcReply::UnknownError:
                options->setExitStatus(S9sOptions::Failed);
                break;

            case S9sRpcReply::AccessDenied:
                options->setExitStatus(S9sOptions::AccessDenied);
                break;

            case S9sRpcReply::AuthRequired:
            case S9sRpcReply::ConnectError:
                options->setExitStatus(S9sOptions::Failed);
                m_priv->m_authenticated = false;
                break;
        }
    }
}

/**
 * \returns the human readable error string stored in the object.
 */
S9sString 
S9sRpcClient::errorString() const
{
    return m_priv->m_errorString;
}

/**
 * Prints the messages from the reply, prints the default message if there are
 * no messages in the reply.
 */
void
S9sRpcClient::printMessages(
        const S9sString &defaultMessage,
        bool             success)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sRpcReply    rpcReply;

    rpcReply = reply();
    if (success)
    {
        rpcReply.printMessages(defaultMessage);
    } else {
        if (options->isJsonRequested())
        {
            printf("%s\n", STR(rpcReply.toString()));
        } else {
            rpcReply.printMessages(errorString());
            //PRINT_ERROR("%s", STR(errorString()));
        }
    }
}

void
S9sRpcClient::printServerRegistered(
        bool success)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sRpcReply    rpcReply;

    rpcReply = reply();
    if (success)
    {
        rpcReply.printMessages("Registered.");
        #if 0
        printf("  Processors\n");
        rpcReply.printProcessors("    ");
        printf("  Memory\n");
        rpcReply.printMemoryBanks("    ");
        #endif
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(rpcReply.toString()));
        else
            PRINT_ERROR("%s", STR(errorString()));
    }
}

bool
S9sRpcClient::isAuthenticated() const
{
    return m_priv->m_authenticated;
}

bool
S9sRpcClient::authenticate()
{
    S9sOptions    *options = S9sOptions::instance();
    bool           retval = false;

    PRINT_LOG("Authenticating...");
    if (options->hasPassword())
        retval = authenticateWithPassword();
    else if (!options->password().empty())
        retval = authenticateWithPassword();
    else 
        retval = authenticateWithKey();

    if (retval)
        PRINT_LOG("Authenticated.");
    else
        PRINT_LOG("Authentication failed.");

    return retval;
}

bool 
S9sRpcClient::authenticateWithPassword()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantMap  request;
    S9sString      uri = "/v2/auth";
    bool           retval;

    request["operation"]    = "authenticateWithPassword";
    request["user_name"]    = options->userName();
    request["password"]     = options->password();
    
    retval = executeRequest(uri, request, false);
    m_priv->m_errorString = reply().errorString();
    if (!retval)
    {
        m_priv->m_authenticated = false;
        return false;
    }
   
    if (!reply().isOk())
        options->setExitStatus(S9sOptions::AccessDenied);

    m_priv->m_authenticated = reply().isOk();
    return m_priv->m_authenticated;
}

/**
 * Does the authentication with the private key.
 */
bool
S9sRpcClient::authenticateWithKey()
{
    S9S_DEBUG(" ");
    S9sOptions    *options = S9sOptions::instance();
    S9sRsaKey      rsa;
    S9sString      uri = "/v2/auth";
    S9sVariantMap  request;
    bool           retval;
    S9sString      privKeyPath = options->privateKeyPath();

    S9S_DEBUG(" privKeyPath : %s", STR(privKeyPath));

    if (privKeyPath.empty())
    {
        S9S_WARNING("Private key not specified.");

        m_priv->m_errorString =
                "Private key not specified, authentication is not possible.";
        
        options->setExitStatus(S9sOptions::BadOptions);
        m_priv->m_authenticated = false;
        return false;
    }

    if (!rsa.loadKeyFromFile(privKeyPath))
    {
        S9S_WARNING(
                "Could not load user private key: %s",
                STR(privKeyPath));

        m_priv->m_errorString.sprintf (
                "Could not load user private key: %s",
                STR(privKeyPath));

        options->setExitStatus(S9sOptions::BadOptions);
        m_priv->m_authenticated = false;
        return false;
    }

    /*
     * First request.
     */
    request = S9sVariantMap();
    request["operation"]    = "authenticate";

    /*
     * Please keep this as 'username' to be compatible with clustercontrol
     * controller 1.4.1 and 1.4.2.
     */
    request["username"]     = options->userName();

    retval = executeRequest(uri, request, false);
    m_priv->m_errorString = reply().errorString ();
    if (!retval)
    {
        m_priv->m_authenticated = false;
        return false;
    }

    S9sRpcReply loginReply = reply();
    S9sString signature;
    S9sString challenge = loginReply["challenge"].toString();

    // create an RSA-SHA256 signature using user's privkey
    S9S_DEBUG("  privKeyPath : %s", STR(privKeyPath));
    rsa.signRsaSha256(challenge, signature);
    S9S_DEBUG("  challenge   : %s", STR(challenge));
    S9S_DEBUG("  signature   : %s", STR(signature));

    /*
     * Second request.
     */
    request = S9sVariantMap();
    request["operation"]    = "authenticateResponse";
    request["signature"]    = signature;

    /*
     * For backward compatibility, if we know that we communicate with a
     * controller < 1.4.3, use the old method name.
     */
    if (serverVersion().startsWith("1.4.2") ||
        serverVersion().startsWith("1.4.1"))
        request["operation"] = "response";

    retval = executeRequest(uri, request, false);
    m_priv->m_errorString = reply().errorString ();
    if (!retval)
    {
        m_priv->m_authenticated = false;
        return false;
    }

    /*
     * If reply doesn't contain an error and we are ok, auth succeed
     */
    m_priv->m_errorString = reply().errorString ();
    
    if (!reply().isOk())
        options->setExitStatus(S9sOptions::AccessDenied);

    m_priv->m_authenticated = reply().isOk();
    return m_priv->m_authenticated;
}

/**
 * \param clusterId The ID of the cluster for which the information is
 *   requested.
 *
 * The method that sends the "getAllClusterInfo" RPC request and reads the
 * reply. This function requests for information about only one cluster
 * referenced by the cluster ID.
 */
bool
S9sRpcClient::getCluster(
        const S9sString  &clusterName, 
        const int         clusterId)
{
    S9sString      uri = "/v2/clusters/";
    S9sVariantMap  request;

    request["operation"]       = "getClusterInfo";
    request["with_hosts"]      = true;
    //request["with_containers"] = true;
    //request["with_sheet_info"] = true;

    if (S9S_CLUSTER_ID_IS_VALID(clusterId))
        request["cluster_id"]   = clusterId;

    if (!clusterName.empty())
        request["cluster_name"] = clusterName;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::getSqlProcesses()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/clusters/";
    S9sVariantMap  request    = composeRequest();

    request["operation"]       = "getSqlProcesses";
    
    if (options->limit() >= 0)
        request["limit"] = options->limit();

    return executeRequest(uri, request);    
}

bool
S9sRpcClient::getTopQueries()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/clusters/";
    S9sVariantMap  request    = composeRequest();

    request["operation"]       = "getTopQueries";
   
    if (options->nExtraArguments() > 0u)
        request["filterStrings"] = options->extraArguments();

    if (options->limit() >= 0)
        request["limit"] = options->limit();
    
    if (options->offset() >= 0)
        request["offset"] = options->offset();

    return executeRequest(uri, request);    
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * The method that sends the "getAllClusterInfo" RPC request and reads the
 * reply.
 */
bool
S9sRpcClient::getClusters(
        bool withHosts,
        bool withSheetInfo)
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sString      clusterName = options->clusterName();
    int            clusterId   = options->clusterId();
    S9sVariantMap  request     = composeRequest();
    S9sString      uri         = "/v2/clusters/";
    bool           retval;
   
    if (options->hasClusterIdOption())
        return getCluster(clusterName, clusterId);
    else if (options->hasClusterNameOption())
        return getCluster(clusterName, clusterId);

    request["operation"]       = "getAllClusterInfo";
    request["with_hosts"]      = withHosts;
    //request["with_containers"] = true;
    request["with_sheet_info"] = withSheetInfo;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * Gets the list of databases from the controller. Getting the databases here is
 * basically getting the clusters, but with requesting the databases of the
 * cluster too.
 */
bool
S9sRpcClient::getDatabases()
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sString      operation   = "getAllClusterInfo";
    S9sString      uri = "/v2/clusters/";
    S9sVariantMap  request;
    bool           retval;
   
    if (options->hasClusterIdOption())
    {
        request["operation"]       = "getClusterInfo";
        request["cluster_id"]      = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["operation"]       = "getClusterInfo";
        request["cluster_name"]    = options->clusterName();
    } else {
        request["operation"]       = "getAllClusterInfo";
    }

    request["with_databases"]  = true;
    
    if (options->isRefreshRequested())
        request["refresh_now"] = true;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This method id used to get the Cmon Directory Tree from the controller.
 */
bool
S9sRpcClient::getTree(
        bool withDotDot)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/tree";
    S9sVariantMap  request;
    
    request["operation"]       = "getTree";

    if (options->nExtraArguments() > 0)
        request["path"] = options->extraArgument(0u);

    if (options->isRefreshRequested())
        request["refresh_now"] = true;

    if (withDotDot)
        request["with_dot_dot"] = true;

    return executeRequest(uri, request);
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * The method that sends the "getConfig" RPC request and reads the
 * reply. The "getConfig" returns the parsed version of the configuration for a
 * given node.
 *
 * Here is an example showing how to use this request:
 * \code
pipas@blade05:tests$ s9s node \
    --batch \
    --list-config \
    --opt-name="log_line_prefix" \
    --nodes="192.168.0.101" 

- log_line_prefix '%m' 
 * \endcode
 */
bool
S9sRpcClient::getConfig(
        const S9sVariantList &hosts)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/config/";
    S9sVariantMap  request;

    request["operation"]  = "getConfig";
    if (hosts.size() == 1u)
    {
        S9sNode node = hosts[0].toNode();

        request["hostname"] = node.hostName();

        if (node.hasPort())
            request["port"] = node.port();
    } else {
        PRINT_ERROR("getConfig only implemented for one host.");
        return false;
    }

    if (options->clusterId() > 0)
        request["cluster_id"] = options->clusterId();

    return executeRequest(uri, request);
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Sends the getLdapConfig request to the server.
 */
bool
S9sRpcClient::getLdapConfig()
{
    S9sString      uri      = "/v2/config/";
    S9sVariantMap  request  = composeRequest();

    request["operation"]  = "getLdapConfig";
    return executeRequest(uri, request);
}

bool
S9sRpcClient::setLdapConfig()
{
    S9sOptions    *options   = S9sOptions::instance();    
    S9sString      uri       = "/v2/config/";
    S9sVariantMap  request   = composeRequest();
    std::string    s; 
    S9sString      content;
    S9sVariantMap  config;

    while(std::getline(std::cin, s))
    {
        content += s;
        content += "\n";
    }

    if (!config.parse(STR(content)))
    {
        PRINT_ERROR("Failed to parse standard input as a JSon string.");
        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    request["operation"]          = "setLdapConfig";
    request["ldap_configuration"] = config;

    return executeRequest(uri, request);

}


/**
 * This function is for changing the configuration through the controller for 
 * one node.
 */
bool
S9sRpcClient::setConfig()
{
    S9sOptions    *options    = S9sOptions::instance();
    S9sVariantList hosts      = options->nodes();
    S9sString      uri        = "/v2/config/";
    S9sVariantMap  request    = composeRequest();
    S9sVariantList optionList;
    S9sVariantMap  optionMap;
    bool           retval;

    request["operation"]  = "setConfig";
    if (hosts.size() == 1u)
    {
        S9sNode node = hosts[0].toNode();

        request["hostname"] = node.hostName();

        if (node.hasPort())
            request["port"] = node.port();
    } else {
        PRINT_ERROR("setConfig only implemented for one host.");
        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    if (options->optName().empty())
    {
        PRINT_ERROR(
                "Configuration option name is not provided.\n"
                "Use the --opt-name command line option to provide"
                " a configuration option name."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    // 
    // The configuration value: here it is implemented for one value.
    //
    optionMap["name"]  = options->optName();
    optionMap["value"] = options->optValue();

    if (!options->optGroup().empty())
        optionMap["group"] = options->optGroup();

    optionList << optionMap;

    request["configuration"] = optionList;

    retval = executeRequest(uri, request);
    return retval;
}

/**
 * This function unsets a configuration name through the controller for 
 * one node.
 */
bool
S9sRpcClient::unsetConfig()
{
    S9sOptions    *options    = S9sOptions::instance();
    S9sVariantList hosts      = options->nodes();
    S9sString      uri        = "/v2/config/";
    S9sVariantMap  request    = composeRequest();
    S9sVariantList optionList;
    S9sVariantMap  optionMap;
    bool           retval;

    request["operation"]  = "unsetConfig";
    if (hosts.size() == 1u)
    {
        S9sNode node = hosts[0].toNode();

        request["hostname"] = node.hostName();

        if (node.hasPort())
            request["port"] = node.port();
    } else {
        PRINT_ERROR("unsetConfig only implemented for one host.");
        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    if (options->optName().empty())
    {
        PRINT_ERROR(
                "Configuration option name is not provided.\n"
                "Use the --opt-name command line option to provide"
                " a configuration option name."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }


    // 
    // The configuration value: here it is implemented for one name.
    //
    optionMap["name"]  = options->optName();

    if (!options->optGroup().empty())
        optionMap["group"] = options->optGroup();

    optionList << optionMap;

    request["configuration"] = optionList;

    retval = executeRequest(uri, request);
    return retval;
}

/**
 * Sends a call to get the cluster configuration.
 */
bool
S9sRpcClient::getClusterConfig()
{
    S9sString      uri     = "/v2/clusters/";
    S9sVariantMap  request = composeRequest();
    bool           retval;

    request["operation"]  = "getConfig";
    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::setClusterConfig()
{
    S9sOptions    *options    = S9sOptions::instance();
    S9sString      uri        = "/v2/clusters/";
    S9sVariantMap  request    = composeRequest();
    S9sVariantList optionList;
    S9sVariantMap  optionMap;
    bool           retval;

    request["operation"]  = "setConfig";

    // 
    // The configuration value: here it is implemented for one value.
    //
    optionMap["name"]  = options->optName();
    optionMap["value"] = options->optValue();

    optionList << optionMap;

    request["configuration"] = optionList;

    retval = executeRequest(uri, request);
    return retval;
}

/**
 * This method implements the cluster ping request. We send a request with a
 * given cluster ID and expect a simple reply from the controller telling us
 * that the controller is up and we have access to the cluster.
 */
bool
S9sRpcClient::pingCluster()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sDateTime    now = S9sDateTime::currentDateTime();
    S9sString      timeString = now.toString(S9sDateTime::TzDateTimeFormat);
    S9sString      uri = "/v2/clusters/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]       = "ping";
    request["request_created"] = timeString;
    
    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }
    
    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::pingController()
{
    S9sDateTime    now = S9sDateTime::currentDateTime();
    S9sString      timeString = now.toString(S9sDateTime::TzDateTimeFormat);
    S9sString      uri = "/v2/controller/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]       = "ping";
    request["request_created"] = timeString;

    retval = executeRequest(uri, request);
    return retval;
}

bool 
S9sRpcClient::setHost()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  hostNames;
    S9sVariantMap   properties;

    hostNames = options->nodes();
    if (hostNames.empty())
    {
        PRINT_ERROR(
                "Node list is empty while setting node.\n"
                "Use the --nodes command line option to provide the node list."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }
    
    properties = options->propertiesOption();
    if (properties.empty())
    {
        PRINT_ERROR(
                "Properties not provided while setting node.\n"
                "Use the --properties command line option to provide"
                " properties."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    return setHost(hostNames, properties);
}



/**
 * \param hosts The list of hosts to change (currently only one host is
 *   supported).
 * \param properties The names and values of the host properties to change.
 */
bool
S9sRpcClient::setHost(
        const S9sVariantList &hosts,
        const S9sVariantMap  &properties)
{
    S9sString      uri = "/v2/host";
    S9sVariantMap  request;
    
    if (hosts.size() != 1u)
    {
        PRINT_ERROR("setHost is currently implemented only for one node.");
        return false;
    }

    request["operation"]  = "setHost";
    request["properties"] = properties;
    if (hosts[0].isNode())
    {
        request["hostname"]   = hosts[0].toNode().hostName();

        if (hosts[0].toNode().hasPort())
            request["port"]   = hosts[0].toNode().port();
    } else {
        request["hostname"]   = hosts[0].toString();
    }
    
    return executeRequest(uri, request);
}

/**
 *
{
    "cc_timestamp": 1475217315,
    "data": [ 
    {
        "class_name": "CmonCpuInfo",
        "cpucores": 4,
        "cpumaxmhz": 2.333e+06,
        "cpumhz": 2000,
        "cpumodel": "Intel(R) Xeon(R) CPU           E5345  @ 2.33GHz",
        "cputemp": 53,
        "hostid": 1,
        "physical_cpu_id": 0,
        "siblings": 4,
        "vendor": "GenuineIntel"
    }, 
    {
        "class_name": "CmonCpuInfo",
        "cpucores": 4,
        "cpumaxmhz": 2.333e+06,
        "cpumhz": 2000,
        "cpumodel": "Intel(R) Xeon(R) CPU           E5345  @ 2.33GHz",
        "cputemp": 53,
        "hostid": 4,
        "physical_cpu_id": 1,
        "siblings": 4,
        "vendor": "GenuineIntel"
    } ],
    "requestStatus": "ok",
    "total": 6
}
 */
bool
S9sRpcClient::getCpuInfo(
        const int clusterId)
{
    S9sString      uri = "/v2/stat";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]  = "getCpuPhysicalInfo";
    request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This function will send the "getInfo" call on the "v2/stat" interface. The
 * reply will be something like this:
 *
 * \code{.js}
 * {
 *     "collected_info": [ 
 *     {
 *         "cluster_id": 1,
 *         "info": 
 *         {
 *             "cluster.status": 2,
 *             "cluster.statustext": "Cluster started.",
 *             "cmon.domainname": "",
 *             "cmon.hostname": "t7500",
 *             "cmon.running": true,
 *             "cmon.starttime": 1501144642,
 *             "cmon.uptime": 15636,
 *             "conf.backup_retention": 31,
 *             "conf.clusterid": 1,
 *             "conf.clustername": "ft_postgresql_48154",
 *             "conf.clustertype": 5,
 *             "conf.configfile": "/tmp/cmon_1.cnf",
 *             "conf.hostname": "192.168.1.127",
 *             "conf.os": "debian",
 *             "conf.statustext": "Configuration loaded.",
 *             "host.1.connected": true,
 *             "host.1.cpu_io_wait_percent": 0.353357,
 *             "host.1.cpu_steal_percent": 0,
 *             "host.1.cpu_usage_percent": 15.5331,
 *             "host.1.cpucores": 16,
 *             "host.1.cpuinfo": [ 
 *             {
 *                 "class_name": "CmonCpuInfo",
 *                 "cpucores": 4,
 *                 "cpumaxmhz": 2.268e+06,
 *                 "cpumhz": 1600,
 *                 "cpumodel": "Intel(R) Xeon(R) CPU           L5520  @ 2.27GHz",
 *                 "cputemp": 55.5,
 *                 "hostid": 1,
 *                 "physical_cpu_id": 0,
 *                 "siblings": 8,
 *                 "vendor": "GenuineIntel"
 *             }, 
 *             {
 *                 "class_name": "CmonCpuInfo",
 *                 "cpucores": 4,
 *                 "cpumaxmhz": 2.268e+06,
 *                 "cpumhz": 1733,
 *                 "cpumodel": "Intel(R) Xeon(R) CPU           L5520  @ 2.27GHz",
 *                 "cputemp": 55.5,
 *                 "hostid": 1,
 *                 "physical_cpu_id": 1,
 *                 "siblings": 8,
 *                 "vendor": "GenuineIntel"
 *             } ],
 *             "host.1.cpumaxmhz": 2268,
 *             "host.1.cpumhz": 1733,
 *             "host.1.cpumodel": "Intel(R) Xeon(R) CPU           L5520  @ 2.27GHz",
 *             "host.1.cputemp": 55.5,
 *             "host.1.devices": [ "/dev/mapper/core1--vg-root" ],
 *             "host.1.free_disk_bytes": 155814936576,
 *             "host.1.hostname": "192.168.1.174",
 *             "host.1.interfaces": [ "eth0" ],
 *             "host.1.ip": "192.168.1.174",
 *             "host.1.membuffer": 0,
 *             "host.1.memcached": 1,
 *             "host.1.memfree": 16031900,
 *             "host.1.memtotal": 16777216,
 *             "host.1.network_interfaces": [ 
 *             {
 *                 "interface_name": "eth0",
 *                 "rx_bytes_per_sec": 11775.8,
 *                 "tx_bytes_per_sec": 17652.3
 *             } ],
 *             "host.1.pingdelay": -1,
 *             "host.1.pingstatustext": "Creating ICMP socket (to ping '192.168.1.174') failed: Operation not permitted.",
 *             "host.1.port": 8089,
 *             "host.1.rx_bytes_per_second": 11775.8,
 *             "host.1.swapfree": 0,
 *             "host.1.swaptotal": 0,
 *             "host.1.total_disk_bytes": 208033853440,
 *             "host.1.tx_bytes_per_second": 17652.3,
 *             "host.1.uptime": 15779,
 *             "host.1.wallclock": 1501160299,
 *             "host.1.wallclocksampled": 1501160235,
 *             "host.2.class_name": "controller",
 *             "host.2.connected": true,
 *             "host.2.cpu_io_wait_percent": 2.89087,
 *             "host.2.cpu_steal_percent": 0,
 *             "host.2.cpu_usage_percent": 204.378,
 *             "host.2.cpucores": 24,
 *             "host.2.cpuinfo": [ 
 *             {
 *                 "class_name": "CmonCpuInfo",
 *                 "cpucores": 6,
 *                 "cpumaxmhz": 2.661e+06,
 *                 "cpumhz": 1596,
 *                 "cpumodel": "Intel(R) Xeon(R) CPU           X5650  @ 2.67GHz",
 *                 "cputemp": 0,
 *                 "hostid": 2,
 *                 "physical_cpu_id": 0,
 *                 "siblings": 12,
 *                 "vendor": "GenuineIntel"
 *             }, 
 *             {
 *                 "class_name": "CmonCpuInfo",
 *                 "cpucores": 6,
 *                 "cpumaxmhz": 2.661e+06,
 *                 "cpumhz": 1596,
 *                 "cpumodel": "Intel(R) Xeon(R) CPU           X5650  @ 2.67GHz",
 *                 "cputemp": 0,
 *                 "hostid": 2,
 *                 "physical_cpu_id": 1,
 *                 "siblings": 12,
 *                 "vendor": "GenuineIntel"
 *             } ],
 *             "host.2.cpumaxmhz": 2661,
 *             "host.2.cpumhz": 1596,
 *             "host.2.cpumodel": "Intel(R) Xeon(R) CPU           X5650  @ 2.67GHz",
 *             "host.2.cputemp": 0,
 *             "host.2.devices": [ "/dev/sda1" ],
 *             "host.2.free_disk_bytes": 1409452683264,
 *             "host.2.hostname": "192.168.1.127",
 *             "host.2.interfaces": [ "eth0" ],
 *             "host.2.ip": "192.168.1.127",
 *             "host.2.membuffer": 318624,
 *             "host.2.memcached": 19415480,
 *             "host.2.memfree": 9243460,
 *             "host.2.memtotal": 49453276,
 *             "host.2.network_interfaces": [ 
 *             {
 *                 "interface_name": "eth0",
 *                 "rx_bytes_per_sec": 29150.4,
 *                 "tx_bytes_per_sec": 18520.8
 *             } ],
 *             "host.2.pingdelay": -1,
 *             "host.2.pingstatustext": "Creating ICMP socket (to ping '192.168.1.127') failed: Operation not permitted.",
 *             "host.2.port": 9555,
 *             "host.2.rx_bytes_per_second": 29150.4,
 *             "host.2.swapfree": 0,
 *             "host.2.swaptotal": 0,
 *             "host.2.total_disk_bytes": 2125259440128,
 *             "host.2.tx_bytes_per_second": 18520.8,
 *             "host.2.uptime": 2.59852e+06,
 *             "host.2.version": "1.4.3",
 *             "host.2.wallclock": 1501160235,
 *             "host.2.wallclocksampled": 1501160235,
 *             "license.expires": -1,
 *             "license.status": false,
 *             "license.statustext": "No license found.",
 *             "mail.statustext": "Created.",
 *             "netStat.1.eth0.rxBytes": 287358005,
 *             "netStat.1.eth0.txBytes": 205682361,
 *             "netStat.2.eth0.rxBytes": 531084878595,
 *             "netStat.2.eth0.txBytes": 503628558465
 *         }
 *     } ],
 *     "request_created": "2017-07-27T12:58:00.283Z",
 *     "request_id": 3,
 *     "request_processed": "2017-07-27T12:58:00.328Z",
 *     "request_status": "Ok",
 *     "request_user_id": 3
 * }
 * \endcode
 */
bool
S9sRpcClient::getInfo()
{
    S9sString      uri = "/v2/stat";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getInfo";

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId the ID of the cluster for which the statistics will be
 *   fetched.
 * \returns true if the request sent and a reply is received (even if the reply
 *   is an error message).
 */
bool
S9sRpcClient::getCpuStats(
        const int clusterId)
{
    return getStats(clusterId, "cpustat");
}

/**
 * \param clusterId the ID of the cluster for which the statistics will be
 *   fetched.
 * \returns true if the request sent and a reply is received (even if the reply
 *   is an error message).
 */
bool
S9sRpcClient::getSqlStats(
        const int clusterId)
{
    return getStats(clusterId, "sqlstat");
}

/**
 * \param clusterId the ID of the cluster for which the statistics will be
 *   fetched.
 * \returns true if the request sent and a reply is received (even if the reply
 *   is an error message).
 */
bool
S9sRpcClient::getMemStats(
        const int clusterId)
{
    return getStats(clusterId, "memorystat");
}

/**
 * Gets the list of metatypes.
 */
bool
S9sRpcClient::getMetaTypes()
{
    S9sString      uri = "/v2/metatype/";
    S9sVariantMap  request = composeRequest() ;
    bool           retval;

    request["operation"] = "getMetaTypes";

    retval = executeRequest(uri, request);
    
    return retval;    
}

/**
 * Gets information about all metatypes.
 */
bool
S9sRpcClient::getMetaTypeProperties(
        const S9sString &typeName)
{
    S9sString      uri     = "/v2/metatype/";
    S9sVariantMap  request = composeRequest() ;
    bool           retval;

    request["operation"] = "getMetaTypeInfo";
    request["type-name"] = typeName;

    retval = executeRequest(uri, request);
    
    return retval;    
}

/**
 * The reply will contain a lot of these:
    {
        "created": 1493297858,
        "hostid": 2,
        "interval": 67335,
        "memoryutilization": 0.0959179,
        "pgpgin": 0,
        "pgpgout": 3124,
        "pswpin": 0,
        "pswpout": 0,
        "rambuffers": 0,
        "ramcached": 7168,
        "ramfree": 15532005376,
        "ramfreemin": 15531171840,
        "ramtotal": 17179869184,
        "sampleends": 1493297918,
        "samplekey": "CmonMemoryStats-2",
        "swapfree": 0,
        "swaptotal": 0,
        "swaputilization": 0
    }, 
 */
bool
S9sRpcClient::getMemoryStats(
        const int clusterId)
{
    S9sString      uri = "/v2/stat";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]  = "statByName";
    request["name"]       = "memorystat";
    request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);
    
    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * A method to get the list of the running processes from all nodes of one
 * particular cluster.
 */
bool 
S9sRpcClient::getRunningProcesses()
{
    S9sString      uri       = "/v2/process";
    S9sVariantMap  request   = composeRequest();
    bool           retval;

    request["operation"]  = "getRunningProcesses";
    
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId the ID of the cluster for which the job instances will be
 *   fetched.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 * 
 * Sends a "getJobInstances" request, receives the reply. We use this RPC call
 * to get the job list (e.g. s9s job --list).
 */
bool
S9sRpcClient::getJobInstances(
        const S9sString  &clusterName, 
        const int         clusterId)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/jobs/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getJobInstances";

    if (options->limit() >= 0)
        request["limit"] = options->limit();

    if (options->offset() >= 0)
        request["offset"] = options->offset();

    if (S9S_CLUSTER_ID_IS_VALID(clusterId))
        request["cluster_id"] = clusterId;
    
    if (!clusterName.empty())
        request["cluster_name"] = clusterName;

    if (options->getBool("show_aborted"))
        request["show_aborted"] = true;
    
    if (options->getBool("show_defined"))
        request["show_defined"] = true;
    
    if (options->getBool("show_failed"))
        request["show_failed"] = true;
    
    if (options->getBool("show_finished"))
        request["show_finished"] = true;
    
    if (options->getBool("show_running"))
        request["show_running"] = true;
    
    if (options->getBool("show_scheduled"))
        request["show_scheduled"] = true;

    if (!options->withTags().empty())
        request["tags"] = options->withTags();

    retval = executeRequest(uri, request);
    return retval;
}

/**
 * \param jobId the ID of the job
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 *
 * This function sends a "getJobInstance" request to the controller and receives
 * its reply. This request can be used to get the properties of one particular
 * job.
 */
bool
S9sRpcClient::getJobInstance(
        const int jobId)
{
    S9sString      uri = "/v2/jobs/";
    S9sVariantMap  request;

    request["operation"] = "getJobInstance";
    request["job_id"]    = jobId;

    return executeRequest(uri, request, false);
}

/**
 * \param jobId the ID of the job
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 *
 * Deletes a job from the controller.
 */
bool
S9sRpcClient::deleteJobInstance(
        const int jobId)
{
    S9sString      uri = "/v2/jobs/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "deleteJobInstance";
    request["job_id"]    = jobId;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * Sends a 'killJobInstance' request to abort a running job.
 */
bool
S9sRpcClient::killJobInstance(
        const int jobId)
{
    S9sString      uri = "/v2/jobs/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "killJobInstance";
    request["job_id"]    = jobId;
    request["signal"]    = 15;

    retval = executeRequest(uri, request);

    return retval;
}


/**
 * \param jobId the ID of the job
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 *
 * This request will clone the job instance, make a new job instance that is
 * exactly the same as the instance that has the given job ID.
 */
bool
S9sRpcClient::cloneJobInstance(
        const int jobId)
{
    S9sString      uri = "/v2/jobs/";
    S9sVariantMap  request = composeRequest();
    bool           retval;

    request["operation"] = "cloneJobInstance";
    request["job_id"]    = jobId;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param jobId the ID of the job
 * \param limit the maximum number of log entries we are ready to process
 * \param offset the number of log entries to skip
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 *
 * This function will get the log entries in ascending order. This is because
 * the terminal normally used like that.
 */
bool
S9sRpcClient::getJobLog(
        const int  jobId,
        const int  limit,
        const int  offset,
        const bool isImportant)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/jobs/";
    S9sVariantMap  request;
    bool           retval;

    // Building the request.
    request["operation"]  = "getJobLog";
    request["job_id"]     = jobId;
    request["ascending"]  = true;
   
    if (options->isDebug())
        request["log_level"] = "DEBUG";

    if (limit != 0)
        request["limit"]  = limit;

    if (offset != 0)
        request["offset"] = offset;

    retval = executeRequest(uri, request, isImportant);

    return retval;

}

/**
 * This function gets the logs from the controller. Not the job messages, but
 * the actual cmon logs.
 *
 * Here is an example of a reply. We might have to double check the format
 * string for this, there are a few fields just added.
 *
 * \code{.js}
 * {
 *     "log_entries": [ 
 *     {
 *         "class_name": "CmonLogMessage",
 *         "component": "ClusterConfiguration",
 *         "created": "2017-07-26T08:35:40.704Z",
 *         "log_class": "LogMessage",
 *         "log_id": 110,
 *         "log_origins": 
 *         {
 *             "sender_binary": "cmon",
 *             "sender_file": "../../src/cmonhostmanager.cpp",
 *             "sender_line": 594,
 *             "sender_pid": 40818,
 *             "tv_nsec": 704637455,
 *             "tv_sec": 1501058140
 *         },
 *         "log_specifics": 
 *         {
 *             "cluster_id": 1,
 *             "message_text": "Registering CmonPostgreSqlHost: 192.168.1.167:8089"
 *         },
 *         "severity": "LOG_DEBUG"
 *     }, 
 *  
 *     . . .
 * 
 *     {
 *         "class_name": "CmonLogMessage",
 *         "created": "2017-07-26T08:38:11.697Z",
 *         "log_class": "LogMessage",
 *         "log_id": 351,
 *         "log_origins": 
 *         {
 *             "sender_binary": "cmon",
 *             "sender_file": "../../src/cmonalarmdb.cpp",
 *             "sender_line": 1467,
 *             "sender_pid": 40818,
 *             "tv_nsec": 697060177,
 *             "tv_sec": 1501058291
 *         },
 *         "log_specifics": 
 *         {
 *             "cluster_id": 1,
 *             "message_text": "hostId: 1, nodeId: 0, alarm: Host is not responding"
 *         },
 *         "severity": "LOG_DEBUG"
 *     } ],
 *     "log_entry_counts": 
 *     {
 *         "component": 
 *         {
 *             "Cluster": 1,
 *             "ClusterConfiguration": 2,
 *             "Node": 3,
 *             "Unknown": 194
 *         },
 *         "hostname": 
 *         {
 *             "": 199,
 *             "192.168.1.167": 1
 *         },
 *         "sender_binary": 
 *         {
 *             "cmon": 200
 *         },
 *         "severity": 
 *         {
 *             "LOG_CRIT": 2,
 *             "LOG_DEBUG": 192,
 *             "LOG_ERR": 1,
 *             "LOG_INFO": 2,
 *             "LOG_WARNING": 3
 *         }
 *     },
 *     "reply_received": "2017-07-26T08:39:53.936Z",
 *     "request_created": "2017-07-26T08:39:53.931Z",
 *     "request_id": 3,
 *     "request_processed": "2017-07-26T08:39:54.019Z",
 *     "request_status": "Ok",
 *     "request_user_id": 3,
 *     "total": 258
 * }
 * \endcode
 */
bool
S9sRpcClient::getLog()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            limit     = options->limit();
    int            offset    = options->offset();
    S9sString      uri       = "/v2/log/";
    S9sVariantMap  request   = composeRequest();
    bool           retval;

    // Building the request.
    if (!options->hasMessageId())
    {
        request["operation"]  = "getLogEntries";
        request["ascending"]  = false;
    
        if (options->isDebug())
            request["severity"] = "LOG_DEBUG";

        else if (options->isWarning())
            request["severity"] = "LOG_WARNING";

        if (!options->from().empty())
            request["created_after"] = options->from();

        if (!options->until().empty())
            request["created_before"] = options->until();

        if (limit > 0)
            request["limit"]  = limit;

        if (offset > 0)
            request["offset"] = offset;
    } else {
        request["operation"]  = "getLogEntry";
        request["message_id"] = options->messageId();
    }

    request["cluster_id"] = options->clusterId();
    
    if (options->hasClusterNameOption())
        request["cluster_name"] = options->clusterName();

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * Gets the statistics about the log. Here is an example reply:
 *
 * \code{.js}
 * {
 *     "log_statistics": 
 *     {
 *         "cluster_log_statistics": [ 
 *         {
 *             "cluster_id": -1,
 *             "disabled": false,
 *             "entries_received": 3,
 *             "format_string": "%C : (%S) %M",
 *             "last_error_message": "",
 *             "lines_written": 0,
 *             "log_file_name": "",
 *             "max_log_file_size": 5242880,
 *             "messages_per_sec": 0.05,
 *             "syslog_enabled": false,
 *             "write_cycle_counter": 1
 *         }, 
 *         {
 *             "cluster_id": 0,
 *             "disabled": false,
 *             "entries_received": 93,
 *             "format_string": "%C : (%S) %M",
 *             "last_error_message": "Success.",
 *             "lines_written": 93,
 *             "log_file_name": "./cmon-ft-install.log",
 *             "max_log_file_size": 5242880,
 *             "messages_per_sec": 0.05,
 *             "syslog_enabled": false,
 *             "write_cycle_counter": 3
 *         }, 
 *         {
 *             "cluster_id": 1,
 *             "disabled": false,
 *             "entries_received": 218,
 *             "format_string": "%C : (%S) %M",
 *             "last_error_message": "Success.",
 *             "lines_written": 218,
 *             "log_file_name": "/tmp/cmon_1.log",
 *             "max_log_file_size": 5242880,
 *             "messages_per_sec": 0.85,
 *             "syslog_enabled": false,
 *             "write_cycle_counter": 11
 *         } ],
 *         "current_time": "2017-07-26T11:01:08.884Z",
 *         "entries_statistics": 
 *         {
 *             "entries_received": 319,
 *             "entries_written_to_cmondb": 314
 *         },
 *         "has_cmondb": true,
 *         "last_error_message": "Success.",
 *         "last_flush_time": "2017-07-26T11:00:59.543Z",
 *         "log_debug_enabled": false,
 *         "writer_thread_running": true,
 *         "writer_thread_started": "2017-07-26T10:55:29.410Z"
 *     },
 *     "request_created": "2017-07-26T11:01:08.840Z",
 *     "request_id": 3,
 *     "request_processed": "2017-07-26T11:01:08.884Z",
 *     "request_status": "Ok",
 *     "request_user_id": 3
 * }
 * \endcode
 */
bool
S9sRpcClient::getLogStatistics()
{
    S9sString      uri = "/v2/log/";
    S9sVariantMap  request;

    // Building the request.
    request["operation"]  = "getLogStatistics";
    return executeRequest(uri, request);
}


/**
 * Gets the active alarms of a cluster. Here is an example that shows the reply:
 * \code{.js}
 * {
 *     "alarms": [ 
 *     {
 *         "alarm_id": 1,
 *         "class_name": "CmonAlarm",
 *         "cluster_id": 1,
 *         "component": 3,
 *         "component_name": "Cluster",
 *         "counter": 1,
 *         "created": "2017-07-27T06:33:38.000Z",
 *         "ignored": 0,
 *         "measured": 0,
 *         "message": "System time is drifting between servers and the distance between the highest system time and lowest is more than one minute.",
 *         "recommendation": "Synchronize the system clock on the servers using e.g NTP or make sure NTP is working. Time drifting may lead to unexpected failures or problems, and makes debugging hard.\n\nThe last seen host time values (in controller's time-zone):\n192.168.1.127: Jul 27 08:32:40\n192.168.1.169: Jul 27 08:33:44\n",
 *         "reported": "2017-07-27T06:33:38.000Z",
 *         "severity": 1,
 *         "severity_name": "ALARM_WARNING",
 *         "title": "System time is drifting",
 *         "type": 40000,
 *         "type_name": "ClusterTimeDrift"
 *     }, 
 *     . . .
 *     {
 *         "alarm_id": 3,
 *         "class_name": "CmonAlarm",
 *         "cluster_id": 1,
 *         "component": 0,
 *         "component_name": "Network",
 *         "counter": 12,
 *         "created": "2017-07-27T06:35:56.000Z",
 *         "host_id": 1,
 *         "hostname": "192.168.1.169",
 *         "ignored": 0,
 *         "measured": 0,
 *         "message": "Server 192.168.1.169 reports: Host 192.168.1.169 is not responding to ping after 3 cycles, the host is most likely unreachable.",
 *         "recommendation": "Restart failed host, check firewall.",
 *         "reported": "2017-07-27T06:35:56.000Z",
 *         "severity": 2,
 *         "severity_name": "ALARM_CRITICAL",
 *         "title": "Host is not responding",
 *         "type": 10006,
 *         "type_name": "HostUnreachable"
 *     } ],
 *     "cluster_id": 1,
 *     "request_created": "2017-07-27T06:36:03.627Z",
 *     "request_id": 3,
 *     "request_processed": "2017-07-27T06:36:03.675Z",
 *     "request_status": "Ok",
 *     "request_user_id": 3
 * }
 * \endcode
 */
bool
S9sRpcClient::getAlarms()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/alarm/";
    S9sVariantMap  request;

    // Building the request.
    request["operation"]  = "getAlarms";
    
    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

bool
S9sRpcClient::getAlarm()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/alarm/";
    S9sVariantMap  request;
    int            alarmId = 1;

    // Building the request.
    request["operation"]  = "getAlarm";
    request["alarm_id"]   = alarmId;

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

bool
S9sRpcClient::ignoreAlarm()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/alarm/";
    S9sVariantMap  request;
    int            alarmId = 2;
    
    if (!options->hasAlarmIdOption())
    {
        PRINT_ERROR("Alarm ID is not provided.");
        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }


    // Building the request.
    request["operation"]  = "ignoreAlarm";
    request["alarm_id"]   = alarmId;
    request["ignore"]     = true;
    request["alarm_id"]   = options->alarmId();

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

bool
S9sRpcClient::getAlarmStatistics()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/alarm/";
    S9sVariantMap  request;

    // Building the request.
    request["operation"]  = "getStatistics";

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

bool
S9sRpcClient::getReports()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/reports/";
    S9sVariantMap  request;

    // Building the request.
    request["operation"]  = "getReports";

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

bool
S9sRpcClient::getReportTemplates()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/reports/";
    S9sVariantMap  request;

    // Building the request.
    request["operation"]  = "getReportTemplates";

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

bool
S9sRpcClient::generateReport()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/reports/";
    S9sVariantMap  request;
    S9sVariantMap  reportMap;
    S9sString      reportType = options->type();

    if (reportType.empty())
        reportType = "default";

    // Building the request.
    reportMap["class_name"]  = "CmonReport";
    reportMap["report_type"] = reportType;
    reportMap["recipients"]  = "laszlo@severalnines.com";
    reportMap["text_format"] = "AnsiTerminal";

    request["operation"]     = "generateReport";
    request["report"]        = reportMap;

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

/**
 * Gets one specific report by the record ID.
 */
bool
S9sRpcClient::getReport()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            reportId  = options->reportId();
    S9sString      uri = "/v2/reports/";
    S9sVariantMap  request;
    S9sVariantMap  reportMap;

    // Building the request.
    reportMap["class_name"]  = "CmonReport";
    reportMap["text_format"] = "AnsiTerminal";
    reportMap["report_id"]   = reportId;

    request["operation"]     = "getReport";
    request["report"]        = reportMap;

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}


bool
S9sRpcClient::deleteReport()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            reportId  = options->reportId();
    S9sString      uri       = "/v2/reports/";
    S9sVariantMap  request;
    S9sVariantMap  reportMap;

    // Building the request.
    reportMap["class_name"]  = "CmonReport";
    reportMap["report_id"]   = reportId;

    request["operation"]     = "deleteReport";
    request["report"]        = reportMap;

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

/**
 * This will initiate a job that creates a local repository.
 */
bool
S9sRpcClient::createLocalRepository(
        const int          clusterId,
        const S9sString   &clusterType,
        const S9sString   &vendor,
        const S9sString   &dbVersion,
        const S9sString   &osRelease)
{
    S9sVariantMap  request;
    S9sVariantMap  job     = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";

    jobData["cluster_type"] = clusterType;
    jobData["vendor"]     = vendor;
    jobData["db_version"] = dbVersion;
    jobData["os_release"] = osRelease;
    //jobData["dry_run"]    = true;

    jobSpec["command"]    = "create_local_repository";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Create Repository";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::createSnapshotJob()
{
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri       = "/v2/jobs/";
    
    // The jobspec describing the command.
    jobSpec["command"]    = "cmon_ha_create_snapshot";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Create Snapshot";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    return executeRequest(uri, request);
}

/**
 * Creates a "fail" job, a job that does nothing and fails.
 */
bool
S9sRpcClient::createFailJob()
{
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri       = "/v2/jobs/";
    
    // The jobspec describing the command.
    jobSpec["command"]    = "fail";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Simulated Failure";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    return executeRequest(uri, request);
}

bool
S9sRpcClient::createSuccessJob()
{
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    
    // The jobspec describing the command.
    jobSpec["command"]    = "success";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Simulated Success";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    return executeRequest(uri, request);
}

/**
 * \param shellCommand The line(s) to execute.
 */
bool
S9sRpcClient::executeSystemCommand(
        const S9sVariant shellCommand)
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sString      clusterName = options->clusterName();
    int            clusterId   = options->clusterId();
    S9sVariantList hosts       = options->nodes();
    S9sVariantMap  request;
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobSpec;
    S9sVariantMap  jobData = composeJobData();    
    S9sString      uri = "/v2/jobs/";

    // JobData
    jobData["shell_command"] = shellCommand;
    
    if (!hosts.empty())
        jobData["nodes"] = nodesField(hosts);
    
    if (options->hasTimeout())
        jobData["timeout"] = options->timeout();

    // JobSpec
    jobSpec["command"]       = "execute";
    jobSpec["job_data"]      = jobData;

    // The job instance describing how the job will be executed.
    job["title"]             = "Execute System Command";
    job["job_spec"]          = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]     = "createJobInstance";
    request["job"]           = job;
    
    if (S9S_CLUSTER_ID_IS_VALID(clusterId))
        request["cluster_id"]   = clusterId;

    if (!clusterName.empty())
        request["cluster_name"] = clusterName;
    
    
    return executeRequest(uri, request);
}

/**
 * \param cdtPath The full path of the CDT entry to execute.
 */
bool
S9sRpcClient::executeCdtEntry(
        const S9sString &cdtPath)
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sVariantList hosts       = options->nodes();
    S9sString      title;
    S9sVariantMap  request;
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobSpec;
    S9sVariantMap  jobData = composeJobData();    
    S9sString      uri = "/v2/jobs/";

    title.sprintf("Execute %s", STR(S9sFile::basename(cdtPath)));

    // JobData
    jobData["path"]          = cdtPath;
    
    // JobSpec
    jobSpec["command"]       = "execute";
    jobSpec["job_data"]      = jobData;

    // The job instance describing how the job will be executed.
    job["title"]             = title;
    job["job_spec"]          = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]     = "createJobInstance";
    request["job"]           = job;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::executeCdtEntry()
{
    S9sOptions    *options = S9sOptions::instance();

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --run option requires ona command line arguments: "
                "The CDT path of the entry to execute.");

        return false;
    }
    
    return executeCdtEntry(options->extraArgument(0u));
}

bool
S9sRpcClient::executeSystemCommand(
        const S9sVariantList &scriptLines)
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sString      clusterName = options->clusterName();
    int            clusterId   = options->clusterId();
    S9sVariantList hosts       = options->nodes();
    S9sVariantMap  request;
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobSpec;
    S9sVariantMap  jobData = composeJobData();    
    S9sString      uri = "/v2/jobs/";

    // JobData
    jobData["script_lines"] = scriptLines;
    
    if (!hosts.empty())
        jobData["nodes"] = nodesField(hosts);
    
    if (options->hasTimeout())
        jobData["timeout"] = options->timeout();

    // JobSpec
    jobSpec["command"]       = "execute";
    jobSpec["job_data"]      = jobData;

    // The job instance describing how the job will be executed.
    job["title"]             = "Execute System Command";
    job["job_spec"]          = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]     = "createJobInstance";
    request["job"]           = job;
    
    if (S9S_CLUSTER_ID_IS_VALID(clusterId))
        request["cluster_id"]   = clusterId;

    if (!clusterName.empty())
        request["cluster_name"] = clusterName;
    
    
    return executeRequest(uri, request);
}

/**
 * \returns True if the request was successfully sent and a reply is received
 *   even if the reply is actually a failure message.
 *
 * Sends a request to create an "inspect_host" request.
 */
bool
S9sRpcClient::inspectHost()
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sVariantList hosts       = options->nodes();
    S9sVariantMap  request;
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobSpec;
    S9sVariantMap  jobData = composeJobData();    
    S9sString      uri = "/v2/jobs/";

    if (!hosts.empty())
        jobData["nodes"] = nodesField(hosts);
    
    if (options->hasTimeout())
        jobData["timeout"] = options->timeout();

    // JobSpec
    jobSpec["command"]       = "inspect_host";
    jobSpec["job_data"]      = jobData;

    // The job instance describing how the job will be executed.
    job["title"]             = "Inspecting Host";
    job["job_spec"]          = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]     = "createJobInstance";
    request["job"]           = job;
    
    return executeRequest(uri, request);
}


/**
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 *
 * Creates a job for "rolling restart" and receives the controller's answer for
 * the request. 
 */
bool
S9sRpcClient::rollingRestart()
{
    S9sVariantMap  request = composeRequest();
    S9sVariantMap  job     = composeJob();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";

    jobSpec["command"]    = "rolling_restart";

    // The job instance describing how the job will be executed.
    job["title"]          = "Rolling Restart";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    return executeRequest(uri, request);
}

/**
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 */
bool
S9sRpcClient::setClusterReadOnly()
{
    S9sVariantMap  request = composeRequest();
    S9sVariantMap  job     = composeJob();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";

    jobSpec["command"]    = "enable_cluster_readonly";

    // The job instance describing how the job will be executed.
    job["title"]          = "Set Cluster Read-only";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    return executeRequest(uri, request);
}


/**
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 *
 * Creates a job for to import config files. Creates a request something like
 * this:
 * 
 * \code{.js}
 * {
 *     "cluster_id": 1,
 *     "job": 
 *     {
 *         "class_name": "CmonJobInstance",
 *         "job_spec": 
 *         {
 *             "command": "import_config"
 *         },
 *         "tags": [ "manual", "transient" ],
 *         "title": "Import Config"
 *     },
 *     "operation": "createJobInstance",
 *     "request_created": "2019-05-23T12:09:34.583Z",
 *     "request_id": 3
 * }
 * \endcode

 */
bool
S9sRpcClient::importConfig()
{
    S9sVariantMap  request = composeRequest();
    S9sVariantMap  job     = composeJob();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";

    jobSpec["command"]    = "import_config";

    // The job instance describing how the job will be executed.
    if (!job.contains("title"))
        job["title"]      = "Import Config";

    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    return executeRequest(uri, request);
}

/**
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 *
 * Creates a job for to collect log files.
 */
bool
S9sRpcClient::collectLogs()
{
    S9sVariantMap  request = composeRequest();
    S9sVariantMap  job     = composeJob();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";

    jobSpec["command"]    = "collect_logs";

    // The job instance describing how the job will be executed.
    job["title"]          = "Collect Logs";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::enableSsl()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantMap  request = composeRequest();
    S9sVariantMap  job     = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";

    // JobData...
    jobData["action"]      = "enable";
    jobData["expire_days"] = 1000;

    jobData["ca_file"]     = options->sslCaFile();
    jobData["cert_file"]   = options->sslCertFile();
    jobData["key_file"]    = options->sslKeyFile();

    // JobSpec...
    jobSpec["command"]     = "setup_ssl";
    jobSpec["job_data"]    = jobData;

    // The job instance describing how the job will be executed.
    job["title"]           = "Enable SSL";
    job["job_spec"]        = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]   = "createJobInstance";
    request["job"]         = job;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::disableSsl()
{
    S9sVariantMap  request = composeRequest();
    S9sVariantMap  job     = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";

    // JobData...
    jobData["action"]      = "disable";
    jobData["expire_days"] = 1000;

    // JobSpec...
    jobSpec["command"]     = "setup_ssl";
    jobSpec["job_data"]    = jobData;

    // The job instance describing how the job will be executed.
    job["title"]           = "Disable SSL";
    job["job_spec"]        = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]   = "createJobInstance";
    request["job"]         = job;
    
    return executeRequest(uri, request);
}

/**
 * This is the method that initiated a saveCluster job.
 *
 * \code{.js}
 * {
 *     "cluster_id": 1,
 *     "job": 
 *     {
 *         "class_name": "CmonJobInstance",
 *         "job_spec": 
 *         {
 *             "command": "save_cluster",
 *             "job_data": 
 *             {
 *                 "backupdir": "/tmp/cmon-saved-clusters"
 *                 "output_file": "ak.tar.gz"
 *             }
 *         },
 *         "title": "Save Cluster"
 *     },
 *     "operation": "createJobInstance",
 *     "request_created": "2018-10-12T06:44:39.634Z",
 *     "request_id": 3
 * }
 * \endcode
 */
bool
S9sRpcClient::saveCluster()
{
    S9sOptions     *options      = S9sOptions::instance();
    S9sString       backupDir    = options->backupDir();
    S9sString       outputFile   = options->outputFile();
    S9sVariantMap   request      = composeRequest();
    S9sVariantMap   job          = composeJob();
    S9sVariantMap   jobData      = composeJobData();
    S9sString       uri          = "/v2/jobs/";
    S9sVariantMap   jobSpec;

    if (!backupDir.empty())
        jobData["backupdir"]     = backupDir;

    if (!outputFile.empty())
        jobData["output_file"]   = outputFile;
    
    if (options->keepTempDir())
        jobData["keep_temp_dir"] = true;
    
    if (!options->tempDirPath().empty())
        jobData["temp_dir_path"] = options->tempDirPath();
    
    jobSpec["command"]    = "save_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Save Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::restoreCluster()
{
    S9sOptions     *options       = S9sOptions::instance();
    S9sString       inputFileName = options->inputFile();
    S9sVariantMap   request       = composeRequest();
    S9sVariantMap   job           = composeJob();
    S9sVariantMap   jobSpec;
    S9sVariantMap   jobData       = composeJobData();
    S9sString       uri           = "/v2/jobs/";
    
    if (!inputFileName.empty())
        jobData["input_file"] = inputFileName;
    
    if (options->keepTempDir())
        jobData["keep_temp_dir"] = true;
    
    if (!options->tempDirPath().empty())
        jobData["temp_dir_path"] = options->tempDirPath();

    jobSpec["command"]    = "restore_cluster";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Restore Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::saveController()
{
    S9sOptions     *options      = S9sOptions::instance();
    S9sString       backupDir    = options->backupDir();
    S9sString       outputFile   = options->outputFile();
    S9sVariantMap   request      = composeRequest();
    S9sVariantMap   job          = composeJob();
    S9sVariantMap   jobData      = composeJobData();
    S9sString       uri          = "/v2/jobs/";
    S9sVariantMap   jobSpec;

    if (!backupDir.empty())
        jobData["backupdir"]     = backupDir;

    if (!outputFile.empty())
        jobData["output_file"]   = outputFile;

    if (options->keepTempDir())
        jobData["keep_temp_dir"] = true;
    
    if (!options->tempDirPath().empty())
        jobData["temp_dir_path"] = options->tempDirPath();
    
    jobSpec["command"]    = "save_controller";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Save Controller";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::restoreController()
{
    S9sOptions     *options       = S9sOptions::instance();
    S9sString       inputFileName = options->inputFile();
    S9sVariantMap   request       = composeRequest();
    S9sVariantMap   job           = composeJob();
    S9sVariantMap   jobSpec;
    S9sVariantMap   jobData       = composeJobData();
    S9sString       uri           = "/v2/jobs/";
    
    if (!inputFileName.empty())
        jobData["input_file"] = inputFileName;
    
    if (options->keepTempDir())
        jobData["keep_temp_dir"] = true;
    
    if (!options->tempDirPath().empty())
        jobData["temp_dir_path"] = options->tempDirPath();

    jobSpec["command"]    = "restore_controller";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Restore Controller";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::setupAuditLogging(
        const int clusterId)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantList hosts = options->nodes();
    S9sVariantMap  request;
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sVariantMap  jobData = composeJobData();

    if (!hosts.empty())
        jobData["nodes"] = nodesField(hosts);

    jobSpec["command"]    = "setup_audit_logging";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Setup Audit Logging";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;
    
    return executeRequest(uri, request);
}


/**
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 */
bool
S9sRpcClient::createReport(
        const int clusterId)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      outputDir = options->outputDir();
    S9sVariantMap  request;
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;

    jobData["extended"]       = options->extended();
    jobData["mask_passwords"] = options->maskPasswords();

    if (!outputDir.empty())
        jobData["report_dir"] = outputDir;

    jobSpec["command"]    = "error_report";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Create Error Report";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;
    
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 */
bool
S9sRpcClient::deployAgents(
        const int clusterId)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantMap  request;
    S9sVariantList hosts = options->nodes();
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;

    // When no node specified, backend will deploy to the controller.
    if (!hosts.empty())
        jobData["node"] = hosts[0].toVariantMap();
    
    jobSpec["command"]    = "deploy_agents";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Deploy Monitoring Agents";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;
    
    retval = executeRequest(uri, request);

    return retval;
}


/**
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply), false if the request was
 *   not even sent.
 *
 */
bool
S9sRpcClient::createCluster()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantList hosts;
    S9sString      osUserName;
    S9sString      vendor;
    S9sString      dbVersion;
    S9sRpcReply    reply;
    bool           success;

    if (options->clusterType().empty())
    {
        PRINT_ERROR(
                 "Cluster type is not set.\n"
                 "Use the --cluster-type command line option to set it.");

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    hosts = options->nodes();
    if (hosts.empty())
    {
        PRINT_ERROR(
            "Node list is empty while creating cluster.\n"
            "Use the --nodes command line option to provide the node list."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    dbVersion    = options->providerVersion(
            options->clusterType() == "postgresql" ? "9.6" : "5.6");

    osUserName     = options->osUser();
    vendor         = options->vendor();

    if (vendor.empty() && options->clusterType() != "postgresql")
    {
        PRINT_ERROR(
            "The vendor name is unknown while creating a cluster.\n"
            "Use the --vendor command line option to provide the vendor."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    if (dbVersion.empty())
    {
        PRINT_ERROR(
            "The SQL server version is unknown while creating a cluster.\n"
            "Use the --provider-version command line option set it."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    /*
     * Running the request on the controller.
     */
    if (options->clusterType() == "mysql_single" ||
            options->clusterType() == "mysql-single")
    {
        success = createMySqlSingleCluster(
                hosts, osUserName, vendor, dbVersion);
    } else if (options->clusterType() == "galera")
    {
        success = createGaleraCluster(
                hosts, osUserName, vendor, dbVersion);
    } else if (options->clusterType() == "mysqlreplication" ||
            options->clusterType() == "mysql")
    {
        success = createMySqlReplication(
                hosts, osUserName, vendor, dbVersion);
    } else if (options->clusterType() == "group_replication" || 
            options->clusterType() == "groupreplication")
    {
        success = createGroupReplication(
                hosts, osUserName, vendor, dbVersion);
    } else if (options->clusterType() == "postgresql")
    {
        success = createPostgreSql(
                hosts, osUserName, dbVersion);
    } else if (options->clusterType() == "mongodb")
    {
        success = createMongoCluster(
                hosts, osUserName, vendor, dbVersion);
    } else if (options->clusterType() == "ndb" || 
            options->clusterType() == "ndbcluster")
    {
        S9sVariantList mySqlHosts, mgmdHosts, ndbdHosts;

        for (uint idx = 0u; idx < hosts.size(); ++idx)
        {
            S9sNode     node     = hosts[idx].toNode();
            S9sString   protocol = node.protocol().toLower();

            if (protocol == "ndbd")
            {
                ndbdHosts << node;
            } else if (protocol == "mgmd" || protocol == "ndb_mgmd")
            {
                mgmdHosts << node;
            } else if (protocol == "mysql" || protocol.empty())
            {
                mySqlHosts << node;
            } else {
                PRINT_ERROR(
                        "The protocol '%s' is not supported.", 
                        STR(protocol));
            
                options->setExitStatus(S9sOptions::BadOptions);
                return false;
            }
        }

        success = createNdbCluster(
                mySqlHosts, mgmdHosts, ndbdHosts,
                osUserName, vendor, dbVersion);
    } else {
        PRINT_ERROR(
            "Not supported cluster type '%s'.", STR(options->clusterType()));

        options->setExitStatus(S9sOptions::BadOptions);

        success = false;
    }

    return success;
}

bool
S9sRpcClient::registerCluster()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantList hosts;
    S9sString      osUserName;
    bool           success = false;

    hosts = options->nodes();
    if (hosts.empty())
    {
        PRINT_ERROR(
            "Node list is empty while registering cluster.\n"
            "Use the --nodes command line option to provide the node list."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return success;
    }
    
    osUserName   = options->osUser();
    
    if (options->clusterType() == "postgresql")
    {
        success = registerPostgreSql(hosts, osUserName);
    } else if (options->clusterType() == "galera")
    {
        success = registerGaleraCluster(hosts, osUserName);
    } else if (options->clusterType() == "mysqlreplication")
    {
        success = registerMySqlReplication(hosts, osUserName);
    } else if (options->clusterType() == "group_replication")
    {
        success = registerGroupReplication(hosts, osUserName);
    } else if (options->clusterType() == "ndb" || 
            options->clusterType() == "ndbcluster")
    {
        S9sVariantList mySqlHosts, mgmdHosts, ndbdHosts;

        for (uint idx = 0u; idx < hosts.size(); ++idx)
        {
            S9sNode     node     = hosts[idx].toNode();
            S9sString   protocol = node.protocol().toLower();

            if (protocol == "ndbd")
            {
                ndbdHosts << node;
            } else if (protocol == "mgmd" || protocol == "ndb_mgmd")
            {
                mgmdHosts << node;
            } else if (protocol == "mysql" || protocol.empty())
            {
                mySqlHosts << node;
            } else {
                PRINT_ERROR(
                        "The protocol '%s' is not supported.", 
                        STR(protocol));
            
                options->setExitStatus(S9sOptions::BadOptions);
                return false;
            }
        }

        success = registerNdbCluster(
                mySqlHosts, mgmdHosts, ndbdHosts, osUserName);
    } else {
        PRINT_ERROR("Register cluster is currently implemented only for "
                "some cluster types.");
        options->setExitStatus(S9sOptions::BadOptions);
        return success;
    }

    return success;
}

/**
 * \param clusterId the ID of the cluster for which the CPU information will be
 *   fetched.
 * \param statName cpustat sqlstatsum sqlstat
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::getStats(
        const int        clusterId,
        const S9sString &statName)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      begin   = options->begin();
    S9sString      end     = options->end();
    S9sString      uri = "/v2/stat";
    S9sVariantMap  request;
    bool           retval;
    time_t         now = time(NULL);

    request["operation"]  = "statByName";
    request["name"]       = statName;
    request["with_hosts"] = true;

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = clusterId;
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    if (!begin.empty())
        request["start_datetime"] = begin;

    if (!end.empty())
        request["end_datetime"] = end;

    if (begin.empty() && end.empty())
    {
        request["startdate"]  = (ulonglong) now - 60 * 60;
        request["enddate"]    = (ulonglong) now;
    }

    retval = executeRequest(uri, request);
    
    return retval;
}


/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 * \param osUserName the user name to be used to SSH to the host.
 * \param vendor the name of the database vendor to install.
 * \param mySqlVersion the MySql version to install.
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 *
 */
bool
S9sRpcClient::createGaleraCluster(
        const S9sVariantList &hosts,
        const S9sString      &osUserName,
        const S9sString      &vendor,
        const S9sString      &mySqlVersion)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job     = composeJob();
    S9sVariantMap   jobData = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       uri = "/v2/jobs/";
    
    if (hosts.size() < 1u)
    {
        PRINT_ERROR("Missing node list while creating Galera cluster.");
        return false;
    }

    addCredentialsToJobData(jobData);
    
    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "galera";
    jobData["nodes"]            = nodesField(hosts);
    jobData["vendor"]           = vendor;
    jobData["version"]          = mySqlVersion;
    jobData["mysql_password"]   = options->dbAdminPassword();
    
    if (options->hasRemoteClusterIdOption())
        jobData["remote_cluster_id"] = options->remoteClusterId();

    if (options->noInstall())
    {
        jobData["install_software"] = false;
        jobData["enable_uninstall"] = false;
    } else {
        jobData["install_software"] = true;
        jobData["enable_uninstall"] = true;
    }
    
    if (options->hasBackupId())
        jobData["backup_id"]        = options->backupId();
    
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();
    
    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]          = "create_cluster";
    jobSpec["job_data"]         = jobData;

    // 
    // The job instance describing how the job will be executed.
    //
    job["title"]                = "Create Galera Cluster";
    job["job_spec"]             = jobSpec;

    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]        = "createJobInstance";
    request["job"]              = job;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::createMySqlSingleCluster(
        const S9sVariantList &hosts,
        const S9sString      &osUserName,
        const S9sString      &vendor,
        const S9sString      &mySqlVersion)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job = composeJob();
    S9sVariantMap   jobData = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       uri = "/v2/jobs/";
    
    if (hosts.size() < 1u)
    {
        PRINT_ERROR("Missing node list while creating Galera cluster.");
        return false;
    }

    addCredentialsToJobData(jobData);
    
    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "mysql_single";
    jobData["nodes"]            = nodesField(hosts);
    jobData["vendor"]           = vendor;
    jobData["version"]          = mySqlVersion;
    jobData["mysql_password"]   = options->dbAdminPassword();
    
    if (options->hasRemoteClusterIdOption())
        jobData["remote_cluster_id"] = options->remoteClusterId();
    
    if (options->noInstall())
    {
        jobData["install_software"] = false;
        jobData["enable_uninstall"] = false;
    } else {
        jobData["install_software"] = true;
        jobData["enable_uninstall"] = true;
    }
    
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();
    
    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]          = "create_cluster";
    jobSpec["job_data"]         = jobData;

    // 
    // The job instance describing how the job will be executed.
    //
    job["title"]                = "Create Single MySql Instance";
    job["job_spec"]             = jobSpec;

    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]        = "createJobInstance";
    request["job"]              = job;
    
    return executeRequest(uri, request);
}

/**
 * http://52.58.107.236/cmon-docs/current/cmonjobs.html#add_cluster1
 */
bool
S9sRpcClient::registerGaleraCluster(
        const S9sVariantList &hosts,
        const S9sString      &osUserName)

{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job = composeJob();
    S9sVariantMap   jobData = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       uri = "/v2/jobs/";

    if (hosts.size() < 1u)
    {
        PRINT_ERROR(
                "Nodes are not specified while registering existing cluster.");
        return false;
    }

    addCredentialsToJobData(jobData);
    
    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "galera";
    jobData["nodes"]            = nodesField(hosts);
    jobData["vendor"]           = options->vendor();
    jobData["db_user"]          = options->dbAdminUserName();
    jobData["db_password"]      = options->dbAdminPassword();
    
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();
    

    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]          = "add_cluster";
    jobSpec["job_data"]         = jobData;
    
    // 
    // The job instance describing how the job will be executed.
    //
    job["title"]                = "Register Galera";
    job["job_spec"]             = jobSpec;
    
    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]        = "createJobInstance";
    request["job"]              = job;
    
    return executeRequest(uri, request);
}

/**
 * This method will create a new job that will register a host into a cluster. 
 *
 * Registering means the host is already installed and set up, it just needs to
 * be added to the controller so it "knows" about it.
 */
bool
S9sRpcClient::registerHost()
{
    S9sOptions     *options      = S9sOptions::instance();
    S9sVariantList  hosts        = options->nodes();
    int             clusterId;
    S9sVariantMap   request = composeRequest();
    S9sVariantMap   job = composeJob();
    S9sVariantMap   jobData = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       uri = "/v2/jobs/";
    S9sString       protocol;
    S9sString       command, title;

    /*
     * Doing some preliminary checks.
     */
    if (hosts.empty())
    {
        PRINT_ERROR(
                "Node list is empty while registering node.\n"
                "Use the --nodes command line option to provide the node list."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    } else if (hosts.size() > 1u)
    {
        PRINT_ERROR("Registering nodes can only be done one-by-one.");
        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }
   
    if (options->hasClusterIdOption())
    {
        clusterId = options->clusterId();
    } else {
        PRINT_ERROR("Cluster ID is missing.");
        PRINT_ERROR("Use the --cluster-id to provide the cluster ID.");
        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    protocol = hosts[0].toNode().protocol().toLower();
    if (protocol == "maxscale")
    {
        command = "maxscale";
        title   = "Register MaxScale Node";
    } else if (protocol == "pgbouncer")
    {
        command = "pgbouncer";
        title   = "Register PgBouncer Node";
    } else if (protocol == "pbmagent")
    {
        command = "pbmagent";
        title   = "Register PBMAgent Node";
    } else if (protocol == "proxysql")
    {
        command = "proxysql";
        title   = "Register ProxySql Node";
    } else if (protocol == "haproxy")
    {
        command = "haproxy";
        title   = "Register HaProxy Node";
    } else if (protocol == "keepalived")
    {
        command = "keepalived";
        title   = "Register Keepalived Node";
    
        jobData["eth_interface"] = options->getString("eth_interface");
        jobData["virtual_ip"]    = options->getString("virtual_ip");
    } else {
        command = protocol;
        title   = "Register Node";
    }

    jobData["action"] = "register";
    jobData["nodes"] = nodesField(hosts);

    // The jobspec describing the command.
    jobSpec["command"]    = command;
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = title;
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;

    return executeRequest(uri, request);
}

/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 * \param osUserName the user name to be used to SSH to the host.
 * \param vendor the name of the database vendor to install.
 * \param mySqlVersion the MySql version to install. 
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 */
bool
S9sRpcClient::createMySqlReplication(
        const S9sVariantList &hosts,
        const S9sString      &osUserName,
        const S9sString      &vendor,
        const S9sString      &mySqlVersion)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job     = composeJob();
    S9sVariantMap   jobData = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       uri = "/v2/jobs/";
    bool            retval;
    
    if (hosts.size() < 1u)
    {
        PRINT_ERROR("Missing node list while creating Galera cluster.");
        return false;
    }

    addCredentialsToJobData(jobData);
    
    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "replication";
    jobData["topology"]         = topologyField(hosts);
    jobData["nodes"]            = nodesField(hosts);
    jobData["vendor"]           = vendor;
    jobData["version"]          = mySqlVersion;
    jobData["type"]             = "mysql";
    jobData["mysql_password"]   = options->dbAdminPassword();
   
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    if (options->noInstall())
    {
        jobData["install_software"] = false;
        jobData["enable_uninstall"] = false;
    } else {
        jobData["install_software"] = true;
        jobData["enable_uninstall"] = true;
    }

    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]    = "create_cluster";
    jobSpec["job_data"]   = jobData;

    // 
    // The job instance describing how the job will be executed.
    //
    job["title"]          = "Create MySQL Replication Cluster";
    job["job_spec"]       = jobSpec;

    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    //request["cluster_id"] = 0;
    
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * http://52.58.107.236/cmon-docs/current/cmonjobs.html#add_cluster1
 */
bool
S9sRpcClient::registerMySqlReplication(
        const S9sVariantList &hosts,
        const S9sString      &osUserName)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job     = composeJob();
    S9sVariantMap   jobData = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       uri = "/v2/jobs/";

    if (hosts.size() < 1u)
    {
        PRINT_ERROR(
                "Nodes are not specified while registering existing cluster.");
        return false;
    }

    addCredentialsToJobData(jobData);
    
    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "replication";
    jobData["nodes"]            = nodesField(hosts);
    jobData["vendor"]           = options->vendor();
    jobData["db_user"]          = options->dbAdminUserName();
    jobData["db_password"]      = options->dbAdminPassword();
    
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]          = "add_cluster";
    jobSpec["job_data"]         = jobData;
    
    // 
    // The job instance describing how the job will be executed.
    //
    job["title"]                = "Register MySql Replication";
    job["job_spec"]             = jobSpec;
    
    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]        = "createJobInstance";
    request["job"]              = job;
    
    return executeRequest(uri, request);
}

/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 * \param osUserName the user name to be used to SSH to the host.
 * \param vendor the name of the database vendor to install.
 * \param mySqlVersion the MySql version to install. 
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 */
bool
S9sRpcClient::createGroupReplication(
        const S9sVariantList &hosts,
        const S9sString      &osUserName,
        const S9sString      &vendor,
        const S9sString      &mySqlVersion)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantList hostNames;
    S9sVariantMap  request;
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;
    
    if (hosts.size() < 1u)
    {
        PRINT_ERROR("Missing node list while creating Galera cluster.");
        return false;
    }

    addCredentialsToJobData(jobData);
    
    for (uint idx = 0; idx < hosts.size(); ++idx)
    {
        if (hosts[idx].isNode())
            hostNames << hosts[idx].toNode().hostName();
        else
            hostNames << hosts[idx];
    }

    // The job_data describing the cluster.
    jobData["cluster_type"]     = "group_replication";
    jobData["nodes"]            = nodesField(hosts);
    // This is the deprecated argument passing... the new is "nodes"
    //jobData["mysql_hostnames"]  = hostNames;
    jobData["master_address"]   = hostNames[0].toString();
    jobData["vendor"]           = vendor;
    jobData["version"]          = mySqlVersion;
    jobData["type"]             = "mysql";
    jobData["mysql_password"]   = options->dbAdminPassword();
    
    if (options->noInstall())
    {
        jobData["install_software"] = false;
        jobData["enable_uninstall"] = false;
    } else {
        jobData["install_software"] = true;
        jobData["enable_uninstall"] = true;
    }
   
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    // The jobspec describing the command.
    jobSpec["command"]    = "create_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Create MySQL Replication Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = 0;
    
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * http://52.58.107.236/cmon-docs/current/cmonjobs.html#add_cluster1
 */
bool
S9sRpcClient::registerGroupReplication(
        const S9sVariantList &hosts,
        const S9sString      &osUserName)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       uri = "/v2/jobs/";

    if (hosts.size() < 1u)
    {
        PRINT_ERROR(
                "Nodes are not specified while registering existing cluster.");
        return false;
    }

    addCredentialsToJobData(jobData);
    
    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "group_replication";
    jobData["nodes"]            = nodesField(hosts);
    jobData["vendor"]           = options->vendor();
    jobData["db_user"]          = options->dbAdminUserName();
    jobData["db_password"]      = options->dbAdminPassword();

    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]          = "add_cluster";
    jobSpec["job_data"]         = jobData;
    
    // 
    // The job instance describing how the job will be executed.
    //
    job["title"]                = "Register MySql Replication";
    job["job_spec"]             = jobSpec;
    
    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]        = "createJobInstance";
    request["job"]              = job;
    
    return executeRequest(uri, request);
}

/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 *
 */
bool
S9sRpcClient::createNdbCluster(
        const S9sVariantList &mySqlHosts,
        const S9sVariantList &mgmdHosts,
        const S9sVariantList &ndbdHosts,
        const S9sString      &osUserName, 
        const S9sString      &vendor,
        const S9sString      &mySqlVersion)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  mySqlHostNames, mgmdHostNames, ndbdHostNames;
    S9sVariantMap   request;
    S9sVariantMap   job = composeJob();
    S9sVariantMap   jobData = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       uri = "/v2/jobs/";
    bool            retval;

    addCredentialsToJobData(jobData);
    
    for (uint idx = 0; idx < mySqlHosts.size(); ++idx)
    {
        if (mySqlHosts[idx].isNode())
            mySqlHostNames << mySqlHosts[idx].toNode().hostName();
        else
            mySqlHostNames << mySqlHosts[idx];
    }
    
    for (uint idx = 0; idx < mgmdHosts.size(); ++idx)
    {
        if (mgmdHosts[idx].isNode())
            mgmdHostNames << mgmdHosts[idx].toNode().hostName();
        else
            mgmdHostNames << mgmdHosts[idx];
    }
    
    for (uint idx = 0; idx < ndbdHosts.size(); ++idx)
    {
        if (ndbdHosts[idx].isNode())
            ndbdHostNames << ndbdHosts[idx].toNode().hostName();
        else
            ndbdHostNames << ndbdHosts[idx];
    }
    
    // The job_data describing the cluster.
    jobData["cluster_type"]     = "mysqlcluster";
    jobData["type"]             = "mysql";
    jobData["mysql_hostnames"]  = mySqlHostNames;
    jobData["mgmd_hostnames"]   = mgmdHostNames;
    jobData["ndbd_hostnames"]   = ndbdHostNames;
    jobData["vendor"]           = vendor;
    jobData["version"]          = mySqlVersion;
    jobData["disable_selinux"]  = true;
    jobData["disable_firewall"] = true;
    
    if (options->hasRemoteClusterIdOption())
        jobData["remote_cluster_id"] = options->remoteClusterId();
    
    if (options->noInstall())
    {
        jobData["install_software"] = false;
        jobData["enable_uninstall"] = false;
    } else {
        jobData["install_software"] = true;
        jobData["enable_uninstall"] = true;
    }
    
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    // The jobspec describing the command.
    jobSpec["command"]    = "create_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Create NDB Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = 0;
    
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 *
 */
bool
S9sRpcClient::registerNdbCluster(
        const S9sVariantList &mySqlHosts,
        const S9sVariantList &mgmdHosts,
        const S9sVariantList &ndbdHosts,
        const S9sString      &osUserName)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  mySqlHostNames, mgmdHostNames, ndbdHostNames;
    S9sVariantMap   request;
    S9sVariantMap   job = composeJob();
    S9sVariantMap   jobData = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       uri = "/v2/jobs/";

    addCredentialsToJobData(jobData);
    
    for (uint idx = 0; idx < mySqlHosts.size(); ++idx)
    {
        if (mySqlHosts[idx].isNode())
            mySqlHostNames << mySqlHosts[idx].toNode().hostName();
        else
            mySqlHostNames << mySqlHosts[idx];
    }
    
    for (uint idx = 0; idx < mgmdHosts.size(); ++idx)
    {
        if (mgmdHosts[idx].isNode())
            mgmdHostNames << mgmdHosts[idx].toNode().hostName();
        else
            mgmdHostNames << mgmdHosts[idx];
    }
    
    for (uint idx = 0; idx < ndbdHosts.size(); ++idx)
    {
        if (ndbdHosts[idx].isNode())
            ndbdHostNames << ndbdHosts[idx].toNode().hostName();
        else
            ndbdHostNames << ndbdHosts[idx];
    }
    
    // The job_data describing the cluster.
    jobData["cluster_type"]     = "mysqlcluster";
    jobData["type"]             = "mysql";
    jobData["mysql_hostnames"]  = mySqlHostNames;
    jobData["mgmd_hostnames"]   = mgmdHostNames;
    jobData["ndbd_hostnames"]   = ndbdHostNames;
    jobData["db_user"]          = options->dbAdminUserName();
    jobData["db_password"]      = options->dbAdminPassword();
    
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();


    // The jobspec describing the command.
    jobSpec["command"]    = "add_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Register NDB Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = 0;
    
    return executeRequest(uri, request);
}

/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 * \param osUserName the user name to be used to SSH to the host.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * This method will create a job that creates a single server PostgreSQL
 * cluster.
 */
bool
S9sRpcClient::createPostgreSql(
        const S9sVariantList &hosts,
        const S9sString      &osUserName,
        const S9sString      &psqlVersion)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job = composeJob();
    S9sVariantMap   jobData = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       uri = "/v2/jobs/";

    if (hosts.size() < 1u)
    {
        PRINT_ERROR("Missing node list while creating PostgreSQL cluster.");
        return false;
    }

    addCredentialsToJobData(jobData);

    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "postgresql_single";
    jobData["type"]             = "postgresql";
    jobData["nodes"]            = nodesField(hosts);
    jobData["version"]          = psqlVersion;
    jobData["postgre_user"]     = options->dbAdminUserName();
    jobData["postgre_password"] = options->dbAdminPassword();

    if (options->withTimescaleDb())
        jobData["install_timescaledb"] = true;
    
    if (options->hasRemoteClusterIdOption())
        jobData["remote_cluster_id"] = options->remoteClusterId();

    if (options->noInstall())
    {
        jobData["install_software"] = false;
        jobData["enable_uninstall"] = false;
    } else {
        jobData["install_software"] = true;
        jobData["enable_uninstall"] = true;
    } 

    if (options->hasBackupId())
        jobData["backup_id"]        = options->backupId();

    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]          = "create_cluster";
    jobSpec["job_data"]         = jobData;

    // 
    // The job instance describing how the job will be executed.
    //
    job["title"]                = "Creating PostgreSQL Cluster";
    job["job_spec"]             = jobSpec;

    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]        = "createJobInstance";
    request["job"]              = job;
    
    return executeRequest(uri, request);
}

/**
 * http://52.58.107.236/cmon-docs/current/cmonjobs.html#add_cluster1
 */
bool
S9sRpcClient::registerPostgreSql(
        const S9sVariantList &hosts,
        const S9sString      &osUserName)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job = composeJob();
    S9sVariantMap   jobData = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       uri = "/v2/jobs/";

    if (hosts.size() < 1u)
    {
        PRINT_ERROR(
                "Nodes are not specified while registering existing cluster.");
        return false;
    }

    addCredentialsToJobData(jobData);

    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "postgresql_single";
    jobData["nodes"]            = nodesField(hosts);

    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]          = "add_cluster";
    jobSpec["job_data"]         = jobData;
    
    // 
    // The job instance describing how the job will be executed.
    //
    job["title"]                = "Register PostgreSQL";
    job["job_spec"]             = jobSpec;
    
    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]        = "createJobInstance";
    request["job"]              = job;
    
    return executeRequest(uri, request);
}

/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 */
bool
S9sRpcClient::createMongoCluster(
        const S9sVariantList &hosts,
        const S9sString      &osUserName,
        const S9sString      &vendor,
        const S9sString      &mongoVersion)
{
    S9sMap<S9sString, S9sVariantList> nodelistMap;
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  mongosList, configList;
    S9sVariantMap   request;
    S9sVariantMap   job = composeJob();
    S9sVariantMap   jobData = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       uri = "/v2/jobs/";
    bool            retval;

    if (hosts.size() < 1u)
    {
        PRINT_ERROR("Missing node list while creating Mongo cluster.");
        return false;
    }

    addCredentialsToJobData(jobData);

    jobData["nodes"]            = nodesField(hosts);

    // The job_data describing the cluster.
    jobData["cluster_type"]     = "mongodb";
    jobData["vendor"]           = vendor;
    jobData["mongodb_version"]  = mongoVersion;

    if (options->hasRemoteClusterIdOption())
        jobData["remote_cluster_id"] = options->remoteClusterId();

    jobData["mongodb_user"]     = options->dbAdminUserName();
    jobData["mongodb_password"] = options->dbAdminPassword();
    
    if (options->noInstall())
    {
        jobData["install_software"] = false;
        jobData["enable_uninstall"] = false;
    } else {
        jobData["install_software"] = true;
        jobData["enable_uninstall"] = true;
    }

    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    // The jobspec describing the command.
    jobSpec["command"]    = "create_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Create Mongo Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This method is executed when the --add-node option is used like in 
 * s9s cluster --add-node --cluster-id=X --nodes=
 */
bool 
S9sRpcClient::createNode()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList hosts;
    S9sRpcReply    reply;
    bool           hasHaproxy    = false;
    bool           hasKeepalived = false;
    bool           hasPgBouncer  = false;
    bool           hasPBMAgent   = false;
    bool           hasNFSClient  = false;
    bool           hasProxySql   = false;
    bool           hasMaxScale   = false;
    bool           hasMongo      = false;
    bool           success;

    hosts = options->nodes();
    if (hosts.empty())
    {
        PRINT_ERROR(
                "Node list is empty while adding node.\n"
                "Use the --nodes command line option to provide the node list."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    for (uint idx = 0u; idx < hosts.size(); ++idx)
    {
        S9sString protocol = hosts[idx].toNode().protocol().toLower();

        if (protocol == "haproxy")
        {
            hasHaproxy = true;
        } else if (protocol == "keepalived")
        {
            hasKeepalived = true;
    	} else if (protocol == "pgbouncer")
        {
            hasPgBouncer = true;
    	} else if (protocol == "pbmagent")
        {
            hasPBMAgent = true;
    	} else if (protocol == "nfsclient")
        {
            hasNFSClient = true;
        } else if (protocol == "proxysql")
        {
            hasProxySql = true;
        } else if (protocol == "maxscale")
        {
            hasMaxScale = true;
        } else if (protocol == "mongodb")
        {
            hasMongo = true;
        } else if (protocol == "mongocfg")
        {
            hasMongo = true;
        } else if (protocol == "mongos")
        {
            hasMongo = true;
        } else if (protocol == "mgmd" || protocol == "ndb_mgmd")
        {
        } else if (protocol == "mysql")
        {
        } else if (protocol == "postgresql")
        {
        } else if (protocol.empty())
        {
        } else {
            PRINT_ERROR(
                    "The protocol '%s' is not supported.", 
                    STR(protocol));
        
            options->setExitStatus(S9sOptions::BadOptions);
            return false;
        }
    }

    /*
     * Running the request on the controller.
     */
    if (hasPgBouncer && hasProxySql) 
    {
        PRINT_ERROR(
                "It is not possible to add a PgBouncer and a ProxySql node "
                "in one call.");

        return false;
    } else if (hasHaproxy && hasProxySql) 
    {
        PRINT_ERROR(
                "It is not possible to add a HaProxy and a ProxySql node "
                "in one call.");

        return false;
    } else if (hasHaproxy && hasMaxScale)
    {
        PRINT_ERROR(
                "It is not possible to add a HaProxy and a MaxScale node "
                "in one call.");

        return false;
    } else if (hasProxySql && hasMaxScale)
    {
        PRINT_ERROR(
                "It is not possible to add a ProxySql and a MaxScale node "
                "in one call.");

        return false;
    } else if (hasProxySql)
    {
        success = addProxySql(hosts);
    } else if (hasHaproxy)
    {
        success = addHaProxy(hosts);
    } else if (hasKeepalived)
    {
        success = addKeepalived(hosts);
    } else if (hasPgBouncer)
    {
        success = addPgBouncer(hosts);
    } else if (hasPBMAgent)
    {
        success = addPBMAgent(hosts);
    } else if (hasNFSClient)
    {
        success = addNFSClient(hosts);
    } else if (hasMaxScale)
    {
        success = addMaxScale(hosts);
    } else if (hasMongo)
    {
        success = addMongoNode(hosts);
    } else {
        int nSlaves  = 0;
        int nMasters = 0;

        for (uint idx = 0; idx < hosts.size(); ++idx)
        {
            const S9sNode &node = hosts[idx].toNode();
            bool           master = node.property("master").toBoolean();
            bool           slave = node.property("slave").toBoolean();

            S9S_DEBUG("[%02u] %5s %s", 
                    idx, 
                    master ? "true" : "false",
                    slave  ? "true" : "false",
                    STR(node.hostName()));

            if (slave)
                nSlaves += 1;

            if (master)
                nMasters += 1;
        }
        
        if (nSlaves == 0 || nMasters == 0)
            success = addNode(hosts);
        else
            success = addReplicationSlave(hosts);
    }

    return success;
}

/**
 * This method is executed when the --reconfigure option is used like in 
 * s9s cluster --reinstall-node --cluster-id=X --nodes=
 */
bool 
S9sRpcClient::reinstallNode()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList hosts;
    S9sRpcReply    reply;
    bool           hasPBMAgent   = false;
    bool           hasNFSClient  = false;
    bool           success       = true;

    hosts = options->nodes();
    if (hosts.empty())
    {
        PRINT_ERROR(
                "Node list is empty while reinstalling node.\n"
                "Use the --nodes command line option to provide the node list."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    for (uint idx = 0u; idx < hosts.size(); ++idx)
    {
        S9sString protocol = hosts[idx].toNode().protocol().toLower();

    	if (protocol == "pbmagent")
        {
            hasPBMAgent = true;
    	} else if (protocol == "nfsclient")
        {
            hasNFSClient = true;
        } else {
            PRINT_ERROR(
                    "The protocol '%s' is not supported.", 
                    STR(protocol));
        
            options->setExitStatus(S9sOptions::BadOptions);
            return false;
        }
    }

    /*
     * Running the request on the controller.
     */
    if (hasPBMAgent)
    {
        success = reinstallPBMAgent(hosts);
    } else if (hasNFSClient)
    {
        success = reinstallNFSClient(hosts);
    }

    return success;
}

/**
 * This method is executed when the --reconfigure option is used like in 
 * s9s cluster --reconfigure-node --cluster-id=X --nodes=
 */
bool 
S9sRpcClient::reconfigureNode()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList hosts;
    S9sRpcReply    reply;
    bool           hasPBMAgent   = false;
    bool           hasNFSClient  = false;
    bool           success       = true;

    hosts = options->nodes();
    if (hosts.empty())
    {
        PRINT_ERROR(
                "Node list is empty while reconfiguring node.\n"
                "Use the --nodes command line option to provide the node list."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    for (uint idx = 0u; idx < hosts.size(); ++idx)
    {
        S9sString protocol = hosts[idx].toNode().protocol().toLower();

    	if (protocol == "pbmagent")
        {
            hasPBMAgent = true;
    	} else if (protocol == "nfsclient")
        {
            hasNFSClient = true;
        } else {
            PRINT_ERROR(
                    "The protocol '%s' is not supported.", 
                    STR(protocol));
        
            options->setExitStatus(S9sOptions::BadOptions);
            return false;
        }
    }

    /*
     * Running the request on the controller.
     */
    if (hasPBMAgent)
    {
        success = reconfigurePBMAgent(hosts);
    } else if (hasNFSClient)
    {
        success = reconfigureNFSClient(hosts);
    }

    return success;
}



/**
 * \param clusterId The ID of the cluster.
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Creates a job that will add a new node to the cluster.
 */
bool
S9sRpcClient::addNode(
        const S9sVariantList &hosts)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;

    if (hosts.size() != 1u)
    {
        PRINT_ERROR("Addnode is currently implemented only for one node.");
        return false;
    }
    
    // The job_data describing the cluster.
    jobData["node"] = hosts[0].toVariantMap();
    if (options->noInstall())
    {
        jobData["install_software"] = false;
        jobData["enable_uninstall"] = false;
    } else {
        jobData["install_software"] = true;
        jobData["enable_uninstall"] = true;
    }
        
    jobData["disable_firewall"] = true;
    jobData["disable_selinux"]  = true;
   
    // The jobspec describing the command.
    jobSpec["command"]    = "addnode";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Add Node to Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
 
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * This method is very similar to the addNode() method, but it adds a slave to
 * an existing cluster.
 */
bool
S9sRpcClient::addReplicationSlave(
        const S9sVariantList &hosts)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;
    S9sNode        master;
    S9sNode        slave;
    int            nSlaves  = 0;
    int            nMasters = 0;
    
    /*
     * Finding the slave and the master.
     */
    for (uint idx = 0; idx < hosts.size(); ++idx)
    {
        const S9sNode &node = hosts[idx].toNode();
        bool           isMaster = node.property("master").toBoolean();
        bool           isSlave = node.property("slave").toBoolean();

        if (isSlave)
        {
            nSlaves += 1;
            slave = hosts[idx].toNode();
        }

        if (isMaster)
        {
            nMasters += 1;
            master = hosts[idx].toNode();
        }
    }

    if (nSlaves != 1 || nMasters != 1)
    {
        PRINT_ERROR("To add a slave to an existing master one slave and"
                " one master has to be specified.");
        return false;
    }

    // The job_data describing the cluster.
    jobData["master_address"]   = master.hostName();
    jobData["slave_address"]    = slave.hostName();

    if (slave.hasPort())
        jobData["port"]         = slave.port();

    jobData["install_software"] = !options->noInstall();
    jobData["disable_firewall"] = true;
    jobData["disable_selinux"]  = true;
   
    // The jobspec describing the command.
    jobSpec["command"]    = "add_replication_slave";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Add Slave to Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::addHaProxy(
        const S9sVariantList &hosts)
{
    S9sVariantMap  request = composeRequest();
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sVariantList haProxyNodes;
    S9sVariantList otherNodes;
    S9sString      nodeAddresses;
    bool           retval;

    S9sNode::selectByProtocol(hosts, haProxyNodes, otherNodes, "haproxy");

    if (haProxyNodes.size() < 1u)
    {
        PRINT_ERROR(
            "To add a HAProxy one needs to specify one or more HAProxy nodes.");
        
        return false;
    }
    
    // The job_data describing the cluster.
    jobData["action"]   = "setupHaProxy";
    jobData["nodes"]    = nodesField(haProxyNodes);        
    
    for (uint idx = 0u; idx < otherNodes.size(); ++idx)
    {
        int       port;
        S9sNode   node;
        S9sString tmp;

        node = otherNodes[idx].toNode();
        port = node.hasPort() ? node.port() : 3306;

        tmp.sprintf("%s:%d:%s", STR(node.hostName()), port, "active");

        if (!nodeAddresses.empty())
            nodeAddresses += ";";

        nodeAddresses += tmp;
    }

    if (!nodeAddresses.empty())
        jobData["node_addresses"] = nodeAddresses;

    // The jobspec describing the command.
    jobSpec["command"]    = "haproxy";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Add HaProxy to Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::addKeepalived(
        const S9sVariantList &hosts)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantMap  request = composeRequest();
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sVariantList keepalivedNodes;
    S9sVariantList otherNodes;
    S9sString      nodeAddresses;
    bool           retval;

    S9sNode::selectByProtocol(hosts, keepalivedNodes, otherNodes, "keepalived");

    if (keepalivedNodes.size() < 1u)
    {
        PRINT_ERROR(
            "To add a Keepalived one needs to specify one or more"
            " Keepalived nodes.");
        
        return false;
    }
    
    // The job_data describing the cluster.
    jobData["action"]   = "setupKeepalived";
    jobData["nodes"]    = nodesField(keepalivedNodes);        
    
    // These are for keepalived.
    jobData["eth_interface"] = options->getString("eth_interface");
    jobData["virtual_ip"]    = options->getString("virtual_ip");
    
    for (uint idx = 0u; idx < otherNodes.size(); ++idx)
    {
        int       port;
        S9sNode   node;
        S9sString tmp;

        node = otherNodes[idx].toNode();
        port = node.hasPort() ? node.port() : 3306;

        tmp.sprintf("%s:%d:%s", STR(node.hostName()), port, "active");

        if (!nodeAddresses.empty())
            nodeAddresses += ";";

        nodeAddresses += tmp;
    }

    if (!nodeAddresses.empty())
        jobData["node_addresses"] = nodeAddresses;

    // The jobspec describing the command.
    jobSpec["command"]    = "keepalived";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Add Keepalived to Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::addPgBouncer(
        const S9sVariantList &hosts)
{
    S9sVariantMap  request = composeRequest();
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sVariantList nodes;
    S9sVariantList otherNodes;
    bool           retval;

    S9sNode::selectByProtocol(hosts, nodes, otherNodes, "pgbouncer");

    if (nodes.size() < 1u)
    {
        PRINT_ERROR(
            "To add a PgBouncer one needs to specify"
            " one or more PgBouncer nodes.");
        
        return false;
    }
    
    // The job_data describing the cluster.
    jobData["action"]   = "setup";
    jobData["nodes"]    = nodesField(nodes);        

    // The jobspec describing the command.
    jobSpec["command"]    = "pgbouncer";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Add PgBouncer to Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::addPBMAgent(
        const S9sVariantList &hosts)
{
    S9sVariantMap  request = composeRequest();
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sVariantList nodes;
    S9sVariantList otherNodes;
    bool           retval;

    S9sNode::selectByProtocol(hosts, nodes, otherNodes, "pbmagent");

    if (nodes.size() < 1u)
    {
        PRINT_ERROR(
            "To add a PBMAgent one needs to specify"
            " one or more PBMAgent nodes.");
        
        return false;
    }
    
    // The job_data describing the cluster.
    jobData["action"]   = "setup";
    jobData["nodes"]    = nodesField(nodes);

    // The jobspec describing the command.
    jobSpec["command"]    = "pbmagent";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Add PBMAgent to Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::addNFSClient(
        const S9sVariantList &hosts)
{
    S9sVariantMap  request             = composeRequest();
    S9sVariantMap  job                 = composeJob();
    S9sVariantMap  jobData             = composeJobData();
    S9sString      uri                 = "/v2/jobs/";
    S9sVariantMap  jobSpec;
    S9sVariantList nodes;
    S9sVariantList otherNodes;
    bool           retval;

    S9sNode::selectByProtocol(hosts, nodes, otherNodes, "nfsclient");

    if (nodes.size() < 1u)
    {
        PRINT_ERROR(
            "To add a NFSClient one needs to specify"
            " one or more NFSClient nodes.");
        
        return false;
    }
    
    // The job_data describing the cluster.
    jobData["action"]   = "setup";
    jobData["nodes"]    = nodesField(nodes);

    // The jobspec describing the command.
    jobSpec["command"]    = "nfsclient";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Add NFSClient to Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * A function to create a job that will add a proxysql host to the cluster.
 */
bool
S9sRpcClient::addProxySql(
        const S9sVariantList &hosts)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sVariantList proxyNodes;
    S9sVariantList otherNodes;
    bool           retval;

    S9sNode::selectByProtocol(hosts, proxyNodes, otherNodes, "proxysql");
    if (proxyNodes.size() < 1u)
    {
        PRINT_ERROR(
               "To add a ProxySql one needs to specify"
               " one or more ProxySql nodes.");
         
        return false;
    }
    
    if (otherNodes.size() > 1u)
    {
        PRINT_ERROR(
                "Specifying extra nodes when adding ProxySql is not"
                " supported.");

        return false;
    }

    // The job_data describing the cluster.
    jobData["action"]   = "setupProxySql";
    jobData["nodes"]    = nodesField(proxyNodes);

    if (options->hasProviderVersion())
        jobData["version"] = options->providerVersion();
        
    // The jobspec describing the command.
    jobSpec["command"]    = "proxysql";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Add ProxySQL to Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * A function to create a job that will add a maxscale host to the cluster.
 */
bool
S9sRpcClient::addMaxScale(
        const S9sVariantList &hosts)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sVariantList maxScaleNodes;
    S9sVariantList otherNodes;
    S9sString      nodeAddresses;
    bool           retval;

    S9sNode::selectByProtocol(hosts, maxScaleNodes, otherNodes, "maxscale");

    if (maxScaleNodes.size() != 1u)
    {
        PRINT_ERROR(
                "To add a MaxScale one needs to specify exactly"
                " one MaxScale node.");

        return false;
    }

    for (uint idx = 0u; idx < otherNodes.size(); ++idx)
    {
        int       port;
        S9sNode   node;
        S9sString tmp;

        node = otherNodes[idx].toNode();
        port = node.hasPort() ? node.port() : 3306;

        tmp.sprintf("%s:%d", STR(node.hostName()), port);

        if (!nodeAddresses.empty())
            nodeAddresses += ";";

        nodeAddresses += tmp;
    }

    if (!nodeAddresses.empty())
        jobData["node_addresses"] = nodeAddresses;
    
    // The job_data describing the cluster.
    jobData["action"]   = "setupMaxScale";
    jobData["nodes"]    = nodesField(maxScaleNodes);

    // The jobspec describing the command.
    jobSpec["command"]    = "maxscale";
    jobSpec["job_data"]   = jobData;

    if (!options->schedule().empty())
        job["scheduled"] = options->schedule(); 

    // The job instance describing how the job will be executed.
    job["title"]          = "Add MaxScale to Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Creates a job that will add a new mongo node to the cluster.
 */
bool
S9sRpcClient::addMongoNode(
        const S9sVariantList &hosts)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job     = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;

    if (hosts.size() != 1u)
    {
        PRINT_ERROR("Addnode is currently implemented only for one node.");
        return false;
    }

    S9sNode node = hosts[0].toNode();
    S9sString protocol = node.protocol().toLower();

    // The job_data describing the cluster.
    #ifdef SEND_NODES
    jobData["nodes"] = nodesField(hosts);
    #else
    if (hosts[0].isNode())
        jobData["hostname"] = hosts[0].toNode().hostName();
    else
        jobData["hostname"] = hosts[0].toString();
    #endif

    if (node.hasProperty("rs"))
        jobData["replicaset"] = node.property("rs").toString();

    if (protocol == "mongos")
    {
        jobData["node_type"] = "mongos";
    } else if (protocol == "mongocfg")
    {
        jobData["node_type"] = "mongocfg";
    } else if (protocol == "mongodb")
    {
        jobData["node_type"] = "mongodb";

        if (node.hasProperty("arbiter_only") &&
                node.property("arbiter_only").toBoolean())
            jobData["node_type"] = "arbiter";
    } //else the caller method is buggy

    jobData["install_software"] = !options->noInstall();
    jobData["disable_firewall"] = true;
    jobData["disable_selinux"]  = true;

    // The jobspec describing the command.
    jobSpec["command"]    = "addnode";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Add Node to Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::reconfigurePBMAgent(
        const S9sVariantList &hosts)
{
    S9sVariantMap  request = composeRequest();
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sVariantList nodes;
    S9sVariantList otherNodes;
    bool           retval;

    S9sNode::selectByProtocol(hosts, nodes, otherNodes, "pbmagent");

    if (nodes.size() < 1u)
    {
        PRINT_ERROR(
            "To reconfigure PBMAgent one needs to specify"
            " one or more PBMAgent nodes.");
        
        return false;
    }
    
    // The job_data describing the cluster.
    jobData["action"]   = "reconfigure";
    jobData["nodes"]    = nodesField(nodes);

    // The jobspec describing the command.
    jobSpec["command"]    = "pbmagent";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Reconfigure PBMAgent node of Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::reconfigureNFSClient(
        const S9sVariantList &hosts)
{
    S9sVariantMap  request             = composeRequest();
    S9sVariantMap  job                 = composeJob();
    S9sVariantMap  jobData             = composeJobData();
    S9sString      uri                 = "/v2/jobs/";
    S9sVariantMap  jobSpec;
    S9sVariantList nodes;
    S9sVariantList otherNodes;
    bool           retval;

    S9sNode::selectByProtocol(hosts, nodes, otherNodes, "nfsclient");

    if (nodes.size() < 1u)
    {
        PRINT_ERROR(
            "To reconfigure NFSClient one needs to specify"
            " one or more NFSClient nodes.");
        
        return false;
    }
    
    // The job_data describing the cluster.
    jobData["action"]   = "reconfigure";
    jobData["nodes"]    = nodesField(nodes);

    // The jobspec describing the command.
    jobSpec["command"]    = "nfsclient";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Reconfigure NFSClient node in Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::reinstallPBMAgent(
        const S9sVariantList &hosts)
{
    S9sVariantMap  request = composeRequest();
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sVariantList nodes;
    S9sVariantList otherNodes;
    bool           retval;

    S9sNode::selectByProtocol(hosts, nodes, otherNodes, "pbmagent");

    if (nodes.size() < 1u)
    {
        PRINT_ERROR(
            "To reinstall PBMAgent one needs to specify"
            " one or more PBMAgent nodes.");
        
        return false;
    }
    
    // The job_data describing the cluster.
    jobData["action"]   = "reinstall";
    jobData["nodes"]    = nodesField(nodes);

    // The jobspec describing the command.
    jobSpec["command"]    = "pbmagent";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Reinstall PBMAgent node of Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::reinstallNFSClient(
        const S9sVariantList &hosts)
{
    S9sVariantMap  request             = composeRequest();
    S9sVariantMap  job                 = composeJob();
    S9sVariantMap  jobData             = composeJobData();
    S9sString      uri                 = "/v2/jobs/";
    S9sVariantMap  jobSpec;
    S9sVariantList nodes;
    S9sVariantList otherNodes;
    bool           retval;

    S9sNode::selectByProtocol(hosts, nodes, otherNodes, "nfsclient");

    if (nodes.size() < 1u)
    {
        PRINT_ERROR(
            "To reconfigure NFSClient one needs to specify"
            " one or more NFSClient nodes.");
        
        return false;
    }
    
    // The job_data describing the cluster.
    jobData["action"]   = "reinstall";
    jobData["nodes"]    = nodesField(nodes);

    // The jobspec describing the command.
    jobSpec["command"]    = "nfsclient";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Reinstall NFSClient node in Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \param hosts the hosts that will be removed from the cluster (variant list
 *   with S9sNode elements).
 * \returns true if the request was sent and the reply was received (even if the
 *   reply is an error notification).
 *
 * This function will create a "removeNode" job on the controller.
 */
bool
S9sRpcClient::removeNode()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList hosts     = options->nodes();
    S9sNode        host;
    S9sString      hostName, title;
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;

    if (hosts.empty())
    {
        PRINT_ERROR(
                "Node list is empty while removing node.\n"
                "Use the --nodes command line option to provide the node list."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }
/*
    if (hosts.size() != 1u)
    {
        PRINT_ERROR("Remove node is currently implemented only for one node.");
        return false;
    }
*/
    host     = hosts[0].toNode();
    hostName = host.hostName();
    title.sprintf("Remove '%s' from the Cluster", STR(hostName));

    // The job_data...
    jobData["nodes"]      = nodesField(hosts);
    jobData["enable_uninstall"] = options->uninstall();
     
    // The jobspec describing the command.
    jobSpec["command"]    = "removenode";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]            = title;
    job["job_spec"]         = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request was sent and the reply was received (even if the
 *   reply is an error notification).
 *
 * This function will stop the cluster by creating a "stop_cluster" job.
 */
bool
S9sRpcClient::stopCluster()
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sVariantMap  request     = composeRequest();
    S9sVariantMap  job         = composeJob();
    S9sVariantMap  jobData     = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      title;
    S9sString      uri         = "/v2/jobs/";
    bool           retval;
    
    title = "Stopping Cluster";

    // The job_data.
    jobData["stop_timeout"]        = 1800;
    
    if (options->hasMinutes())
        jobData["maintenance_minutes"] = options->minutes();

    if (!options->reason().empty())
        jobData["reason"] = options->reason();
    
    // The jobspec describing the command.
    jobSpec["command"]    = "stop_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = title;
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This method will create and send a start_cluster job.
 */
bool
S9sRpcClient::startCluster()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      title;
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;
   
    // The job_data
    if (!options->donor().empty())
        jobData["donor_address"] = options->donor();

    // The jobspec describing the command.
    jobSpec["command"]    = "start_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Starting Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This function will create and send a job to start a node of a cluster.
 */
bool
S9sRpcClient::startNode()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList hosts     = options->nodes();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sNode        node;
    bool           retval;
    
    if (hosts.size() != 1u)
    {
        PRINT_ERROR("To start a node exactly one node must be specified.");
        return false;
    } else {
        node = hosts[0].toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    #if 1
    jobData["node"]       = hosts[0].toVariantMap();
    #else
    jobData["hostname"]   = node.hostName();
    #endif
    
    if (node.hasPort())
        jobData["port"]   = node.port();
     
    // The jobspec describing the command.
    jobSpec["command"]    = "start";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Starting Node";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This function will create and send a job to stop a node of a cluster.
 */
bool
S9sRpcClient::stopNode()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList nodes     = options->nodes();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sNode        node;
    bool           retval;
    
    if (nodes.size() != 1u)
    {
        PRINT_ERROR("To stop a node exactly one node must be specified.");
        return false;
    } else {
        node = nodes[0].toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    jobData["node"]       = nodes[0].toVariantMap();
     
    if (options->force())
        jobData["force_stop"] = true;

    // The jobspec describing the command.
    jobSpec["command"]    = "stop";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Stopping Node";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest("/v2/jobs/", request);

    return retval;
}

/**
 *
 * { "command": "enable_binary_logging",  "job_data": { "auto_restart": "yes", "expire_logs_days": "1", "master_address": "10.10.10.20:3306" }}
 */
bool
S9sRpcClient::enableBinaryLogging()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList nodes     = options->nodes();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sNode        node;
    bool           retval;
    
    if (nodes.size() != 1u)
    {
        PRINT_ERROR("To enable binary logging one node must be specified.");
        return false;
    } else {
        node = nodes[0].toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]        = clusterId;
    //jobData["node"]       = nodes[0].toVariantMap();
    jobData["master_address"]   = nodes[0].toNode().hostName();
    jobData["auto_restart"]     = true;
    jobData["expire_logs_days"] = 1;
     
    // The jobspec describing the command.
    jobSpec["command"]    = "enable_binary_logging";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Enable Binary Logging";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest("/v2/jobs/", request);

    return retval;
}


bool
S9sRpcClient::setNodeReadOnly()
{
    return startNodeJob(
            "enable_db_readonly",
            "Setting Node to Read-only");
}

bool
S9sRpcClient::setNodeReadWrite()
{
    return startNodeJob(
            "disable_db_readonly",
            "Setting Node to Read-write");
}

/**
 * \returns true if the request was sent and the reply was received (even if the
 *   reply is an error notification).
 *
 * A private method to create a job that has one node of a cluster as argument.
 * We have many of these jobs, makes sense to have a method like this to make
 * them.
 */
bool
S9sRpcClient::startNodeJob(
        const S9sString &command,
        const S9sString &title)
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList hosts     = options->nodes();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sNode        node;
    bool           retval;
    
    if (hosts.size() != 1u)
    {
        PRINT_ERROR("Node was not provided.");
        return false;
    } else {
        node = hosts[0].toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    jobData["node"]       = hosts[0].toVariantMap();
     
    if (options->force())
        jobData["force_stop"] = true;

    // The jobspec describing the command.
    jobSpec["command"]    = command;
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = title;
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest("/v2/jobs/", request);

    return retval;
}

bool
S9sRpcClient::stopSlave()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sNode        node;
    bool           retval;
    
    if (!options->hasSlave())
    {
        PRINT_ERROR("To stop replication the slave must be specified.");
        PRINT_ERROR("Use the --slave or --replication-slave option.");
        return false;
    } else {
        node = options->slave().toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    jobData["replication_slave"] = node.toVariantMap();
     
    //if (options->force())
    //    jobData["force_stop"] = true;

    // The jobspec describing the command.
    jobSpec["command"]    = "stop_replication_slave";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Stopping Replication Slave";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::startSlave()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sNode        node;
    bool           retval;
    
    if (!options->hasSlave())
    {
        PRINT_ERROR("To start replication the slave must be specified.");
        PRINT_ERROR("Use the --slave or --replication-slave option.");
        return false;
    } else {
        node = options->slave().toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    jobData["replication_slave"] = node.toVariantMap();
     
    //if (options->force())
    //    jobData["force_stop"] = true;

    // The jobspec describing the command.
    jobSpec["command"]    = "start_replication_slave";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Starting Replication Slave";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::promoteReplicationSlave()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sNode        node;
    bool           retval;
    
    if (!options->hasSlave())
    {
        PRINT_ERROR("To promote a node the slave must be specified.");
        PRINT_ERROR("Use the --slave or --replication-slave option.");
        return false;
    } else {
        node = options->slave().toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    jobData["replication_slave"] = node.toVariantMap();
     
    //if (options->force())
    //    jobData["force_stop"] = true;

    // The jobspec describing the command.
    jobSpec["command"]    = "promote_replication_slave";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Promoting Replication Slave";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::resetSlave()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sNode        node;
    bool           retval;
    
    if (!options->hasSlave())
    {
        PRINT_ERROR("To reset the slave, the slave must be specified.");
        PRINT_ERROR("Use the --slave or --replication-slave option.");
        return false;
    } else {
        node = options->slave().toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    jobData["replication_slave"] = node.toVariantMap();
     
    // The jobspec describing the command.
    jobSpec["command"]    = "reset_replication_slave";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Resetting Replication Slave";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::failoverMaster()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sNode        node;
    bool           retval;
    
    if (!options->hasSlave())
    {
        PRINT_ERROR("To failover the slave must be specified.");
        PRINT_ERROR("Use the --slave or --replication-slave option.");
        return false;
    } else {
        node = options->slave().toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    jobData["replication_slave"] = node.toVariantMap();
    if (options->hasRemoteClusterIdOption())
        jobData["remote_cluster_id"] = options->remoteClusterId();
    
    if (options->hasMaster())
        jobData["replication_master"] = options->master().toVariantMap();
     
    // The jobspec describing the command.
    jobSpec["command"]    = "failover_replication_slave";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Failover Replication Slave";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::stageSlave()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sNode        node;
    bool           retval;
    
    if (!options->hasSlave())
    {
        PRINT_ERROR("To stage replication slave the slave must be specified.");
        PRINT_ERROR("Use the --slave or --replication-slave option.");
        return false;
    } else {
        node = options->slave().toNode();
    }

    if (!options->hasMaster())
    {
        PRINT_ERROR("To stage replication slave the master must be specified.");
        PRINT_ERROR("Use the --master or --replication-master option.");
        return false;
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    jobData["replication_slave"] = node.toVariantMap();
    
    if (options->hasRemoteClusterIdOption())
        jobData["remote_cluster_id"] = options->remoteClusterId();

    if (options->hasMaster())
        jobData["replication_master"] = options->master().toVariantMap();

    if (options->hasSynchronous())
        jobData["synchronous"] = options->isSynchronous();
     
    // The jobspec describing the command.
    jobSpec["command"]    = "stage_replication_slave";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Stage Replication Slave";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::toggleSync()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sNode        node;
    bool           retval;

    if (!options->hasSlave())
    {
        PRINT_ERROR("To toggle synchronous replication a slave must be specified.");
        PRINT_ERROR("Use the --slave or --replication-slave option.");
        return false;
    } else {
        node = options->slave().toNode();
    }

    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    jobData["node"]       = node.toVariantMap();

    if (options->hasSynchronous())
        jobData["synchronous"] = options->isSynchronous();

    // The jobspec describing the command.
    jobSpec["command"]    = "toggle_replication_sync";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Toggle Synchronous Replication";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}


/**
 * This function will create and send a job to stop and then start a node of a
 * cluster.
 */
bool
S9sRpcClient::restartNode()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList hosts     = options->nodes();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sNode        node;
    bool           retval;
    
    if (hosts.size() != 1u)
    {
        PRINT_ERROR("To restart a node exactly one node must be specified.");
        return false;
    } else {
        node = hosts[0].toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    #if 1
    jobData["node"] = hosts[0].toVariantMap();
    #else
    jobData["hostname"]   = node.hostName();
    #endif
    
    if (node.hasPort())
        jobData["port"]   = node.port();
     
    if (options->force())
        jobData["force_stop"] = true;

    // The jobspec describing the command.
    jobSpec["command"]    = "restart";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Restarting Node";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This function will create and send a job to stop a node of a cluster.
 */
bool
S9sRpcClient::promoteSlave()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList hosts     = options->nodes();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sNode        node;
    bool           retval;
    
    if (hosts.size() != 1u)
    {
        PRINT_ERROR("To promote a slave exactly one node must be specified.");
        return false;
    } else {
        node = hosts[0].toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    jobData["node"]       = hosts[0].toVariantMap();
     
    if (options->force())
        jobData["force_stop"] = true;

    // The jobspec describing the command.
    jobSpec["command"]    = "promote_replication_slave";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Promoting Slave";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::demoteNode()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList hosts     = options->nodes();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sNode        node;
    bool           retval;
    
    if (hosts.size() != 1u)
    {
        PRINT_ERROR("To demote a node exactly one node must be specified.");
        return false;
    } else {
        node = hosts[0].toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    jobData["node"]       = hosts[0].toVariantMap();
     
    if (options->force())
        jobData["force_stop"] = true;

    // The jobspec describing the command.
    jobSpec["command"]    = "demote_node";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Demoting Node";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::availableUpgrades()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList hosts     = options->nodes();
    S9sString      uri       = "/v2/clusters/";
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  database;
    bool           retval;

    request["operation"]      = "availableUpgrades";

    request["clusterid"]      = clusterId;
    if (hosts.size() != 0)
    {
	request["nodes"]      = nodesField(hosts);
    }

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This function will create and send a job to upgrade the
 * software packages for the cluster.
 */
bool
S9sRpcClient::upgradeCluster()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList hosts     = options->nodes();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;


    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    if (hosts.size() != 0)
    {
	jobData["nodes"]      = nodesField(hosts);
    }
    if (options->force())
        jobData["force"] = true;

    // The jobspec describing the command.
    jobSpec["command"]    = "upgrade_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Upgrade Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This function will create and send a job to check for
 * pacakge versions and possible upgrades of the
 * cluster softwares.
 */
bool
S9sRpcClient::checkPkgUpgrades()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList hosts     = options->nodes();
    S9sVariantMap  request   = composeRequest();
    S9sVariantMap  job = composeJob();
    S9sVariantMap  jobData = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;


    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    if (hosts.size() != 0)
    {
	jobData["nodes"]      = nodesField(hosts);
    }
    if (options->force())
        jobData["force"] = true;

    // The jobspec describing the command.
    jobSpec["command"]    = "check_pkg_upgrades";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Check for package upgrades";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request was sent and the reply was received (even if the
 *   reply is an error notification).
 *
 * This function will create a "remove_cluster" job that will eventually drop
 * the cluster from the cmon controller.
 */
bool
S9sRpcClient::dropCluster()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      title;
    S9sVariantMap  request;
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;
    
    title = "Remove Cluster";

    // The job_data describing the cluster that will be deleted. The drop
    // cluster can only be executed in cluster 0, so we can't put this into the
    // request. 
    if (options->hasClusterIdOption())
        jobData["clusterid"]    = options->clusterId();

    // Well, this is not going to work.
    if (options->hasClusterNameOption())
        jobData["cluster_name"] = options->clusterName();
    
    // The jobspec describing the command.
    jobSpec["command"]    = "remove_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = title;
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::checkHosts()
{
    S9sString      uri         = "/v2/discovery/";
    S9sVariantMap  request     = composeRequest();
    S9sOptions    *options     = S9sOptions::instance();
    S9sVariantList hosts       = options->nodes();
    S9sString      clusterType = options->clusterType();

    if (hosts.empty())
        return true;

    request["operation"]      = "checkHosts";
    request["nodes"]          = nodesField(hosts);
    request["check_if_already_registered"] = true;
    request["check_ssh_sudo"] = true;

    if (!clusterType.empty())
    {
        S9sVariantMap job, jobData, jobSpec;

        jobData["cluster_type"]   = clusterType;
        jobData["nodes"]          = nodesField(hosts);
        jobData["vendor"]         = options->vendor();
        jobData["version"]        = options->providerVersion();
    
        jobSpec["command"]        = "create_cluster";
        jobSpec["job_data"]       = jobData;

        job["class_name"]         = "CmonJobInstance";
        job["job_spec"]           = jobSpec;

        request["check_job"]      = true;
        request["job"]            = job;
    }

    #if 0
    S9sVariantMap credentials;

    credentials["class_name"]      = "CmonSshCredentials";
    credentials["user_name"]       = "pipas";
    credentials["password"]        = "";
    credentials["public_key_file"] = "./configs/testing_id_rsa";
    //credentials["port"]            = ;
    //credentials["timeout"]         = ;
    //credentials["tty_for_sudo"]    = ;
    request["ssh_credentials"]     = credentials;
    #endif

    return executeRequest(uri, request);
}

/**
 * This is where we send the "registerServers" request that will notify the
 * controller about a container server we want to use.
 */
bool
S9sRpcClient::registerServers()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList servers   = options->servers();
   
    request["operation"]      = "registerServers";
    request["servers"]        = serversField(servers);
    
    return executeRequest(uri, request);
}

/**
 * This function will install the cmon-cloud software on a computer and create a
 * new server of it. 
 */
bool
S9sRpcClient::createServer()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList servers   = options->servers();
    S9sVariantMap  serverMap;
    S9sString      uri       = "/v2/jobs/";
    S9sVariantMap  request;
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobData   = composeJobData();
    S9sVariantMap  jobSpec;
    
    if (servers.size() != 1u)
    {
        PRINT_ERROR(
                "The createServer is currently implemented only for"
                " one server at a time.");

        return false;
    }
    
    serverMap = servers[0].toVariantMap();
    if (options->hasSshCredentials())
    {
        serverMap["ssh_credentials"] = 
            options->sshCredentials(
                    "", serverMap["hostname"].toString()).toVariantMap();
    }
    
    jobData["server"]           = serverMap;
    jobData["install_software"] = !options->noInstall();
    jobData["disable_firewall"] = true;
    jobData["disable_selinux"]  = true;
    
    if (options->hasTimeout())
        jobData["timeout"] = options->timeout();

    // The jobspec describing the command.
    jobSpec["command"]          = "create_container_server";
    jobSpec["job_data"]         = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Create Container Server";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    return executeRequest(uri, request);
}

/**
 * Moves a CDT entry into a new folder.
 */
bool
S9sRpcClient::moveInTree(
        const S9sString &sourcePath,
        const S9sString &targetPath)
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;

    request["operation"]      = "move";
    request["source_path"]    = sourcePath;
    request["target_path"]    = targetPath;
    
    return executeRequest(uri, request);
}

/**
 * Calls the "move" or "rename" on the tree. If the second argument has a "/" it
 * is move, if it has no "/" it is rename.
 */
bool
S9sRpcClient::renameOrMove()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      source, target;

    if (options->nExtraArguments() != 2)
    {
        PRINT_ERROR(
                "The --move option requires two command line arguments: "
                "the source path and the target path.");

        return false;
    }
  
    source = options->extraArgument(0u);
    target = options->extraArgument(1u);

    if (target.contains("/"))
        return moveInTree(source, target);

    return rename(source, target);
}

bool
S9sRpcClient::rename(
        const S9sString &sourcePath,
        const S9sString &targetName)
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;

    request["operation"]      = "rename";
    request["source_path"]    = sourcePath;
    request["target_name"]    = targetName;
    
    return executeRequest(uri, request);
}

/**
 * Executes the cat command, gets a content of a node in the CDT tree.
 */
bool
S9sRpcClient::cat()
{
    S9sString        uri = "/v2/tree/";
    S9sVariantMap    request;
    S9sOptions      *options = S9sOptions::instance();

    if (options->nExtraArguments() == 1)
    {
        request["path"] = options->extraArgument(0u);
    } else {
        PRINT_ERROR(
                "The --cat option requires one command line argument: "
                "the path of the object.");

        return false;
    }
   
    request["operation"]      = "cat";
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::getObject()
{
    S9sOptions      *options = S9sOptions::instance();

    if (options->nExtraArguments() == 1)
        return getObject(options->extraArgument(0u));

    PRINT_ERROR("The path is not provided.");
    return false;
}


bool
S9sRpcClient::getObject(
        const S9sString &path)
{
    S9sString        uri = "/v2/tree/";
    S9sVariantMap    request;

    request["path"] = path;
    request["operation"]      = "getObject";
    
    return executeRequest(uri, request);
}


/**
 * Sends the getAcl request to the controller.
 */
bool
S9sRpcClient::getAcl()
{
    S9sString        uri = "/v2/host/";
    S9sVariantMap    request;
    S9sOptions      *options = S9sOptions::instance();
    S9sVariantList   servers = options->servers();

    if (!servers.empty())
    {
        uri = "/v2/host/";
        request["servers"] = serversField(servers);
    } else if (options->nExtraArguments() == 1)
    {
        uri = "/v2/tree/";
        request["path"] = options->extraArgument(0u);
    } else {
        PRINT_ERROR(
                "The --get-acl option requires one command line argument: "
                "the path of the object.");

        return false;
    }
   
    request["operation"]      = "getAcl";
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::startServers()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantList servers = options->servers();
   
    request["operation"]      = "startServers";
    request["servers"]        = serversField(servers);
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::stopServers()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList servers   = options->servers();
   
    request["operation"]      = "shutdownServers";
    request["servers"]        = serversField(servers);
    
    return executeRequest(uri, request);
}

/**
 * This method will start an object found in the tree.
 */
bool
S9sRpcClient::startInTree()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    
    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --start option requires one command line argument: "
                "the path of the object.");

        return false;
    }
 
    request["operation"]      = "start";
    request["path"]           = options->extraArgument(0u);
    
    return executeRequest(uri, request);
}

/**
 * Sends a request to add an ACL.
 */
bool
S9sRpcClient::addAcl()
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      aclString = options->acl();

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --add-acl option requires one command line argument: "
                "the path of the object.");

        return false;
    }
   
    if (aclString.empty())
    {
        PRINT_ERROR("The --add-acl requires the --acl=STRING option.");

        return false;
    }

    request["operation"]      = "addAcl";
    request["path"]           = options->extraArgument(0u);
    request["acl"]            = aclString;

    return executeRequest(uri, request);
}

bool
S9sRpcClient::addTag()
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      tagString = options->getString("tag");

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --add-tag option requires one command line argument: "
                "the path of the object.");

        return false;
    }
   
    if (tagString.empty())
    {
        PRINT_ERROR("The --add-tag requires the --tag=STRING option.");
        return false;
    }

    request["operation"]      = "appendTag";
    request["path"]           = options->extraArgument(0u);
    request["tag"]            = tagString;

    return executeRequest(uri, request);
}

bool
S9sRpcClient::removeTag()
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      tagString = options->getString("tag");

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --remove-tag option requires one command line argument: "
                "the path of the object.");

        return false;
    }
   
    if (tagString.empty())
    {
        PRINT_ERROR("The --remove-tag requires the --tag=STRING option.");
        return false;
    }

    request["operation"]      = "removeTag";
    request["path"]           = options->extraArgument(0u);
    request["tag"]            = tagString;

    return executeRequest(uri, request);
}

/**
 * FIXME: This is not fully imlemented.
 */
bool
S9sRpcClient::checkAccess()
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      privileges = options->privileges();

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --access option requires one command line argument: "
                "the path of the object.");

        return false;
    }
   
    if (privileges.empty())
    {
        PRINT_ERROR("The --access requires the --privileges=STRING option.");

        return false;
    }

    request["operation"]      = "checkAccess";
    request["path"]           = options->extraArgument(0u);
    request["privileges"]     = privileges;

    return executeRequest(uri, request);
}


/**
 * Sends a request to add an ACL.
 */
bool
S9sRpcClient::removeAcl()
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      aclString = options->acl();

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --add-acl option requires one command line argument: "
                "the path of the object.");

        return false;
    }
   
    if (aclString.empty())
    {
        PRINT_ERROR("The --add-acl requires the --acl=STRING option.");

        return false;
    }
   
    request["operation"]      = "removeAcl";
    request["path"]           = options->extraArgument(0u);
    request["acl"]            = aclString;

    return executeRequest(uri, request);
}

/**
 * Sends a request to change the owner of a CDT entry. 
 */
bool
S9sRpcClient::chOwn()
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      aclString = options->acl();

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --chown option requires one command line argument: "
                "the path of the object.");

        return false;
    }
   
    if (!options->hasOwner())
    {
        PRINT_ERROR("The --chown requires the --owner=USERNAME option.");
        return false;
    }
   
    request["operation"]        = "chown";
    request["path"]             = options->extraArgument(0u);
    request["recursive"]        = options->isRecursiveRequested();
    request["owner_user_name"]  = options->ownerUserName();
    request["owner_group_name"] = options->ownerGroupName();

    return executeRequest(uri, request);
}

bool
S9sRpcClient::mkdir()
{
    S9sOptions    *options   = S9sOptions::instance();
    
    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --mkdir option requires one command line argument: "
                "the full path of the folder to be created.");

        return false;
    }

    return mkdir(options->extraArgument(0u));
}

/**
 * Makes a folder in the CDT tree.
 */
bool
S9sRpcClient::mkdir(
        const S9sString &fullPath)
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;

    request["operation"]      = "mkdir";
    request["path"]           = fullPath;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::rmdir()
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    
    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --rmdir option requires one command line argument: "
                "the full path of the folder to be removed.");

        return false;
    }

    request["operation"]      = "rmdir";
    request["path"]           = options->extraArgument(0u);
    
    return executeRequest(uri, request);
}

/**
 * Makes a file in the CDT tree.
 */
bool
S9sRpcClient::mkfile(
        const S9sString &fullPath)
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;

    request["operation"]      = "mkfile";
    request["path"]           = fullPath;
    
    return executeRequest(uri, request);
}

/**
 * Makes a file in the CDT tree.
 */
bool
S9sRpcClient::mkfile()
{
    S9sOptions    *options   = S9sOptions::instance();
    
    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --create option requires one command line argument: "
                "the full path of the file to be created.");

        return false;
    }

    return mkfile(options->extraArgument(0u));    
}

bool
S9sRpcClient::enableCmonHa()
{
    S9sString      fullPath = "/.runtime/cmon_ha/enabled";
    S9sString      content  = "true";
    
    return setContent(fullPath, content);
}

bool
S9sRpcClient::setContent(
        const S9sString &fullPath,
        const S9sString &content)
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;

    request["operation"]      = "setcontent";
    request["path"]           = fullPath;
    request["content"]        = content;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::setContent()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      fullPath;
    S9sString      content;

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --save option requires one command line argument: "
                "the full path of the CDT entry to be saved.");

        return false;
    } else if (!options->inputFile().empty())
    {
        S9sFile inputFile(options->inputFile());
        if (!inputFile.readTxtFile(content))
        {
            PRINT_ERROR("%s\n", STR(inputFile.errorString()));
            return false;
        }
    } else {
        std::string s; 
        while(std::getline(std::cin, s))
        {
            content += s;
            content += "\n";
        }
    }
    
    fullPath = options->extraArgument(0u);


    return setContent(fullPath, content);
}


bool
S9sRpcClient::deleteFromTree()
{
    S9sOptions    *options   = S9sOptions::instance();
    
    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --delete option requires one command line argument: "
                "the full path of the object to be removed.");

        return false;
    }

    return deleteFromTree(options->extraArgument(0u));
}

bool
S9sRpcClient::deleteFromTree(
        const S9sString &path)
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;

    request["operation"]      = "delete";
    request["path"]           = path;
    
    return executeRequest(uri, request);
}

/**
 * Unregisters one or more CmonContainerServer in one step.
 */
bool
S9sRpcClient::unregisterServers()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList servers   = options->servers();
   
    request["operation"]      = "unregisterServers";
    request["servers"]        = serversField(servers);
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::unregisterHost()
{
    S9sString      uri       = "/v2/host/";
    S9sVariantMap  request   = composeRequest();
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList servers   = options->servers();
    S9sVariantList hosts;
    
    /*
     * Finding the host(s).
     */
    hosts = options->nodes();
    if (hosts.empty())
    {
        PRINT_ERROR(
            "Node list is empty while unregistering nodes.\n"
            "Use the --nodes command line option to provide a node."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    } else if (hosts.size() > 1u)
    {
        PRINT_ERROR(
            "Only one node can be unregister at a time."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    request["operation"]      = "unregisterHost";
    request["host"]           = hosts[0];

    return executeRequest(uri, request);
}

bool
S9sRpcClient::getContainers()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList servers   = options->servers();
   
    request["operation"]     = "getContainers";

    if (!servers.empty())
        request["servers"]   = serversField(servers);
    
    return executeRequest(uri, request);
}

/**
 * Gets the servers from the controller.
 */
bool
S9sRpcClient::getServers()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList servers   = options->servers();
   
    request["operation"]      = "getServers";

    if (!servers.empty())
        request["servers"]    = serversField(servers);
    
    if (options->isRefreshRequested())
        request["refresh_now"] = true;
    
    return executeRequest(uri, request);
}

/**
 * Gets the controllers from the controller. :)
 */
bool
S9sRpcClient::getControllers()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList servers   = options->servers();
   
    request["operation"]      = "getControllers";
 
    return executeRequest(uri, request);
}

/**
 * Creates a container by initiating a job. Like this:
 * \code
   #s9s container --create --log --debug
 * \endcode
 *
 * Here is an example of what we send:
 * \code
 * {
 *     "job": 
 *     {
 *         "class_name": "CmonJobInstance",
 *         "job_spec": 
 *         {
 *             "command": "create_container",
 *             "job_data": 
 *             {
 *                 "container": 
 *                 {
 *                     "class_name": "CmonContainer",
 *                     "parent_server": "core2"
 *                 }
 *             }
 *         },
 *         "title": "Create Container"
 *     },
 *     "operation": "createJobInstance",
 *     "request_created": "2019-10-28T06:40:58.472Z",
 *     "request_id": 3
 * }
 * \endcode
 */
bool
S9sRpcClient::createContainerWithJob()
{
    S9sOptions    *options      = S9sOptions::instance();
    S9sString      templateName = options->templateName();
    S9sString      imageName    = options->imageName();
    S9sString      imageOsUser  = options->imageOsUser();
    S9sString      cloudName    = options->cloudName();
    S9sVariantList servers      = options->servers();
    S9sString      subnetId     = options->subnetId();
    S9sString      vpcId        = options->vpcId();
    S9sVariantList volumes      = options->volumes();
    S9sVariantMap  request;
    S9sVariantMap  job          = composeJob();
    S9sVariantMap  jobData      = composeJobData(true);
    S9sVariantMap  jobSpec;
    S9sContainer   container;
    S9sString      uri = "/v2/jobs/";
  
    addCredentialsToJobData(jobData);

    /*
     * Setting up a container object from the command line options.
     */
    if (!templateName.empty())
        container.setTemplate(templateName);
    
    if (!imageName.empty())
        container.setImage(imageName);

    if (!imageOsUser.empty())
        container.setImageOsUser(imageOsUser);

    if (!cloudName.empty())
        container.setProvider(cloudName);

    if (options->nExtraArguments() == 1)
        container.setAlias(options->extraArgument(0));
    
    if (!cloudName.empty())
        container.setProvider(cloudName);

    if (!subnetId.empty())
        container.setSubnetId(subnetId);
            
    if (!volumes.empty())
        container.setVolumes(volumes);

    if (options->hasFirewalls())
        container.setFirewalls(options->firewalls());

    if (!options->region().empty())
        container.setRegion(options->region());

    if (servers.size() > 1)
    {
        PRINT_ERROR("Currently only one server can be defined for containers.");
        return false;
    } else if (servers.size() == 1) 
    {
        container.setParentServerName(servers[0u].toNode().hostName());
    }
        
    /*
     * Composing the request.
     */
    if (jobData.contains("containers"))
    {
        jobSpec["command"]    = "create_containers";
        job["title"]          = "Create Containers";
    } else {
        jobData["container"]  = container.toVariantMap();
        jobSpec["command"]    = "create_container";
        job["title"]          = "Create Container";
    }

    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["job_spec"]       = jobSpec;
    
    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    return executeRequest(uri, request);
}

/**
 * This will delete a container by registering a "delete_container" job.
 *
 * \code{.js}
 * {
 *     "command": "delete_container",
 *     "id": 6,
 *     "job_data": 
 *     {
 *         "container": 
 *         {
 *             "alias": "ft_containers_12848",
 *             "parent_server": "core1"
 *         },
 *         "servers": [ 
 *         {
 *             "class_name": "CmonContainerServer",
 *             "hostname": "core1"
 *         } ]
 *     },
 *     "owner_user_id": 3,
 *     "owner_user_name": "pipas"
 * }
 * \endcode
 */
bool
S9sRpcClient::deleteContainerWithJob()
{
    S9sVariantMap  job      = composeJob();
    S9sVariantMap  jobData  = composeJobDataOneContainer();
    S9sVariantMap  jobSpec;
    S9sVariantMap  request;
    S9sString      uri = "/v2/jobs/";
   
    /*
     * Checking the command line options happen in previously called functions.
     */
    if (jobData.empty())
        return false;

    /*
     * Composing the request.
     */
    jobSpec["command"]    = "delete_container";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Delete Container";
    job["job_spec"]       = jobSpec;
    
    // The request describing we want to start a new job. 
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::startContainerWithJob()
{
    S9sVariantMap  job      = composeJob();
    S9sVariantMap  jobData  = composeJobDataOneContainer();
    S9sVariantMap  jobSpec;
    S9sVariantMap  request;
    S9sString      uri = "/v2/jobs/";
   
    /*
     * Checking the command line options happen in previously called functions.
     */
    if (jobData.empty())
        return false;

    /*
     * Composing the request.
     */
    jobSpec["command"]    = "start_container";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Start Container";
    job["job_spec"]       = jobSpec;
    
    // The request describing we want to start a new job. 
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::stopContainerWithJob()
{
    S9sVariantMap  job      = composeJob();
    S9sVariantMap  jobData  = composeJobDataOneContainer();
    S9sVariantMap  jobSpec;
    S9sVariantMap  request;
    S9sString      uri = "/v2/jobs/";
   
    /*
     * Checking the command line options happen in previously called functions.
     */
    if (jobData.empty())
        return false;

    /*
     * Composing the request.
     */
    jobSpec["command"]    = "stop_container";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["title"]          = "Stop Container";
    job["job_spec"]       = jobSpec;
    
    // The request describing we want to start a new job. 
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    return executeRequest(uri, request);
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::getSupportedClusterTypes()
{
    S9sString      uri = "/v2/discovery/";
    S9sVariantMap  request;
   
    request["operation"]     = "getSupportedClusterTypes";
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::checkClusterName()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/discovery/";
    S9sVariantMap  request;
   
    request["operation"]        = "checkClusterName";
    request["new_cluster_name"] = options->clusterName();
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::getSshCredentials()
{
    S9sString      uri       = "/v2/discovery/";
    S9sVariantMap  request   = composeRequest();
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList hosts     = options->nodes();
   
    if (hosts.empty())
        return true;

    request["operation"]      = "getSshCredentials";
    
    return executeRequest(uri, request);
}

/**
 * Places a job that will create a new backup.
 *
 * Using this function to place a scheduled backup is not working because it is
 * incompatible with the backend.
 */
bool
S9sRpcClient::createBackup()
{
    S9sOptions     *options      = S9sOptions::instance();
    S9sVariantList  hosts        = options->nodes();
    S9sString       backupMethod = options->backupMethod();
    S9sVariantMap   request      = composeRequest();
    S9sVariantMap   job          = composeBackupJob();
    S9sNode         backupHost;
    S9sString       uri = "/v2/jobs/";
    bool            retval;

    // The cluster must be specified.
    if (!options->hasClusterIdOption() && !options->hasClusterNameOption())
    {
        PRINT_ERROR("The cluster ID or the cluster name must be specified.");
        return false;
    }

    // There can be 1 or 0 nodes specified.
    if (hosts.size() > 1u)
    {
        PRINT_ERROR("Multiple nodes are specified while creating a backup.");
        return false;
    }

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::createBackupSchedule()
{
    S9sOptions     *options      = S9sOptions::instance();    
    S9sVariantMap   schedule;
    S9sVariantMap   request      = composeRequest();
    S9sVariantMap   job          = composeBackupJob();
    S9sString       uri          = "/v2/backup/";

    // The cluster must be specified.
    if (!options->hasClusterIdOption() && !options->hasClusterNameOption())
    {
        PRINT_ERROR("The cluster ID or the cluster name must be specified.");
        return false;
    }

    if (options->recurrence().empty())
    {
        PRINT_ERROR("The recurrence must be specified.");
        return false;
    }


    // The recurrence is not for the job, it is for the backup here.
    job.erase("recurrence");

    schedule["class_name"] = "CmonBackupSchedule";
    schedule["enabled"]    = true;
    schedule["job"]        = job["job_spec"].toVariantMap();
    // For some reasong the CmonBackupSchedule calls the recurrence "schedule".
    schedule["schedule"]   = options->recurrence();

    request["operation"]   = "scheduleBackup";
    request["schedule"]    = schedule;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::verifyBackup()
{
    S9sOptions     *options      = S9sOptions::instance();
    S9sString       clusterName  = options->clusterName();
    S9sVariantMap   request      = composeRequest();
    S9sVariantMap   job          = composeJob();
    S9sVariantMap   jobData      = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       title;
    S9sString       uri = "/v2/jobs/";
    bool            retval;

    if (!options->hasBackupId())
    {
        PRINT_ERROR("To verify a backup a backup ID has to be provided.");
        return false;
    }

    if (options->testServer().empty())
    {
        PRINT_ERROR("To verify a backup a test server has to be provided.");
        return false;
    }
    
    title.sprintf("Verify Backup %d", options->backupId());

    jobData["backupid"]            = options->backupId();
    jobData["server_address"]      = options->testServer();
    jobData["disable_firewall"]    = true;
    jobData["disable_selinux"]     = true;
    jobData["install_software"]    = !options->noInstall();
    jobData["terminate_db_server"] = !options->noTerminate();
    
    // The jobspec describing the command.
    jobSpec["command"]    = "verify_backup";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = title;
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    retval = executeRequest(uri, request);

    return retval;

}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Creates a job to restore a pre-existing backup.
 */
bool
S9sRpcClient::restoreBackup()
{
    S9sOptions     *options      = S9sOptions::instance();
    int             backupId     = options->backupId();
    S9sString       backupMethod = options->backupMethod();
    S9sVariantMap   request      = composeRequest();
    S9sVariantMap   job          = composeJob();
    S9sVariantMap   jobData      = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       uri          = "/v2/jobs/";
    S9sString       pitrStopTime = options->pitrStopTime();
    bool            retval;

    // The job_data describing how the backup will be created.
    jobData["backupid"]   = backupId;
    jobData["bootstrap"]  = true;
    jobData["backup_datadir_before_restore"] = options->backupDatadir();

    if (!pitrStopTime.empty())
        jobData["pitr_stop_time"] = pitrStopTime;

    if (!options->nodes().empty())
    {
        // on which node we want to restore the backup
        S9sNode   node    = options->nodes()[0].toNode();
        S9sString address = node.hostName();

        if (node.hasPort())
            address.sprintf("%s:%d", STR(node.hostName()), node.port());

        jobData["server_address"] = address;
    }

    if (!options->databases().empty())
        jobData["database"] = options->databases();

    // The jobspec describing the command.
    jobSpec["command"]    = "restore_backup";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = "Restore Backup";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    retval = executeRequest(uri, request);
    
    return retval;
}

/**
 * \param clusterId The cluster to read or <= 0 to read all the clusters
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * The method that gets the list of backups from the server.
 */
bool
S9sRpcClient::getBackups(
        const int clusterId)
{
    S9sString      uri = "/v2/backup/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getBackups";
    request["ascending"] = true;

    if (clusterId > 0)
        request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * Gets the list of backup schedules from the controller.
 */
bool
S9sRpcClient::getBackupSchedules(
        const int clusterId)
{
    S9sString      uri = "/v2/backup/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getBackupSchedules";

    if (clusterId > 0)
        request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);

    return retval;
}


/**
 * \param backupId The ID of the backup to delete.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 */
bool
S9sRpcClient::deleteBackupRecord()
{
    S9sString      uri = "/v2/backup/";
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantMap  request;
    S9sVariantMap  backupMap;
    bool           retval;

    if (!options->hasBackupId())
    {
        PRINT_ERROR(
                "To delete a backup a backup ID has to be provided "
                "with the --backup-id command line option.");
        return false;
    }

    backupMap["backup_id"]   = options->backupId();

    request["operation"]     = "deleteBackupRecord";
    request["backup_record"] = backupMap;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::deleteOldBackups()
{
    S9sOptions     *options      = S9sOptions::instance();
    S9sVariantMap   request      = composeRequest();
    S9sVariantMap   job          = composeJob();
    S9sVariantMap   jobData      = composeJobData();
    S9sVariantMap   jobSpec;
    S9sString       title;
    S9sString       uri = "/v2/jobs/";
    bool            retval;

    if (!options->hasClusterIdOption() && !options->hasClusterNameOption())
    {
        PRINT_ERROR("The cluster ID or the cluster name must be specified.");
        return false;
    }

    title.sprintf("Delete Old Backups");
    
    if (options->hasBackupRetention())
        jobData["backup_retention"] = options->backupRetention();
    
    if (options->hasCloudRetention())
        jobData["cloud_retention"] = options->cloudRetention();
    
    if (options->hasSafetyCopies())
        jobData["safety_copies"] = options->safetyCopies();

    // The jobspec describing the command.
    jobSpec["command"]    = "delete_old_backups";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = title;
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * A function to add a new account to the cluster. With this account the user
 * can work on the production database.
 */
bool
S9sRpcClient::createAccount()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri     = "/v2/clusters/";
    S9sVariantMap  request = composeRequest();
    S9sAccount     account;
    bool           retval;

    account = options->account();
    account.setWithDatabase(options->withDatabase());
    account.setGrants(options->privileges());

    request["operation"]  = "createAccount";
    request["account"]    = account;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::getAccounts()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri       = "/v2/clusters/";
    S9sVariantMap  request   = composeRequest();

    // Building the request.
    request["operation"]  = "getAccounts";
    
    if (options->limit() >= 0)
        request["limit"] = options->limit();

    if (options->offset() >= 0)
        request["offset"] = options->offset();

    return executeRequest(uri, request);
}

/**
 * Gets the names of the spreadsheets.
 */
bool
S9sRpcClient::getSpreadsheets()
{
    //S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri       = "/v2/spreadsheets/";
    S9sVariantMap  request   = composeRequest();

    // Building the request.
    request["operation"]  = "getSpreadsheetNames";
    return executeRequest(uri, request);
}

bool
S9sRpcClient::getSpreadsheet()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri       = "/v2/spreadsheets/";
    S9sVariantMap  request   = composeRequest();
    
    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The command line argument should be the name "
                "of the spreadsheet.");

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    // Building the request.
    request["operation"]  = "getSpreadsheet";
    request["spreadsheet_name"] =  options->extraArgument(0);

    return executeRequest(uri, request);
}

bool
S9sRpcClient::createSpreadsheet()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri       = "/v2/spreadsheets/";
    S9sVariantMap  request   = composeRequest();
    S9sString      inputFileName = options->inputFile();

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The command line argument should be the name "
                "of the spreadsheet.");

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    if (!inputFileName.empty())
    {
        S9sFile   inputFile(inputFileName);
        S9sString content;
        bool      success;

        success = inputFile.readTxtFile(content);
        if (!success)
        {
            PRINT_ERROR("%s", STR(inputFile.errorString()));
            options->setExitStatus(S9sOptions::BadOptions);
            return false;
        }

        request["content"] = content;

        if (inputFileName.toLower().endsWith(".csv"))
            request["format"] = "csv";
    }

    // Building the request.
    request["operation"]  = "createSpreadsheet";
    request["spreadsheet_name"] =  options->extraArgument(0);

    return executeRequest(uri, request);
}

bool
S9sRpcClient::setCell(
        const S9sString &spreadsheetName,
        const int        sheetIndex,
        const int        columnIndex,
        const int        rowIndex,
        const S9sString &content)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri       = "/v2/spreadsheets/";
    S9sVariantMap  request   = composeRequest();
    
    // Building the request.
    request["operation"]        = "setCell";
    request["spreadsheet_name"] =  options->extraArgument(0);
    request["sheetIndex"]       = sheetIndex;
    request["rowIndex"]         = rowIndex;
    request["columnIndex"]      = columnIndex;
    request["contentString"]    = content;

    return executeRequest(uri, request);
}

bool
S9sRpcClient::getRepositories()
{
    #if 1
    static int testJob = 0;
    if (testJob == 0)
    {
        createLocalRepository(1, "galera", "percona", "5.6", "precise");
        ++testJob;
    }
    #endif

    S9sString      uri       = "/v2/repositories/";
    S9sVariantMap  request   = composeRequest();

    // Building the request.
    request["operation"]  = "getRepositories";

    return executeRequest(uri, request);
}

bool
S9sRpcClient::getSupportedSetups()
{
    S9sString      uri = "/v2/repositories/";
    S9sVariantMap  request;

    // Building the request.
    request["operation"]  = "getSupportedSetups";

    return executeRequest(uri, request);
}

/**
 * \param account The account for which we are granting privileges.
 * \param privileges The privileges in GSL language.
 * \returns True if the request sent and the reply received even if that reply
 *   is an error reply.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * A function to grant rights for an account that already exists.
 */
bool
S9sRpcClient::grantPrivileges(
        const S9sAccount &account,
        const S9sString  &privileges)
{
    S9sString      uri     = "/v2/clusters/";
    S9sVariantMap  request = composeRequest();

    request["operation"]  = "grantPrivileges";
    request["account"]    = account;
    request["privileges"] = privileges;

    return executeRequest(uri, request);
}

bool
S9sRpcClient::revokePrivileges(
        const S9sAccount &account,
        const S9sString  &privileges)
{
    S9sString      uri     = "/v2/clusters/";
    S9sVariantMap  request = composeRequest();

    request["operation"]  = "revokePrivileges";
    request["account"]    = account;
    request["privileges"] = privileges;

    return executeRequest(uri, request);
}

bool
S9sRpcClient::subscribeEvents(
    S9sJSonHandler  callbackFunction,
    void           *userData)
{
    bool retval;

    m_priv->m_callbackFunction = callbackFunction;
    m_priv->m_callbackUserData = userData;

    S9sString      uri     = "/v2/subscribe_events";
    S9sVariantMap  request = composeRequest();

    // FIXME: add filters or any other arguments
    request["operation"]  = "subscribe";

    // NOTE: this wont return (unless error happens or callback is NULL) as 
    // the JSon stream will be stopped only if the client (so S9S CLI)
    // closes the connection.
    retval = executeRequest(uri, request);
    setExitStatus();
    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * A function to delete an account from the cluster. 
 */
bool
S9sRpcClient::deleteAccount()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri     = "/v2/clusters/";
    S9sVariantMap  request = composeRequest();
    S9sAccount     account;
    bool           retval;

    account = options->account();
    // We don't need these, do we?
    //account.setWithDatabase(options->withDatabase());
    //account.setGrants(options->privileges());

    request["operation"]  = "deleteAccount";
    request["account"]    = account;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::createDatabase()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri     = "/v2/clusters/";
    S9sVariantMap  request = composeRequest();
    S9sVariantMap  database;
    bool           retval;

    database["class_name"]    = "CmonDataBase";
    database["database_name"] = options->dbName();

    request["operation"]      = "createDatabase";
    request["database"]       = database;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::saveScript(
        S9sString remoteFileName,
        S9sString content)
{
    S9sString      uri     = "/v2/imperative/";
    S9sVariantMap  request = composeRequest();

    request["operation"]      = "saveScript";
    request["filename"]       = remoteFileName;
    request["content"]        = content;

    return executeRequest(uri, request);
}

bool
S9sRpcClient::executeExternalScript(
        S9sString localFileName,
        S9sString content,
        S9sString arguments)
{
    S9sString      uri     = "/v2/imperative/";
    S9sVariantMap  request = composeRequest();

    request["operation"]      = "executeExternalScript";
    request["filename"]       = localFileName;
    request["content"]        = content;
    request["arguments"]      = arguments;

    return executeRequest(uri, request);
}


bool
S9sRpcClient::executeScript(
        S9sString remoteFileName,
        S9sString arguments)
{
    S9sString      uri     = "/v2/imperative/";
    S9sVariantMap  request = composeRequest();

    request["operation"]      = "executeScript";
    request["filename"]       = remoteFileName;
    request["arguments"]      = arguments;

    return executeRequest(uri, request);
}

bool
S9sRpcClient::removeScript(
        S9sString remoteFileName)
{
    S9sString      uri     = "/v2/imperative/";
    S9sVariantMap  request = composeRequest();

    request["operation"]      = "removeScript";
    request["filename"]       = remoteFileName;

    return executeRequest(uri, request);
}

bool
S9sRpcClient::treeScripts()
{
    S9sString      uri     = "/v2/imperative/";
    S9sVariantMap  request = composeRequest();

    request["operation"]      = "dirTree";

    return executeRequest(uri, request);
}

S9sVariantMap
S9sRpcClient::createUserRequest(
        const S9sUser   &user,
        const S9sString &newPassword,
        bool             createGroup)
{
    S9sVariantMap  request;

    request["operation"]    = "createUser";
    request["user"]         = user.toVariantMap();
    request["create_group"] = createGroup;

    if (!newPassword.empty())
        request["new_password"] = newPassword;

    return request;
}

bool
S9sRpcClient::createUser(
        const S9sUser   &user,
        const S9sString &newPassword,
        bool             createGroup)
{
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    
    request = createUserRequest(user, newPassword, createGroup);

    return executeRequest(uri, request);
}

/**
 * This call is for adding a new CmonGroup.
 */
bool
S9sRpcClient::createGroup()
{
    S9sOptions    *options  = S9sOptions::instance();
    S9sString      uri = "/v2/users/";
    S9sVariantMap  groupMap;
    S9sVariantMap  request;

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "One group name should be passed as command line argument "
                "when creating a new group.");

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    groupMap["class_name"] = "CmonGroup";
    groupMap["group_name"] =  options->extraArgument(0);

    request["operation"]    = "createGroup";
    request["group"]        = groupMap;

    return executeRequest(uri, request);
}

/**
 * This call is for deleting CmonGroup objects.
 */
bool
S9sRpcClient::deleteGroup()
{
    S9sOptions    *options  = S9sOptions::instance();
    S9sString      uri = "/v2/users/";
    S9sVariantMap  groupMap;
    S9sVariantMap  request;

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "One group name should be passed as command line argument "
                "when deleting a group.");

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    groupMap["class_name"] = "CmonGroup";
    groupMap["group_name"] =  options->extraArgument(0);

    request["operation"]    = "deleteGroup";
    request["group"]        = groupMap;

    return executeRequest(uri, request);
}

/**
 * \param user The user to be changed.
 * \param groupName The name of the group.
 * \param replacePrimaryGroup If true the primary group will be set, replaced by
 *   the group denoted by the group name.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Adds a user to a group or sets the user's primary group.
 */
bool
S9sRpcClient::addToGroup(
        const S9sUser     &user,
        const S9sString   &groupName,
        bool               replacePrimaryGroup)
{
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    
    request["operation"]    = "addToGroup";
    request["user"]         = user.toVariantMap();
    request["group_name"]   = groupName;
    request["replace_primary_group"] = replacePrimaryGroup;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::addToGroup()
{
    S9sOptions    *options  = S9sOptions::instance();
    S9sUser        user;
    S9sString      groupName;

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "One username should be passed as command line argument "
                "when adding a user to a group.");

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    user.setProperty("user_name", options->extraArgument(0));
    return addToGroup(user, options->group(), false);
}

bool
S9sRpcClient::setGroup()
{
    S9sOptions    *options  = S9sOptions::instance();
    S9sUser        user;

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "One username should be passed as command line argument "
                "when changing the group for a user.");

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    user.setProperty("user_name", options->extraArgument(0));
    return addToGroup(user, options->group(), true);
}

bool
S9sRpcClient::resetPassword()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/auth/";
    S9sVariantMap  request;
    
    request["operation"]    = "passwordReset"; 
    request["user_name"]    = options->userName();

    if (!options->token().empty())
        request["password_reset_token"]    = options->token();

    if (options->hasNewPassword())
        request["new_password"] = options->newPassword();

    return executeRequest(uri, request);
}

bool
S9sRpcClient::deleteUser()
{
    S9sOptions    *options  = S9sOptions::instance();
    S9sString      uri = "/v2/users/";
    S9sUser        user;
    S9sVariantMap  request;

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "One username should be passed as command line argument "
                "when deleting a user.");

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }
    
    user.setProperty("user_name", options->extraArgument(0));
    request["operation"]    = "deleteUser";
    request["user"]         = user.toVariantMap();

    return executeRequest(uri, request);    
}

bool
S9sRpcClient::removeFromGroup()
{
    S9sOptions    *options  = S9sOptions::instance();
    S9sString      uri = "/v2/users/";
    S9sUser        user;
    S9sVariantMap  request;
    S9sString      groupName;
    
    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "One username should be passed as command line argument "
                "when changing the group for a user.");

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }
    
    user.setProperty("user_name", options->extraArgument(0));
    groupName = options->group();
    
    request["operation"]    = "removeFromGroup";
    request["user"]         = user.toVariantMap();
    request["group_name"]   = groupName;
    
    return executeRequest(uri, request);
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * The method that gets the list of users from the server.
 */
bool
S9sRpcClient::getUsers()
{
    S9sString      uri          = "/v2/users/";
    S9sVariantMap  request      = composeRequest();
    bool           retval;

    request["operation"] = "getUsers";

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::whoAmI()
{
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]                = "whoAmI";
    request["with_extended_privileges"] = true;

    retval = executeRequest(uri, request);

    return retval;
}


bool
S9sRpcClient::getGroups()
{
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getGroups";

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::canCreateUser()
{
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "canCreateUser";

    retval = executeRequest(uri, request);

    return retval;
}



/**
 * \returns true if the request sent and a reply is received (even if the reply
 *   is an error message).
 *
 * This method is used to modify an existing Cmon User on the controller.
 */
bool
S9sRpcClient::setUser()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    S9sVariantMap  properties;
    bool           retval;

    if (options->nExtraArguments() > 1)
    {
        PRINT_ERROR("Only one user can be modified at once.");
        return false;
    }

    properties["class_name"] = "CmonUser";

    if (options->nExtraArguments() > 0)
    {
        properties["user_name"]  = options->extraArgument(0);
    } else {
        properties["user_name"] = options->userName();
    }

    if (!options->firstName().empty())
        properties["first_name"] = options->firstName();
    
    if (!options->lastName().empty())
        properties["last_name"] = options->lastName();
    
    if (!options->title().empty())
        properties["title"] = options->title();
    
    if (!options->emailAddress().empty())
        properties["email_address"] = options->emailAddress();

    request["operation"]  = "setUser";
    request["user"]       = properties;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::enableUser()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    S9sVariantMap  properties;
    bool           retval;

    if (options->nExtraArguments() > 1)
    {
        PRINT_ERROR("Only one user can be modified at once.");
        return false;
    }

    properties["class_name"] = "CmonUser";

    if (options->nExtraArguments() > 0)
    {
        properties["user_name"]  = options->extraArgument(0);
    } else {
        PRINT_ERROR("The user name should be passed as command line argument.");
    }

    request["operation"]  = "enable";
    request["user"]       = properties;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::disableUser()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    S9sVariantMap  properties;
    bool           retval;

    if (options->nExtraArguments() > 1)
    {
        PRINT_ERROR("Only one user can be modified at once.");
        return false;
    }

    properties["class_name"] = "CmonUser";

    if (options->nExtraArguments() > 0)
    {
        properties["user_name"]  = options->extraArgument(0);
    } else {
        PRINT_ERROR("The user name should be passed as command line argument.");
    }

    request["operation"]  = "disable";
    request["user"]       = properties;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::getKeys()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    S9sVariantMap  properties;

    if (options->nExtraArguments() > 1)
    {
        PRINT_ERROR("More than one user when getting keys.");
        return false;
    }
    
    properties["class_name"] = "CmonUser";
    if (options->nExtraArguments() > 0)
    {
        properties["user_name"]  = options->extraArgument(0);
    } else {
        properties["user_name"] = options->userName();
    }
    
    request["operation"]  = "getKeys";
    request["user"]       = properties;

    return executeRequest(uri, request);
}

bool
S9sRpcClient::addKey()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    S9sVariantMap  userMap, keyMap;
    S9sString      keyFileName = options->publicKeyPath();
    S9sFile        keyFile(keyFileName);
    S9sString      key;
    bool           success;

    /*
     * The user.
     */
    if (options->nExtraArguments() > 1)
    {
        PRINT_ERROR("More than one user when getting keys.");
        return false;
    }
    
    userMap["class_name"] = "CmonUser";
    if (options->nExtraArguments() > 0)
    {
        userMap["user_name"]  = options->extraArgument(0);
    } else {
        userMap["user_name"] = options->userName();
    }

    /*
     * The key.
     */
    if (keyFileName.empty())
    {
        PRINT_ERROR("The public key file was not specified.");
        PRINT_ERROR("Use the --public-key-path command line option to "
                "specify the public key file.");
        return false;
    }

    success = keyFile.readTxtFile(key);
    if (!success)
    {
        PRINT_ERROR("%s", STR(keyFile.errorString()));
    }

    if (key.empty())
    {
        PRINT_ERROR("Invalid key in file '%s'.", STR(keyFileName));
        return false;
    }
    
    keyMap["key"]          = key;
    keyMap["name"]         = options->publicKeyName();

    request["operation"]   = "addKey";
    request["user"]        = userMap;
    request["public_key"]  = keyMap;

    return executeRequest(uri, request);
}

bool
S9sRpcClient::setPassword()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    S9sVariantMap  properties;
    bool           changingSelf = false;

    if (options->nExtraArguments() > 1)
    {
        PRINT_ERROR("Only one user can be modified at once.");
        return false;
    }

    //
    //
    //
    properties["class_name"] = "CmonUser";
    if (options->nExtraArguments() > 0)
    {
        properties["user_name"]  = options->extraArgument(0);
    } else {
        properties["user_name"] = options->userName();
        changingSelf = true;
    }

    //
    // 
    //
    request["operation"]  = "changePassword";
    request["user"]       = properties;

    if (options->hasOldPassword())
    {
        request["old_password"] = options->oldPassword();
    } else if (changingSelf && options->hasPassword())
    {
        request["old_password"] = options->password();
    }

    if (options->hasNewPassword())
        request["new_password"] = options->newPassword();
    
    return executeRequest(uri, request);
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * The function to create maintenance as it is defined in the S9sOptions
 * singleton.
 */
bool 
S9sRpcClient::createMaintenance()
{
    S9sOptions    *options = S9sOptions::instance();
    bool           success;
    S9sString      begin, end;

    if (options->hasBegin())
        begin = options->begin();
    else if (options->hasStart())
        // Only for backward compatibility.
        begin = options->start();

    end = options->end();
    /*
     * Two ways: host maintenance or cluster maintenance.
     */
    if (options->hasClusterIdOption())
    {
        success = createMaintenance(
                options->clusterId(), begin, end, options->reason());
    } else {
        success = createMaintenance(
                options->nodes(), begin, end, options->reason());
    }

    return success;
}

/**
 * This is the function that creates a job that is made for registering
 * maintenance periods.
 */
bool
S9sRpcClient::createMaintenanceWithJob()
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sVariantMap  request     = composeRequest();
    S9sVariantMap  job         = composeJob();
    S9sVariantMap  jobData     = composeJobData();
    S9sVariantList hosts       = options->nodes();
    S9sVariantMap  jobSpec;
    S9sString      title;
    S9sString      uri         = "/v2/jobs/";
    bool           retval;
    
    title = "Registering Maintenance";
  
    if (options->hasBegin())
        jobData["begin"] = options->begin();
    else if (options->hasBeginRelative())
        jobData["begin_relative"] = options->beginRelative();
    else if (options->hasStart())
        jobData["begin"] = options->start();
    
    if (options->hasEnd())
        jobData["end"] = options->end();

    jobData["reason"]              = options->reason();
    
    if (options->hasMinutes())
        jobData["maintenance_minutes"] = options->minutes();
        
    if (!hosts.empty())
        jobData["nodes"] = nodesField(hosts);

    // The jobspec describing the command.
    jobSpec["command"]    = "create_maintenance";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = title;
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::disableRecoveryWithJob()
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sVariantMap  request     = composeRequest();
    S9sVariantMap  job         = composeJob();
    S9sVariantMap  jobData     = composeJobData();
    S9sVariantList hosts       = options->nodes();
    S9sVariantMap  jobSpec;
    S9sString      title;
    S9sString      uri         = "/v2/jobs/";
    bool           retval;
    
    title = "Disable Recovery";
  
    if (!options->reason().empty())
        jobData["reason"] = options->reason();
    
    if (options->hasMinutes())
        jobData["maintenance_minutes"] = options->minutes();
        
    if (!hosts.empty())
        jobData["nodes"] = nodesField(hosts);

    // The jobspec describing the command.
    jobSpec["command"]    = "disable_recovery";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = title;
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::enableRecoveryWithJob()
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sVariantMap  request     = composeRequest();
    S9sVariantMap  job         = composeJob();
    S9sVariantMap  jobData     = composeJobData();
    S9sVariantList hosts       = options->nodes();
    S9sVariantMap  jobSpec;
    S9sString      title;
    S9sString      uri         = "/v2/jobs/";
    bool           retval;
    
    title = "Enable Recovery";
   
    //jobData["reason"]              = options->reason();
    
    if (options->hasMinutes())
        jobData["maintenance_minutes"] = options->minutes();
        
    if (!hosts.empty())
        jobData["nodes"] = nodesField(hosts);

    // The jobspec describing the command.
    jobSpec["command"]    = "enable_recovery";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["title"]          = title;
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Overloaded version of the function that create a maintence period as defined
 * in the function arguments.
 */
bool 
S9sRpcClient::createMaintenance(
        const S9sVariantList &hosts,
        const S9sString      &start,
        const S9sString      &end,
        const S9sString      &reason)
{
    S9sString      uri = "/v2/maintenance/";
    S9sVariantMap  request;
    bool           retval;

    if (hosts.size() != 1)
    {
        PRINT_ERROR(
                "To create a maintenance period one"
                " hostname has to be provided.");
        return false;
    }
    
    request["operation"] = "addMaintenance";
    request["hostname"]  = hosts[0].toNode().hostName();
    request["initiate"]  = start;
    request["deadline"]  = end;
    request["reason"]    = reason;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Overloaded version of the function that create a maintence period as defined
 * in the function arguments.
 */
bool 
S9sRpcClient::createMaintenance(
        const int            &clusterId,
        const S9sString      &start,
        const S9sString      &end,
        const S9sString      &reason)
{
    S9sString      uri = "/v2/maintenance/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]  = "addMaintenance";
    request["cluster_id"] = clusterId;
    request["initiate"]   = start;
    request["deadline"]   = end;
    request["reason"]     = reason;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::deleteMaintenance()
{
    S9sOptions    *options = S9sOptions::instance();

    return deleteMaintenance(options->uuid());
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Overloaded function that will delete a maintence as it is defined in the
 * command line argument.
 */
bool
S9sRpcClient::deleteMaintenance(
        const S9sString &uuid)
{
    S9sString      uri = "/v2/maintenance/";
    S9sVariantMap  request;
    bool           retval;

    if (uuid.empty())
    {
        PRINT_ERROR("Missing UUID.");
        PRINT_ERROR("Use the --uuid command line option to provide the UUID.");
        return false;
    }
    
    request["operation"] = "removeMaintenance";
    request["UUID"]      = uuid;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Get the maintenance periods from the server.
 */
bool
S9sRpcClient::getMaintenance()
{
    S9sString      uri = "/v2/maintenance/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getMaintenance";
    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::getCurrentMaintenance()
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sVariantList hosts       = options->nodes();
    S9sString      uri = "/v2/maintenance/";
    S9sVariantMap  request;
    bool           retval;

    S9S_DEBUG("Number of hosts: %zu", hosts.size());
    if (hosts.size() > 1)
    {
        PRINT_ERROR("Only one no name is supported for this request.");
        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    request["operation"]   = "getCurrentMaintenance";
    request["cluster_id"]  = options->clusterId();
    
    if (hosts.size() > 0)
        request["hostname"] = hosts[0].toNode().hostName();

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::getNextMaintenance()
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sVariantList hosts       = options->nodes();
    S9sString      uri = "/v2/maintenance/";
    S9sVariantMap  request;
    bool           retval;
    
    if (hosts.size() > 1)
    {
        PRINT_ERROR("Only one no name is supported for this request.");
        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    request["operation"] = "getNextMaintenance";
    request["cluster_id"]  = options->clusterId();
    
    if (hosts.size() > 0)
        request["hostname"] = hosts[0].toNode().hostName();

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns A prepared request that after further settings added can be sent to
 *   the controller.
 */
S9sVariantMap 
S9sRpcClient::composeRequest() 
{
    S9sOptions    *options     = S9sOptions::instance();
    int            clusterId   = options->clusterId();
    S9sString      clusterName = options->clusterName();
    S9sVariantMap  request;

    request["class_name"] = "CmonRpcRequest";

    if (S9S_CLUSTER_ID_IS_VALID(clusterId) || options->hasClusterIdOption())
        request["cluster_id"] = clusterId;

    if (!clusterName.empty())
        request["cluster_name"] = clusterName;

    if (!options->withTags().empty())
        request["with_tags"] = options->withTags();
    
    if (!options->withoutTags().empty())
        request["without_tags"] = options->withoutTags();

    return request;
}

S9sVariantMap
S9sRpcClient::composeBackupJob()
{
    S9sOptions     *options      = S9sOptions::instance();
    S9sVariantList  hosts        = options->nodes();
    S9sString       backupMethod = options->backupMethod();
    S9sString       backupDir    = options->backupDir();
    S9sString       databases    = options->databases();
    S9sNode         backupHost;
    S9sString       title;

    S9sVariantMap   job          = composeJob();
    S9sVariantMap   jobData      = composeJobData();
    S9sVariantMap   jobSpec;
   
    if (!backupMethod.empty())
    {
        title.sprintf("Create %s Backup", STR(backupMethod));
    } else {
        title = "Create Backup";
    }

    // Composing jobData
    // There can be 1 or 0 nodes specified.
    if (hosts.size() > 0u)
    {
        backupHost = hosts[0].toNode();
        jobData["hostname"]          = backupHost.hostName();
    }

    if (!backupMethod.empty())
        jobData["backup_method"] = backupMethod;
    
    // The job_data describing how the backup will be created.
    jobData["description"]       = "Backup created by s9s-tools.";

    if (backupHost.hasPort())
        jobData["port"]          = backupHost.port();

    if (!backupDir.empty())
        jobData["backupdir"]     = backupDir;

    if (!options->subDirectory().empty())
        jobData["backupsubdir"]  = options->subDirectory();

    if (!options->backupUser().empty())
        jobData["backup_user"] = options->backupUser();

    if (!options->backupPassword().empty())
        jobData["backup_user_password"] = options->backupPassword();

    if (!databases.empty())
        jobData["include_databases"] = databases;

    if (options->pitrCompatible())
        jobData["pitr_compatible"] = true;

    if (options->noCompression())
        jobData["compression"]   = false;

    if (options->usePigz())
        jobData["use_pigz"]      = true;

    if (options->onNode())
        jobData["cc_storage"]    = false;
    
    // or just else branch of onNode ?
    if (options->onController())
        jobData["cc_storage"]    = true;

    if (options->hasParallellism())
        jobData["xtrabackup_parallellism"] = options->parallellism();

    if (options->encryptBackup())
        jobData["encrypt_backup"] = true;
    
    // 0: prefer global setting, -1: never delete, >0 delete after N days
    if (options->hasBackupRetention())
        jobData["backup_retention"] = options->backupRetention();

    if (!options->title().empty())
        jobData["backup_title"] = options->title();

    if (options->toIndividualFiles())
        jobData["backup_individual_schemas"] = true;
    
    if (!options->testServer().empty())
    {
        S9sVariantMap tmpMap;

        tmpMap["disable_firewall"] = true;
        tmpMap["disable_selinux"]  = true;
        tmpMap["install_software"] = !options->noInstall();
        tmpMap["server_address"]   = options->testServer();
        
        jobData["verify_backup"]   = tmpMap;
        jobData["terminate_db_server"] = true;
    }
    
    // The jobspec describing the command.
    jobSpec["command"]    = "backup";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["job_spec"]       = jobSpec;
    job["title"]          = title;
    
    return job;
}

/**
 * This will compose the skeleton job specification map that holds generic
 * information every job specification can hold.
 */
S9sVariantMap 
S9sRpcClient::composeJob() const
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantMap  job;
    
    job["class_name"]     = "CmonJobInstance";
    
    if (!options->schedule().empty())
        job["scheduled"]  = options->schedule(); 
    
    if (!options->recurrence().empty())
        job["recurrence"]  = options->recurrence(); 

    // The tags of the job.
    if (options->hasJobTags())
        job["tags"] = options->jobTags();

    if (!options->jobTitle().empty())
        job["title"] = options->jobTitle();

    return job;
}

S9sVariantMap 
S9sRpcClient::composeJobData(
        bool argumentsAreContainers) const
{
    S9sOptions    *options      = S9sOptions::instance();
    S9sString      templateName = options->templateName();
    S9sString      cloudName    = options->cloudName();
    S9sString      imageName    = options->imageName();
    S9sString      imageOsUser  = options->imageOsUser();
    S9sString      subnetId     = options->subnetId();
    S9sString      vpcId        = options->vpcId();
    S9sVariantList volumes      = options->volumes();
    S9sVariantList servers      = options->servers();

    S9sVariantMap  jobData;
    S9sVariantList containers;

    // The --containers option.
    if (options->hasContainers())
    {
        S9sVariantList   theList = options->containers();

        for (uint idx = 0u; idx < theList.size(); ++idx)
        {
            S9sVariantMap containerMap = theList[idx].toVariantMap();
            S9sContainer  container(containerMap);

            if (!templateName.empty())
                container.setTemplate(templateName);

            if (!imageName.empty())
                container.setImage(imageName);
    
            if (!imageOsUser.empty())
                container.setImageOsUser(imageOsUser);

            if (!cloudName.empty())
                container.setProvider(cloudName);

            if (!subnetId.empty())
                container.setSubnetId(subnetId);
    
            if (options->hasFirewalls())
                container.setFirewalls(options->firewalls());
    
            if (!options->region().empty())
                container.setRegion(options->region());

            if (!vpcId.empty())
                container.setSubnetVpcId(vpcId);

            if (!volumes.empty())
                container.setVolumes(volumes);
    
            if (servers.size() > 1)
            {
                PRINT_ERROR("Currently only one server can be"
                        " defined for containers.");
            } else if (servers.size() == 1) 
            {
                container.setParentServerName(servers[0u].toNode().hostName());
            }

            containers << container.toVariantMap();
        }
    }

    // Command line arguments interpreted as containers.
    if (argumentsAreContainers)
    {
        for (uint idx = 0; idx < options->nExtraArguments(); ++idx)
        {
            S9sContainer container;
    
            container.setAlias(options->extraArgument(idx));
            
            if (!templateName.empty())
                container.setTemplate(templateName);
            
            if (!imageName.empty())
                container.setImage(imageName);
    
            if (!imageOsUser.empty())
                container.setImageOsUser(imageOsUser);
            
            if (!cloudName.empty())
                container.setProvider(cloudName);
            
            if (!subnetId.empty())
                container.setSubnetId(subnetId);
    
            if (options->hasFirewalls())
                container.setFirewalls(options->firewalls());
    
            if (!options->region().empty())
                container.setRegion(options->region());

            if (!vpcId.empty())
                container.setSubnetVpcId(vpcId);
            
            if (!volumes.empty())
                container.setVolumes(volumes);

            containers << container.toVariantMap();
        }
    }

    if (!containers.empty())
        jobData["containers"]   = containers;

    addCredentialsToJobData(jobData);

    /*
     * --with-ssl  --> /job_data/enable_ssl = true
     * --without-ssl  --> /job_data/enable_ssl = false
     */
    if (options->getBool("with_ssl"))
        jobData["enable_ssl"] = true;
    else if (options->getBool("without_ssl"))
        jobData["enable_ssl"] = false;

    if (options->dry())
        jobData["dry_run"] = true;

    if (options->useInternalRepos())
        jobData["use_internal_repos"] = true;

    if (options->isGenerateKeyRequested())
        jobData["ssh_generate_key"] = true;
    
    if (options->hasTimeout())
        jobData["timeout"] = options->timeout();

    if (options->hasMemory())
        jobData["xtrabackup_use_memory"] = options->memory().toInt();

    if (options->hasCredentialIdOption())
        jobData["cloud_credentials_id"] = options->credentialId();

    if (options->noInstall())
    {
        jobData["install_software"] = false;
        jobData["enable_uninstall"] = false;
    }

    if (!options->configTemplate().empty())
        jobData["config_template"] = options->configTemplate();
    
    if (!options->configTemplate("haproxy").empty())
        jobData["haproxy_config_template"] = options->configTemplate("haproxy");
 
    if (options->hasForceOption())
        jobData["force"]  = options->force();
    
    if (!options->dataDir().empty())
        jobData["datadir"] = options->dataDir();

    /*
     * If the command line options has a proxysql node we add a number of
     * options the proxysql jobs use. I hope this logic is airtight.
     */
    if (options->hasProxySql())
    {
        S9sString adminUser = options->getString("admin_user", "");
        S9sString adminPassword = options->getString("admin_password", "");
        S9sString monitorUser = options->getString("monitor_user", "proxysql-monitor");
        S9sString monitorPassword = options->getString("monitor_password", "proxysql-monitor");

	if (! adminUser.empty())
            jobData["admin_user"] = adminUser;

	if (! adminPassword.empty())
            jobData["admin_password"] = adminPassword;
    
	if (! monitorUser.empty())
	    jobData["monitor_user"] = monitorUser;

	if (! monitorPassword.empty())
            jobData["monitor_password"] = monitorPassword;

        jobData["import_accounts"]  = 
            !options->getBool("dont_import_accounts");
    }
    
    // The tags of the thing we do, not the tags of the job.
    if (!options->withTags().empty())
        jobData["with_tags"] = options->withTags();

    return jobData;
}

void
S9sRpcClient::addCredentialsToJobData(
		S9sVariantMap & jobData) const
{
    S9sOptions    *options      = S9sOptions::instance();

    S9sString      osUserName     = options->osUser(false);
    S9sString      osKeyFile      = options->osKeyFile();
    S9sString      osPassword     = options->osPassword();
    S9sString      osSudoPassword = options->osSudoPassword();

    if (!osUserName.empty())
        jobData["ssh_user"]     = osUserName;

    if (!osKeyFile.empty())
        jobData["ssh_keyfile"]  = osKeyFile;

    if (!osPassword.empty())
        jobData["ssh_password"] = osPassword;

    if (!osSudoPassword.empty())
        jobData["sudo_password"] = osSudoPassword;

    // The SSL options for create cluster jobs
    if (!options->sslCaFile().empty())
        jobData["ca_file"]     = options->sslCaFile();

    if (!options->sslCertFile().empty())
        jobData["cert_file"]   = options->sslCertFile();

    if (!options->sslKeyFile().empty())
        jobData["key_file"]    = options->sslKeyFile();
}

/**
 * \returns The "job_data" structure or an empty map if an error happened.
 *
 * This method is used to create the "job_data" part of the request for requests
 * that needs exactly one container to be passed.
 */
S9sVariantMap 
S9sRpcClient::composeJobDataOneContainer() const
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantMap  jobData;
    S9sVariantList containerList;
    S9sString      templateName = options->templateName();
    S9sVariantMap  containerMap;
    S9sVariantList servers  = options->servers();
    
    if (options->hasContainers())
        containerList = options->containers();

    if (options->nExtraArguments() + containerList.size() > 1)
    {
        PRINT_ERROR("Multiple container names in the command line.");
        return jobData;
    } if (options->nExtraArguments() + containerList.size() == 0)
    {
        PRINT_ERROR("No container is specified in the command line.");
        return jobData;
    }

    // Finding the container.
    if (options->hasContainers() && !containerList.empty())
    {
        containerMap = containerList[0u].toVariantMap();
    } else if (options->nExtraArguments() == 1)
    {
        containerMap["alias"]      = options->extraArgument(0);
        containerMap["class_name"] = "CmonContainer";
    }
    
    // Adding extra properties to the container.
    if (!templateName.empty())
        containerMap["template"] = templateName;

    if (servers.size() == 1)
        containerMap["parent_server"] = servers[0u].toNode().hostName();

    //
    jobData["container"] = containerMap;
    
    if (!servers.empty())
        jobData["servers"] = serversField(servers);
    
    if (options->isGenerateKeyRequested())
        jobData["ssh_generate_key"] = true;

    return jobData;
}

/**
 * \param uri The URI where we send the request.
 * \param request The request we send.
 * \param important If it is true and the requests are printed in debug messages
 *   this request is going to be included in the debug output.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * \code{.js}
 * {
 *     "controllers": 
 *     {
 *         "192.168.0.100:9501": 
 *         {
 *             "class_name": "CmonController",
 *             "hostname": "192.168.0.100",
 *             "ip": "192.168.0.100",
 *             "port": 9501
 *         },
 *         "192.168.0.223:9501": 
 *         {
 *             "class_name": "CmonController",
 *             "hostname": "192.168.0.223",
 *             "ip": "192.168.0.223",
 *             "port": 9501
 *         },
 *         "192.168.0.235:9501": 
 *         {
 *             "class_name": "CmonController",
 *             "hostname": "192.168.0.235",
 *             "ip": "192.168.0.235",
 *             "port": 9501
 *         }
 *     },
 *     "error_string": "Redirect notification.",
 *     "reply_received": "2019-03-20T09:44:17.987Z",
 *     "request_created": "2019-03-20T09:44:17.979Z",
 *     "request_id": 1,
 *     "request_processed": "2019-03-20T09:44:17.988Z",
 *     "request_status": "Redirect"
 * }
 * \endcode
 */
bool
S9sRpcClient::executeRequest(
        const S9sString &uri,
        S9sVariantMap   &request,
        bool             important)
{
    S9sDateTime    now = S9sDateTime::currentDateTime();
    S9sString      timeString = now.toString(S9sDateTime::TzDateTimeFormat);
    bool           retval;
    int            nTry = 0;
    S9sVariantMap  triedKeys;
   
    request["request_created"] = timeString;
    request["request_id"]      = ++m_priv->m_requestId;
    
    if (important)
        printRequestForDebug(request);

    while (true)
    {
        S9sString      hostName;
        int            port = 0;

        retval = doExecuteRequest(uri, request);
            
        if (retval && m_priv->m_reply.isRedirect())
            m_priv->rememberRedirect();

        if (retval && m_priv->m_reply.isRedirect())
        {
            S9sVariantMap        controllers;
            S9sVariantMap        controller;
            S9sVector<S9sString> keys;

            PRINT_LOG("Redirect notification received.");
            PRINT_VERBOSE("Redirect notification received.");

            controllers = m_priv->m_reply["controllers"].toVariantMap();
            PRINT_VERBOSE(
                    "Has %u controller(s) in redirect.", 
                    controllers.size());

            keys = controllers.keys();
            for (uint idx = 0u; idx < keys.size(); ++idx)
            {
                S9sString key = keys[idx];

                PRINT_VERBOSE("Investigating %s...", STR(key));
                if (triedKeys.contains(key))
                {
                    PRINT_VERBOSE("Already tried %s.", STR(key));
                    continue;
                }

                triedKeys[key] = true;
                controller = controllers[key].toVariantMap();
                hostName   = controller["hostname"].toString();
                port       = controller["port"].toInt();

                if (m_priv->m_hostName == hostName &&
                        m_priv->m_port == port)
                {
                    PRINT_VERBOSE("We just tried this %s.", STR(key));
                    continue;
                }

                break;
            }

            if (hostName.empty())
            {
                PRINT_VERBOSE("Could not find controller to try.");
                return retval;
            }
    
            //S9S_WARNING("Trying %s:%d", STR(hostName), port);
            PRINT_VERBOSE("Redirected to %s:%d.", STR(hostName), port);
            PRINT_LOG("Redirected to %s:%d.", STR(hostName), port);

            m_priv->close();
            m_priv->m_hostName = hostName;
            m_priv->m_port     = port;

            // This is just an unnecessary protection: if the code is ok this
            // will never happen.
            ++nTry;
            if (nTry > 15) 
            {
                PRINT_LOG("Too many redirects (%d), aborting.", nTry);
                PRINT_VERBOSE("Too many redirects (%d), aborting.", nTry);
                break;
            }
        } else {
            break;
        }
    }

    return retval;
}

/**
 * \param uri the file path part of the URL where we send the request
 * \param payload the JSON request string
 * \returns true if the request sent and the reply received (even if the reply
 *   suggests an error happened in the controller).
 *
 * This function will set the exit status if the communication with the server
 * fails for some reason.
 */
bool
S9sRpcClient::doExecuteRequest(
        const S9sString     &uri,
        S9sVariantMap       &request)
{
    S9sString    payload = request.toString();
    S9sOptions  *options = S9sOptions::instance();    
    S9sDateTime  replyReceived;
    S9sString    header;
    S9sString    myUri = uri;
    ssize_t      readLength;
    ssize_t      writtenLength;
    S9sString    dataToSend; 
    size_t       dataSize;
    size_t       payloadSize = 0;
    bool         isJSonStream = false;

    PRINT_LOG("Sending request to '%s'.", STR(uri));
    PRINT_VERBOSE("Preparing to send request.");

    if (!m_priv->m_path.empty())
        myUri = m_priv->m_path + uri;

    PRINT_VERBOSE("URI is '%s'", STR(myUri));
    //PRINT_LOG("     uri: %s\n", STR(myUri));
    //PRINT_LOG(" request: \n%s\n", STR(payload));

    m_priv->m_jsonReply.clear();
    m_priv->m_reply.clear();

    if (!m_priv->connect())
    {
        PRINT_LOG("%s", STR(m_priv->m_errorString));
        PRINT_VERBOSE("Connection failed: %s", STR(m_priv->m_errorString));
        options->setExitStatus(S9sOptions::ConnectionError);

        setError(m_priv->m_errorString);
        return false;
    }
        
    /*
     * Printing the request we are sending.
     */
    if (options->isJsonRequested() && options->isVerbose())
    {
        printf("Preparing to send request on %s: \n%s\n", 
                STR(myUri), STR(payload));
    }

    if (!payload.empty())
        payloadSize = strlen(STR(payload));

    header.sprintf(
        "POST %s HTTP/1.0\r\n"
        "Host: %s:%d\r\n"
        "User-Agent: s9s-tools/1.0\r\n"
        "Connection: close\r\n"
        "Accept: application/json\r\n"
        "Transfer-Encoding: identity\r\n"
        "%s"
        "Content-Type: application/json\r\n"
        "Content-Length: %zd\r\n"
        "\r\n",
        STR(myUri),
        STR(m_priv->m_hostName),
        m_priv->m_port,
        STR(m_priv->cookieHeaders()),
        payloadSize);

    dataToSend = header + payload;
    dataSize   = strlen(STR(dataToSend));
    
    PRINT_VERBOSE("Sending: \n%s\n", STR(dataToSend));
    writtenLength = m_priv->write(STR(dataToSend), dataSize);


    S9S_DEBUG("%s: Size: %zd, written: %zd", 
            STR(timeStampString()), dataSize, writtenLength);

    //S9S_WARNING("dataToSend: \n%s\n", STR(dataToSend));

    if (writtenLength < 0)
    {
        // we shall use m_priv->m_errorString TODO
        S9S_WARNING("Error writing socket: %m");

        // priv shall do this:
        m_priv->m_errorString.sprintf("Error writing socket: %m");
        m_priv->close();

        options->setExitStatus(S9sOptions::ConnectionError);
        setError(m_priv->m_errorString);
        return false;
    }
            
    if (options->isJsonRequested() && options->isVerbose())
    {
        printf("Sent request.\n");
    }

    /*
     * Reading the reply from the server.
     */
    m_priv->clearBuffer();
    
    for (;;)
    {
        m_priv->ensureHasBuffer(m_priv->m_dataSize + READ_SIZE);

        readLength = m_priv->read(
                m_priv->m_buffer + m_priv->m_dataSize, READ_SIZE - 1);

        if (readLength > 0)
        {
            m_priv->m_dataSize += readLength;

            // read may got interrupted due to too small buffer
            if (readLength >= READ_SIZE - 1)
            {
                continue;
            }
        } else if (readLength < 0)
        {
            m_priv->m_errorString.sprintf(
                    "Error while reading from controller (%s:%d TLS: %s): %m",
                    STR(m_priv->m_hostName), m_priv->m_port,
                    m_priv->m_useTls ? "yes" : "no");

            options->setExitStatus(S9sOptions::ConnectionError);
            setError(m_priv->m_errorString);
            return false;
        }

        /*
         * JSon stream records always starts by <RS> (\036) and ending by \n
         */
        if (m_priv->m_buffer[0] == '\036')
            isJSonStream = true;

        m_priv->m_jsonReply.clear();

        /*
         * If this is a JSon stream we process the JSon messages until we done
         * with all of them in the buffer. The buffer might have zero or more 
         * complete JSon messages.
         */
        while (isJSonStream && m_priv->hasCompleteJSon())
        {
            S9sVariantMap jsonRecord;

            m_priv->m_jsonReply = m_priv->getCompleteJSon();
            //S9S_WARNING("json: %s", STR(m_priv->m_jsonReply));
            //S9S_WARNING("1 Parsing json");

            m_priv->skipRecord();
            if (!jsonRecord.parse(STR(m_priv->m_jsonReply)))
            {
                PRINT_ERROR("Failed to parse JSon string.");
                return false;
            } else if (m_priv->m_callbackFunction == 0)
            {
                m_priv->m_errorString.sprintf(
                        "Got JSon stream when expecting JSon object:\n%s.",
                        STR(m_priv->m_jsonReply));
                PRINT_ERROR("%s", STR(m_priv->m_errorString));

                options->setExitStatus(S9sOptions::ConnectionError);
                setError(m_priv->m_errorString);

                return false;
            }

            (*m_priv->m_callbackFunction)(
                        jsonRecord, m_priv->m_callbackUserData);

            m_priv->m_jsonReply = "";
        }

        if (isJSonStream)
        {
            // If we read no data in streaming mode that simply means the
            // connection ended by the server.
            if (readLength == 0)
                return true;

            // We continue reading the connection.
            continue;
        }
            
        // If this is not a JSon stream and we could not read data we break the
        // read-loop, the next lines will handle this.
        if (readLength == 0)
            break;
    } // for(;;)

    // Closing the buffer with a null terminating byte.
    m_priv->ensureHasBuffer(m_priv->m_dataSize + 1);
    m_priv->m_buffer[m_priv->m_dataSize] = '\0';
    m_priv->m_dataSize += 1;

    // This is producing a lot of lines.
    //S9S_DEBUG("reply: '%s'", m_priv->m_buffer); 

    // Closing the socket.
    m_priv->close();
   
    S9S_DEBUG("%s: total received: %zd bytes", 
            STR(timeStampString()), m_priv->m_dataSize);
    if (m_priv->m_dataSize > 1)
    {
        // Lets parse the cookie/HTTP session info from server reply
        m_priv->parseHeaders();

        char *tmp = strstr(m_priv->m_buffer, "\r\n\r\n");
        if (tmp)
        {
            m_priv->m_jsonReply = (tmp + 4);

            if (options->isJsonRequested() && options->isVerbose())
            {
                printf("Reply: \n%s\n", STR(m_priv->m_jsonReply));
            }

            //PRINT_LOG("    reply: \n%s\n", STR(m_priv->m_jsonReply));
        }
    } else {
        m_priv->m_errorString.sprintf(
                "No data received from controller (%d, %s:%d TLS: %s): %m",
                m_priv->m_dataSize,
                STR(m_priv->m_hostName), m_priv->m_port,
                m_priv->m_useTls ? "yes" : "no");

        options->setExitStatus(S9sOptions::ConnectionError);
        setError(m_priv->m_errorString);
        return false;
    }

    replyReceived = S9sDateTime::currentDateTime();
    if (!m_priv->m_reply.parse(STR(m_priv->m_jsonReply)))
    {
        PRINT_VERBOSE("Error in reply: \n%s\n", STR(m_priv->m_jsonReply));

        m_priv->m_errorString.sprintf("Error parsing JSON reply.");
        options->setExitStatus(S9sOptions::ConnectionError);
        setError(m_priv->m_errorString);

        return false;
    } else {
        m_priv->m_reply["reply_received"] = 
            replyReceived.toString(S9sDateTime::TzDateTimeFormat);

        if (m_priv->m_reply.requestStatus() == S9sRpcReply::AuthRequired)
            m_priv->m_authenticated = false;
    }

    saveRequestAndReply(request, m_priv->m_reply);
    //printf("-> \n%s\n", STR(m_priv->m_reply.toString()));
    return true;
}

/**
 * \param request The request to print out.
 *
 * This method is made for printing out the RPC requests for debugging.
 */
void
S9sRpcClient::printRequestForDebug(
        S9sVariantMap &request)
{
    S9sOptions     *options = S9sOptions::instance();
    bool            isBatch = options->isBatchRequested();
    bool     isPrintRequest = options->isJsonRequestRequested();

    // We print out the requests only if this environment variable is set.
    if (getenv("S9S_DEBUG_PRINT_REQUEST") != NULL)
	    isPrintRequest = true;

    if (isPrintRequest && !isBatch)
    {
        bool            syntaxHighlight = options->useSyntaxHighlight();
        S9sFormatFlags  format  = S9sFormatIndent;

        if (syntaxHighlight)
            format = format | S9sFormatColor;

        ::fprintf(stderr, "%s\n", STR(request.toJsonString(format)));
    }
}

void 
S9sRpcClient::saveRequestAndReply(
        const S9sVariantMap &request,
        const S9sVariantMap &reply) const
{
    S9sString directory = getenv("S9S_DEBUG_SAVE_REQUEST_EXAMPLES");

    if (!directory.empty())
    {
        S9sString operation, status, content;
        S9sString fileName;
        S9sFile   file;

        if (request.contains("operation"))
            operation = request.at("operation").toString();
        
        if (reply.contains("request_status"))
            status = reply.at("request_status").toString();

        if (operation.empty())
            operation = "noOperation";

        if (status.empty())
            status = "NoStatus";

        fileName.sprintf("%s/%s-%s-req.json", 
                STR(directory), STR(status), STR(operation));
        file = S9sFile(fileName);
        content = request.toJsonString(S9sFormatIndent);
        if (!file.writeTxtFile(content))
        {
            S9S_WARNING("ERROR: %s", STR(file.errorString()));
        }

        fileName.sprintf("%s/%s-%s-rep.json", 
                STR(directory), STR(status), STR(operation));
        file = S9sFile(fileName);
        content = reply.toJsonString(S9sFormatIndent);
        file.writeTxtFile(content);
    }
}

/**
 * \code{.js}
 * {
 *     "error_string": "User 'pipas' is not found.",
 *     "reply_received": "2018-05-07T12:32:23.400Z",
 *     "request_created": "2018-05-07T12:32:23.395Z",
 *     "request_id": 2,
 *     "request_processed": "2018-05-07T12:32:23.405Z",
 *     "request_status": "AccessDenied"
 * }
 * \endcode
 */
void
S9sRpcClient::setError(
        const S9sString &errorString,
        const S9sString &errorCode)
{
    m_priv->m_reply.clear();

    m_priv->m_reply["error_string"]   = errorString;
    m_priv->m_reply["request_status"] = errorCode;
}

S9sVariant
S9sRpcClient::topologyField(
        const S9sVariantList &nodes)
{
    S9sVariantList masterSlaveLinks;
    S9sVariantMap  topology;
    S9sVariant     retval;
    S9sString      masterHostName;

    for (uint idx = 0u; idx < nodes.size(); ++idx)
    {
        const S9sNode &node       = nodes[idx].toNode();
        const S9sString className = node.className();
        bool  isMaster            = node.property("master").toBoolean();
        bool  isSlave             = node.property("slave").toBoolean();

        // Only data nodes supposed to have replication links.
        if (node.isContainerServer() || node.isLoadBalaner())
            continue;

        // If the user did not provide information about the master/slave status
        // we consider the first node a master and the others slave.
        if (!isMaster && !isSlave)
        {
            isMaster = idx == 0u;
            isSlave  = !isMaster;
        }

        if (isMaster)
        {
            if (!masterHostName.empty())
            {
                S9sVariantMap map1, map2;

                map1[masterHostName]  = node.hostName();
                map2[node.hostName()] = masterHostName;

                masterSlaveLinks << map1;
                masterSlaveLinks << map2;
            }

            masterHostName = node.hostName();
        } else if (isSlave)
        {
            if (!masterHostName.empty())
            {
                S9sVariantMap map1;

                map1[masterHostName] = node.hostName();
                masterSlaveLinks << map1;
            }
        }
    }

    topology["master_slave_links"] = masterSlaveLinks;
    retval = topology;
    return retval;
}

S9sVariant
S9sRpcClient::nodesField(
        const S9sVariantList &nodes) 
{
    S9sVariant     retval;

    retval = nodes;
    return retval;
}

S9sVariant
S9sRpcClient::nodesField() 
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sVariantList nodes       = options->nodes();
    S9sVariant     retval;

    retval = nodes;
    return retval;
}

/**
 * \param servers The list of objects that will be returned in proper format.
 *
 * Certain requests hold the property "servers", a list of objects representing
 * servers. Currently we only support CmonContainerServer objects there. We
 * compose this list here from the variant list passed as argument.
 *
 * \code{.js}
 * {
 *     "operation": "registerServers",
 *     "request_created": "2017-10-12T12:39:01.252Z",
 *     "request_id": 3,
 *     "servers": [ 
 *     {
 *         "class_name": "CmonContainerServer",
 *         "hostname": "host04",
 *         "protocol": "lxc"
 *     } ]
 * }
 * \endcode
 */
S9sVariant
S9sRpcClient::serversField(
        const S9sVariantList &servers)
{
    S9sVariantList  retval;
    S9sOptions     *options = S9sOptions::instance();

    for (uint idx = 0u; idx < servers.size(); ++idx)
    {
        S9sVariantMap thisMap = servers[idx].toVariantMap();

        if (thisMap["class_name"].toString().empty())
            thisMap["class_name"] = "CmonContainerServer";

        if (options->hasSshCredentials())
        {
            thisMap["ssh_credentials"] = 
                options->sshCredentials(
                        "", thisMap["hostname"].toString()).toVariantMap();
        }

        retval << thisMap;
    }

    return S9sVariant(retval);
}

/**
 * Returns the current controller's version the client communicates with, please
 * note this method will give valid value after any RPC request has been made
 * (eg.: authenticate()).
 */
S9sString
S9sRpcClient::serverVersion() const
{
    S9sString versionString;
    S9sVariantList parts;

    // cmon/1.4.3
    if (m_priv)
        versionString = m_priv->serverVersionString();

    if (!versionString.contains('/'))
        return "";

    parts = versionString.split("/");
    if (parts.size() == 2u)
        return parts.at(1).toString();

    return "";
}

bool
S9sRpcClient::detectVersion()
{
    S9S_DEBUG(" ");
    S9sVariantMap  request;
    S9sString      uri = "/v2/auth";

    /*
     * Firing an intentionally empty request, so we can parse the controllers
     * version out from the returned Server: HTTP header
     */
    executeRequest(uri, request);

    return !serverVersion().empty();
}

S9sString 
S9sRpcClient::timeStampString()
{
    S9sDateTime dt = S9sDateTime::currentDateTime();

    return dt.toString();
}

bool
S9sRpcClient::syncClusters()
{
    S9sString      title;
    S9sVariantMap  request;
    S9sVariantMap  job       = composeJob();
    S9sVariantMap  jobSpec;
    S9sString      uri = "/v2/jobs/";

    title = "Synchronize clusters";

    // The jobspec describing the command.
    jobSpec["command"]    = "sync_clusters";

    // The job instance describing how the job will be executed.
    job["title"]          = title;
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    return executeRequest(uri, request);
}

