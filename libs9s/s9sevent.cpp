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
#include "s9sevent.h"

#include "s9soptions.h"
#include "s9sjob.h"
#include "s9snode.h"
#include "s9sserver.h"
#include "s9scluster.h"
#include "s9sdatetime.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sEvent::S9sEvent() :
    S9sObject()
{
    m_properties["class_name"] = "CmonContainer";
}

S9sEvent::S9sEvent(
        const S9sVariantMap &properties) :
    S9sObject(properties)
{
}

S9sEvent::~S9sEvent()
{
}

S9sString
S9sEvent::toString() const
{
    return toVariantMap().toString();
}

S9sString 
S9sEvent::toOneLiner(
        bool useSyntaxHighlight,
        bool isDebugMode) const
{
    S9sString   retval;
    S9sString   eventClass;
    S9sString   eventName;
    S9sString   tmp;

    if (className() == "S9sObject")
        return retval;

    eventClass = property("event_class").toString();
    eventName  = property("event_name").toString();

    // The source file and source line.
    if (isDebugMode)
    {
        if (useSyntaxHighlight)
        {
            tmp.sprintf("%s%28s%s:%-5d ",
                    XTERM_COLOR_BLUE, STR(senderFile()), TERM_NORMAL,
                    senderLine());
        } else {
            tmp.sprintf("%28s:%-5d ",
                    STR(senderFile()), senderLine());
        }
        retval += tmp;
    }

    if (useSyntaxHighlight)
    {
        tmp.sprintf("%s%12s%s %s%-13s%s ",
                XTERM_COLOR_CLASS, STR(eventClass), TERM_NORMAL,
                XTERM_COLOR_SUBCLASS, STR(eventName), TERM_NORMAL
                );
    } else {
        tmp.sprintf("%12s %-13s ", STR(eventClass), STR(eventName));
    }

    retval += tmp;

    switch (eventType())
    {
        case EventStart:
            retval += "EventStart";
            break;
        
        case EventExit:
            retval += "EventExit";
            break;

        case EventHost:
            retval += eventHostToOneLiner(useSyntaxHighlight);
            break;

        case EventAlarm:
            retval += eventAlarmToOneLiner();
            break;

        case EventJob:
            retval += eventJobToOneLiner();
            break;
        
        case EventDebug:
            retval += "eventDebugToOneLiner()";
            break;

        case EventFile:
            retval += "eventFileToOneLiner()";
            break;

        case EventMaintenance:
            retval += eventMaintenanceToOneLiner();
            break;
        
        case EventCluster:
            retval += eventClusterToOneLiner();
            break;

        case EventLog:
            retval += eventLogToOneLiner();
            break;

        default:
            retval = "m_properties.toString()";
            break;
    }

    return retval;
}

/**
 * \returns One of the NoEvent, EventExit, EventStart, EventCluster, EventJob,
 *   EventHost, EventMaintenance, EventAlarm, EventFile, EventDebug, EventLog
 *   strings.
 */
S9sString
S9sEvent::eventTypeString() const
{
    return property("event_class").toString();
}

S9sEvent::EventType 
S9sEvent::eventType() const
{
    return stringToEventType(property("event_class").toString());
}

S9sString
S9sEvent::eventName() const
{
    return property("event_name").toString();
}

S9sEvent::EventSubClass
S9sEvent::eventSubClass() const
{
    return stringToEventSubClass(property("event_name").toString());
}

S9sDateTime
S9sEvent::created() const
{
    S9sVariantMap theMap = property("event_origins").toVariantMap();
    S9sDateTime   retval;

    retval.setFromVariantMap(theMap);
    return retval;
}

S9sEvent::EventType
S9sEvent::stringToEventType(
        const S9sString &eventTypeString)
{
    if (eventTypeString == "NoEvent")
        return NoEvent;
    else if (eventTypeString == "EventExit")
        return EventExit;
    else if (eventTypeString == "EventStart")
        return EventStart;
    else if (eventTypeString == "EventCluster")
        return EventCluster;
    else if (eventTypeString == "EventJob")
        return EventJob;
    else if (eventTypeString == "EventHost")
        return EventHost;
    else if (eventTypeString == "EventMaintenance")
        return EventMaintenance;
    else if (eventTypeString == "EventAlarm")
        return EventAlarm;
    else if (eventTypeString == "EventFile")
        return EventFile;
    else if (eventTypeString == "EventDebug")
        return EventDebug;
    else if (eventTypeString == "EventLog")
        return EventLog;
        
    return NoEvent;
}

