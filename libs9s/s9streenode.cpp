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
#include "s9streenode.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sTreeNode::S9sTreeNode() :
    m_childNodesParsed(false)
{
    m_properties["class_name"] = "CmonTreeNode";
}
 
S9sTreeNode::S9sTreeNode(
        const S9sVariantMap &properties) :
    m_properties(properties),
    m_childNodesParsed(false)
{
    m_properties["class_name"] = "CmonTreeNode";
}

S9sTreeNode::~S9sTreeNode()
{
}

S9sTreeNode &
S9sTreeNode::operator=(
        const S9sVariantMap &rhs)
{
    setProperties(rhs);
    
    return *this;
}

/**
 * \returns True if a property with the given key exists.
 */
bool
S9sTreeNode::hasProperty(
        const S9sString &key) const
{
    return m_properties.contains(key);
}

/**
 * \returns The value of the property with the given name or the empty
 *   S9sVariant object if the property is not set.
 */
S9sVariant
S9sTreeNode::property(
        const S9sString &name) const
{
    if (m_properties.contains(name))
        return m_properties.at(name);

    return S9sVariant();
}

/**
 * \param properties The properties to be set as a name -> value mapping.
 *
 * Sets all the properties in one step. All the existing properties will be
 * deleted, then the new properties set.
 */
void
S9sTreeNode::setProperties(
        const S9sVariantMap &properties)
{
    m_properties = properties;
}

S9sString
S9sTreeNode::name() const
{
    S9sString retval = property("item_name").toString();

    // Root node has no name, just a path.
    if (retval.empty())
        retval = property("item_path").toString();

    return retval;
}

S9sString
S9sTreeNode::path() const
{
    return property("item_path").toString();
}

S9sString
S9sTreeNode::fullPath() const
{
    S9sString retval;

    retval = path();

    if (!retval.endsWith("/"))
        retval += "/";

    retval += name();
    return retval;
}

S9sString
S9sTreeNode::spec() const
{
    return property("item_spec").toString();
}

S9sString
S9sTreeNode::linkTarget() const
{
    return property("link_target").toString();
}

S9sString
S9sTreeNode::ownerUserName() const
{
    S9sString retval;

    retval = property("owner_user_name").toString();
    if (retval.empty())
    {
        if (hasProperty("owner_user_id"))
            retval.sprintf("%d", property("owner_user_id").toInt());
    }

    return retval;
}

S9sString
S9sTreeNode::ownerGroupName() const
{
    S9sString retval;

    retval = property("owner_group_name").toString();
    if (retval.empty())
    {
        if (hasProperty("owner_group_id"))
            retval.sprintf("%d", property("owner_group_id").toInt());
    }

    return retval;
}

S9sString
S9sTreeNode::acl() const
{
    return property("item_acl").toString();
}

S9sString 
S9sTreeNode::sizeString() const
{
    S9sString retval;

    if (hasProperty("major_device_number") && 
            hasProperty("minor_devide_number"))
    {
        int major = property("major_device_number").toInt();
        int minor = property("minor_devide_number").toInt();

        retval.sprintf("%d, %d", major, minor);
    } else if (hasProperty("size")) 
    {
        ulonglong size = property("size").toULongLong();

        retval.sprintf("%'llu", size);
    } else {
        retval = "-";
    }

    return retval;
}

S9sString
S9sTreeNode::type() const
{
    return property("item_type").toString().toLower();
}

S9sString
S9sTreeNode::typeName() const
{
    return property("item_type").toString();
}

int
S9sTreeNode::typeAsChar() const
{
    if (type() == "folder")
        return 'd';
    else if (type() == "file")
        return '-';
    else if (type() == "cluster")
        return 'c';
    else if (type() == "node")
        return 'n';
    else if (type() == "server")
        return 's';
    else if (type() == "user")
        return 'u';
    else if (type() == "group")
        return 'g';
    else if (type() == "container")
        return 'c';
    else if (type() == "database")
        return 'b';

    return '?';
}

bool
S9sTreeNode::isFolder() const
{
    return type() == "folder";
}

bool
S9sTreeNode::isFile() const
{
    return type() == "file";
}

bool
S9sTreeNode::isCluster() const
{
    return type() == "cluster";
}

bool
S9sTreeNode::isNode() const
{
    return type() == "node";
}

bool
S9sTreeNode::isServer() const
{
    return type() == "server";
}

bool
S9sTreeNode::isUser() const
{
    return type() == "user";
}

bool
S9sTreeNode::isGroup() const
{
    return type() == "group";
}

bool
S9sTreeNode::isContainer() const
{
    return type() == "container";
}

bool
S9sTreeNode::isDatabase() const
{
    return type() == "database";
}

bool
S9sTreeNode::hasChild(
        const S9sString &name)
{
    S9sVector<S9sTreeNode> nodes = childNodes();

    for (uint idx = 0u; idx < nodes.size(); ++idx)
    {
        if (nodes[idx].name() == name)
            return true;
    }

    return false;
}

const S9sVector<S9sTreeNode> &
S9sTreeNode::childNodes() const
{
    if (!m_childNodesParsed)
    {
        S9sVariantList  variantList = property("sub_items").toVariantList();
        
        for (uint idx = 0; idx < variantList.size(); ++idx)
            m_childNodes << S9sTreeNode(variantList[idx].toVariantMap());

        m_childNodesParsed = true;
    }

    return m_childNodes;
}

int
S9sTreeNode::nChildren() const
{
    return (int) childNodes().size();
}

S9sTreeNode
S9sTreeNode::childNode(
        int idx) const
{
    const S9sVector<S9sTreeNode> &children = S9sTreeNode::childNodes();

    if (idx >= 0 && idx < (int) children.size())
        return children[idx];

    return S9sTreeNode();
}

/**
 * \returns True if the given path exists in the tree.
 */
bool
S9sTreeNode::pathExists(
        const S9sString   &path)
{
    S9sTreeNode  tmp;

    return subTree(path, tmp);
}

/**
 * \param path The starting point of the sub-tree to return.
 * \param retval The place where the function returns the sub-tree.
 * \returns True if the sub-tree found.
 */
bool
S9sTreeNode::subTree(
        const S9sString   &path,
        S9sTreeNode       &retval) const
{
    S9sVariantList pathList = path.split("/");

    if (pathList.size() > 0u)
    {
        if (pathList[0u].toString() == "/")
            pathList.takeFirst();
    }

    if (pathList.size() == 0u)
    {
        retval = *this;
        return true;
    }

    return subTree(pathList, retval);
}

/**
 * Overloaded private method for the recursive call.
 */
bool
S9sTreeNode::subTree(
        const S9sVariantList  &pathList,
        S9sTreeNode           &retval) const 
{
    // This can never happen, this is just a check against takeFirst() crash.
    if (pathList.empty())
        return false;

    /*
     * We take the first element on the path.
     */
    S9sVariantList reducedList = pathList;
    S9sString      nextName    = reducedList.takeFirst().toString();
    const S9sVector<S9sTreeNode> &children = childNodes();
       
    for (uint idx = 0u; idx < children.size(); ++idx)
    {
        const S9sTreeNode &child = children[idx];

        if (child.name() == nextName)
        {
            // We found the next level belonging to the next element in the 
            // path.
            if (reducedList.empty())
            {
                retval = child;
                return true;
            }
            
            return child.subTree(reducedList, retval);
        }
    }

    // The next item was not found.
    return false;
}
