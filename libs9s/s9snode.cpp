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
#include "s9snode.h"

#include "s9srpcreply.h"
#include <s9ssshcredentials.h>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sNode::S9sNode() :
    S9sObject()
{
    m_properties["class_name"] = "CmonHost";
}
 
S9sNode::S9sNode(
        const S9sVariantMap &properties) :
    S9sObject(properties)
{
    if (!m_properties.contains("class_name"))
        m_properties["class_name"] = "CmonHost";
}

/**
 * \param stringRep The string representation of the host, either a JSon string
 *   or an url (e.g. "192.168.1.100:3306".
 */
S9sNode::S9sNode(
        const S9sString &stringRep)
{
    bool success;

    S9S_WARNING("stringRep : %s", STR(stringRep));
    // Parsing as a JSon string, that's more specific.
    success = m_properties.parse(STR(stringRep));
    if (success)
    {
        S9S_WARNING("parsed as json");
        m_url = m_properties["hostname"].toString();

        if (m_properties.contains("port"))
            m_url.setPort(m_properties["port"].toInt());

        m_url.setProperties(m_properties);
    } else {
        S9S_WARNING("parsing as url");
        // If not ok then parsing as an URL.
        m_url = S9sUrl(stringRep);

        m_properties = m_url.properties();
        m_properties["hostname"] = m_url.hostName();

        if (m_url.hasPort())
            m_properties["port"] = m_url.port();
    }
  
    /*
     * Here we try to guess the class name from the protocol. This means for
     * example if you say "lxc://hostname" we will send a "CmonLxcServer"
     * class object. If you have more protocols you can add them here.
     */
    if (m_url.hasProtocol())
    {
        S9sString protocol = m_url.protocol().toLower();

        if (m_url.protocol() == "lxc")
            m_properties["class_name"] = "CmonLxcServer";
        else if (m_url.protocol() == "cmon-cloud")
            m_properties["class_name"] = "CmonCloudServer";
        else if (m_url.protocol().toLower() == "haproxy")
            m_properties["class_name"] = "CmonHaProxyHost";
        else if (m_url.protocol().toLower() == "pgbouncer")
            m_properties["class_name"] = "CmonPgBouncerHost";
        else if (m_url.protocol().toLower() == "pgbackrest")
            m_properties["class_name"] = "CmonPgBackRestHost";
        else if (m_url.protocol().toLower() == "pbmagent")
            m_properties["class_name"] = "CmonPBMAgentHost";
        else if (m_url.protocol().toLower() == "nfsclient")
            m_properties["class_name"] = "CmonNFSClientHost";
        else if (m_url.protocol().toLower() == "nfsserver")
            m_properties["class_name"] = "CmonNFSServerHost";
        else if (m_url.protocol().toLower() == "proxysql")
            m_properties["class_name"] = "CmonProxySqlHost";
        else if (m_url.protocol().toLower() == "proxysql")
            m_properties["class_name"] = "CmonProxySqlHost";
        else if (m_url.protocol().toLower() == "maxscale")
            m_properties["class_name"] = "CmonMaxScaleHost";
        else if (m_url.protocol().toLower() == "keepalived")
            m_properties["class_name"] = "CmonKeepalivedHost";
        else if (m_url.protocol().toLower() == "redis")
            m_properties["class_name"] = "CmonRedisHost";
        else if (m_url.protocol().toLower() == "redis-sentinel" ||
            m_url.protocol().toLower() == "sentinel")
            m_properties["class_name"] = "CmonRedisSentinelHost";
        else if (m_url.protocol().toLower() == "mssql")
            m_properties["class_name"] = "CmonMsSqlHost";
        else if (m_url.protocol().toLower() == "elastic")
            m_properties["class_name"] = "CmonElasticHost";
        else
            m_properties["protocol"] = m_url.protocol();
    }

    // The new argument passing allows the class_name left out, but does not
    // allow a missleading class name as an argument.
    //if (!m_properties.contains("class_name"))
    //    m_properties["class_name"] = "CmonHost";
}

S9sNode::~S9sNode()
{
}

S9sNode &
S9sNode::operator=(
        const S9sVariantMap &rhs)
{
    setProperties(rhs);
    
    return *this;
}

/**
 * \returns The S9sNode converted to a variant map.
 */
const S9sVariantMap &
S9sNode::toVariantMap() const
{
    return m_properties;
}

void 
S9sNode::setSshCredentials(
        const S9sSshCredentials &credentials)
{
    m_properties["ssh_credentials"] = credentials.toVariantMap();
}

void
S9sNode::setCluster(
        const S9sCluster &cluster)
{
    m_cluster = cluster;
}

const S9sCluster &
S9sNode::cluster() const
{
    return m_cluster;
}

