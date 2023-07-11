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
#pragma once

#include "s9sobject.h"
#include "s9svariantmap.h"
#include "s9scluster.h"
#include "s9snode.h"

/**
 * Represents a replication link between a master and a slave. This is an
 * artificial class, the controller does not have objects like this, but it
 * makes implementing certain features easier.
 */
class S9sReplication : public S9sObject
{
    public:
        S9sReplication();

        S9sReplication(
                const S9sCluster &cluster,
                const S9sNode    &slave);

        bool isValid() const;

        S9sString slaveHostName() const;
        S9sString masterHostName() const;

        int slavePort() const;
        int masterPort() const;

        S9sString slaveName() const;
        S9sString masterName() const;

        S9sString slaveStatusShort() const;

        S9sString slaveMessage() const;
        int secondsBehindMaster() const;

        S9sString slavePosition() const;
        S9sString masterPosition() const;

        S9sString toString(
                const bool       syntaxHighlight,
                const S9sString &formatString) const;

        bool matchSlave(const S9sNode &slave);
        bool matchMaster(const S9sNode &master);

    private:
        S9sVariantMap slaveInfo() const;
        S9sVariantMap masterInfo() const;

        S9sNode node(
                const S9sString &hostName,
                const int        port) const;

    private:
        /** The cluster in which the slave can be found. */
        S9sCluster   m_cluster;
        /** It is the slave that knows most of the master and not the master
         * that knows the slave best.*/
        S9sNode      m_slave;
};
