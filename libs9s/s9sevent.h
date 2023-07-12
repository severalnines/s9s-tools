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
#pragma once

#include "s9sobject.h"
#include "s9sformatter.h"

class S9sNode;
class S9sServer;
class S9sCluster;
class S9sJob;
class S9sDateTime;

/**
 * A class that represents an event sent by the controller. 
 */
class S9sEvent : public S9sObject
{
    public:
                
        enum EventType 
        {
            /** For keepalive kind of events. */
            NoEvent,
            /** Shows when the event loop ended, controller exits. */
            EventExit,
            /** Shows when the event loop starts, controller starts. */
            EventStart,
            /** Cluster related events. */
            EventCluster,
            /** Job related events. */
            EventJob,
            /** Host related events. */
            EventHost,
            /** Maintenance related events. */
            EventMaintenance,
            /** Events related to alarms. */
            EventAlarm,
            /** Events related to files. */
            EventFile,
            /** Events related to debug messages.*/
            EventDebug,
            /** Events created by the log subsystem. */
            EventLog,
        };

        enum EventSubClass
        {
            /** For simple events. */
            NoSubClass,
            /** Something is created. */
            Created,
            /** Something is destroyed. */
            Destroyed,
            /** Something is changed. */
            Changed, 
            /** Something is started. */
            Started,
            /** Something is closed or ended. */
            Ended,
            /** State is changed. */
            StateChanged,
            /** The user got a new message or warning. */
            UserMessage,
            /** A new log entry was created. */
            LogMessage,
            /** The measurements that are coming from data collectors.*/
            Measurements,
        };

        S9sEvent();
        S9sEvent(const S9sVariantMap &properties);

        virtual ~S9sEvent();

        S9sString eventTypeString() const;
        S9sEvent::EventType eventType() const;
        S9sString eventName() const;
        S9sEvent::EventSubClass eventSubClass() const;
        S9sDateTime created() const;

        S9sString 
            cmonDiskInfoToOneLiner(
                S9sVariantMap sample,
                bool          useSyntaxHighlight) const;

        S9sString senderFile() const;
        int senderLine() const;

        int clusterId() const;

        S9sString toString() const;

        S9sString toOneLiner(
                bool useSyntaxHighlight,
                bool isDebugMode) const;

        static S9sEvent::EventType 
            stringToEventType(
                    const S9sString &eventTypeString);

        static S9sEvent::EventSubClass
            stringToEventSubClass(
                    const S9sString &subClassString);

        bool hasHost() const;
        S9sNode host() const;
        
        bool hasServer() const;
        S9sServer server() const;

        bool hasCluster() const;
        S9sCluster cluster() const;

        bool hasJob() const;
        S9sJob job() const;

    protected:
        S9sString eventLogToOneLiner() const;
        S9sString eventHostToOneLiner(bool useSyntaxHighlight) const;
        S9sString eventJobToOneLiner() const;
        S9sString eventAlarmToOneLiner() const;
        S9sString eventMaintenanceToOneLiner() const;
        S9sString eventClusterToOneLiner() const;

        S9sString 
            measurementToOneLiner(
                S9sVariantMap specifics,
                bool          useSyntaxHighlight) const;
        
        S9sString getString(const S9sString &path) const;
        int getInt(const S9sString &path) const;

    private:
        S9sFormatter m_formatter;
};

