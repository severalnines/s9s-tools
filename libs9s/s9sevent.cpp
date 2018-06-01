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

    if (className() == "S9sObject")
        return retval;

    #if 0
    if (senderFile().empty())
    {
        ::printf("\n\n%s\n", STR(m_properties.toString()));
        exit(1);
    }
    #endif

    retval.sprintf("%s%28s%s:%-5d ",
            XTERM_COLOR_BLUE, STR(senderFile()), TERM_NORMAL,
            senderLine());

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
            retval += "eventAlarmToOneLiner()";
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
            retval += "eventClusterToOneLiner()";
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
#if 0
    S9sString retval;

    retval = m_properties.toString();
    return retval;
#else
    S9sString eventName;
    S9sString hostName;
    S9sString retval;

    eventName = m_properties.valueByPath("event_name").toString();
    hostName = m_properties.valueByPath(
            "event_specifics/host/hostname").toString();

    retval.sprintf("Host %s %s", STR(hostName), STR(eventName));
    return retval;
#endif
}

S9sString
S9sEvent::eventJobToOneLiner() const
{
#if 1
    S9sString retval;

    retval = m_properties.toString();
    return retval;
#else
    S9sString eventName;
    S9sString hostName;
    S9sString retval;

    eventName = m_properties.valueByPath("event_name").toString();
    hostName = m_properties.valueByPath(
            "event_specifics/host/hostname").toString();

    retval.sprintf("Host %s %s", STR(hostName), STR(eventName));
    return retval;
#endif
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
S9sEvent::senderFile() const
{
    return m_properties.valueByPath("event_origins/sender_file").toString();
}

int
S9sEvent::senderLine() const
{
    return m_properties.valueByPath("event_origins/sender_line").toInt();
}

