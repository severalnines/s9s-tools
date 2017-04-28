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
#include "s9snode.h"

#include <S9sUrl>
#include <S9sVariantMap>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sNode::S9sNode()
{
}
 
S9sNode::S9sNode(
        const S9sVariantMap &properties) :
    m_properties(properties)
{
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

/**
 * \returns True if a property with the given key exists.
 */
bool
S9sNode::hasProperty(
        const S9sString &key) const
{
    return m_properties.contains(key);
}

/**
 * \returns The value of the property with the given name or the empty
 *   S9sVariant object if the property is not set.
 */
S9sVariant
S9sNode::property(
        const S9sString &name) const
{
    if (m_properties.contains(name))
        return m_properties.at(name);

    return S9sVariant();
}

/**
 * \param name The name of the property to set.
 * \param value The value of the property as a string.
 *
 * This function will investigate the value represented as a string. If it looks
 * like a boolean value (e.g. "true") then it will be converted to a boolean
 * value, if it looks like an integer (e.g. 42) it will be converted to an
 * integer. Then the property will be set accordingly.
 */
void
S9sNode::setProperty(
        const S9sString &name,
        const S9sString &value)
{
    if (value.looksBoolean())
    {
        m_properties[name] = value.toBoolean();
    } else if (value.looksInteger())
    {
        m_properties[name] = value.toInt();
    } else {
        m_properties[name] = value;
    }
}

/**
 * \param properties The properties to be set as a name -> value mapping.
 *
 * Sets all the properties in one step. All the existing properties will be
 * deleted, then the new properties set.
 */
void
S9sNode::setProperties(
        const S9sVariantMap &properties)
{
    m_properties = properties;
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
 * \returns The name of the node that shall be used to represent it in user
 *   output.
 *
 * The return value might be the alias, the host name or even the IP address.
 * Currently this function is not fully implemented and it does not consider any
 * settings.
 */
S9sString
S9sNode::name() const
{
    S9sString retval;

    retval = alias();
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
S9sNode::alias() const
{
    if (m_properties.contains("alias"))
        return m_properties.at("alias").toString();

    return S9sString();
}

/**
 * \returns The "role" property.
 */
S9sString
S9sNode::role() const
{
    if (m_properties.contains("role"))
        return m_properties.at("role").toString();

    return S9sString();
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

    if (theRole == "master")
        return 'M';
    else if (theRole == "slave")
        return 'S';
    else if (theRole == "multi")
        return 'U';
    else if (theRole == "controller")
        return 'C';

    return '-';
}

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
            variant = m_properties.at("configfile").toString();
        }
    }

    return retval;
}

S9sString
S9sNode::logFile() const
{
    if (m_properties.contains("logfile"))
        return m_properties.at("logfile").toString();

    return S9sString();
}

S9sString
S9sNode::pidFile() const
{
    if (m_properties.contains("pidfile"))
        return m_properties.at("pidfile").toString();

    return S9sString();
}

S9sString
S9sNode::dataDir() const
{
    if (m_properties.contains("datadir"))
        return m_properties.at("datadir").toString();

    return S9sString();
}

/**
 * \returns true if the node has a port number set.
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

char 
S9sNode::hostStatusFlag() const
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
    else if (theNodeType == "garbd")
        return 'a';

    if (className() == "CmonMySqlHost")
        return 's';
    
    return '?';
}

S9sString
S9sNode::version() const
{
    if (m_properties.contains("version"))
        return m_properties.at("version").toString();

    return S9sString();
}

S9sString
S9sNode::message() const
{
    S9sString retval;

    if (m_properties.contains("message"))
        retval = m_properties.at("message").toString();
    
    if (retval.empty() && m_properties.contains("errormsg"))
        retval = m_properties.at("errormsg").toString();

    return retval;
}

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

int
S9sNode::pid() const
{
    int retval = -1;

    if (m_properties.contains("pid"))
        retval = m_properties.at("pid").toInt();

    return retval;
}

ulonglong
S9sNode::uptime() const
{
    ulonglong retval = 0ull;

    if (m_properties.contains("uptime"))
        retval = m_properties.at("uptime").toULongLong();

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
