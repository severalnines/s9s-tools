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
 * S9sTools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with S9sTools. If not, see <http://www.gnu.org/licenses/>.
 */
#include "s9scluster.h"
#include "S9sRpcReply"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sCluster::S9sCluster()
{
}

S9sCluster::S9sCluster(
        const S9sVariantMap &properties) :
    m_properties(properties)
{
}

S9sCluster::~S9sCluster()
{
}

S9sCluster &
S9sCluster::operator=(
        const S9sVariantMap &rhs)
{
    m_properties = rhs;
    
    return *this;
}

const S9sVariantMap &
S9sCluster::toVariantMap() const
{
    return m_properties;
}

/**
 * \returns the "class_name" property Cmon uses to represent the object type.
 */
S9sString
S9sCluster::className() const
{
    if (m_properties.contains("class_name"))
        return m_properties.at("class_name").toString();

    return S9sString();
}

/**
 * \returns The "cluster_name" property Cmon uses to represent the name of the
 *   cluster.
 */
S9sString
S9sCluster::name() const
{
    if (m_properties.contains("cluster_name"))
        return m_properties.at("cluster_name").toString();

    return S9sString();
}

/**
 * \returns The name of the user that owns the cluster.
 */
S9sString
S9sCluster::ownerName() const
{
    S9sString retval;

    if (m_properties.contains("owner"))
    {
        S9sVariantMap map = m_properties.at("owner").toVariantMap();
        
        if (map.contains("user_name"))
        {
            retval = map["user_name"].toString();
        } else {
            retval.sprintf("%d", map["user_id"].toInt());
        }
    }

    return retval;
}

/**
 * \returns The name of the group that owns the cluster.
 */
S9sString
S9sCluster::groupOwnerName() const
{
    S9sString retval;

    if (m_properties.contains("group_owner"))
    {
        S9sVariantMap map = m_properties.at("group_owner").toVariantMap();
        
        if (map.contains("group_name"))
        {
            retval = map["group_name"].toString();
        } else {
            retval.sprintf("%d", map["group_id"].toInt());
        }
    }

    return retval;
}

/**
 * \returns The cluster ID of the cluster.
 */
int 
S9sCluster::clusterId() const
{
    if (m_properties.contains("cluster_id"))
        return m_properties.at("cluster_id").toInt();

    return 0;
}

/**
 * \returns The type of the cluster as in "MYSQLCLUSTER", "REPLICATION", 
 *   "GALERA", "MYSQL_SINGLE", "MONGODB", "POSTGRESQL_SINGLE" or "GROUP_REPL".
 *
 */
S9sString
S9sCluster::clusterType() const
{
    if (m_properties.contains("cluster_type"))
        return m_properties.at("cluster_type").toString();

    return 0;
}

/**
 * \returns The state of the cluster as "MGMD_NO_CONTACT", "STARTED",
 *   "NOT_STARTED", "DEGRADED", "FAILURE", "SHUTTING_DOWN", "RECOVERING",
 *   "STARTING", "UNKNOWN" or "STOPPED".
 */
S9sString
S9sCluster::state() const
{
    if (m_properties.contains("state"))
        return m_properties.at("state").toString();

    return 0;
}

/**
 * \returns The configuration file for the cluster, that is the Cmon
 * configuration file where this cluster is configured.
 */
S9sString
S9sCluster::configFile() const
{
    if (m_properties.contains("configuration_file"))
        return m_properties.at("configuration_file").toString();

    return S9sString();
}

/**
 * \returns The full path of the Cmon log file for the cluster.
 */
S9sString
S9sCluster::logFile() const
{
    if (m_properties.contains("log_file"))
        return m_properties.at("log_file").toString();

    return S9sString();
}

/**
 * \returns The vendor name and the version number of the cluster software.
 */
S9sString
S9sCluster::vendorAndVersion() const
{
    S9sString retval;

    if (m_properties.contains("vendor"))
        retval = m_properties.at("vendor").toString();

    if (m_properties.contains("version"))
        retval.appendWord(m_properties.at("version").toString());

    return retval;
}

/**
 * \returns A short human readable string describing the cluster state.
 */
S9sString
S9sCluster::statusText() const
{
    if (m_properties.contains("status_text"))
        return m_properties.at("status_text").toString();

    return S9sString();
}

/**
 * \returns The host name one would get if executed the "hostname" command on
 *   the controller host.
 */
S9sString
S9sCluster::controllerName() const
{
    return sheetInfo("cmon.hostname").toString();
}

/**
 * \returns The host name one would get if executed the "domainname" command on
 *   the controller host.
 */