/**
 * \returns the "class_name" property Cmon uses to represent the object type.
 */
S9sString
S9sNode::className() const
{
    if (m_properties.contains("class_name"))
        return m_properties.at("class_name").toString();

    return S9sString();
}

/**
 * \param syntaxHighlight Controls if the string will have colors or not.
 * \param formatString The formatstring with markup.
 * \returns The string representation according to the format string.
 *
 * Converts the node to a string using a special format string that may
 * contain field names of node properties.
 */
S9sString
S9sNode::toString(
        const bool       syntaxHighlight,
        const S9sString &formatString) const
{
    S9sFormatter formatter;
    S9sString    retval;
    S9sString    tmp;
    char         c;
    S9sString    partFormat;
    bool         percent      = false;
    bool         escaped      = false;
    bool         modifierFree = false;

    for (uint n = 0; n < formatString.size(); ++n)
    {
        c = formatString[n];
       
        if (c == '%' && !percent)
        {
            percent    = true;
            partFormat = "%";
            continue;
        } else if (percent && c == 'f')
        {
            modifierFree = true;
            continue;
        } else if (c == '\\' && !escaped)
        {
            escaped = true;
            continue;
        }

        if (escaped)
        {
            switch (c)
            {
                case '\"':
                    retval += '\"';
                    break;

                case '\\':
                    retval += '\\';
                    break;
       
                case 'a':
                    retval += '\a';
                    break;

                case 'b':
                    retval += '\b';
                    break;

                case 'e':
                    retval += '\027';
                    break;

                case 'n':
                    retval += '\n';
                    break;

                case 'r':
                    retval += '\r';
                    break;

                case 't':
                    retval += '\t';
                    break;
            }
        } else if (percent)
        {
            switch (c)
            {
                case 'A':
                    // The ip address of the node.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(ipAddress()));
                    retval += tmp;
                    break;
                
                case 'a':
                    // Maintenance flag.
                    partFormat += 's';
                    
                    tmp.sprintf(STR(partFormat), 
                            isMaintenanceActive() ? "M" : "-");

                    retval += tmp;
                    break;

                case 'b':
                    // The master, this node is following.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(masterHost()));
                    retval += tmp;
                
                    break;
 
                case 'C':
                    // The configuration file. 
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(configFile()));

                    if (syntaxHighlight)
                        retval += S9sRpcReply::fileColorBegin(configFile());

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += S9sRpcReply::fileColorEnd();

                    break;

                case 'c':
                    // The total number of CPU cores in the cluster.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), nCpuCores().toInt());
                    retval += tmp;
                    break;

                case 'D':
                    // The data directory.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(dataDir()));

                    if (syntaxHighlight)
                        retval += XTERM_COLOR_BLUE;

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += TERM_NORMAL;

                    break;
                
                case 'd':
                    // The PID file.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(pidFile()));

                    if (syntaxHighlight)
                        retval += S9sRpcReply::fileColorBegin(pidFile());

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += S9sRpcReply::fileColorEnd();

                    break;
                
                case 'E':
                    // The replication state.
                    partFormat += "s";
                    tmp.sprintf(STR(partFormat), STR(replicationState()));
                    retval += tmp;
                    break; 

                case 'G':
                    // The name of the group owner.
                    partFormat += 's';
                    tmp.sprintf(
                            STR(partFormat),
                            STR(groupOwnerName()));

                    if (syntaxHighlight)
                        retval += S9sRpcReply::groupColorBegin();

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += S9sRpcReply::groupColorEnd();

                    break;

                case 'g':
                    // The log file. 
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(logFile()));

                    if (syntaxHighlight)
                        retval += S9sRpcReply::fileColorBegin(logFile());

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += S9sRpcReply::fileColorEnd();

                    break;
                
                case 'h':
                    // The CDT path 
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(cdtPath()));

                    if (syntaxHighlight)
                        retval += formatter.folderColorBegin();

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += formatter.folderColorEnd();

                    break;

                case 'I':
                    // The ID of the node.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), hostId());

                    retval += tmp;
                    break;

                case 'i':
                    // The total number of monitored disk devices.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), nDevices().toInt());

                    retval += tmp;
                    break;

                case 'k':
                    // The total disk size found in the node.
                    partFormat += 'f';

                    if (modifierFree)
                    {
                        tmp.sprintf(
                                STR(partFormat), 
                                freeDiskBytes().toTBytes());
                    } else {
                        tmp.sprintf(
                                STR(partFormat), 
                                totalDiskBytes().toTBytes());
                    }

                    retval += tmp;
                    break;

                case 'N':
                    // The name of the node.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(name()));

                    if (syntaxHighlight)
                        retval += XTERM_COLOR_BLUE;

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += TERM_NORMAL;

                    break;
                
                case 'M':
                    // The message describing the node's status. 
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(message()));
                    retval += tmp;
                    break;

                case 'm':
                    // The total memory size found on the host.
                    partFormat += 'f';
                    if (modifierFree)
                        tmp.sprintf(STR(partFormat), memFree().toGBytes());
                    else
                        tmp.sprintf(STR(partFormat), memTotal().toGBytes());

                    retval += tmp;
                    break;

                case 'n':
                    // The total number of monitored network interfaces.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), nNics().toInt());

                    retval += tmp;
                    break;

                case 'O':
                    // The name of the owner.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(ownerName()));

                    if (syntaxHighlight)
                        retval += S9sRpcReply::userColorBegin();

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += S9sRpcReply::userColorEnd();

                    break;

                case 'o':
                    // The OS version string.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(osVersionString()));
                    retval += tmp;
                    break;
                
                case 'L':
                    // The replay location.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(replayLocation()));
                    retval += tmp;
                    break;
                
                case 'l':
                    // The received location.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(receivedLocation()));
                    retval += tmp;
                    break;

                case 'P':
                    // The Port.
                    partFormat += "d";
                    tmp.sprintf(STR(partFormat), port());
                    retval += tmp;
                    break;
                
                case 'p':
                    // The PID.
                    partFormat += "d";
                    tmp.sprintf(STR(partFormat), pid());
                    retval += tmp;
                    break;
                
                case 'R':
                    // The role.
                    partFormat += "s";
                    tmp.sprintf(STR(partFormat), STR(role()));
                    retval += tmp;
                    break;
                
                case 'r':
                    // A string 'read-only' or 'read-write'.
                    partFormat += "s";
                    tmp.sprintf(STR(partFormat), 
                            readOnly() ? "read-only" : "read-write");
                    retval += tmp;
                    break;

                case 'S':
                    // The state of the node.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(hostStatus()));

                    if (syntaxHighlight)
                    {
                        retval += formatter.hostStateColorBegin(hostStatus());
                    }

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += formatter.hostStateColorEnd();

                    break;
                
                case 's':
                    // The list of slaves in one string.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(slavesAsString()));
                    retval += tmp;
                
                    break;

                case 'T':
                    // The type of the node.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(nodeType()));
                    retval += tmp;
                    break;

                case 't':
                    // The network traffic found in the cluster.
                    partFormat += 'f';
                    tmp.sprintf(STR(partFormat), 
                            netBytesPerSecond().toMBytes());

                    retval += tmp;
                    break;

