/*
 * Severalnines Tools
 * Copyright (C) 2018 Severalnines AB
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
#include "s9sjob.h"

S9sJob::S9sJob() :
    S9sObject()
{
    m_properties["class_name"] = "CmonJobInstance";
}
 
S9sJob::S9sJob(
        const S9sVariantMap &properties) :
    S9sObject(properties)
{
    if (!m_properties.contains("class_name"))
        m_properties["class_name"] = "CmonJobInstance";
}

S9sJob::~S9sJob()
{
}

S9sJob &
S9sJob::operator=(
        const S9sVariantMap &rhs)
{
    setProperties(rhs);
    
    return *this;
}

/**
 * \returns The job ID, a unique ID that identifies the job itself.
 */
int
S9sJob::id() const
{
    return property("job_id").toInt();
}

int
S9sJob::clusterId() const
{
    return property("job_id").toInt();
}

S9sString
S9sJob::title() const
{
    return property("title").toString();
}

S9sString
S9sJob::userName() const
{
    return property("user_name").toString();
}

int
S9sJob::userId() const
{
    return property("user_id").toInt();
}

S9sString
S9sJob::groupName() const
{
    return property("group_name").toString();
}

S9sString
S9sJob::status() const
{
    return property("status").toString();
}

S9sString
S9sJob::createdString() const
{
    return property("created").toString();
}

S9sString
S9sJob::endedString() const
{
    return property("ended").toString();
}

S9sString
S9sJob::startedString() const
{
    return property("started").toString();
}

S9sString
S9sJob::scheduledString() const
{
    return property("scheduled").toString();
}

bool
S9sJob::hasProgressPercent() const
{
    return m_properties.contains("progress_percent");
}

double
S9sJob::progressPercent() const
{
    return property("progress_percent").toDouble();
}

S9sString 
S9sJob::tags(
        const S9sString defaultValue) const
{
    S9sString      retval;
    S9sVariantList theList = property("tags").toVariantList();

    for (uint idx = 0u; idx < theList.size(); ++idx)
    {
        S9sString tag = theList[idx].toString();

        if (tag.empty())
            continue;

        tag = "#" + tag;
        if (!retval.empty())
            retval += ", ";

        retval += tag;
    }

    if (retval.empty())
        retval = defaultValue;

    return retval;
}
