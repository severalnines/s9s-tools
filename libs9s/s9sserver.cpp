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
#include "s9sserver.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sServer::S9sServer()
{
    m_properties["class_name"] = "CmonHost";
}

S9sServer::S9sServer(
        const S9sVariantMap &properties) :
    m_properties(properties)
{
    if (!m_properties.contains("class_name"))
        m_properties["class_name"] = "CmonHost";
}

S9sServer::~S9sServer()
{
}

S9sServer &
S9sServer::operator=(
        const S9sVariantMap &rhs)
{
    setProperties(rhs);
    
    return *this;
}

/**
 * \returns True if a property with the given key exists.
 */
bool
S9sServer::hasProperty(
        const S9sString &key) const
{
    return m_properties.contains(key);
}

/**
 * \returns The value of the property with the given name or the empty
 *   S9sVariant object if the property is not set.
 */
S9sVariant
S9sServer::property(
        const S9sString &name) const
{
    if (m_properties.contains(name))
        return m_properties.at(name);

    return S9sVariant();
}

/**
 * \param properties The properties to be set as a name -> value mapping.
 *
 * Sets all the properties in one step. All the existing properties will be
 * deleted, then the new properties set.
 */
void
S9sServer::setProperties(
        const S9sVariantMap &properties)
{
    m_properties = properties;
}

S9sString
S9sServer::hostName() const
{
    return property("hostname").toString();
}

S9sString
S9sServer::alias() const
{
    return property("alias").toString();
}

S9sString
S9sServer::version() const
{
    return property("version").toString();
}

S9sString
S9sServer::ipAddress() const
{
    return property("ip").toString();
}

int
S9sServer::nContainers() const
{
    return property("containers").size();
}

S9sString
S9sServer::ownerName() const
{
    return property("owner_user_name").toString();
}

S9sString
S9sServer::groupOwnerName() const
{
    return property("owner_group_name").toString();
}

S9sString
S9sServer::className() const
{
    return property("class_name").toString();
}

S9sString
S9sServer::model() const
{
    return property("model").toString();
}

/**
 * \returns A string encodes the operating system name and version.
 */
S9sString
S9sServer::osVersionString() const
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
 * \returns A compact list enumerating the processors found in the host.
 *
 * The strings look like this: "2 x Intel(R) Xeon(R) CPU L5520 @ 2.27GHz"
 */
S9sVariantList
S9sServer::processorNames() const
{
    S9sVariantList  retval;
    S9sVariantMap   cpuModels;
    S9sVariantList  processorList = property("processors").toVariantList();

    for (uint idx1 = 0; idx1 < processorList.size(); ++idx1)
    {
        S9sVariantMap processor = processorList[idx1].toVariantMap();
        S9sString     model     = processor["model"].toString();

        cpuModels[model] += 1;
    }

    for (uint idx = 0; idx < cpuModels.keys().size(); ++idx)
    {
        S9sString  name = cpuModels.keys().at(idx);
        int        volume = cpuModels[name].toInt();
        S9sString  line;

        line.sprintf("%2d x %s", volume, STR(name));

        retval << line;
    }

    return retval;
}

S9sVariantList
S9sServer::nicNames() const
{
    S9sVariantList  retval;
    S9sVariantMap   nicModels;
    S9sVariantList  nicList = property("network_interfaces").toVariantList();

    for (uint idx1 = 0; idx1 < nicList.size(); ++idx1)
    {
        S9sVariantMap nic    = nicList[idx1].toVariantMap();
        S9sString     model  = nic["model"].toString();

        if (model.empty())
            continue;

        nicModels[model] += 1;
    }

    for (uint idx = 0; idx < nicModels.keys().size(); ++idx)
    {
        S9sString  name   = nicModels.keys().at(idx);
        int        volume = nicModels[name].toInt();
        S9sString  line;

        line.sprintf("%2d x %s", volume, STR(name));

        retval << line;
    }

    return retval;
}

S9sVariantList
S9sServer::memoryBankNames() const
{
    S9sVariantList  retval;
    S9sVariantMap   bankModels;
    S9sVariantMap   memory  = property("memory").toVariantMap();
    S9sVariantList  bankList = memory["banks"].toVariantList();

    for (uint idx1 = 0; idx1 < bankList.size(); ++idx1)
    {
        S9sVariantMap bank   = bankList[idx1].toVariantMap();
        S9sString     model  = bank["name"].toString();

//        if (model.empty())
//            continue;

        bankModels[model] += 1;
    }

    for (uint idx = 0; idx < bankModels.keys().size(); ++idx)
    {
        S9sString  name   = bankModels.keys().at(idx);
        int        volume = bankModels[name].toInt();
        S9sString  line;

        line.sprintf("%2d x %s", volume, STR(name));

        retval << line;
    }

    return retval;
}

