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
 * \returns the "cluster_name" property Cmon uses to represent the name of the
 *   cluster.
 */
S9sString
S9sCluster::name() const
{
    if (m_properties.contains("cluster_name"))
        return m_properties.at("cluster_name").toString();

    return S9sString();
}

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

int 
S9sCluster::clusterId() const
{
    if (m_properties.contains("cluster_id"))
        return m_properties.at("cluster_id").toInt();

    return 0;
}

S9sString
S9sCluster::clusterType() const
{
    if (m_properties.contains("cluster_type"))
        return m_properties.at("cluster_type").toString();

    return 0;
}

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

S9sString
S9sCluster::logFile() const
{
    if (m_properties.contains("log_file"))
        return m_properties.at("log_file").toString();

    return S9sString();
}

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

S9sString
S9sCluster::statusText() const
{
    if (m_properties.contains("status_text"))
        return m_properties.at("status_text").toString();

    return 0;
}

int
S9sCluster::alarmsCritical() const
{
    S9sVariantMap statMap;

    if (m_properties.contains("alarm_statistics"))
        statMap = m_properties.at("alarm_statistics").toVariantMap();

    return statMap["critical"].toInt();
}

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
