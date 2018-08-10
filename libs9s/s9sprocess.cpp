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
#include "s9sprocess.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sProcess::S9sProcess() :
    S9sObject()
{
    m_properties["class_name"] = "CmonHost";
}

S9sProcess::S9sProcess(
        const S9sProcess &orig) :
    S9sObject(orig)
{
}

S9sProcess::S9sProcess(
        const S9sVariantMap &properties) :
    S9sObject(properties)
{
    if (!m_properties.contains("class_name"))
        m_properties["class_name"] = "CmonHost";
}

S9sProcess::~S9sProcess()
{
}

S9sProcess &
S9sProcess::operator=(
        const S9sVariantMap &rhs)
{
    setProperties(rhs);
    
    return *this;
}

S9sString 
S9sProcess::className() const
{
    return property("class_name").toString();
}

int 
S9sProcess::pid() const
{
    return property("pid").toInt();
}

S9sString
S9sProcess::userName() const
{
    return property("user").toString();
}

S9sString
S9sProcess::hostName() const
{
    return property("hostname").toString();
}

int
S9sProcess::priority() const
{
    return property("priority").toInt();
}

ulonglong
S9sProcess::virtMem() const
{
    return property("virt_mem").toULongLong();
}

S9sString
S9sProcess::virtMem(
        const char *ignored) const
{
    ulonglong value = virtMem();
    S9sString retval;

    value /= 1024;
    retval.sprintf("%llu", value);
    return retval;
}

longlong
S9sProcess::resMem() const
{
    return property("res_mem").toULongLong();
}

S9sString
S9sProcess::resMem(
        const char *ignored) const
{
    ulonglong value = resMem();
    S9sString retval;

    value /= 1024;
    retval.sprintf("%llu", value);
    return retval;
}

double
S9sProcess::cpuUsage() const
{
    return property("cpu_usage").toDouble();
}

S9sString
S9sProcess::cpuUsage(
        const char *ignored) const
{
    double    value = cpuUsage();
    S9sString retval;

    retval.sprintf("%6.2f", value);
    return retval;
}

double
S9sProcess::memUsage() const
{
    return property("mem_usage").toDouble();
}

S9sString
S9sProcess::memUsage(
        const char *ignored) const
{
    double    value = memUsage();
    S9sString retval;

    retval.sprintf("%6.2f", value);
    return retval;
}

S9sString
S9sProcess::executable() const
{
    return property("executable").toString();
}

S9sString
S9sProcess::state() const
{
    return property("state").toString();
}
