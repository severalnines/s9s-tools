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
#include "S9sUrl"
#include "S9sCluster"

/**
 * A class that represents a node/host/server. 
 */
class S9sNode
{
    public:
        S9sNode();
        S9sNode(const S9sVariantMap &properties);
        S9sNode(const S9sString &stringRep);

        virtual ~S9sNode();

        S9sNode &operator=(const S9sVariantMap &rhs);

        bool hasProperty(const S9sString &key) const;
        S9sVariant property(const S9sString &name) const;

        void setProperty(const S9sString &name, const S9sString &value);

        const S9sVariantMap &toVariantMap() const;
        void setProperties(const S9sVariantMap &properties);

        void setCluster(const S9sCluster &cluster);
        const S9sCluster &cluster() const;

        S9sString protocol() const { return m_url.protocol(); };
        S9sString className() const;

        S9sString 
            toString(
                const bool       syntaxHighlight,
                const S9sString &formatString) const;

        int id() const;
        int clusterId() const;
        S9sString name() const;
        S9sString hostName() const;
        S9sString ipAddress() const;
        S9sString alias() const;
        S9sString role() const;
        bool isMaster() const;
        bool isSlave() const;
        char roleFlag() const;
        S9sString configFile() const;
        S9sString logFile() const;
        S9sString pidFile() const;
        S9sString dataDir() const;
        int pid() const;
        ulonglong uptime() const;
        S9sString replicationState() const;

        bool hasPort() const;
        int port() const;

        bool hasError() const;
        S9sString fullErrorString() const;

        S9sString hostStatus() const;
        char hostStatusFlag() const;
        S9sString nodeType() const;
        char nodeTypeFlag() const;
        S9sString version() const;
        S9sString message() const;
        S9sString osVersionString() const;
        bool isMaintenanceActive() const;
        bool readOnly() const;
        bool superReadOnly() const;
        S9sString receivedLocation() const;
        S9sString replayLocation() const;
        bool connected() const;
        bool managed() const;
        bool nodeAutoRecovery() const;
        bool skipNameResolve() const;
        time_t lastSeen() const;
        int sshFailCount() const;
        S9sString slavesAsString() const;

        // %Z
        S9sString cpuModel() const;
        // %m
        S9sVariant memTotal() const;
        S9sVariant memFree() const;
        // %c
        S9sVariant nCpuCores() const;
        // %U
        S9sVariant nCpus() const;
        // %n
        S9sVariant nNics() const;
        // %i
        S9sVariant nDevices() const;
        // %k
        S9sVariant totalDiskBytes() const;
        S9sVariant freeDiskBytes() const;
        // %t
        S9sVariant netBytesPerSecond() const;
        // %u
        S9sVariant cpuUsagePercent() const;
        // %w 
        S9sVariant swapTotal() const;
        S9sVariant swapFree() const;

        S9sVariant rxBytesPerSecond() const;
        S9sVariant txBytesPerSecond() const;

        static void 
            selectByProtocol(
                    const S9sVariantList &theList,
                    S9sVariantList       &matchedNodes,
                    S9sVariantList       &otherNodes,
                    const S9sString      &protocol);

    private:
        S9sVariantMap    m_properties;
        S9sUrl           m_url;
        S9sCluster       m_cluster;
};
