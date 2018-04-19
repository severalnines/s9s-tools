/*
 * Severalnines Tools
 * Copyright (C) 2018  Severalnines AB
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
#include "s9suser.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sUser::S9sUser() : 
    S9sObject()
{
    m_properties["class_name"] = "CmonUser";
}
 
S9sUser::S9sUser(
        const S9sVariantMap &properties) :
    S9sObject(properties)
{
    m_properties["class_name"] = "CmonUser";
}

S9sUser::~S9sUser()
{
}

S9sUser &
S9sUser::operator=(
        const S9sVariantMap &rhs)
{
    setProperties(rhs);
    
    return *this;
}

/**
 * \returns The S9sUser converted to a variant map.
 *
\code{.js}
    {
        "class_name": "CmonUser",
        "email_address": "warrior@ds9.com",
        "first_name": "Worf",
        "groups": [ 
        {
            "class_name": "CmonGroup",
            "group_id": 4,
            "group_name": "ds9"
        } ],
        "title": "Lt.",
        "user_id": 12,
        "user_name": "worf"
    }
\endcode
 */
const S9sVariantMap &
S9sUser::toVariantMap() const
{
    return m_properties;
}

/**
 * \returns The JSON string representing the user.
 */
S9sString 
S9sUser::toString() const
{
    return m_properties.toString();
}

/**
 * \returns The username of the user.
 */
S9sString
S9sUser::userName() const
{
    if (m_properties.contains("user_name"))
        return m_properties.at("user_name").toString();

    return S9sString();
}

/**
 * \returns The email address of the user.
 */
S9sString
S9sUser::emailAddress(
        const S9sString &defaultValue) const
{
    if (m_properties.contains("email_address"))
        return m_properties.at("email_address").toString();

    return defaultValue;
}

/**
 * \returns The unique numerical ID of the user.
 */
int
S9sUser::userId() const
{
    if (m_properties.contains("user_id"))
        return m_properties.at("user_id").toInt();

    return 0;
}

/**
 * \returns The first name of the user.
 */
S9sString
S9sUser::firstName() const
{
    if (m_properties.contains("first_name"))
        return m_properties.at("first_name").toString();

    return S9sString();
}

/**
 * \returns The last name of the user.
 */
S9sString
S9sUser::lastName() const
{
    if (m_properties.contains("last_name"))
        return m_properties.at("last_name").toString();

    return S9sString();
}

/**
 * \returns The middle name of the user.
 */
S9sString
S9sUser::middleName() const
{
    if (m_properties.contains("middle_name"))
        return m_properties.at("middle_name").toString();

    return S9sString();
}

/**
 * \returns The title of the user (e.g. "Dr.").
 */
S9sString
S9sUser::title() const
{
    if (m_properties.contains("title"))
        return m_properties.at("title").toString();

    return S9sString();
}

/**
 * \returns The job title of the user (e.g. "programmer").
 */
S9sString
S9sUser::jobTitle() const
{
    if (m_properties.contains("job_title"))
        return m_properties.at("job_title").toString();

    return S9sString();
}

/**
 * \returns The full name of the user in one string.
 */
S9sString
S9sUser::fullName() const
{
    S9sString retval;

    if (!title().empty())
    {
        if (!retval.empty())
            retval += " ";

        retval += title();
    }

    if (!firstName().empty())
    {
        if (!retval.empty())
            retval += " ";

        retval += firstName();
    }

    if (!lastName().empty())
    {
        if (!retval.empty())
            retval += " ";

        retval += lastName();
    }

    return retval;
}

S9sString
S9sUser::lastLoginString(
        const S9sString &defaultValue) const
{
    S9sString retval = defaultValue;

    if (m_properties.contains("last_login"))
        retval = m_properties.at("last_login").toString();

    return retval;
}

/**
 * \param defaultValue A string that will be sent back if this information is
 *   not available.
 * \returns A date&time string showing when the user was created.
 */
S9sString
S9sUser::createdString(
        const S9sString &defaultValue) const
{
    S9sString retval = defaultValue;

    if (m_properties.contains("created"))
        retval = m_properties.at("created").toString();

    return retval;
}

/**
 * \param defaultValue A string that will be sent back if this information is
 *   not available.
 * \returns A date&time string showing when the last failed login attempt
 *   happened. 
 */
