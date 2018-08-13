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
#include "S9sContainer"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sServer::S9sServer() :
    S9sObject()
{
    m_properties["class_name"] = "CmonHost";
}

S9sServer::S9sServer(
        const S9sServer &orig) :
    S9sObject(orig)
{
}

S9sServer::S9sServer(
        const S9sVariantMap &properties) :
    S9sObject(properties)
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

S9sString 
S9sServer::className() const
{
    return property("class_name").toString();
}

S9sString 
S9sServer::name() const
{
    return hostName();
}

S9sString
S9sServer::id(
        const S9sString &defaultValue) const
{
    //S9sString retval = property("hostId").toString();
    S9sString retval = property("unique_id").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

S9sString
S9sServer::hostName() const
{
    return property("hostname").toString();
}

S9sString
S9sServer::alias(
        const S9sString &defaultValue) const
{
    if (hasProperty("alias"))
        return property("alias").toString();

    return defaultValue;
}

S9sString
S9sServer::message(
        const S9sString &defaultValue) const
{
    S9sString retval;

    if (hasProperty("message"))
        retval = property("message").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

S9sString
S9sServer::version(
        const S9sString &defaultValue) const
{
    S9sString retval = property("version").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

S9sString
S9sServer::ipAddress(
        const S9sString &defaultValue) const
{
    S9sString retval;

    if (hasProperty("ip"))
        retval = property("ip").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

S9sString
S9sServer::protocol() const
{
    return property("protocol").toString();
}

S9sString
S9sServer::status() const
{
    return property("hoststatus").toString();
}

/**
 * \returns The same as status(). 
 */
S9sString
S9sServer::hostStatus() const
{
    if (m_properties.contains("hoststatus"))
        return m_properties.at("hoststatus").toString();

    return S9sString();
}

S9sVariantList 
S9sServer::subnets() const
{
    return property("subnets").toVariantList();
}

/**
 *
 * Here is how the "regions" property looks like:
 * \code{.js}
 * "regions": [ 
 * {
 *     "has_credentials": false,
 *     "name": "ap-south-1",
 *     "provider": "aws"
 * }, 
 * . . . 
 * \endcode
 */
S9sVariantList
S9sServer::regions() const
{
    return property("regions").toVariantList();
}

int
S9sServer::nSubnets() const
{
    return (int) subnets().size();
}

S9sString
S9sServer::subnetCidr(
        const int idx) const
{
    S9sVariantList theList = subnets();
    S9sString      retval;

    if (idx < 0 || idx >= (int)theList.size())
        return retval;

    retval = theList[idx]["cidr"].toString();
    return retval;
}

S9sString
S9sServer::subnetRegion(
        const int idx) const
{
    S9sVariantList theList = subnets();
    S9sString      retval;

    if (idx < 0 || idx >= (int)theList.size())
        return retval;

    retval = theList[idx]["region"].toString();
    return retval;
}

S9sString
S9sServer::subnetProvider(
        const int idx) const
{
    S9sVariantList theList = subnets();
    S9sString      retval;

    if (idx < 0 || idx >= (int)theList.size())
        return retval;

    retval = theList[idx]["provider"].toString();
    return retval;
}

S9sString
S9sServer::subnetId(
        const int idx) const
{
    S9sVariantList theList = subnets();
    S9sString      retval;

    if (idx < 0 || idx >= (int)theList.size())
        return retval;

    retval = theList[idx]["id"].toString();
    return retval;
}

S9sString
S9sServer::subnetVpcId(
        const int idx) const
{
    S9sVariantList theList = subnets();
    S9sString      retval;

    if (idx < 0 || idx >= (int)theList.size())
        return retval;

    retval = theList[idx]["vpc_id"].toString();
    return retval;
}

S9sVariantList 
S9sServer::templates() const
{
    return property("templates").toVariantList();
}

int
S9sServer::nTemplates() const
{
    return (int) templates().size();
}

S9sString
S9sServer::templateName(
        const int idx) const
{
    S9sVariantList theList = templates();
    S9sString      retval;

    if (idx < 0 || idx >= (int)theList.size())
        return retval;

    retval = theList[idx]["name"].toString();
    return retval;
}

S9sString
S9sServer::templateRegion(
        const int idx) const
{
    S9sVariantList theList = templates();
    S9sString      retval;

    if (idx < 0 || idx >= (int)theList.size())
        return retval;

    retval = theList[idx]["region"].toString();
    return retval;
}

S9sString
S9sServer::templateProvider(
        const int idx) const
{
    S9sVariantList theList = templates();
    S9sString      retval;

    if (idx < 0 || idx >= (int)theList.size())
        return retval;

    retval = theList[idx]["provider"].toString();
    return retval;
}


const char *
S9sServer::colorBegin(
        bool    useSyntaxHighLight) const
{
    const char *retval = "";

    if (useSyntaxHighLight)
    {
        S9sString myStatus = status();

        if (myStatus == "CmonHostRecovery" ||
                myStatus == "CmonHostShutDown")
        {
            retval = XTERM_COLOR_YELLOW;
        } else if (myStatus == "CmonHostUnknown" ||
                myStatus == "CmonHostOffLine")
        {
            retval = XTERM_COLOR_RED;
        } else {
            retval = XTERM_COLOR_GREEN;
        }
    }

    return retval;
}

const char *
S9sServer::colorEnd(
        bool    useSyntaxHighLight) const
{
    return useSyntaxHighLight ? TERM_NORMAL : "";
}

/**
 * \returns How many containers this server has.
 */
int
S9sServer::nContainers() const
{
    return property("containers").size();
}

int
S9sServer::nContainersMax() const
{
    return property("max_containers").toInt();
}

S9sString
S9sServer::nContainersMaxString() const
{
    int       integerValue = nContainersMax();
    S9sString stringValue;

    if (integerValue >= 0)
        stringValue.sprintf("%d", integerValue);
    else
        stringValue = "-";

    return stringValue;
}

int
S9sServer::nRunningContainersMax() const
{
    return property("max_containers_running").toInt();
}

S9sString
S9sServer::nRunningContainersMaxString() const
{
    int       integerValue = nRunningContainersMax();
    S9sString stringValue;

    if (integerValue >= 0)
        stringValue.sprintf("%d", integerValue);
    else
        stringValue = "-";

    return stringValue;
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

/**
 * \param defaultValue The string that will be returned if the requested
 *   information is not available.
 */
S9sString
S9sServer::model(
        const S9sString &defaultValue) const
{
    S9sString retval = property("model").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 * \param defaultValue The string that will be returned if the requested
 *   information is not available.
 * \returns A string encodes the operating system name and version.
 */
S9sString
S9sServer::osVersionString(
        const S9sString &defaultValue) const
{
    S9sString retval;

    if (hasProperty("distribution"))
    {
        S9sVariantMap map = property("distribution").toVariantMap();
        S9sString     name, release, codeName;
        
        name     = map["name"].toString();
        release  = map["release"].toString();
        codeName = map["codename"].toString();

        retval.appendWord(name);
        retval.appendWord(release);
        retval.appendWord(codeName);
    }

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 * \returns A compact list enumerating the processors found in the server.
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

/**
 * \returns A compact list enumerating the NICs found in the server.
 *
 */
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

/**
 * \returns A compact list enumerating the memory banks found in the server.
 *
 */
S9sVariantList
S9sServer::memoryBankNames() const
{
    S9sVariantList  retval;
    S9sVariantMap   bankModels;
    S9sVariantMap   bankSizes;
    S9sVariantMap   memory  = property("memory").toVariantMap();
    S9sVariantList  bankList = memory["banks"].toVariantList();

    for (uint idx1 = 0; idx1 < bankList.size(); ++idx1)
    {
        S9sVariantMap bank   = bankList[idx1].toVariantMap();
        S9sString     model  = bank["name"].toString();
        ulonglong     size   = bank["size"].toULongLong();

        bankModels[model] += 1;
        bankSizes[model]   = size;
    }

    for (uint idx = 0; idx < bankModels.keys().size(); ++idx)
    {
        S9sString  name   = bankModels.keys().at(idx);
        int        volume = bankModels[name].toInt();
        ulonglong  size   = bankSizes[name].toULongLong();
        S9sString  line;

        if (size != 0ull)
        {
            line.sprintf("%2d x %d Gbyte %s", 
                    volume, size / (1024ull * 1024ull * 1024ull),
                    STR(name));
        } else {
            line.sprintf("%2d x %s", volume, STR(name));
        }


        retval << line;
    }

    return retval;
}

/**
 * \returns A compact list enumerating the disks found in the server.
 *
 */
S9sVariantList
S9sServer::diskNames() const
{
    S9sVariantList  retval;
    S9sVariantMap   diskModels;
    S9sVariantList  diskList = property("disk_devices").toVariantList();

    for (uint idx1 = 0; idx1 < diskList.size(); ++idx1)
    {
        S9sVariantMap disk   = diskList[idx1].toVariantMap();
        S9sString     model  = disk["model"].toString();
        bool          isHw   = disk["is_hardware_storage"].toBoolean();

        if (!isHw)
            continue;

        diskModels[model] += 1;
    }

    for (uint idx = 0; idx < diskModels.keys().size(); ++idx)
    {
        S9sString  name   = diskModels.keys().at(idx);
        int        volume = diskModels[name].toInt();
        S9sString  line;

        line.sprintf("%2d x %s", volume, STR(name));

        retval << line;
    }

    return retval;
}

S9sVariantList
S9sServer::containers() const
{
    S9sVariantList origList = property("containers").toVariantList();
    S9sVariantList retval;

    for (uint idx = 0u; idx < origList.size(); ++idx)
        retval << S9sContainer(origList[idx].toVariantMap());

    return retval;
}

double
S9sServer::totalMemoryGBytes() const
{
    S9sVariantMap  memory = property("memory").toVariantMap();
    S9sVariantList banks  = memory["banks"].toVariantList();
    ulonglong      sum    = 0ull;
    double         retval;// = memory["memory_total_mb"].toDouble();
    
    for (uint idx = 0u; idx < banks.size(); ++idx)
        sum += banks[idx]["size"].toULongLong();

    if (sum > 0ull)
    {
        retval = sum / (1024.0 * 1024.0 * 1024.0);
    } else {
        retval  = memory["memory_total_mb"].toDouble();
        retval /= 1024.0;
    }

    return retval;
}

int 
S9sServer::nCpus() const
{
    S9sVariantList  cpus = property("processors").toVariantList();
    return (int) cpus.size();
}

int 
S9sServer::nCores() const
{
    S9sVariantList cpus   = property("processors").toVariantList();
    int            retval = 0;

    for (uint idx = 0u; idx < cpus.size(); ++idx)
    {
        S9sVariantMap cpuMap = cpus[idx].toVariantMap();

        retval += cpuMap["cores"].toInt();
    }

    return retval;
}

int 
S9sServer::nThreads() const
{
    S9sVariantList cpus   = property("processors").toVariantList();
    int            retval = 0;

    for (uint idx = 0u; idx < cpus.size(); ++idx)
    {
        S9sVariantMap cpuMap = cpus[idx].toVariantMap();

        retval += cpuMap["siblings"].toInt();
    }

    return retval;
}