S9sString
S9sCluster::controllerDomainName() const
{
    return sheetInfo("cmon.domainname").toString();
}

/**
 * \returns The number of critical alarms active on the cluster.
 */
int
S9sCluster::alarmsCritical() const
{
    S9sVariantMap statMap;

    if (m_properties.contains("alarm_statistics"))
        statMap = m_properties.at("alarm_statistics").toVariantMap();

    return statMap["critical"].toInt();
}

/**
 * \returns The number of warning level alarms active on the cluster.
 */
int
S9sCluster::alarmsWarning() const
{
    S9sVariantMap statMap;

    if (m_properties.contains("alarm_statistics"))
        statMap = m_properties.at("alarm_statistics").toVariantMap();

    return statMap["warning"].toInt();
}

int
S9sCluster::jobsAborted() const
{
    S9sVariantMap stateMap = S9sCluster::jobStatistics() ;

    return stateMap["ABORTED"].toInt();
}

int
S9sCluster::jobsDefined() const
{
    S9sVariantMap stateMap = S9sCluster::jobStatistics() ;

    return stateMap["DEFINED"].toInt();
}

int
S9sCluster::jobsDequeued() const
{
    S9sVariantMap stateMap = S9sCluster::jobStatistics() ;

    return stateMap["DEQUEUED"].toInt();
}

int
S9sCluster::jobsFailed() const
{
    S9sVariantMap stateMap = S9sCluster::jobStatistics() ;

    return stateMap["FAILED"].toInt();
}

int
S9sCluster::jobsFinished() const
{
    S9sVariantMap stateMap = S9sCluster::jobStatistics() ;

    return stateMap["FINISHED"].toInt();
}

int
S9sCluster::jobsRunning() const
{
    S9sVariantMap stateMap = S9sCluster::jobStatistics() ;

    return stateMap["RUNNING"].toInt();
}


/**
 * Private function to get the job statistics map.
 */
S9sVariantMap 
S9sCluster::jobStatistics() const
{
    S9sVariantMap jobsMap;

    if (m_properties.contains("job_statistics"))
        jobsMap = m_properties.at("job_statistics").toVariantMap();

    return jobsMap["by_state"].toVariantMap();
}

/**
 * \returns The total memory size of the hosts in the cluster.
 */
S9sVariant
S9sCluster::memTotal() const
{
    S9sVariantList ids = hostIds();
    S9sVariant     retval;

    for (uint idx = 0u; idx < ids.size(); ++idx)
    {
        retval += memTotal(ids[idx].toInt());
    }

    return retval;
}

/**
 * \returns The total number of CPU cores (siblings actually) in the cluster.
 */
S9sVariant
S9sCluster::nCpuCores() const
{
    S9sVariantList ids = hostIds();
    S9sVariant     retval;

    for (uint idx = 0u; idx < ids.size(); ++idx)
    {
        retval += nCpuCores(ids[idx].toInt());
    }

    return retval;
}

S9sVariant
S9sCluster::nNics() const
{
    S9sVariantList ids = hostIds();
    S9sVariant     retval;

    for (uint idx = 0u; idx < ids.size(); ++idx)
    {
        retval += nNics(ids[idx].toInt());
    }

    return retval;
}

S9sVariant
S9sCluster::nDevices() const
{
    S9sVariantList ids = hostIds();
    S9sVariant     retval;

    for (uint idx = 0u; idx < ids.size(); ++idx)
    {
        retval += nDevices(ids[idx].toInt());
    }

    return retval;
}

S9sVariant
S9sCluster::totalDiskBytes() const
{
    S9sVariantList ids = hostIds();
    S9sVariant     retval;

    for (uint idx = 0u; idx < ids.size(); ++idx)
    {
        retval += totalDiskBytes(ids[idx].toInt());
    }

    return retval;
}

S9sVariant
S9sCluster::freeDiskBytes() const
{
    S9sVariantList ids = hostIds();
    S9sVariant     retval;

    for (uint idx = 0u; idx < ids.size(); ++idx)
    {
        retval += freeDiskBytes(ids[idx].toInt());
    }

    return retval;
}

/**
 * \returns How many hosts the cluster have including the controller.
 */
int
S9sCluster::nHosts() const
{
    return hostIds().size();
}

/**
 * \returns A list with all the host IDs of the cluster including the
 *   controller.
 */