S9sString
S9sUser::failedLoginString(
        const S9sString &defaultValue) const
{
    S9sString retval;

    if (m_properties.contains("last_failed_login"))
        retval = m_properties.at("last_failed_login").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 * \returns True if the user is disabled, false otherwise.
 */
bool 
S9sUser::isDisabled() const
{
    bool retval = false;

    if (m_properties.contains("disabled"))
        retval = m_properties.at("disabled").toBoolean();

    return retval;
}

/**
 * \returns True if the user is suspended, false otherwise.
 */
bool 
S9sUser::isSuspended() const
{
    bool retval = false;

    if (m_properties.contains("suspended"))
        retval = m_properties.at("suspended").toBoolean();

    return retval;
}

/**
 * \returns How many failed logins the user has. 
 *
 * This counter is reset to 0 when the user successfully authenticates.
 */
int
S9sUser::nFailedLogins() const
{
    int retval = 0;

    if (m_properties.contains("n_failed_logins"))
        retval = m_properties.at("n_failed_logins").toInt();

    return retval;
}


void
S9sUser::setGroup(
        const S9sString &groupName)
{
    S9sVariantMap  groupMap;
    S9sVariantList groupList;

    groupMap["class_name"] = "CmonGroup";
    groupMap["group_name"] = groupName;

    groupList << groupMap;

    m_properties["groups"] = groupList;
}

void
S9sUser::setPublicKey(
        const S9sString &name,
        const S9sString &key)
{
    S9sVariantMap  keyMap;
    S9sVariantList keysList;

    keyMap["name"]              = name;
    keyMap["key"]               = key;

    keysList << keyMap;
    m_properties["public_keys"] = keysList;
}

/**
 * \param separator The field separator to put in between the group names.
 * \returns All the group names concatenated with a field separator.
 */
S9sString
S9sUser::groupNames(
        const S9sString separator) const
{
    S9sVariantList groupList;
    S9sString      retval;

    if (m_properties.contains("groups"))
        groupList = m_properties.at("groups").toVariantList();

    //
    // Concatenating the group names into one string.
    //
    for (uint idx = 0u; idx < groupList.size(); ++idx)
    {
        S9sVariantMap groupMap  = groupList[idx].toVariantMap();
        S9sString     groupName = groupMap["group_name"].toString();

        if (!retval.empty())
            retval += separator;

        retval += groupName;
    }

    return retval;
}

bool
S9sUser::isMemberOf(
        const S9sString &groupName) const
{
    S9sVariantList groupList;
    S9sString      retval;

    if (m_properties.contains("groups"))
        groupList = m_properties.at("groups").toVariantList();

    for (uint idx = 0u; idx < groupList.size(); ++idx)
    {
        S9sVariantMap groupMap  = groupList[idx].toVariantMap();
        S9sString     thisName  = groupMap["group_name"].toString();

        if (thisName == groupName)
            return true;
    }

    return false;
}

/**
 * \param syntaxHighlight Controls if the string will have colors or not.
 * \param formatString The formatstring with markup.
 * \returns The string representation according to the format string.
 *
 * Converts the message to a string using a special format string that may
 * contain field names of properties of the user.
 */
S9sString
S9sUser::toString(
        const bool       syntaxHighlight,
        const S9sString &formatString) const
{
    S9sString    retval;
    S9sString    tmp;
    char         c;
    S9sString    partFormat;
    bool         percent      = false;
    bool         escaped      = false;
    bool         modifierFree = false;

    for (uint n = 0; n < formatString.size(); ++n)
    {
        c = formatString[n];
       
        if (c == '%' && !percent)
        {
            percent    = true;
            partFormat = "%";
            continue;
#if 0
        } else if (percent && c == 'f')
        {
            modifierFree = true;
            continue;
#endif
        } else if (c == '\\')
        {
            escaped = true;
            continue;
        }

        if (modifierFree)
        {
            S9S_DEBUG("Modifier 'f' is not used here.");
        }

        if (escaped)
        {
            switch (c)
            {
                case '\"':
                    retval += '\"';
                    break;

                case '\\':
                    retval += '\\';
                    break;
       
                case 'a':
                    retval += '\a';
                    break;

                case 'b':
                    retval += '\b';
                    break;

                case 'e':
                    retval += '\027';
                    break;

                case 'n':
                    retval += '\n';
                    break;

                case 'r':
                    retval += '\r';
                    break;

                case 't':
                    retval += '\t';
                    break;
            }
        } else if (percent)
        {
            switch (c)
            {
                case 'F':
                    // The full name of the user.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(fullName()));
                    retval += tmp;
                    break;
                
                case 'f':
                    // The first name of the user.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(firstName()));
                    retval += tmp;
                    break;
                
                case 'G':
                    // The group names of the user.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(groupNames()));
                    retval += tmp;
                    break;

                case 'I':
                    // The user ID.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), userId());
                    retval += tmp;
                    break;
                
                case 'j':
                    // The job title of the user.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(jobTitle()));
                    retval += tmp;
                    break;
                
                case 'l':
                    // The last name of the user.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(lastName()));
                    retval += tmp;
                    break;

                case 'M':
                    // The email address. 
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(emailAddress()));
                    retval += tmp;
                    break;
                
                case 'm':
                    // The middle name of the user.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(middleName()));
                    retval += tmp;
                    break;
                
                case 'N':
                    // The username of the user.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(userName()));
                    retval += tmp;
                    break;
                
                case 't':
                    // The title of the user.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(title()));
                    retval += tmp;
                    break;

                
                case '%':
                    retval += '%';
                    break;

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case '-':
                case '+':
                case '.':
                case '\'':
                    partFormat += c;
                    continue;
            }
        } else {
            retval += c;
        }

        percent      = false;
        escaped      = false;
        modifierFree = false;
    }

    return retval;
}

