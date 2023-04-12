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
#include "s9sobject.h"

/*
 * Names for the properties that are used in more than one object classes.
 */
const char S9sObject::propClassName[]        = "class_name";
const char S9sObject::propName[]             = "name";
const char S9sObject::propPath[]             = "cdt_path";
const char S9sObject::propOwnerId[]          = "owner_user_id";
const char S9sObject::propOwnerName[]        = "owner_user_name";
const char S9sObject::propGroupId[]          = "owner_group_id";
const char S9sObject::propGroupName[]        = "owner_group_name";
const char S9sObject::propAcl[]              = "acl";
const char S9sObject::propTags[]             = "tags";

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sObject::S9sObject()
{
    m_properties["class_name"] = className();
}

S9sObject::S9sObject(
        const S9sObject &orig)
{
    m_properties = orig.m_properties;
}

/**
 * A constructor to create an object when we have a variant map with the
 * properties. This map usually comes from the controller.
 */
S9sObject::S9sObject(
        const S9sVariantMap &properties) :
    m_properties(properties)
{
    if (!m_properties.contains("class_name"))
        m_properties["class_name"] = className();
}

S9sObject::~S9sObject()
{
}

S9sString 
S9sObject::className() const 
{
    if (hasProperty("class_name"))
        return property("class_name").toString();

    return "S9sObject"; 
}


/**
 * \returns True if a property with the given key exists.
 */
bool
S9sObject::hasProperty(
        const S9sString &key) const
{
    return m_properties.contains(key);
}

/**
 * \returns The value of the property with the given name or the empty
 *   S9sVariant object if the property is not set.
 */
S9sVariant
S9sObject::property(
        const S9sString &name) const
{
    if (m_properties.contains(name))
        return m_properties.at(name);

    return S9sVariant();
}

/**
 * \param name The name of the property to set.
 * \param value The value of the property as a string.
 *
 * This function will investigate the value represented as a string. If it looks
 * like a boolean value (e.g. "true") then it will be converted to a boolean
 * value, if it looks like an integer (e.g. 42) it will be converted to an
 * integer. Then the property will be set accordingly.
 */
void
S9sObject::setProperty(
        const S9sString &name,
        const S9sString &value)
{
    if (value.empty())
    {
        m_properties.erase(name);
        return;
    }

    if (value.looksBoolean())
    {
        m_properties[name] = value.toBoolean();
    } else if (value.looksInteger())
    {
        m_properties[name] = value.toInt();
    } else {
        m_properties[name] = value;
    }
}

/**
 * \param name The name of the property to set.
 * \param value The value of the property as a boolean.
 */
void
S9sObject::setProperty(
        const S9sString &name,
        const bool       value)
{
    m_properties[name] = value;
}

/**
 * \param name The name of the property to set.
 * \param value The value of the property as an integer.
 */
void
S9sObject::setProperty(
        const S9sString &name,
        const int        value)
{
    m_properties[name] = value;
}

/**
 * \param name The name of the property to set.
 * \param value The value of the property as a variant map.
 */
void
S9sObject::setProperty(
        const S9sString     &name,
        const S9sVariantMap &value)
{
    m_properties[name] = value;
}

/**
 * \param name The name of the property to set.
 * \param value The value of the property as a variant list.
 */
void
S9sObject::setProperty(
        const S9sString      &name,
        const S9sVariantList &value)
{
    m_properties[name] = value;
}

/**
 * \param properties The properties to be set as a name -> value mapping.
 *
 * Sets all the properties in one step. All the existing properties will be
 * deleted, then the new properties set.
 */
void
S9sObject::setProperties(
        const S9sVariantMap &properties)
{
    m_properties = properties;
}

/**
 * \returns The properties of the object in a variant map.
 */
const S9sVariantMap &
S9sObject::toVariantMap() const
{
    return m_properties;
}

/**
 * The name of the object. This is a virtual function so that other classes can
 * have names represented by other property names.
 */
S9sString 
S9sObject::name() const
{
    return property("alias").toString();
}

/**
 * \param columns Controls how many columns will be used on the terminal to
 *   print the name. This is an upper limit with ellipsis.
 * \returns The name of the object (as the name() method), but this time there
 * is a limit for the number of characters, so that we can pretty print names in
 * columns.
 */
S9sString
S9sObject::name(
        const int columns) const
{
    S9sString retval = name();

    if (columns <= 0)
        return retval;

    if ((int)retval.length() > columns)
    {
        retval.resize(columns);
        retval += "…";
    }

    return retval;
}

/**
 * \param defaultValue The value that will be returned if the property is not
 *   available/set.
 * \returns The LDAP distinguished name if one is available amongst the
 *   properties.
 */
S9sString 
S9sObject::distinguishedName(
        const S9sString &defaultValue) const
{
    S9sString retval = property("distinguished_name").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

/**
 * \param defaultValue The value that will be returned if the property is not
 *   available/set.
 * \returns The string value of the "origin" property, that currently can be
 *   "CmonDb" or "LDAP" showing what the original source of the object is.
 */
S9sString 
S9sObject::origin(
        const S9sString &defaultValue) const
{
    S9sString retval = property("origin").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}


S9sString 
S9sObject::aclString() const
{
    return property("acl").toString();
}

/**
 * Converts the ACL string as it is received from the controller to a short
 * string 
 */
S9sString 
S9sObject::aclShortString() const
{
    S9sString retval = property("acl").toString();
    return aclStringToUiString(retval);
}

/**
 * \param defaultValue The value that will be returned if the property is not
 *   available/set.
 */
S9sString 
S9sObject::id(
        const S9sString &defaultValue) const
{
    S9sString retval = property("id").toString();

    if (hasProperty("hostId"))
        retval = property("hostId").toString();

    if (retval.empty())
        retval = defaultValue;

    return retval;
}

S9sString 
S9sObject::cdtPath() const
{
    return property("cdt_path").toString();
}

/**
 * \param defaultValue The value that will be returned if the property is not
 *   available/set.
 * \returns The user name of the user that owns this object.
 */
S9sString 
S9sObject::ownerName(
        const S9sString defaultValue) const
{
    S9sString retval;

    if (m_properties.contains("owner_user_name"))
    {
        retval = m_properties.at("owner_user_name").toString();
    } else if (m_properties.contains("owner_user_id"))
    {
        retval.sprintf("%d", m_properties.at("owner_user_id").toInt());
    } else {
        retval = defaultValue;
    }

    return retval;
}

/**
 * \param defaultValue The value that will be returned if the property is not
 *   available/set.
 * \returns The group name of the group that owns the object.
 */
S9sString 
S9sObject::groupOwnerName(
        const S9sString defaultValue) const
{
    S9sString retval;

    if (m_properties.contains("owner_group_name"))
    {
        retval = m_properties.at("owner_group_name").toString();
    } else if (m_properties.contains("owner_group_id"))
    {
        retval.sprintf("%d", m_properties.at("owner_group_id").toInt());
    } else {
        retval = defaultValue;
    }

    return retval;
}

int
S9sObject::stateAsChar() const
{
    return '\0';
}

/**
 * \returns A list with all the tags associated with the object. 
 */
S9sVariantList
S9sObject::tags() const
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
S9sObject::tags(
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
 * \returns True if the job has at least one of the required tags.
 */
bool
S9sObject::hasTags(
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

        if (found)
            return true;
    }

    return requiredTags.empty();
}