#if 0
                case 'U':
                    // The uptime.
                    partFormat += "s";
                    tmp.sprintf(STR(partFormat), 
                            STR(S9sString::uptime(uptime())));
                    retval += tmp;
                    break;
#endif
                case 'V':
                    // The version.
                    partFormat += "s";
                    tmp.sprintf(STR(partFormat), STR(version()));
                    retval += tmp;
                    break;
                
                case 'v':
                    // The container/vm ID.
                    partFormat += "s";
                    tmp.sprintf(STR(partFormat), STR(containerId("-")));
                    retval += tmp;
                    break;

                case 'U':
                    // The number of CPUs.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), nCpus().toInt());
                    retval += tmp;
                    break;

                case 'u':
                    // The cpu usage percent. 
                    partFormat += 'f';
                    tmp.sprintf(STR(partFormat), cpuUsagePercent().toDouble());
                    retval += tmp;
                    break;

                case 'w':
                    // The total swap space found in the host.
                    partFormat += 'f';
                    if (modifierFree)
                        tmp.sprintf(STR(partFormat), swapTotal().toGBytes());
                    else
                        tmp.sprintf(STR(partFormat), swapFree().toGBytes());

                    retval += tmp;
                    break;

                case 'Z':
                    // The CPU model.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(cpuModel()));
                    retval += tmp;
                    break;
                
                case 'z':
                    // The class name.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(className()));
                    
                    if (syntaxHighlight)
                        retval += XTERM_COLOR_GREEN;

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += TERM_NORMAL;
                    
                    break;

                case '%':
                    retval += '%';
                    break;

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case '-':
                case '+':
                case '.':
                case '\'':
                    partFormat += c;
                    continue;
            }
        } else {
            retval += c;
        }

        percent      = false;
        escaped      = false;
        modifierFree = false;
    }

    return retval;
}

/**
 * \returns The host ID, a unique ID that identifies the host itself.
 */
int
S9sNode::hostId() const
{
    if (m_properties.contains("hostId"))
        return m_properties.at("hostId").toInt();

    return 0;
}

/**
 * \returns The ID of the cluster holding the node.
 */
