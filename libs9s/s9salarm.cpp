/*
 * Severalnines Tools
 * Copyright (C) 2016-2018 Severalnines AB
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
#include "s9salarm.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sAlarm::S9sAlarm() :
    S9sObject()
{
    m_properties["class_name"] = "CmonAlarm";
}

S9sAlarm::S9sAlarm(
        const S9sAlarm &orig) :
    S9sObject(orig)
{
    m_properties = orig.m_properties;
}
 
S9sAlarm::S9sAlarm(
        const S9sVariantMap &properties) :
    S9sObject(properties)
{
    if (!m_properties.contains("class_name"))
        m_properties["class_name"] = "CmonAlarm";
}

S9sAlarm::~S9sAlarm()
{
}

S9sAlarm &
S9sAlarm::operator=(
        const S9sVariantMap &rhs)
{
    setProperties(rhs);
    
    return *this;
}

/**
 * \returns The S9sAlarm converted to a variant map.
 *
 * \code{.js}
    {
        "alarm_id": 3,
        "class_name": "CmonAlarm",
        "cluster_id": 1,
        "component": 0,
        "component_name": "Network",
        "counter": 1,
        "created": "2018-09-25T05:09:24.000Z",
        "host_id": 1,
        "hostname": "192.168.0.79",
        "ignored": 0,
        "measured": 0,
        "message": "Server 192.168.0.79 reports: Host 192.168.0.79 is not responding to ping after 3 cycles, the host is most likely unreachable.",
        "recommendation": "Restart failed host, check firewall.",
        "reported": "2018-09-25T05:09:24.000Z",
        "severity": 2,
        "severity_name": "ALARM_CRITICAL",
        "title": "Host is not responding",
        "type": 10006,
        "type_name": "HostUnreachable"
    }
 * \endcode
 */
const S9sVariantMap &
S9sAlarm::toVariantMap() const
{
    return m_properties;
}

S9sString 
S9sAlarm::title() const
{
    S9sString retval = property("title").toString();

    if (retval.empty())
        retval = "-";

    return retval;
}

int
S9sAlarm::alarmId() const
{
    return property("alarm_id").toInt();
}

int
S9sAlarm::clusterId() const
{
    return property("cluster_id").toInt();
}

S9sString
S9sAlarm::typeName(
        const S9sString &defaultValue)
{
    S9sString retval = property("type_name").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

S9sString
S9sAlarm::componentName(
        const S9sString &defaultValue)
{
    S9sString retval = property("component_name").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

S9sString
S9sAlarm::severityName(
        const S9sString &defaultValue)
{
    S9sString retval = property("severity_name").toString();

    retval.replace("ALARM_", "");

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

S9sString
S9sAlarm::hostName(
        const S9sString &defaultValue)
{
    S9sString retval = property("hostname").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

const char *
S9sAlarm::severityColorBegin(
        const bool syntaxHighlight)
{
    if (!syntaxHighlight)
        return "";

    if (severityName() == "CRITICAL")
        return XTERM_COLOR_RED;

    return XTERM_COLOR_ORANGE;
}

const char *
S9sAlarm::severityColorEnd(
        const bool syntaxHighlight)
{
    if (!syntaxHighlight)
        return "";

    return TERM_NORMAL;
}

int
S9sAlarm::counter() const
{
    return property("counter").toInt();
}

int
S9sAlarm::ignoredCounter() const
{
    return property("ignored").toInt();
}

bool
S9sAlarm::isIgnored() const
{
    return counter() > 0 &&
        ignoredCounter() >= counter();
}

