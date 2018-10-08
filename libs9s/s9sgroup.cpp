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
#include "s9sgroup.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sGroup::S9sGroup() : 
    S9sObject()
{
    m_properties["class_name"] = "CmonGroup";
}

S9sGroup::S9sGroup(
        const S9sVariantMap &properties) :
        S9sObject(properties)
{
    if (!m_properties.contains("class_name"))
        m_properties["class_name"] = "CmonGroup";
}

S9sGroup::~S9sGroup()
{
}

S9sGroup &
S9sGroup::operator=(
        const S9sVariantMap &rhs)
{
    setProperties(rhs);
    
    return *this;
}

/**
 * \returns The groupname of the group.
 */
S9sString
S9sGroup::groupName() const
{
    if (m_properties.contains("group_name"))
        return m_properties.at("group_name").toString();

    return S9sString();
}

/**
 * \returns The unique numerical ID of the group.
 */
int
S9sGroup::groupId() const
{
    if (m_properties.contains("group_id"))
        return m_properties.at("group_id").toInt();

    return 0;
}