S9sEvent::EventSubClass
S9sEvent::stringToEventSubClass(
        const S9sString &subClassString)
{
    if (subClassString == "NoSubClass")
        return NoSubClass;
    else if (subClassString == "Created")
        return Created;
    else if (subClassString == "Destroyed")
        return Destroyed;
    else if (subClassString == "Changed")
        return Changed;
    else if (subClassString == "Started")
        return Started;
    else if (subClassString == "Ended")
        return Ended;
    else if (subClassString == "StateChanged")
        return StateChanged;
    else if (subClassString == "UserMessage")
        return UserMessage;
    else if (subClassString == "LogMessage")
        return LogMessage;
    else if (subClassString == "Measurements")
        return Measurements;
        
    return NoSubClass;
}


S9sString
S9sEvent::eventHostToOneLiner(
        bool useSyntaxHighlight) const
{
    EventSubClass subClass = eventSubClass();
    S9sString     hostName, statusName, reason;
    S9sString     retval;
    S9sVariantMap tmpMap;
    S9sString     name, value;
    const char   *nodeColorStart = "";
    const char   *nodeColorEnd   = "";

    if (useSyntaxHighlight)
    {
        nodeColorStart = XTERM_COLOR_NODE;
        nodeColorEnd   = TERM_NORMAL;
    }

    switch (subClass)
    {
        case Created:
            retval.sprintf(
                    "Host %s%s%s created.", 
                    nodeColorStart, 
                    STR(host().hostName()), 
                    nodeColorEnd);
            break;

        case StateChanged:
            hostName   = getString("event_specifics/host_name");
            statusName = getString("event_specifics/hoststatus");
            reason     = getString("event_specifics/reason");
            retval.sprintf(
                    "Host %s%s%s state: %s reason: %s", 
                    nodeColorStart, STR(hostName), nodeColorEnd,
                    STR(statusName), STR(reason));
            break;

        case Changed:
            name  = getString("event_specifics/property_name");
            value = getString("event_specifics/property_value");
            if (!name.empty())
            {
                retval.sprintf(
                        "Host %s%s%s updated: %s = %s", 
                        nodeColorStart, 
                        STR(host().hostName()), 
                        nodeColorEnd,
                        STR(name), STR(value));
            } else {
                retval.sprintf(
                        "Host %s%s%s updated: %s", 
                        nodeColorStart, 
                        STR(host().hostName()), 
                        nodeColorEnd,
                        STR(host().message()));
            }
            break;

        case Measurements:
            #if 1
            hostName = getString("event_specifics/host_name");
            tmpMap   = m_properties.valueByPath(
                    "event_specifics").toVariantMap();

            retval.sprintf(
                    "Host %s%s%s %s", 
                    nodeColorStart, STR(hostName), nodeColorEnd,
                    STR(measurementToOneLiner(tmpMap, useSyntaxHighlight)));
            #else
            retval = m_properties.toString();
            #endif
            break;

        case NoSubClass:
            retval.sprintf(
                    "Host %s%s%s ping.", 
                    nodeColorStart, STR(host().hostName()), nodeColorEnd);
            break;

        default:
            retval = "Unknown host event";
    }

    //retval.sprintf("Host %s", STR(hostName));
    return retval;
}

