/*
 * Severalnines Tools
 * Copyright (C) 2018  Severalnines AB
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
#include <S9sOptions>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sContainer::S9sContainer() :
    S9sObject()
{
    m_properties["class_name"] = "CmonContainer";
}

S9sContainer::S9sContainer(
        const S9sContainer &orig) :
    S9sObject(orig)
{
    m_properties = orig.m_properties;
    m_url        = orig.m_url;
}
 
S9sContainer::S9sContainer(
        const S9sVariantMap &properties) :
    S9sObject(properties)
{
    if (!m_properties.contains("class_name"))
        m_properties["class_name"] = "CmonContainer";
}

/**
 * \param stringRep The string representation of the host, either a JSon string
 *   or an url (e.g. "192.168.1.100:3306".
 */
S9sContainer::S9sContainer(
        const S9sString &stringRep) :
    S9sObject()
{
    bool success;

    S9S_WARNING("stringRep : %s", STR(stringRep));
    // Parsing as a JSon string, that's more specific.
    success = m_properties.parse(STR(stringRep));
    if (success)
    {
        S9S_WARNING("parsed as json");
        m_url = m_properties["alias"].toString();

        if (m_properties.contains("port"))
            m_url.setPort(m_properties["port"].toInt());

        m_url.setProperties(m_properties);
    } else {
        S9S_WARNING("parsing as url");
        // If not ok then parsing as an URL.
        m_url = S9sUrl(stringRep);

        m_properties = m_url.properties();
        m_properties["alias"] = m_url.hostName();

        if (m_url.hasPort())
            m_properties["port"] = m_url.port();
    }
  
    #if 0
    if (m_url.hasProtocol())
    {
        S9sString protocol = m_url.protocol().toLower();

        if (m_url.protocol() == "lxc")
            m_properties["class_name"] = "CmonContainerServer";
    }
    #endif

    if (!m_properties.contains("class_name"))
        m_properties["class_name"] = "CmonContainer";
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

S9sString 
S9sContainer::name() const
{
    S9sString retval = property("alias").toString();

    if (retval.empty())
        retval = "-";

    return retval;
}

S9sString
S9sContainer::name(
        const int columns) const
{
    S9sString retval = name();

    if (columns <= 0)
        return retval;

    if ((int)retval.length() > columns)
    {
        retval.resize(columns);
        retval += "…";
    }

    return retval;
}

S9sString 
S9sContainer::alias() const
{
    return name();
}

void
S9sContainer::setAlias(
        const S9sString &alias)
{
    setProperty("alias", alias);
}

S9sString 
S9sContainer::className() const
{
    return property("class_name").toString();
}

/**
 * \param syntaxHighlight Controls if the string will have colors or not.
 * \param formatString The formatstring with markup.
 * \returns The string representation according to the format string.
 *
 * Converts the container to a string using a special format string that may
 * contain field names of node properties.
 */
S9sString
S9sContainer::toString(
        const bool       syntaxHighlight,
        const S9sString &formatString) const
{
    S9sString      retval;
    S9sString      tmp;
    char           c;
    S9sString      partFormat;
    bool           percent      = false;
    bool           escaped      = false;
    S9sOptions    *options = S9sOptions::instance();
    //bool         modifierFree = false;

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
            //modifierFree = true;
            continue;
        } else if (c == '\\')
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
                    tmp.sprintf(STR(partFormat), 
                            STR(ipAddress(options->addressType())));
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
                    // 
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(provider()));
                    retval += tmp;
                    break;

                case 'F':
                    // The first firewall.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(firewall()));
                    retval += tmp;
                    break;

                case 'I':
                    // The ID of the node.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(id("-")));
                    retval += tmp;
                    break;
                
                case 'i':
                    // 
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(image("-")));
                    retval += tmp;
                    break;

                case 'N':
                    // The name of the container.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(alias()));

                    if (syntaxHighlight)
                        retval += XTERM_COLOR_BLUE;

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += TERM_NORMAL;

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
                
                case 'P':
                    // The name of the parent server.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(parentServerName()));
                    retval += tmp;
                    break;
                
                case 'R':
                    // 
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(region("-")));
                    retval += tmp;
                    break;

                case 'S':
                    // The state of the container.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(state()));

                    if (syntaxHighlight)
                    {
                        retval += 
                            S9sRpcReply::clusterStateColorBegin(state());
                    }

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += S9sRpcReply::clusterStateColorEnd();

                    break;
                    
                case 'T':
                    // The type of the container.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(type()));
                    retval += tmp;
                    break;
                
                case 't':
                    // 
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(templateName("-")));
                    retval += tmp;
                    break;
                
                case 'U':
                    // The type of the container.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(subnetId()));
                    retval += tmp;
                    break;
                
                case 'V':
                    // The type of the container.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(subnetVpcId()));
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
        //modifierFree = false;
    }

    return retval;
}

S9sString 
S9sContainer::hostname() const
{
    return property("hostname").toString();
}

