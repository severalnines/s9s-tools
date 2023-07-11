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
#include "s9scontainer.h"
#include "s9sregexp.h"
#include "s9sformatter.h"
#include "s9srpcreply.h"

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

/**
 * \param syntaxHighlight Controls if the string will have colors or not.
 * \param formatString The formatstring with markup.
 * \returns The string representation according to the format string.
 *
 * Converts the node to a string using a special format string that may
 * contain field names of node properties.
 */
S9sString
S9sServer::toString(
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
    //bool         modifierFree = false;

    for (uint n = 0; n < formatString.size(); ++n)
    {
        c = formatString[n];
       
        if (c == '%' && !percent)
        {
            percent    = true;
            partFormat = "%";
            continue;
#if 0
        } else if (percent && c == 'f')
        {
            modifierFree = true;
            continue;
#endif
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
#if 0                
                case 'a':
                    // Maintenance flag.
                    partFormat += 's';
                    
                    tmp.sprintf(STR(partFormat), 
                            isMaintenanceActive() ? "M" : "-");

                    retval += tmp;
                    break;
#endif
#if 0 
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
#endif
#if 0
                case 'c':
                    // The total number of CPU cores in the cluster.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), nCpuCores().toInt());
                    retval += tmp;
                    break;
#endif
#if 0
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
#endif
#if 0                
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
#endif
#if 0                
                case 'E':
                    // The replication state.
                    partFormat += "s";
                    tmp.sprintf(STR(partFormat), STR(replicationState()));
                    retval += tmp;
                    break; 
#endif
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
#if 0
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
#endif                
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
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(id()));

                    retval += tmp;
                    break;
#if 0
                case 'i':
                    // The total number of monitored disk devices.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), nDevices().toInt());

                    retval += tmp;
                    break;
#endif
#if 0
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
#endif
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
                    // The model of the server. 
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(model("-")));
                    retval += tmp;
                    break;

#if 0
                case 'n':
                    // The total number of monitored network interfaces.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), nNics().toInt());

                    retval += tmp;
                    break;
#endif
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
#if 0                
                case 'L':
                    // The replay location.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(replayLocation()));
                    retval += tmp;
                    break;
#endif               
#if 0
                case 'l':
                    // The received location.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(receivedLocation()));
                    retval += tmp;
                    break;
#endif
#if 0
                case 'P':
                    // The Port.
                    partFormat += "d";
                    tmp.sprintf(STR(partFormat), port());
                    retval += tmp;
                    break;
#endif
#if 0
                case 'p':
                    // The PID.
                    partFormat += "d";
                    tmp.sprintf(STR(partFormat), pid());
                    retval += tmp;
                    break;
#endif
#if 0                
                case 'R':
                    // The role.
                    partFormat += "s";
                    tmp.sprintf(STR(partFormat), STR(role()));
                    retval += tmp;
                    break;
#endif
#if 0                
                case 'r':
                    // A string 'read-only' or 'read-write'.
                    partFormat += "s";
                    tmp.sprintf(STR(partFormat), 
                            readOnly() ? "read-only" : "read-write");
                    retval += tmp;
                    break;
#endif
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
#if 0                
                case 's':
                    // The list of slaves in one string.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(slavesAsString()));
                    retval += tmp;
                
                    break;
#endif
#if 1
                case 'T':
                    // The type of the server.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(type()));
                    retval += tmp;
                    break;
#endif
#if 0
                case 't':
                    // The network traffic found in the cluster.
                    partFormat += 'f';
                    tmp.sprintf(STR(partFormat), 
                            netBytesPerSecond().toMBytes());

                    retval += tmp;
                    break;
#endif
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
#if 0                
                case 'v':
                    // The container/vm ID.
                    partFormat += "s";
                    tmp.sprintf(STR(partFormat), STR(containerId("-")));
                    retval += tmp;
                    break;
#endif
#if 0
                case 'U':
                    // The number of CPUs.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), nCpus().toInt());
                    retval += tmp;
                    break;
#endif
#if 0
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
#endif
#if 0
                case 'Z':
                    // The CPU model.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(cpuModel()));
                    retval += tmp;
                    break;
#endif                
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
        //modifierFree = false;
    }

    return retval;
}


S9sString 
S9sServer::name() const
{
    return hostName();
}