/*
    "class_name": "CmonEvent",
    "event_class": "EventJob",
    "event_name": "UserMessage",
    "event_origins": 
    {
        "sender_file": "Communication.cpp",
        "sender_line": 5208,
        "tv_nsec": 891800276,
        "tv_sec": 1527842479
    },
    "event_specifics": 
    {
        "message": 
        {
            "class_name": "CmonJobMessage",
            "created": "2018-06-01T08:41:19.000Z",
            "file_name": "Communication.cpp",
            "job_id": 1,
            "line_number": 5208,
            "message_id": 120,
            "message_status": "JOB_SUCCESS",
            "message_text": "<em style='color: #c0392b;'>192.168.0.140</em>: Installing <strong style='color: #110679;'>augeas-tools</strong>."
        }
    }
}
*/
S9sString
S9sEvent::eventJobToOneLiner() const
{
    EventSubClass subClass = eventSubClass();
    S9sJob        job; 
    S9sString     message;
    S9sString     eventName;
    S9sString     hostName;
    S9sString     retval;
    int           intTmp;

    job      = m_properties.valueByPath("event_specifics/job").toVariantMap();
    message  = getString("event_specifics/message/message_text");
    message  = S9sString::html2ansi(message);

    hostName = getString("event_specifics/host/hostname");

    switch (subClass)
    {
        case Created:
            retval.sprintf("%4d %s", job.jobId(), STR(job.title()));
            break;

        case Changed:
            retval.sprintf("%4d %s", job.jobId(), STR(job.title()));
            break;

        case UserMessage:
            intTmp = getInt("event_specifics/message/job_id");
            if (hostName.empty())
            {
                retval.sprintf("%4d %s", intTmp, STR(message));
            } else {
                retval.sprintf("%4d Host %s %s", 
                        intTmp,
                        STR(hostName), STR(message));
            }
            break;
    
        default:
            retval = "";
    }

    return retval;
}

S9sString
S9sEvent::eventLogToOneLiner() const
{
    S9sVariantMap logEntryMap = 
        m_properties.valueByPath("event_specifics/log_entry").toVariantMap();

    logEntryMap = m_properties.valueByPath("event_specifics/log_entry").toVariantMap();
    #if 0
    return m_properties.toString();
    #else
    S9sString retval;
    S9sVariantMap specifics;

    if (logEntryMap.contains("log_specifics"))
        specifics = logEntryMap.at("log_specifics").toVariantMap();

    if (specifics.contains("message_text"))
    {
        S9sString message = specifics.at("message_text").toString();

        retval.sprintf("%s", STR(message));
    } else {
        S9sVariant variant = logEntryMap;

        retval = variant.toString();
    }

    return retval;
    #endif
}

S9sString
S9sEvent::eventAlarmToOneLiner() const
{
    S9sString retval;

#if 0
    retval = m_properties.toString();
#else
    EventSubClass subClass = eventSubClass();
    S9sString message;

    message = getString("event_specifics/alarm/message");

    switch (subClass)
    {
        case Changed:
        case Ended:
        case Started:
        case Created:
            retval.sprintf("%s", STR(message));
            break;

        default:
            retval = m_properties.toString();
    }

#endif
    return retval;
}

/**
 * \code{.js}
 * {
 *     "class_name": "CmonEvent",
 *     "event_class": "EventMaintenance",
 *     "event_name": "Ended",
 *     "event_origins": 
 *     {
 *         "sender_file": "cmonhostmanager.cpp",
 *         "sender_line": 1629,
 *         "tv_nsec": 316022057,
 *         "tv_sec": 1532675444
 *     },
 *     "event_specifics": 
 *     {
 *         "host_name": "192.168.0.105",
 *         "maintenance": 
 *         {
 *             "class_name": "CmonMaintenanceInfo",
 *             "hostname": "192.168.0.105",
 *             "is_active": false,
 *             "maintenance_periods": [ 
 *             {
 *                 "UUID": "038d9695-6f93-4e4a-8a6a-4e8f2e573cda",
 *                 "deadline": "2018-07-27T07:10:43.000Z",
 *                 "groupid": 1,
 *                 "groupname": "admins",
 *                 "initiate": "2018-07-27T07:07:43.683Z",
 *                 "is_active": false,
 *                 "reason": "Rolling restart job starts.",
 *                 "userid": 1,
 *                 "username": "system"
 *             } ]
 *         }
 *     }
 * }
 * \endcode
 */

