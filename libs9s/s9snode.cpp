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

S9sNode::~S9sNode()
{
}

void
S9sNode::setProperties(
        const S9sVariantMap &properties)
{
    m_properties = properties;
}

S9sString
S9sNode::className() const
{
    if (m_properties.contains("class_name"))
        return m_properties.at("class_name").toString();

    return S9sString();
}

S9sString
S9sNode::hostName() const
{
    if (m_properties.contains("hostname"))
        return m_properties.at("hostname").toString();

    return S9sString();
}

bool
S9sNode::hasPort()
{
    return m_properties.contains("port");
}

int
S9sNode::port() const
{
    if (m_properties.contains("port"))
        return m_properties.at("port").toInt();

    return 0;
}

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
S9sNode::isMaintenanceAcrtive() const
{
    if (m_properties.contains("maintenance_mode_active"))
        return m_properties.at("maintenance_mode_active").toBoolean();

    return false;
}

