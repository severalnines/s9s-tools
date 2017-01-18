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
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar. If not, see <http://www.gnu.org/licenses/>.
 */
#include "s9scluster.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sCluster::S9sCluster()
{
}

S9sCluster::S9sCluster(
        const S9sVariantMap &properties) :
    m_properties(properties)
{
}

S9sCluster::~S9sCluster()
{
}

S9sCluster &
S9sCluster::operator=(
        const S9sVariantMap &rhs)
{
    m_properties = rhs;
    
    return *this;
}

const S9sVariantMap &
S9sCluster::toVariantMap() const
{
    return m_properties;
}

/**
 * \returns the "class_name" property Cmon uses to represent the object type.
 */
S9sString
S9sCluster::className() const
{
    if (m_properties.contains("class_name"))
        return m_properties.at("class_name").toString();

    return S9sString();
}

/**
 * \returns the "cluster_name" property Cmon uses to represent the name of the
 *   cluster.
 */
S9sString
S9sCluster::name() const
{
    if (m_properties.contains("cluster_name"))
        return m_properties.at("cluster_name").toString();

    return S9sString();
}

