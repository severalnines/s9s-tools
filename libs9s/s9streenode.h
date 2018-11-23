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

#include "S9sVariantMap"

/**
 * A class that represents a node in the CDT as they are returned by the tree
 * RPC. 
 */
class S9sTreeNode
{
    public:
        S9sTreeNode();
        S9sTreeNode(const S9sVariantMap &properties);

        virtual ~S9sTreeNode();

        S9sTreeNode &operator=(const S9sVariantMap &rhs);

        bool hasProperty(const S9sString &key) const;
        S9sVariant property(const S9sString &name) const;
        void setProperty(const S9sString &name, const S9sString &value);
        void setProperties(const S9sVariantMap &properties);

        S9sString name() const;
        S9sString path() const;
        S9sString fullPath() const;
        S9sString spec() const;
        S9sString linkTarget() const;
        S9sString ownerUserName() const;
        S9sString ownerGroupName() const;
        S9sString acl() const;
        S9sString sizeString() const;

        S9sString type() const;
        bool isFolder() const;

        S9sVector<S9sTreeNode> childNodes() const;

    private:
        S9sVariantMap    m_properties;
};