S9sString
S9sEvent::eventMaintenanceToOneLiner() const
{
    EventSubClass  subClass = eventSubClass();    
    S9sString      reason;
    S9sVariantList periods;
    S9sString      retval;
    bool           isCluster;

    isCluster = 
        getString("event_specifics/maintenance/cluster/class_name") == 
        "CmonClusterInfo";
    if (isCluster)
    {
        retval = 
            "Cluster " + 
            getString("event_specifics/maintenance/cluster/cluster_name");
    } else {
        retval = 
            "Host " +
            getString("event_specifics/maintenance/hostname");
    }
    
    switch (subClass)
    {
        case Ended:
            retval += " maintenance ended";
            break;

        case Started:
            retval += " maintenance started";
            break;

        case Created:
            retval += " maintenance created";
            break;

        default:
            break;
    }

    // Printing the reason only makes sense when the maintenance created or
    // started, at end it looks weird to print the reson.
    if (subClass != Ended)
    {
        periods = m_properties.valueByPath(
            "event_specifics/maintenance/maintenance_periods").toVariantList();

        for (uint idx = 0u; idx < periods.size(); ++idx)
        {
            S9sVariantMap period = periods[idx].toVariantMap();

            if (reason.empty() || period["is_active"].toBoolean())
                reason = period["reason"].toString();
        }
    }

    if (!reason.empty())
        retval += ": " + reason;
    else
        retval += ".";

    return retval;
}

S9sString
S9sEvent::eventClusterToOneLiner() const
{
#if 0
    S9sString retval;

    retval = m_properties.toString();
    return retval;
#else
    EventSubClass subClass = eventSubClass();
    int           clusterId;
    S9sString     stateName;
    S9sString     reason;
    S9sString     retval;

    clusterId = getInt("event_specifics/cluster_id");
    stateName = getString("event_specifics/cluster_state");
    reason    = getString("event_specifics/reason");

    switch (subClass)
    {
        case StateChanged:
            retval.sprintf(
                    "Cluster %d state %s: %s",
                    clusterId, STR(stateName), STR(reason));
            break;
        
        case NoSubClass:
            retval.sprintf("Cluster %d ping.", clusterId);
            break;

        default:
            retval = m_properties.toString();
            break;
    }

    return retval;
#endif
}

S9sString 
S9sEvent::measurementToOneLiner(
        S9sVariantMap specifics,
        bool          useSyntaxHighlight) const
{
    S9sVariantMap measurements;
    S9sString     className;
    S9sString     retval;

    if (specifics["measurements"].isVariantList())
    {
        S9sVariantList sampleList = specifics["measurements"].toVariantList();

        for (uint idx = 0; idx < sampleList.size(); ++idx)
        {
            S9sVariantMap sample = sampleList[idx].toVariantMap();

            if (sample["class_name"].toString() == "CmonDiskInfo")
            {
                if (!retval.empty())
                    retval += "; ";

                retval += cmonDiskInfoToOneLiner(sample, useSyntaxHighlight);
            } else {
                retval += sample.toString();
                return retval;
            }
        }

        return retval;
    } else {
        measurements = specifics["measurements"].toVariantMap();
        className    = measurements["class_name"].toString();
    }

    if (className == "CmonMemoryStats")
    {
        ulonglong ramfree = measurements["ramfree"].toULongLong();
        double    utilization =  measurements["memoryutilization"].toDouble();

        retval.sprintf(
                "Memory %lluGB (%.2f%%) free",
                ramfree / (1024ull * 1024ull * 1024ull), 
                (1.0 - utilization) * 100.0);
    } else {
        retval = specifics.toString();
    }

    return retval;
}

/**
 *
 * \code{.js}
 * {
 *      "capacity": 0,
 *      "class_name": "CmonDiskInfo",
 *      "device": "/dev/sdb",
 *      "model": "",
 *      "model-family": "",
 *      "partitions": [ 
 *      {
 *          "blocksize": 4096,
 *          "class_name": "CmonDiskStats",
 *          "created": 1532589216,
 *          "device": "/dev/sdb2",
 *          "filesystem": "ext4",
 *          "free": 306192207872,
 *          "hostid": 1,
 *          "interval": 0,
 *          "mount_option": "rw,relatime,errors=remount-ro,data=ordered",
 *          "mountpoint": "/",
 *          "reads": 0,
 *          "readspersec": 0,
 *          "sampleends": 1532589216,
 *          "samplekey": "CmonDiskStats-1-/dev/sdb2",
 *          "sectorsread": 0,
 *          "sectorswritten": 0,
 *          "total": 572729753600,
 *          "utilization": 0,
 *          "writes": 0,
 *          "writespersec": 0
 *      } ],
 *      "power-cycle-count": 0,
 *      "power-on-hours": 0,
 *      "reallocated-sector-counter": 0,
 *      "self-health-assessment": "",
 *      "serial-number": "",
 *      "temperature-celsius": 0
 * }
 * \endcode
 */
