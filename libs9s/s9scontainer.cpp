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
#include "s9scontainer.h"

#include <S9sUrl>
#include <S9sVariantMap>
#include <S9sRpcReply>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sContainer::S9sContainer()
{
    //m_properties["class_name"] = "CmonContainer";
}
 
S9sContainer::S9sContainer(
        const S9sVariantMap &properties) :
    m_properties(properties)
{
    //if (!m_properties.contains("class_name"))
    //    m_properties["class_name"] = "CmonContainer";
}

/**
 * \param stringRep The string representation of the host, either a JSon string
 *   or an url (e.g. "192.168.1.100:3306".
 */
S9sContainer::S9sContainer(
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
   
    if (m_url.hasProtocol())
    {
        S9sString protocol = m_url.protocol().toLower();

        if (m_url.protocol() == "lxc")
            m_properties["class_name"] = "CmonContainerServer";
    }

    if (!m_properties.contains("class_name"))
        m_properties["class_name"] = "CmonHost";
}

S9sContainer::~S9sContainer()
{
}

S9sContainer &
S9sContainer::operator=(
        const S9sVariantMap &rhs)
{
    setProperties(rhs);
    
    return *this;
}

/**
 * \returns The S9sContainer converted to a variant map.
 *
 * \code{.js}
 * {
 *     "alias": "mqtt",
 *     "class_name": "CmonContainer",
 *     "hostname": "192.168.1.5",
 *     "ip": "192.168.1.5",
 *     "ipv4_addresses": [ "192.168.1.5" ],
 *     "owner_group_id": 4,
 *     "owner_group_name": "testgroup",
 *     "owner_user_id": 3,
 *     "owner_user_name": "pipas",
 *     "parent_server": "core1",
 *     "status": "RUNNING",
 *     "type": "lxc"
 * }, 
 * \endcode
 */
const S9sVariantMap &
S9sContainer::toVariantMap() const
{
    return m_properties;
}

/**
 * \returns True if a property with the given key exists.
 */
bool
S9sContainer::hasProperty(
        const S9sString &key) const
{
    return m_properties.contains(key);
}

/**
 * \returns The value of the property with the given name or the empty
 *   S9sVariant object if the property is not set.
 */
S9sVariant
S9sContainer::property(
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
S9sContainer::setProperty(
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
S9sContainer::setProperties(
        const S9sVariantMap &properties)
{
    m_properties = properties;
}

S9sString 
S9sContainer::aclString() const
{
    return property("acl").toString();
}

S9sString 
S9sContainer::alias() const
{
    return property("alias").toString();
}

S9sString 
S9sContainer::cdtPath() const
{
    return property("cdt_path").toString();
}

S9sString 
S9sContainer::className() const
{
    return property("class_name").toString();
}

int
S9sContainer::containerId() const
{
    return property("container_id").toInt();
}

S9sString 
S9sContainer::hostname() const
{
    return property("hostname").toString();
}

S9sString 
S9sContainer::ipAddress(
        const S9sString &defaultValue) const
{
    S9sString retval = property("ip").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

S9sString 
S9sContainer::ipv4Addresses(
        const S9sString &separator,
        const S9sString &defaultValue)
{
    S9sVariantList theList =  property("ipv4_addresses").toVariantList();
    S9sString      retval;

    for (uint idx = 0u; idx < theList.size(); ++idx)
    {
        if (!retval.empty())
            retval += separator;

        retval += theList[idx].toString();
    }

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 * \returns The user name of the user that owns this object.
 */
S9sString 
S9sContainer::ownerName() const
{
    return property("owner_user_name").toString();
}

/**
 * \returns The group name of the group that owns the object.
 */
S9sString 
S9sContainer::groupOwnerName() const
{
    return property("owner_group_name").toString();
}

/**
 * \returns The name of the server that holds the container.
 */
S9sString 
S9sContainer::parentServerName() const
{
    return property("parent_server").toString();
}

/**
 * \returns "STOPPED" or "RUNNING".
 */
S9sString 
S9sContainer::state() const
{
    return property("status").toString();
}

S9sString 
S9sContainer::templateName() const
{
    return property("template").toString();
}

S9sString 
S9sContainer::type() const
{
    return property("type").toString();
}

S9sString 
S9sContainer::configFile() const
{
    return property("configfile").toString();
}

S9sString 
S9sContainer::rootFsPath() const
{
    return property("root_fs_path").toString();
}

/**
 * \returns The processor architecture (e.g. "x86_64").
 */
S9sString 
S9sContainer::architecture(
        const S9sString &defaultValue) const
{
    S9sString retval;

    retval = property("architecture").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 * \returns A string encodes the operating system name and version.
 */
S9sString
S9sContainer::osVersionString(
        const S9sString &defaultValue) const
{
    S9sString retval;

    S9sVariantMap map = property("os_version").toVariantMap();
    S9sString     name, release, codeName;
    
    name     = map["name"].toString();
    release  = map["release"].toString();
    codeName = map["codename"].toString();

    retval.appendWord(name);
    retval.appendWord(release);
    retval.appendWord(codeName);

    if (retval.empty())
        retval = defaultValue;

    return retval;
}
