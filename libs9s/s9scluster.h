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
 * S9sTools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with S9sTools. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <S9sVariantMap>

class S9sCluster
{
    public:
        S9sCluster();
        S9sCluster(const S9sVariantMap &properties);

        virtual ~S9sCluster();

        S9sCluster &operator=(const S9sVariantMap &rhs);

        const S9sVariantMap &toVariantMap() const;

        S9sString className() const;
        S9sString name() const;
        S9sString ownerName() const;
        S9sString groupOwnerName() const;
        int clusterId() const;
        S9sString clusterType() const;
        S9sString state() const;
        S9sString configFile() const;
        S9sString logFile() const;

        S9sString vendorAndVersion() const;
        S9sString statusText() const;
        S9sString controllerName() const;
        S9sString controllerDomainName() const;

        int alarmsCritical() const;
        int alarmsWarning() const;

        int jobsAborted() const;
        int jobsDefined() const;
        int jobsDequeued() const;
        int jobsFailed() const;
        int jobsFinished() const;
        int jobsRunning() const;

        int nHosts() const;
        S9sVariantList hostIds() const;
        S9sString hostName(const int hostId);
        S9sVariant nCpuCores(const int hostId);
        S9sVariant cpuUsagePercent(const int hostId);
        S9sVariant memTotal(const int hostId);
        S9sVariant memUsed(const int hostId);

        S9sVariant swapTotal(const int hostId);
        S9sVariant swapFree(const int hostId);

        S9sVariant rxBytesPerSecond(const int hostId);
        S9sVariant txBytesPerSecond(const int hostId);

        S9sVariant totalDiskBytes(const int hostId);
        S9sVariant freeDiskBytes(const int hostId);
        
        S9sString toString(
                const bool       syntaxHighlight,
                const S9sString &formatString) const;

    private:
        S9sVariantMap jobStatistics() const;
        S9sVariant sheetInfo(const S9sString &key) const;
        
    private:
        S9sVariantMap    m_properties;
};