S9sString 
S9sServer::type() const
{
    if (className() == "CmonLxcServer")
        return "lxc"; 
    else if (className() == "CmonCloudServer")
        return "cmon-cloud";

    return "";
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

int 
S9sServer::port() const
{
    return property("port").toInt();
}

/**
 * The CmonController uses this to report the PID of the controller.
 */
int 
S9sServer::pid() const
{
    return property("pid").toInt();
}

/**
 * CmonController uses this to report its data directory.
 */
S9sString
S9sServer::dataDir(
        const S9sString &defaultValue) const
{
    if (hasProperty("datadir"))
        return property("datadir").toString();

    return defaultValue;
}

S9sString
S9sServer::configFile(
        const S9sString &defaultValue) const
{
    if (hasProperty("configfile"))
        return property("configfile").toString();

    return defaultValue;
}

S9sString
S9sServer::logFile(
        const S9sString &defaultValue) const
{
    if (hasProperty("logfile"))
        return property("logfile").toString();

    return defaultValue;
}

S9sString
S9sServer::role(
        const S9sString &defaultValue) const
{
    if (hasProperty("role"))
        return property("role").toString();

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

/**
 * \returns The host status encoded into one character.
 */
int
S9sServer::stateAsChar() const
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
        const int idx, 
        bool      truncate) const
{
    S9sVariantList theList = templates();
    S9sString      retval;

    if (idx >= 0 && idx < (int)theList.size())
    {
        retval = theList[idx]["name"].toString();
    }


    if (truncate)
    {
        S9sString shortVersion;

        for (uint idx = 0u; idx < retval.length(); ++idx)
        {
            if (retval[idx] == ' ')
                break;

            shortVersion += retval[idx];
        }

        retval = shortVersion;
    }

    return retval;
}

/**
 * \param idx The index of the template in the list of available templates.
 * \param defaultValue The value returned if the region of the given template is
 *   not set (template available for all regions).
 */
S9sString
S9sServer::templateRegion(
        const int       idx,
        const S9sString defaultValue) const
{
    S9sVariantList theList = templates();
    S9sString      retval;

    if (idx < 0 || idx >= (int)theList.size())
        return retval;

    retval = theList[idx]["region"].toString();
    if (retval.empty())
        retval = defaultValue;

    return retval;
}

int
S9sServer::templatenVcpus(
        const int        idx) const
{
    S9sString   theName = templateName(idx);
    S9sRegExp   regexp("[^ ]* \\(([0-9]+) ?vCPUs, ([0-9]+[a-z]+)\\)");

    if (theName.empty())
        return 0;

    regexp.setIgnoreCase(true);    
    if (regexp == theName)
        return regexp[1].toInt();

    return 0;
}

S9sString
S9sServer::templateMemory(
        const int        idx,
        const S9sString  defaultValue) const
{
    S9sString   theName = templateName(idx);
    S9sRegExp   regexp("[^ ]* \\(([0-9]+) ?vCPUs, ([0-9]+[a-z]+)\\)");

    if (theName.empty())
        return defaultValue;

    regexp.setIgnoreCase(true);    
    if (regexp == theName)
    {
        if (!regexp[2].empty())
            return regexp[2];
    }

    return defaultValue;
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
S9sServer::ownerName(const S9sString defaultValue) const
{
    if (hasProperty("owner_user_name"))
        return property("owner_user_name").toString();

    return defaultValue;
}

S9sString
S9sServer::groupOwnerName(const S9sString defaultValue) const
{
    if (hasProperty("owner_group_name"))
        return property("owner_group_name").toString();

    return defaultValue;
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
        //ulonglong     sizeMb = disk["total_mb"].toULongLong();
        bool          isHw   = disk["is_hardware_storage"].toBoolean();

        if (!isHw)
            continue;

        #if 0
        // These sizes are bogus, lshw reports wrong values.
        if (sizeMb > 0ull)
        {
            model.sprintf("%s (%s)",
                    STR(model),
                    STR(S9sFormatter::mBytesToHuman(sizeMb)));
        }
        #endif

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

bool 
S9sServer::compareByName(
        const S9sServer &server1,
        const S9sServer &server2)
{
    if (server1.status() == "CmonHostOnline" &&
            server2.status() != "CmonHostOnline")
    {
        return true;
    }
    
    if (server1.status() != "CmonHostOnline" &&
            server2.status() == "CmonHostOnline")
    {
        return false;
    }

    return server1.hostName() < server2.hostName();
}


