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
#include "s9ssqlprocess.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sSqlProcess::S9sSqlProcess() :
    S9sObject()
{
    m_properties["class_name"] = "CmonHost";
}

S9sSqlProcess::S9sSqlProcess(
        const S9sSqlProcess &orig) :
    S9sObject(orig)
{
}

S9sSqlProcess::S9sSqlProcess(
        const S9sVariantMap &properties) :
    S9sObject(properties)
{
    if (!m_properties.contains("class_name"))
        m_properties["class_name"] = "CmonHost";
}


S9sSqlProcess::~S9sSqlProcess()
{
}

S9sSqlProcess &
S9sSqlProcess::operator=(
        const S9sVariantMap &rhs)
{
    setProperties(rhs);
    
    return *this;
}

/**
 * This method can be used to sort the SQL process objects. This method
 * implements the default sorting order, first key is the SQL server, second is
 * the process ID.
 */
bool
S9sSqlProcess::compareSqlProcess(
        const S9sSqlProcess &a,
        const S9sSqlProcess &b)
{
    if (a.instance() != b.instance())
        return a.instance() < b.instance();

    return a.pid() < b.pid();
}

bool
S9sSqlProcess::compareSqlProcessByTime(
        const S9sSqlProcess &a,
        const S9sSqlProcess &b)
{
    if (a.time() != b.time())
        return a.time() > b.time();
    
    if (a.instance() != b.instance())
        return a.instance() < b.instance();

    return a.pid() < b.pid();
}



S9sString 
S9sSqlProcess::className() const
{
    return property("class_name").toString();
}

/**
 * On MySql the SQL processes will have a unique process ID that is not the UNIX
 * process ID, but an internal ID assigned by the SQL server process.
 */
int 
S9sSqlProcess::pid() const
{
    return property("pid").toInt();
}

/**
 * This is actually the type of the process. On MySql this can be "Sleep",
 * "Daemon" and "Query".
 */
S9sString
S9sSqlProcess::command() const
{
    S9sString retval;

    if (className() == "CmonPostgreSqlDbProcess")
    {
        retval = property("waiting").toString();
        
        if (retval.empty() && !query("").empty())
            retval = "Query";

    } else {
        retval = property("command").toString();
    }

    return retval;
}

/**
 * \returns The name of the account that is running the query. 
 */
S9sString
S9sSqlProcess::userName(
        const S9sString &defaultValue) const
{
    S9sString retval;

    // FIXME: On MySQL it is "user", on postgres it is "userName".
    if (hasProperty("user"))
    {
        retval = property("user").toString();
    } else {
        retval = property("userName").toString();
    }

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 * On MySql this shows how many seconds the process is holding its state.
 */
int 
S9sSqlProcess::time() const
{
    int retval = 0;

    if (className() == "CmonPostgreSqlDbProcess")
    {
        S9sString elapsedTime = property("elapsedTime").toString();

        // "elapsedTime": "00:00:00.00155"
        if (elapsedTime.length() >= 8)
        {
            retval += (elapsedTime[3] - '0') * 10 * 60;
            retval += (elapsedTime[4] - '0') * 60;
            retval += (elapsedTime[6] - '0') * 10;
            retval += (elapsedTime[7] - '0');
            #if 0
            S9S_WARNING("%c%c:%c%c", 
                    elapsedTime[3], elapsedTime[4], elapsedTime[6], 
                    elapsedTime[7]);
            #endif
        }
    } else {
        retval = property("time").toInt();
    }

    return retval;
}

/**
 * This is bogus, there is a mess. I redirected this to the client() method.
 */
S9sString
S9sSqlProcess::hostName() const
{
    //return property("hostname").toString();
    return client();
}

/**
 * \returns The client where the query is originated in HOSTNAME:PORT format.
 */
S9sString
S9sSqlProcess::client(
        const S9sString &defaultValue) const
{
    S9sString retval = property("client").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 * \returns The SQL server where the process was found in HOSTNAME:PORT format.
 */
S9sString
S9sSqlProcess::instance() const
{
    if (className() == "CmonPostgreSqlDbProcess")
    {
        return property("hostname").toString();
    }

    return property("instance").toString();
}

/**
 * \returns The executed SQL query or the empty string if the process has no
 *   query.
 */
S9sString
S9sSqlProcess::query(
        const S9sString &defaultValue) const
{
    S9sString retval = property("query").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}
