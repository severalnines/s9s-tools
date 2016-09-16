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
#include "s9snode.h"

#include "S9sUrl"

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

    success = m_properties.parse(STR(stringRep));
    if (!success)
    {
        S9sUrl url(stringRep);

        m_properties.clear();
        m_properties["hostname"] = url.hostName();

        if (url.hasPort())
            m_properties["port"] = url.port();
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

const S9sVariantMap &
S9sNode::toVariantMap() const
{
    return m_properties;
}

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
 * \returns true if the node has a port number set.
 */
bool
S9sNode::hasPort() const
{
    return m_properties.contains("port");
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
S9sNode::nodeType() const
{
    if (m_properties.contains("nodetype"))
        return m_properties.at("nodetype").toString();

    return S9sString();
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
    if (m_properties.contains("message"))
        return m_properties.at("message").toString();

    return S9sString();
}

bool
S9sNode::isMaintenanceActive() const
{
    if (m_properties.contains("maintenance_mode_active"))
        return m_properties.at("maintenance_mode_active").toBoolean();

    return false;
}

