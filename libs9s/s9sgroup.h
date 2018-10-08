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
#pragma once

#include "S9sObject"

/**
 * A class that represents a group on the controller. 
 *
 * Here is how the group looks like now:
 * \code{.js}
 * {
 *     "acl": "user::rwx,group::rw-,other::---",
 *     "cdt_path": "/groups",
 *     "class_name": "CmonGroup",
 *     "group_id": 5,
 *     "group_name": "ds9",
 *     "owner_group_id": 1,
 *     "owner_group_name": "admins",
 *     "owner_user_id": 1,
 *     "owner_user_name": "system"
 * }
 * \endcode
 */
class S9sGroup : public S9sObject
{
    public:
        S9sGroup();
        S9sGroup(const S9sVariantMap &properties);
        S9sGroup(const S9sString &stringRep);

        virtual ~S9sGroup();

        S9sGroup &operator=(const S9sVariantMap &rhs);
        
        S9sString groupName() const;
        int groupId() const;
};

