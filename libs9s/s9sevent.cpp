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

//#define DEBUG
#define WARNING
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
S9sEvent::toOneLiner() const
{
    S9sString retval;
    S9sString eventClass;
    S9sString eventName;

    if (className() == "S9sObject")
        return retval;

    #if 0
    if (senderFile().empty())
    {
        ::printf("\n\n%s\n", STR(m_properties.toString()));
        exit(1);
    }
    #endif

    eventClass = property("event_class").toString();
    eventName  = property("event_name").toString();

    retval.sprintf("%s%28s%s:%-5d %s%12s%s %s%-13s%s ",
            XTERM_COLOR_BLUE, STR(senderFile()), TERM_NORMAL,
            senderLine(),
            XTERM_COLOR_CLASS, STR(eventClass), TERM_NORMAL,
            XTERM_COLOR_SUBCLASS, STR(eventName), TERM_NORMAL
            );

    switch (eventType())
    {
        case EventStart:
            retval += "EventStart";
            break;
        
        case EventExit:
            retval += "EventExit";
            break;

        case EventHost:
            retval += eventHostToOneLiner();
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
            retval += "eventMaintenanceToOneLiner()";
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

S9sEvent::EventSubClass
S9sEvent::eventSubClass() const
{
    return stringToEventSubClass(property("event_name").toString());
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
S9sEvent::eventHostToOneLiner() const
{
    EventSubClass subClass = eventSubClass();
    S9sString     hostName, statusName, reason;
    S9sString     retval;
    S9sVariantMap tmpMap;

    hostName = getString("event_specifics/host/hostname");

    switch (subClass)
    {
        case Created:
            retval.sprintf(
                    "Host %s%s%s created.", 
                    XTERM_COLOR_NODE, STR(hostName), TERM_NORMAL);
            break;

        case StateChanged:
            hostName   = getString("event_specifics/host_name");
            statusName = getString("event_specifics/hoststatus");
            reason     = getString("event_specifics/reason");
            retval.sprintf(
                    "Host %s%s%s state: %s reason: %s", 
                    XTERM_COLOR_NODE, STR(hostName), TERM_NORMAL,
                    STR(statusName), STR(reason));
            break;

        case Changed:
            retval.sprintf(
                    "Host %s%s%s updated.", 
                    XTERM_COLOR_NODE, STR(hostName), TERM_NORMAL);
            break;

        case Measurements:
            #if 1
            hostName = getString("event_specifics/host_name");
            tmpMap   = m_properties.valueByPath(
                    "event_specifics").toVariantMap();

            retval.sprintf(
                    "Host %s%s%s %s", 
                    XTERM_COLOR_NODE, STR(hostName), TERM_NORMAL,
                    STR(measurementToOneLiner(tmpMap)));
            #else
            retval = m_properties.toString();
            #endif
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
    S9sString     message;
    S9sString     eventName;
    S9sString     hostName;
    S9sString     title;
    S9sString     retval;
    int           jobId;

    message  = getString("event_specifics/message/message_text");
    message  = S9sString::html2ansi(message);

    hostName = getString("event_specifics/host/hostname");
    title    = getString("event_specifics/job/title");
    jobId    = getInt("event_specifics/job/job_id");

    switch (subClass)
    {
        case Created:
            retval.sprintf("%4d %s", jobId, STR(title));
            //retval = m_properties.toString();
            break;

        case Changed:
            //retval = m_properties.toString();
            retval.sprintf("%4d %s", jobId, STR(title));
            break;

        case UserMessage:
            jobId = getInt("event_specifics/message/job_id");
            if (hostName.empty())
            {
                retval.sprintf("%4d %s", jobId, STR(message));
            } else {
                retval.sprintf("%4d Host %s %s", 
                        jobId,
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

        default:
            retval = m_properties.toString();
            break;
    }

    return retval;
#endif
}

S9sString 
S9sEvent::measurementToOneLiner(
        S9sVariantMap specifics) const
{
    S9sVariantMap measurements;
    S9sString     className;
    S9sString     retval;

    if (specifics["measurements"].isVariantList())
    {
        S9sVariantList sampleList = specifics["measurements"].toVariantList();
        int            nDisks     = 0;

        for (uint idx = 0; idx < sampleList.size(); ++idx)
        {
            S9sVariantMap sample = sampleList[idx].toVariantMap();

            if (sample["class_name"].toString() == "CmonDiskInfo")
            {
                nDisks += 1;
                
                if (!retval.empty())
                    retval += "; ";

                retval += cmonDiskInfoToOneLiner(sample);
            } else {
                retval += sample.toString();
                return retval;
            }
        }

        //retval.sprintf("%d disk(s)", nDisks);
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
                "Memory %lluGB free, %.2f%% used.",
                ramfree / (1024ull * 1024ull * 1024ull), 
                utilization * 100.0);
    } else {
        retval = specifics.toString();
    }

    return retval;
}

/**
 *
       {
            "capacity": 0,
            "class_name": "CmonDiskInfo",
            "device": "/dev/sdb",
            "model": "",
            "model-family": "",
            "partitions": [ 
            {
                "blocksize": 4096,
                "class_name": "CmonDiskStats",
                "created": 1532589216,
                "device": "/dev/sdb2",
                "filesystem": "ext4",
                "free": 306192207872,
                "hostid": 1,
                "interval": 0,
                "mount_option": "rw,relatime,errors=remount-ro,data=ordered",
                "mountpoint": "/",
                "reads": 0,
                "readspersec": 0,
                "sampleends": 1532589216,
                "samplekey": "CmonDiskStats-1-/dev/sdb2",
                "sectorsread": 0,
                "sectorswritten": 0,
                "total": 572729753600,
                "utilization": 0,
                "writes": 0,
                "writespersec": 0
            } ],
            "power-cycle-count": 0,
            "power-on-hours": 0,
            "reallocated-sector-counter": 0,
            "self-health-assessment": "",
            "serial-number": "",
            "temperature-celsius": 0
        }
 */
S9sString
S9sEvent::cmonDiskInfoToOneLiner(
        S9sVariantMap sample) const
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

        tmp.sprintf("%s: %s/%s", 
                STR(mountPoint), 
                STR(m_formatter.bytesToHuman(totalBytes - freeBytes)),
                STR(m_formatter.bytesToHuman(totalBytes)));
                
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