int
S9sNode::clusterId() const
{
    if (m_properties.contains("clusterid"))
        return m_properties.at("clusterid").toInt();

    return 0;
}


/**
 * \returns The hostname.
 *
 * This method will not return the alias, only the hostname, but that's usually
 * the IP address in our tests.
 */
S9sString
S9sNode::name() const
{
    S9sString retval;

    //retval = alias();
    if (retval.empty())
        retval = hostName();

    return retval;
}

/**
 * \returns The host name, the name that used in the Cmon Configuration file to
 *   register the node.
 */
S9sString
S9sNode::hostName() const
{
    if (m_properties.contains("hostname"))
        return m_properties.at("hostname").toString();

    return S9sString();
}

S9sString
S9sNode::fullCdtPath() const
{
    S9sString retval;

    retval.sprintf("%s/%s:%d", 
            STR(cdtPath()),
            STR(hostName()),
            port());

    return retval;
}

/**
 * \returns The IP address of the node.
 */
S9sString
S9sNode::ipAddress() const
{
    if (m_properties.contains("ip"))
        return m_properties.at("ip").toString();

    return S9sString();
}


/**
 * \returns The alias name (or nickname) of the node if there is one, returns
 *   the empty string if not.
 */
S9sString
S9sNode::alias(
        const S9sString defaultValue) const
{
    S9sString retval;

    if (m_properties.contains("alias"))
        retval = m_properties.at("alias").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 * \returns The "role" property (e.g. "controller", "master", "slave" or 
 * "none").
 */
S9sString
S9sNode::role() const
{
    if (m_properties.contains("role"))
        return m_properties.at("role").toString();

    return S9sString();
}

/**
 * sent to controller
 * \returns The "roles" property for an elastic node (e.g. "data", "master", ...).
 */
S9sString
S9sNode::roles() const
{
    if (m_properties.contains("roles"))
        return m_properties.at("roles").toString();

    return S9sString();
}

/**
 * returned from controller
 * \returns The "elastic_roles" property for an elastic node (e.g. "data", "master", ...).
 */
S9sString
S9sNode::elasticRole() const
{
    if (m_properties.contains("elastic_roles"))
        return m_properties.at("elastic_roles").toString();

    return S9sString();
}


/**
 * \returns The "member_role" property (e.g. "Primary", "Secondary", "Arbiter", etc).
 */
S9sString
S9sNode::memberRole() const
{
    if (m_properties.contains("member_role"))
        return m_properties.at("member_role").toString();

    return S9sString();
}

/**
 * \returns True if the class name suggests this is a container server.
 */
bool
S9sNode::isContainerServer() const
{
    if (className() == "CmonLxcServer" ||
            className() == "CmonCloudServer")
    {
        return true;
    }

    return false;
}

/**
 * \returns True if the class name suggests this is a load balancer.
 */
bool
S9sNode::isLoadBalaner() const
{

    if (className() == "CmonHaProxyHost" ||
            className() == "CmonProxySqlHost" ||
            className() == "CmonProxySqlHost" ||
            className() == "CmonMaxScaleHost" ||
            className() == "CmonKeepalivedHost")
    {
        return true;
    }
    
    return false;
}

/**
 * \returns True if the host is a master.
 */
bool
S9sNode::isMaster() const
{
    if (m_properties.contains("master"))
        return m_properties.at("master").toBoolean();
    else if (m_properties.contains("role"))
        return m_properties.at("role") == "master";

    return false;
}

/**
 * \returns True if the host is a slave.
 */
bool
S9sNode::isSlave() const
{
    if (m_properties.contains("slave"))
        return m_properties.at("slave").toBoolean();
    else if (m_properties.contains("role"))
        return m_properties.at("role") == "slave";

    return false;
}

/**
 * \returns A one character representation of the "role" property. We use this
 *   in lists we print because it takes less space and so we can print compact
 *   lists.
 */
char
S9sNode::roleFlag() const
{
    S9sString theRole = role();

    S9sString roles= this->elasticRole();

    if(roles.empty())
    {
        if (theRole == "master" || theRole.toLower() == "primary")
            return 'M';
        else if (theRole == "slave" || theRole.toLower() == "secondary")
            return 'S';
        else if (theRole == "multi")
            return 'U';
        else if (theRole == "controller")
            return 'C';
        else if (theRole == "bvs")
            return 'V';
        else if (theRole == "arbiter")
            return 'A';
        else if (theRole == "backuprepo")
            return 'R';
        else if (theRole == "shardsvr")
        {
            S9sString mRole = memberRole();
            if (mRole == "Primary")
                return 'M';
            else if (mRole == "Secondary")
                return 'S';
        }
    }
    else // on elasticsearch nodes the may have multiple roles
    {
        if (roles == "master")
            return 'M';
        else if (roles == "data")
            return 'D';
        else if (roles == "master-data")
            return 'U';  // multi
        //else if (roles == "master-data_hot")
        //    return 'U';  // multi
        //else if (roles == "master-data_warm")
        //    return 'U';  // multi
        //else if (roles == "master-data_frozen")
        //    return 'U';  // multi
        //else if (roles == "master-data_content")
        //    return 'U';  // multi
        else if (roles == "coordinator_only")
            return 'c';  // coordinator only
        else 
            return '-';  // others
    }

    return '-';
}

/**
 * \returns The full path of the configuration file of the node.
 */
S9sString
S9sNode::configFile() const
{
    S9sString retval;

    if (m_properties.contains("configfile"))
    {
        S9sVariant variant = m_properties.at("configfile");

        if (variant.isVariantList())
        {
            for (uint idx = 0u; idx < variant.toVariantList().size(); ++idx)
            {
                if (!retval.empty())
                    retval += "; ";

                retval += variant.toVariantList()[idx].toString();
            }
        } else {
            retval = m_properties.at("configfile").toString();
        }
    }

    return retval;
}

/**
 * \returns The full path of the log file for the node.
 */
S9sString
S9sNode::logFile() const
{
    if (m_properties.contains("logfile"))
        return m_properties.at("logfile").toString();

    return S9sString();
}

/**
 * \returns The PID file of the host, the full path of the file that on the host
 *   stores the PID of the most important process (e.g. the mysql daemon).
 */
S9sString
S9sNode::pidFile() const
{
    if (m_properties.contains("pidfile"))
        return m_properties.at("pidfile").toString();

    return S9sString();
}

/**
 * \returns The data directory of the node.
 */
S9sString
S9sNode::dataDir() const
{
    if (m_properties.contains("datadir"))
        return m_properties.at("datadir").toString();

    return S9sString();
}

/**
 * \returns True if the node has a port number set.
 */
bool
S9sNode::hasPort() const
{
    return m_properties.contains("port");
}

/**
 * \returns True if an error found while parsing the string representation of
 *   the node.
 */
bool
S9sNode::hasError() const
{
    return m_url.hasError();
}

/**
 * \returns A multi-line error message discribing the error found while parsing
 *   the string representation or the empty string if no error was found.
 */
S9sString
S9sNode::fullErrorString() const
{
    return m_url.fullErrorString();
}

/**
 * \returns the port number for the node.
 */
int
S9sNode::port() const
{
    if (m_properties.contains("port"))
        return m_properties.at("port").toInt();

    return 0;
}

/**
 * \returns The host status as a string.
 */
S9sString
S9sNode::hostStatus() const
{
    if (m_properties.contains("hoststatus"))
        return m_properties.at("hoststatus").toString();

    return S9sString();
}

S9sString
S9sNode::hostStatusShort() const
{
    S9sString retval = hostStatus();

    retval.replace("CmonHost", "");
    return retval;
}


/**
 * \returns The host status encoded into one character.
 */
int
S9sNode::stateAsChar() const
{
    S9sString theHostStatus = hostStatus();

    if (theHostStatus == "CmonHostUnknown")
        return '?';
    else if (theHostStatus == "CmonHostOnline")
        return 'o';
    else if (theHostStatus == "CmonHostOffLine")
        return 'l';
    else if (theHostStatus == "CmonHostFailed")
        return 'f';
    else if (theHostStatus == "CmonHostRecovery")
        return 'r';
    else if (theHostStatus == "CmonHostShutDown")
        return '-';

    return '?';
}

/**
 * \returns The "nodetype" property string. It can be strings like "controller",
 *   "galera", "postgres".
 */
S9sString
S9sNode::nodeType() const
{
    if (m_properties.contains("nodetype"))
        return m_properties.at("nodetype").toString();

    return S9sString();
}

/**
 * \returns A one character representation of the node type property. We use
 *   this to print it out in node lists.
 */
char 
S9sNode::nodeTypeFlag() const
{
    S9sString theNodeType = nodeType();
    
    if (theNodeType == "controller")
        return 'c';
    else if (theNodeType == "galera")
        return 'g';
    else if (theNodeType == "maxscale")
        return 'x';
    else if (theNodeType == "keepalived")
        return 'k';
    else if (theNodeType == "postgres")
        return 'p';
    else if (theNodeType == "mongo")
        return 'm';
    else if (theNodeType == "memcached")
        return 'e';
    else if (theNodeType == "proxysql")
        return 'y';
    else if (theNodeType == "haproxy")
        return 'h';
    else if (theNodeType == "pgbouncer")
        return 'b';
    else if (theNodeType == "pgbackrest")
        return 'B';
    else if (theNodeType == "pbmagent")
        return 't';
    else if (theNodeType == "nfsclient")
        return 'n';
    else if (theNodeType == "nfsserver")
        return 'f';
    else if (theNodeType == "garbd")
        return 'a';
    else if (theNodeType == "grouprepl")
        return 'r';
    else if (theNodeType == "cmonagent")
        return 'A';
    else if (theNodeType == "prometheus")
        return 'P';

    else if (className() == "CmonMySqlHost")
        return 's';
    else if (className() == "CmonRedisSentinelHost")
        return 'S';
    else if (className() == "CmonRedisHost")
        return 'R';
    else if (className() == "CmonElasticHost")
        return 'E';
    else if (className() == "CmonMsSqlHost")
        return '$';

    return '?';
}

char
S9sNode::maintenanceFlag() const
{
    return isMaintenanceActive() ? 'M' : '-';
}

/**
 * \returns The software version of the node.
 *
 * This method will return the version string of the most important software on
 * the node. It is in most cases the version number of the Cmon or the version
 * number of the SQL server.
 *
 * This method also truncates the version string. Sometimes we get insanely
 * long version number with the provider, build and everything. This takes up
 * half of the screen width, so we truncate to get some meaningful, but short
 * strings.
 */
S9sString
S9sNode::version() const
{
    S9sString retval;

    if (m_properties.contains("version"))
        retval = m_properties.at("version").toString();

    if (retval.contains(" "))
        retval = retval.substr(0, retval.find(' '));
    
    if (retval.contains("-"))
        retval = retval.substr(0, retval.find('-'));

    return retval;
}

S9sString 
S9sNode::containerId(
        const S9sString &defaultValue) const
{
    S9sString retval;

    if (m_properties.contains("container_id"))
        retval = m_properties.at("container_id").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 * \returns A human readable message about the current status of the node.
 */
S9sString
S9sNode::message() const
{
    S9sString retval;

    if (m_properties.contains("message"))
        retval = m_properties.at("message").toString();

    if (!retval.empty() &&
        m_properties.contains("readonly"))
    {
        retval.replace(".", "");        
        if (m_properties.at("readonly").toBoolean())
            retval += " (read-only).";
        else
            retval += " (read-write).";
    }
    
    if (retval.empty() && m_properties.contains("errormsg"))
        retval = m_properties.at("errormsg").toString();
    
    retval.replace("\n", "\\n");
    retval.replace("\r", "\\r");
    return retval;
}

/**
 * \returns A string encodes the operating system name and version.
 */
S9sString
S9sNode::osVersionString() const
{
    S9sString retval;

    if (m_properties.contains("distribution"))
    {
        S9sVariantMap map = m_properties.at("distribution").toVariantMap();
        S9sString     name, release, codeName;
        
        name     = map["name"].toString();
        release  = map["release"].toString();
        codeName = map["codename"].toString();

        retval.appendWord(name);
        retval.appendWord(release);
        retval.appendWord(codeName);
    }

    return retval;
}

/**
 * \returns The process ID of the most important process on the node (e.g. the
 *   PID of MySQL daemon on a Galera host).
 */
int
S9sNode::pid() const
{
    int retval = -1;

    if (m_properties.contains("pid"))
        retval = m_properties.at("pid").toInt();

    return retval;
}

/**
 * \returns How many seconds elapsed since the most important service on the
 *   host was started.
 */
ulonglong
S9sNode::uptime() const
{
    ulonglong retval = 0ull;

    if (m_properties.contains("uptime"))
        retval = m_properties.at("uptime").toULongLong();

    return retval;
}

S9sString
S9sNode::replicationState() const
{
    S9sString retval;

    if (m_properties.contains("replication_state"))
        retval = m_properties.at("replication_state").toString();

    if (m_properties.contains("galera"))
    {
        S9sVariantMap map;
        
        map = m_properties.at("galera").toVariantMap();
        retval = map["localstatusstr"].toString().toLower();
    }

    if (retval.empty() && m_properties.contains("replication_master"))
    {
        S9sVariantMap map;
        
        map = m_properties.at("replication_master").toVariantMap();

        if (map["semisync_status"] == "ON")
            retval = "semisync";
    } 
    
    if (retval.empty() && m_properties.contains("replication_slave"))
    {
        S9sVariantMap map;
        
        map = m_properties.at("replication_slave").toVariantMap();

        if (map["semisync_status"] == "ON")
            retval = "semisync";
    } 

    if (retval.empty() && m_properties.contains("connected_slaves"))
    {
        if (m_properties.at("connected_slaves").toInt() > 0)
            retval = "connected";
    }

    if (retval.empty())
        retval = "-";

    return retval;
}

/**
 * \returns true if the maintenance mode is active for the given node.
 */
bool
S9sNode::isMaintenanceActive() const
{
    if (m_properties.contains("maintenance_mode_active"))
        return m_properties.at("maintenance_mode_active").toBoolean();

    return false;
}

/**
 * \returns True if the node is read-only.
 */
bool
S9sNode::readOnly() const
{
    if (m_properties.contains("readonly"))
        return m_properties.at("readonly").toBoolean();

    return false;
}

bool
S9sNode::superReadOnly() const
{
    if (m_properties.contains("super_read_only"))
        return m_properties.at("super_read_only").toBoolean();

    return false;
}

/**
 * \returns True if the node is connected (reachable).
 */
bool
S9sNode::connected() const
{
    if (m_properties.contains("connected"))
        return m_properties.at("connected").toBoolean();

    return false;
}

S9sString
S9sNode::receivedLocation() const
{
    if (m_properties.contains("received_location"))
        return m_properties.at("received_location").toString();

    return S9sString();
}

S9sString
S9sNode::replayLocation() const
{
    if (m_properties.contains("replay_location"))
        return m_properties.at("replay_location").toString();

    return S9sString();
}


/**
 * \returns The value of the "managed" property, that shows if the node is
 *   actually managed by cmon.
 */
bool
S9sNode::managed() const
{
    if (m_properties.contains("managed"))
        return m_properties.at("managed").toBoolean();

    return false;
}

bool
S9sNode::nodeAutoRecovery() const
{
    if (m_properties.contains("node_auto_recovery"))
        return m_properties.at("node_auto_recovery").toBoolean();

    return false;
}

bool
S9sNode::skipNameResolve() const
{
    if (m_properties.contains("skip_name_resolve"))
        return m_properties.at("skip_name_resolve").toBoolean();

    return false;
}

time_t
S9sNode::lastSeen() const
{
    if (m_properties.contains("lastseen"))
        return m_properties.at("lastseen").toTimeT();

    return false;
}

int
S9sNode::sshFailCount() const
{
    if (m_properties.contains("sshfailcount"))
        return m_properties.at("sshfailcount").toInt();

    return 0;
}

/**
 * \returns One string that contains the names of all slaves.
 */
S9sString
S9sNode::slavesAsString() const
{
    S9sVariantList list;
    S9sString      retval;

    if (m_properties.contains("slaves"))
        list = m_properties.at("slaves").toVariantList();

    for (uint idx = 0u; idx < list.size(); ++idx)
    {
        if (!retval.empty())
            retval += "; ";

        retval += list[idx].toString();
    }

    return retval;
}

/**
 * replication_slave/master_host
 */
S9sString
S9sNode::masterHost() const
{
    S9sString retval;

    if (hasReplicationSlaveInfo())
        retval = replicationSlaveInfo()["master_host"].toString();

    return retval;
}

int
S9sNode::masterPort() const
{
    int retval = -1;

    if (hasReplicationSlaveInfo())
        retval = replicationSlaveInfo()["master_port"].toInt();

    return retval;
}

bool
S9sNode::hasMasterClusterId() const
{
    bool retval = false;

    if (hasReplicationSlaveInfo())
        retval = replicationSlaveInfo().contains("master_cluster_id");

    return retval;
}

int
S9sNode::masterClusterId() const
{
    int retval = -1;

    if (hasMasterClusterId())
        retval = replicationSlaveInfo()["master_cluster_id"].toInt();

    return retval;
}

S9sString
S9sNode::cpuModel() const
{
    return m_cluster.cpuModel(hostId());
}

S9sVariant 
S9sNode::memTotal() const
{
    return m_cluster.memTotal(hostId());
}

S9sVariant 
S9sNode::memFree() const
{
    return m_cluster.memFree(hostId());
}

/**
 * \returns The number of CPU cores (siblings actually) in the host.
 */
S9sVariant 
S9sNode::nCpuCores() const
{
    return m_cluster.nCpuCores(hostId());
}

/**
 * \returns The number of physical CPUs in the host.
 */
S9sVariant 
S9sNode::nCpus() const
{
    return m_cluster.nCpus(hostId());
}

S9sVariant 
S9sNode::nNics() const
{
    return m_cluster.nNics(hostId());
}

/**
 * \returns The number of monitored disk devices on the host.
 */
S9sVariant 
S9sNode::nDevices() const
{
    return m_cluster.nDevices(hostId());
}

S9sVariant 
S9sNode::totalDiskBytes() const
{
    return m_cluster.totalDiskBytes(hostId());
}

S9sVariant 
S9sNode::freeDiskBytes() const
{
    return m_cluster.freeDiskBytes(hostId());
}

/**
 * \returns The measured network traffic both sent and received.
 */
S9sVariant
S9sNode::netBytesPerSecond() const
{
    S9sVariant retval;

    retval  = rxBytesPerSecond();
    retval += txBytesPerSecond();

    return retval;
}

S9sVariant
S9sNode::cpuUsagePercent() const
{
    return m_cluster.cpuUsagePercent(hostId());
}

S9sVariant
S9sNode::swapTotal() const
{
    return m_cluster.swapTotal(hostId());
}

S9sVariant
S9sNode::swapFree() const
{
    return m_cluster.swapFree(hostId());
}

/**
 * \return The current download speed of the computer in bytes/sec.
 *
 */
S9sVariant
S9sNode::rxBytesPerSecond() const
{
    return m_cluster.rxBytesPerSecond(hostId());
}

/**
 * \return The current upload speed of the computer in bytes/sec.
 *
 */
S9sVariant
S9sNode::txBytesPerSecond() const
{
    return m_cluster.txBytesPerSecond(hostId());
}

/**
 * \param theList List of S9sNode objects to select from.
 * \param matchedNodes The list where the matching nodes will be placed.
 * \param otherNodes The list where non-matching nodes are placed.
 * \param protocol The protocol to select.
 *
 * This function goes through a list of nodes and selects those that have
 * matching protocol.
 */
void
S9sNode::selectByProtocol(
        const S9sVariantList &theList,
        S9sVariantList       &matchedNodes,
        S9sVariantList       &otherNodes,
        const S9sString      &protocol)
{
    S9sString protocolToFind = protocol.toLower();

    for (uint idx = 0u; idx < theList.size(); ++idx)
    {
        S9sNode   node;
        S9sString protocol;

        node     = theList[idx].toNode();
        protocol = node.protocol().toLower();

        if (protocol == protocolToFind)
            matchedNodes << node;
        else 
            otherNodes << node;
    }
}

/**
 *
 * \code{.js}
 * {
 *     "admin_user": "admin2",
 *     "backend_info": 
 *     {
 *         "backend_servers": [ 
 *         {
 *             "Bytes_data_recv": "0",
 *             "Bytes_data_sent": "0",
 *             "ConnERR": "0",
 *             "ConnFree": "0",
 *             "ConnUsed": "0",
 *             "Latency": "284",
 *             "Queries": "0",
 *             "class_name": "CmonProxySqlServer",
 *             "comment": "read server",
 *             "compression": "0",
 *             "hostgroup_id": "20",
 *             "hostname": "192.168.0.118",
 *             "max_connections": "100",
 *             "max_latency_ms": "0",
 *             "max_replication_lag": "10",
 *             "port": "3306",
 *             "status": "ONLINE",
 *             "use_ssl": "0",
 *             "weight": "1000"
 *         }, 
 *         . . .
 * \endcode
 */
S9sVariantList 
S9sNode::backendServers() const
{
    S9sVariantList retval;

    if (m_properties.contains("backend_info"))
    {
        S9sVariantMap backendInfo = 
            m_properties.at("backend_info").toVariantMap();

        if (backendInfo.contains("backend_servers"))
            retval = backendInfo.at("backend_servers").toVariantList();
    }

    return retval;
}

bool
S9sNode::hasBackendServers() const
{
    return !backendServers().empty();
}

uint 
S9sNode::numberOfBackendServers() const
{
    return backendServers().size();
}

S9sString
S9sNode::backendServerName(
        uint index) const
{
    S9sVariantList theList = backendServers();
    S9sString      retval;

    if (index >= 0u && index < theList.size())
    {
        retval = theList[index]["hostname"].toString();

        // Haproxy seems to have "name" instead of "hostname".
        if (retval.empty())
            retval = theList[index]["name"].toString();
    }

    return retval;
}

int
S9sNode::backendServerPort(
        uint index) const
{
    S9sVariantList theList = backendServers();
    int            retval  = -1;

    if (index < theList.size())
        retval = theList[index]["port"].toInt();

    return retval;
}

S9sString
S9sNode::backendServerStatus(
        uint index) const
{
    S9sVariantList theList = backendServers();
    S9sString      retval;

    if (index < theList.size())
        retval = theList[index]["status"].toString();

    return retval;
}

S9sString
S9sNode::backendServerComment(
        uint             index,
        const S9sString &defaultValue) const
{
    S9sVariantList theList = backendServers();
    S9sString      retval;

    if (index < theList.size())
        retval = theList[index]["comment"].toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

bool 
S9sNode::hasReplicationSlaveInfo() const
{
    return m_properties.contains("replication_slave");
}

S9sVariantMap 
S9sNode::replicationSlaveInfo() const
{
    if (m_properties.contains("replication_slave"))
    {
        return m_properties.at("replication_slave").toVariantMap();
    }

    return S9sVariantMap();
}