S9sVariantList 
S9sCluster::hostIds() const
{
    S9sVariantList retval;
    S9sVariantList tmpList;

    if (m_properties.contains("hosts"))
        tmpList = m_properties.at("hosts").toVariantList();

    for (uint idx = 0u; idx < tmpList.size(); ++idx)
    {
        S9sVariantMap theMap = tmpList[idx].toVariantMap();
        int           hostId = theMap["hostId"].toInt();

        retval << S9sVariant(hostId);
    }

    return retval;
}

/**
 * \param hostId The ID of the host for which we return information.
 */
S9sString 
S9sCluster::hostName(
        const int hostId)
{
    S9sString key;

    key.sprintf("host.%d.hostname", hostId);

    return sheetInfo(key).toString();
}

S9sVariant
S9sCluster::nNics(
        const int hostId) const
{
    S9sString key;

    key.sprintf("host.%d.interfaces", hostId);

    return sheetInfo(key).size();
}

S9sVariant
S9sCluster::nDevices(
        const int hostId) const
{
    S9sString key;

    key.sprintf("host.%d.devices", hostId);

    return sheetInfo(key).size();
}

/**
 * \param hostId The ID of the host for which we return information.
 * \returns The number of cores (actually the number of siblinkgs that is cores
 *   time threads) in the given host.
 */
S9sVariant
S9sCluster::nCpuCores(
        const int hostId) const
{
    S9sString key;

    key.sprintf("host.%d.cpucores", hostId);

    return sheetInfo(key);
}

/**
 * \param hostId The ID of the host for which we return information.
 */
S9sVariant
S9sCluster::cpuUsagePercent(
        const int hostId)
{
    S9sString key;

    key.sprintf("host.%d.cpu_usage_percent", hostId);

    return sheetInfo(key);
}
        
/**
 * \param hostId The ID of the host for which we return information.
 * \returns How many total memory bytes the given host has.
 */
S9sVariant
S9sCluster::memTotal(
        const int hostId) const
{
    S9sString key;

    key.sprintf("host.%d.memtotal", hostId);

    return S9sVariant(sheetInfo(key).toULongLong() * 1024ull);
}



/**
 * \param hostId The ID of the host for which we return information.
 * \returns How many memory bytes is actually used.
 *
 * In this function we don't consider the cache and disk buffer area as used
 * memory for practical reasons.
 */
S9sVariant
S9sCluster::memUsed(
        const int hostId)
{
    S9sString key1, key2, key3, key4;
    ulonglong retval;

    key1.sprintf("host.%d.memtotal",  hostId);
    key2.sprintf("host.%d.membuffer", hostId);
    key3.sprintf("host.%d.memcached", hostId);
    key4.sprintf("host.%d.memfree",   hostId);

    retval = 
        sheetInfo(key1).toULongLong() -
        sheetInfo(key2).toULongLong() -
        sheetInfo(key3).toULongLong() -
        sheetInfo(key4).toULongLong();

    return S9sVariant(retval * 1024ull);
}

/**
 * \param hostId The ID of the host for which we return information.
 * \returns The size of the free the swap.
 *
 */
S9sVariant
S9sCluster::swapTotal(
        const int hostId)
{
    S9sString key;

    key.sprintf("host.%d.swaptotal", hostId);

    return sheetInfo(key).toULongLong();
}

/**
 * \param hostId The ID of the host for which we return information.
 * \returns The size of the free area on the swap.
 *
 */
S9sVariant
S9sCluster::swapFree(
        const int hostId)
{
    S9sString key;

    key.sprintf("host.%d.swapfree", hostId);

    return sheetInfo(key).toULongLong();
}

/**
 * \param hostId The ID of the host for which we return information.
 * \return The current download speed of the computer in bytes/sec.
 *
 */
S9sVariant
S9sCluster::rxBytesPerSecond(
        const int hostId)
{
    S9sString key;

    key.sprintf("host.%d.rx_bytes_per_second", hostId);

    return sheetInfo(key).toULongLong();
}

/**
 * \param hostId The ID of the host for which we return information.
 * \return The current upload speed of the computer in bytes/sec.
 *
 */
S9sVariant
S9sCluster::txBytesPerSecond(
        const int hostId)
{
    S9sString key;

    key.sprintf("host.%d.tx_bytes_per_second", hostId);

    return sheetInfo(key).toULongLong();
}

/**
 * \param hostId The ID of the host for which we return information.
 * \returns How many total disk bytes the memory stat collector found on the
 *   host.
 */
S9sVariant
S9sCluster::totalDiskBytes(
        const int hostId) const
{
    S9sString key;

    key.sprintf("host.%d.total_disk_bytes", hostId);

    return sheetInfo(key);
}

