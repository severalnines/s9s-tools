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

S9sString 
S9sSqlProcess::className() const
{
    return property("class_name").toString();
}

int 
S9sSqlProcess::pid() const
{
    return property("pid").toInt();
}

S9sString
S9sSqlProcess::command() const
{
    return property("command").toString();
}

S9sString
S9sSqlProcess::userName() const
{
    return property("user").toString();
}

int 
S9sSqlProcess::time() const
{
    return property("time").toInt();
}

S9sString
S9sSqlProcess::hostName() const
{
    return property("hostname").toString();
}

S9sString
S9sSqlProcess::instance() const
{
    return property("instance").toString();
}

S9sString
S9sSqlProcess::query(
        const S9sString &defaultValue) const
{
    S9sString retval = property("query").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}