/**
 *
        "network": 
        {
            "private_ip": [ "172.31.12.5", "ip-1.compute.internal" ],
            "public_ip": [ "54.93.99.244", "ec2.com" ]
        },
*/
S9sString 
S9sContainer::ipAddress(
        const S9s::AddressType    addressType,
        const S9sString          &defaultValue) const
{
    S9sString retval; 
   
    if (addressType == S9s::AnyIpv4Address)
    {
        retval = ipAddress(S9s::PublicIpv4Address);

        if (retval.empty())
            retval = ipAddress(S9s::PrivateIpv4Address, defaultValue);

        return retval;
    }

    if (hasProperty("network"))
    {
        S9sVariantList addressList;
        
        if (addressType == S9s::PublicIpv4Address ||
                addressType == S9s::PublicDnsName)
        {
            addressList = property("network")["public_ip"].toVariantList();
        } else {
            addressList = property("network")["private_ip"].toVariantList();
        }

        if (addressType == S9s::PublicIpv4Address ||
            addressType == S9s::PrivateIpv4Address)
        {
            if (addressList.size() > 0u)
                retval = addressList[0u].toString();
        } else {
            if (addressList.size() > 1u)
                retval = addressList[1u].toString();
        }
    } else {
        retval = property("ip").toString();
    }

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 *
 * \code{.js}
 * "network": 
 * {
 *     "private_ip": [ "172.31.5.246", "ip-172-31-5-246.eu" ],
 *     "public_ip": [ "52.59.220.19", "ec2-52-59-220-19.eu" ]
 * },
 * \endcode
 */
S9sString 
S9sContainer::ipv4Addresses(
        const S9sString &separator,
        const S9sString &defaultValue)
{
    S9sString      retval;

    if (hasProperty("network"))
    {
        // The new way.
        S9sVariantList addressList;

        addressList = property("network")["public_ip"].toVariantList();
        for (uint idx = 0u; idx < addressList.size(); ++idx)
        {
            S9sString address = addressList[idx].toString();

            if (!address.looksLikeIpAddress())
                continue;

            if (!retval.empty())
                retval += separator;

            retval += address;
        }

        addressList = property("network")["private_ip"].toVariantList();
        for (uint idx = 0u; idx < addressList.size(); ++idx)
        {
            S9sString address = addressList[idx].toString();

            if (!address.looksLikeIpAddress())
                continue;

            if (!retval.empty())
                retval += separator;

            retval += address;
        }

        if (retval.empty())
            retval = defaultValue;
#if 0
    } else {
        // This is the old way around.
        S9sVariantList theList =  property("ipv4_addresses").toVariantList();

        for (uint idx = 0u; idx < theList.size(); ++idx)
        {
            if (!retval.empty())
                retval += separator;

            retval += theList[idx].toString();
        }

        if (retval.empty())
            retval = defaultValue;
#endif
    }
        
    return retval;
}

/**
 * \returns The name of the server that holds the container.
 */
S9sString 
S9sContainer::parentServerName() const
{
    return property("parent_server").toString();
}

void
S9sContainer::setParentServerName(
        const S9sString &value)
{
    setProperty("parent_server", value);
}

/**
 * \returns "STOPPED" or "RUNNING".
 */
S9sString 
S9sContainer::state() const
{
    return property("status").toString();
}

int
S9sContainer::stateAsChar() const
{
    S9sString    theStatus = state();

    if (theStatus == "RUNNING")
        return 'u';
    else if (theStatus == "TERMINATED")
        return 't';
    else if (theStatus == "QUEUED")
        return 'q';
    else if (theStatus == "STOPPED")
        return 's';
        
    return '?';
}

/**
 * \returns True if the container is automatically started when the server is
 * started.
 */
bool
S9sContainer::autoStart() const
{
    return property("autostart").toBoolean();
}

/**
 * \code{.js}
 * "subnet": 
 * {
 *     "az": "",
 *     "cidr": "10.0.0.0/24",
 *     "class_name": "CmonSubnet",
 *     "id": "bunin-vpc - bunin-subnet",
 *     "name": "bunin-subnet",
 *     "provider": "az",
 *     "region": "southeastasia",
 *     "status": "Succeeded",
 *     "vpc_id": "bunin-vpc"
 * },
 * \endcode
 */
S9sVariantMap
S9sContainer::subNet() const
{
    return property("subnet").toVariantMap();
}

S9sString 
S9sContainer::subnetId(
        const S9sString &defaultValue) const
{
    S9sString retval;
    
    retval = subNet()["id"].toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

void
S9sContainer::setSubnetId(
        const S9sString &value)
{
    S9sVariantMap subnetMap = subNet();

    subnetMap["id"] = value;
    setProperty("subnet", subnetMap);
}

S9sString 
S9sContainer::subnetCidr(
        const S9sString &defaultValue) const
{
    S9sString retval;
    
    retval = subNet()["cidr"].toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

S9sString 
S9sContainer::subnetVpcId(
        const S9sString &defaultValue) const
{
    S9sString retval;
    
    retval = subNet()["vpc_id"].toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

void
S9sContainer::setSubnetVpcId(
        const S9sString &value)
{
    S9sVariantMap subnetMap = subNet();

    subnetMap["vpc_id"] = value;
    setProperty("subnet", subnetMap);
}


/**
 * \param truncate If this is true the template name will be truncated at the
 *   first space.
 * \returns The template name that was used to create the container.
 */
S9sString 
S9sContainer::templateName(
        const S9sString  &defaultValue,
        bool              truncate) const
{
    S9sString retval = property("template").toString();

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

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

void
S9sContainer::setTemplate(
        const S9sString &templateName)
{
    setProperty("template", templateName);
}

/**
 * \returns The provider (AKA the cloud) of the container where it can be 
 *   found.
 */
S9sString 
S9sContainer::provider(
        const S9sString &defaultValue) const
{
    S9sString retval = property("provider").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

void
S9sContainer::setProvider(
        const S9sString &providerName)
{
    setProperty("provider", providerName);
}

S9sString 
S9sContainer::image(
        const S9sString &defaultValue) const
{
    S9sString retval = property("image").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

void
S9sContainer::setImage(
        const S9sString &image)
{
    setProperty("image", image);
}

/**
 * \returns The name of the region the container can be found or the default
 *   value if the region is unknown.
 */
S9sString 
S9sContainer::region(
        const S9sString &defaultValue) const
{
    S9sString retval = property("region").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;

}

/**
 * \param value The name of the region the new container should be created.
 */
void
S9sContainer::setRegion(
        const S9sString &value)
{
    setProperty("region", value);
}

/**
 * \returns The type of the container (e.g. "lxc" or "cmon-cloud").
 */
S9sString 
S9sContainer::type() const
{
    return property("type").toString();
}

double
S9sContainer::memoryLimitGBytes() const
{
    ulonglong retval;

    retval  = property("memory_limit").toULongLong();
    retval /= (1024ull * 1024ull * 1024ull);

    return (double) retval;
}

/**
 * \returns The full path of the configuration file storing the container
 *   settings if such a file exists.
 */
S9sString 
S9sContainer::configFile() const
{
    return property("configfile").toString();
}

/**
 * \returns The full path of the root filesystem of the container if this
 *   information is available.
 */
S9sString 
S9sContainer::rootFsPath() const
{
    return property("root_fs_path").toString();
}

/**
 * \returns The ID of the first firewall or the default value if the container
 *   has no forewalls.
 */
S9sString 
S9sContainer::firewall(
        const S9sString &defaultValue) const
{
    S9sString      retval;
    S9sVariantList allFirewalls = firewalls();

    if (!allFirewalls.empty())
        retval = allFirewalls[0].toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 * \returns The list of firewalls as a string list.
 */
S9sVariantList
S9sContainer::firewalls() const
{
    return property("firewalls").toVariantList();
}

/**
 * \returns The list of firewall names fit to be printed for the user (uses not
 *   ',' and ' ' for separator.
 */
S9sString
S9sContainer::firewalls(
        const S9sString &defaultValue) const
{
    S9sString       retval;
    S9sVariantList  list = firewalls();

    for (uint idx = 0u; idx < list.size(); ++idx)
    {
        S9sString firewall = list[idx].toString();

        if (firewall.empty())
            continue;

        if (!retval.empty() && !retval.endsWith(", "))
            retval += ", ";

        retval += firewall;
    }

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 * \param value The list of firewall IDs separated by ',' or ';' to be set.
 *
 * We use this method when the user wants to create a firewall with a specified
 * firewall (AKA security group).
 */
void
S9sContainer::setFirewalls(
        const S9sString &value)
{
    setProperty("firewalls", value.split(";,"));
}

/**
 *
 * The volumes look like this:
 * \code{.js}
 * "volumes": [ 
 * {
 *     "name": "volume-1",
 *     "size": 10,
 *     "type": "hdd"
 * } ]
 * \endcode
 *
 * Note: when the controller creates a container it remembers the volumes it
 * creates for the container. Pre-existing containers may not have this
 * information.
 */
S9sVariantList 
S9sContainer::volumes() const
{
    return property("volumes").toVariantList();
}

void
S9sContainer::setVolumes(
        const S9sVariantList &volumes)
{
    setProperty("volumes", volumes);
}

/**
 *
 */
uint
S9sContainer::nVolumes() const
{
    S9sVariantList volumes = property("volumes").toVariantList();

    return volumes.size();
}

int 
S9sContainer::volumeGigaBytes(
        uint idx) const
{
    S9sVariantList volumes = property("volumes").toVariantList();
    
    if (idx < volumes.size())
        return volumes[idx]["size"].toInt();

    return 0;
}

S9sString 
S9sContainer::volumeType(
        uint idx) const
{
    S9sVariantList volumes = property("volumes").toVariantList();
    
    if (idx < volumes.size())
        return volumes[idx]["type"].toString();

    return "";
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