/**
 * \param hostId The ID of the host for which we return information.
 * \returns How many free disk bytes the memory stat collector found on the
 *   host.
 */
S9sVariant
S9sCluster::freeDiskBytes(
        const int hostId) const
{
    S9sString key;

    key.sprintf("host.%d.free_disk_bytes", hostId);

    return sheetInfo(key);
}

S9sVariant 
S9sCluster::sheetInfo(
        const S9sString &key) const
{
    S9sVariant retval;

    if (m_properties.contains("info"))
    {
        const S9sVariantMap &infoMap = m_properties.at("info").toVariantMap();
        
        if (infoMap.contains(key))
            retval = infoMap.at(key);
    }

    return retval;
}

/**
 * \param syntaxHighlight Controls if the string will have colors or not.
 * \param formatString The formatstring with markup.
 * \returns The string representation according to the format string.
 *
 * Converts the message to a string using a special format string that may
 * contain field names of message properties.
 */
S9sString
S9sCluster::toString(
        const bool       syntaxHighlight,
        const S9sString &formatString) const
{
    S9sString    retval;
    S9sString    tmp;
    char         c;
    S9sString    partFormat;
    bool         percent = false;
    bool         escaped = false;
    
    for (uint n = 0; n < formatString.size(); ++n)
    {
        c = formatString[n];
        
        if (c == '%')
        {
            percent    = true;
            partFormat = "%";
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
                case 'a':
                    // The number of active alarms on the cluster.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), 
                            alarmsCritical() + alarmsWarning());

                    retval += tmp;
                    break;

                case 'C':
                    // The configuration file for the cluster.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(configFile()));
                    
                    if (syntaxHighlight)
                        retval += S9sRpcReply::fileColorBegin(configFile());
                    
                    retval += tmp;

                    if (syntaxHighlight)
                        retval += S9sRpcReply::fileColorEnd();

                    break;
                
                case 'D':
                    // The controller domain name for the cluster.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(controllerDomainName()));
                    
                    retval += tmp;

                    break;
                
                case 'd':
                    // The total disk size found in the cluster.
                    partFormat += 'f';
                    tmp.sprintf(STR(partFormat), totalDiskBytes().toTBytes());

                    retval += tmp;
                    break;

                case 'G':
                    // The name of the group owner.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(groupOwnerName()));

                    if (syntaxHighlight)
                    {
                        retval += S9sRpcReply::groupColorBegin(
                                groupOwnerName());
                    }

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += S9sRpcReply::groupColorEnd();

                    break;

                case 'H':
                    // The controller host name for the cluster.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(controllerName()));
                    
                    retval += tmp;

                    break;
                
                case 'h':
                    // The number of the hosts in the cluster.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), nHosts());

                    retval += tmp;
                    break;

                case 'I':
                    // The ID of the cluster.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), clusterId());

                    retval += tmp;
                    break;
                
                case 'i':
                    // The total number of monitored disk devices.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), nDevices().toInt());

                    retval += tmp;
                    break;
                
                case 'L':
                    // The log file for the cluster.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(logFile()));
                    
                    if (syntaxHighlight)
                        retval += S9sRpcReply::fileColorBegin(logFile());
                    
                    retval += tmp;

                    if (syntaxHighlight)
                        retval += S9sRpcReply::fileColorEnd();

                    break;

                case 'M':
                    // The ID of the cluster.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(statusText()));

                    retval += tmp;
                    break;
                
                case 'm':
                    // The total memory size found in the cluster.
                    partFormat += 'f';
                    tmp.sprintf(STR(partFormat), memTotal().toGBytes());

                    retval += tmp;
                    break;

                case 'N':
                    // The name of the cluster.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(name()));

                    if (syntaxHighlight)
                        retval += XTERM_COLOR_BLUE;

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += TERM_NORMAL;

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

                case 'S':
                    // The state of the cluster.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(state()));

                    if (syntaxHighlight)
                        retval += S9sRpcReply::clusterStateColorBegin(state());

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += S9sRpcReply::clusterStateColorEnd();

                    break;
                
                case 'T':
                    // The type of the cluster.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(clusterType()));
                    retval += tmp;
                    break;

                case 'V':
                    // The vendor and version of the node.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(vendorAndVersion()));
                    retval += tmp;

                    break;
                
                case 'u':
                    // The total number of CPU cores in the cluster.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), nCpuCores().toInt());
                    retval += tmp;

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
                    partFormat += c;
                    continue;
            }
        } else {
            retval += c;
        }

        percent = false;
        escaped    = false;
    }

    return retval;
}


