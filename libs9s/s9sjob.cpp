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

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

/**
 * The default constructor.
 */
S9sJob::S9sJob() :
    S9sObject()
{
    m_properties["class_name"] = "CmonJobInstance";
}

/**
 * Handy constructor that will create a job object from a variant map that holds
 * the properties.
 */
S9sJob::S9sJob(
        const S9sVariantMap &properties) :
    S9sObject(properties)
{
    if (!m_properties.contains("class_name"))
        m_properties["class_name"] = "CmonJobInstance";
}

/**
 * Yep, destructor.
 */
S9sJob::~S9sJob()
{
}

/**
 * Handy operator to put a set of properties and initialize a job object. This
 * function will drop all the existing properties in the object.
 */
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

/**
 * \returns The ID of the cluster that the job belongs to.
 */
int
S9sJob::clusterId() const
{
    return property("cluster_id").toInt();
}

/**
 * \returns The title UI string for the job.
 */
S9sString
S9sJob::title() const
{
    return property("title").toString();
}

/**
 * \returns The user name of the owner of the job.
 */
S9sString
S9sJob::userName() const
{
    return property("user_name").toString();
}

/**
 * \returns The user ID of the owner of the job.
 */
int
S9sJob::userId() const
{
    return property("user_id").toInt();
}

/**
 * \returns The name of the group that owns the job.
 */
S9sString
S9sJob::groupName() const
{
    return property("group_name").toString();
}

/**
 * \returns The status of the job. 
 *
 * The status is one of the following: 'DEFINED', 'DEQUEUED', 'RUNNING',
 * 'RUNNING2', 'RUNNING3', 'RUNNING_EXT', 'SCHEDULED', 'ABORTED', 'FINISHED',
 * 'FAILED'.
 */
S9sString
S9sJob::status() const
{
    return property("status").toString();
}

S9sString
S9sJob::statusText() const
{
    S9sString retval = property("status_text").toString();

    if (!retval.empty() && !retval.endsWith("."))
        retval += ".";

    return retval;
}

/**
 * \returns The creation date&time as a string.
 */
S9sString
S9sJob::createdString() const
{
    return property("created").toString();
}

/**
 * \returns The end date&time of the job if it is already ended or the empty
 *   string if not.
 */
S9sString
S9sJob::endedString() const
{
    return property("ended").toString();
}

/**
 * \returns The start date&time of the job if the job is already started or the
 *   empty string if not.
 */
S9sString
S9sJob::startedString() const
{
    return property("started").toString();
}

/**
 * \returns The schedule date&time of the job if it is scheduled or the empty
 *   string if not.
 */
S9sString
S9sJob::scheduledString() const
{
    return property("scheduled").toString();
}

/**
 * \returns True if the job has information about the progress.
 */
bool
S9sJob::hasProgressPercent() const
{
    return m_properties.contains("progress_percent");
}

/**
 * \returns How many percent of the job is done.
 */
double
S9sJob::progressPercent() const
{
    return property("progress_percent").toDouble();
}

/**
 * \returns A list with all the tags associated with the job. 
 */
S9sVariantList
S9sJob::tags() const
{
    return property("tags").toVariantList();
}

/**
 * \param useSyntaxHightlight True to produce colored output.
 * \param defaultValue The string to return if there are no tags.
 * \returns The list of tags formatted into a string as it can be printed for
 *   the user. 
 */
S9sString 
S9sJob::tags(
        bool            useSyntaxHightlight,
        const S9sString defaultValue) const
{
    S9sString      retval;
    S9sVariantList theList = property("tags").toVariantList();

    for (uint idx = 0u; idx < theList.size(); ++idx)
    {
        S9sString tag = theList[idx].toString();

        if (tag.empty())
            continue;

        if (useSyntaxHightlight)
            tag = XTERM_COLOR_TAG + tag + TERM_NORMAL;

        tag = "#" + tag;
        if (!retval.empty())
            retval += ", ";

        retval += tag;
    }

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 * \param requiredTags Tags to be checked.
 * \returns True if the job has all the required tags.
 */
bool
S9sJob::hasTags(
        const S9sVariantList &requiredTags)
{
    S9sVariantList myTags = tags();

    S9S_DEBUG(" requiredTags : %s", STR(S9sVariant(requiredTags).toString()));
    S9S_DEBUG("       myTags : %s", STR(S9sVariant(myTags).toString()));
    for (uint idx1 = 0u; idx1 < requiredTags.size(); ++idx1)
    {
        S9sString requiredTag = requiredTags[idx1].toString();
        bool      found = false;

        if (requiredTag.empty())
            continue;

        for (uint idx2 = 0u; idx2 < myTags.size(); ++idx2)
        {
            S9sString myTag = myTags[idx2].toString();

            if (requiredTag.toLower() == myTag.toLower())
            {
                found = true;
                break;
            }
        }

        if (!found)
            return false;
    }

    return true;
}
