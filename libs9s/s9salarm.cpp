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