S9sString
S9sEvent::cmonDiskInfoToOneLiner(
        S9sVariantMap sample,
        bool          useSyntaxHighlight) const
{
    S9sVariantList partitions = sample["partitions"].toVariantList();
    S9sString      retval;

    for (uint idx = 0u; idx < partitions.size(); ++idx)
    {
        S9sVariantMap partition  = partitions[idx].toVariantMap();
        S9sString     mountPoint = partition["mountpoint"].toString();
        ulonglong     freeBytes  = partition["free"].toULongLong();
        ulonglong     totalBytes = partition["total"].toULongLong();
        S9sString     tmp;

        if (!retval.empty())
            retval += ", ";

        if (useSyntaxHighlight)
        {
            tmp.sprintf("%s%s%s: %s (%s) free", 
                    m_formatter.directoryColorBegin(),
                    STR(mountPoint), 
                    m_formatter.directoryColorEnd(),
                    STR(m_formatter.bytesToHuman(freeBytes)),
                    STR(m_formatter.percent(totalBytes, freeBytes)));
        } else {
            tmp.sprintf("%s: %s (%s) free", 
                    STR(mountPoint), 
                    STR(m_formatter.bytesToHuman(freeBytes)),
                    STR(m_formatter.percent(totalBytes, freeBytes)));
        }
                
        retval+= tmp;
    }

    return retval;
}

S9sString
S9sEvent::senderFile() const
{
    return getString("event_origins/sender_file");
}

int
S9sEvent::senderLine() const
{
    return m_properties.valueByPath("event_origins/sender_line").toInt();
}


S9sString 
S9sEvent::getString(
        const S9sString &path) const
{
    return m_properties.valueByPath(path).toString();
}

int
S9sEvent::getInt(
        const S9sString &path) const
{
    return m_properties.valueByPath(path).toInt();
}

int
S9sEvent::clusterId() const
{
    return getInt("event_specifics/cluster_id");
}

bool
S9sEvent::hasHost() const
{
    S9sString className;
    if (!m_properties.valueByPath("/event_specifics/host").isVariantMap())
        return false;

    className = m_properties.valueByPath("/event_specifics/host/class_name").
        toString();

    // We handle these using a different class.
    if (className == "CmonLxcServer")
        return false;
    else if (className == "CmonCloudServer")
        return false;
    else if (className == "CmonContainerServer")
        return false;

    return true;
}

S9sNode
S9sEvent::host() const
{
    S9sNode host =
        m_properties.valueByPath("/event_specifics/host").toVariantMap();

    return host;
}

bool
S9sEvent::hasServer() const
{
    S9sString className;
    if (!m_properties.valueByPath("/event_specifics/host").isVariantMap())
        return false;

    className = m_properties.valueByPath("/event_specifics/host/class_name").
        toString();

    // We handle these using a different class.
    if (className == "CmonLxcServer")
        return true;
    else if (className == "CmonCloudServer")
        return true;
    else if (className == "CmonContainerServer")
        return true;

    return false;
}

S9sServer
S9sEvent::server() const
{
    S9sServer server =
        m_properties.valueByPath("/event_specifics/host").toVariantMap();

    return server;
}

bool
S9sEvent::hasCluster() const
{
    return m_properties.valueByPath("/event_specifics/cluster").isVariantMap();
}

S9sCluster
S9sEvent::cluster() const
{
    S9sCluster cluster =
        m_properties.valueByPath("/event_specifics/cluster").toVariantMap();

    return cluster;
}

bool
S9sEvent::hasJob() const
{
    return m_properties.valueByPath("/event_specifics/job").isVariantMap();
}

S9sJob
S9sEvent::job() const
{
    S9sJob job =
        m_properties.valueByPath("/event_specifics/job").toVariantMap();

    return job;
}
