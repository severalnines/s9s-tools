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
#include <S9sObject>

#define S9S_INVALID_CLUSTER_ID -1
#define S9S_CLUSTER_ID_IS_VALID(_id) (_id > 0)

class S9sCluster : public S9sObject
{
    public:
        S9sCluster();
        S9sCluster(const S9sVariantMap &properties);

        virtual ~S9sCluster();

        S9sCluster &operator=(const S9sVariantMap &rhs);

        S9sString toString() const;

        virtual S9sString className() const;
        virtual S9sString name() const;
        S9sString fullCdtPath() const;

        virtual S9sString id(const S9sString &defaultValue) const;

        virtual S9sString 
            ownerName(
                    const S9sString defaultValue = "-") const;

        virtual S9sString 
            groupOwnerName(
                    const S9sString defaultValue = "-") const;

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

        // %h
        int nHosts() const;
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

        S9sVariantList hostIds() const;
        S9sString hostName(const int hostId);
        S9sString cpuModel(const int hostId) const;
        S9sVariant nNics(const int hostId) const;
        S9sVariant nDevices(const int hostId) const;
        S9sVariant nCpuCores(const int hostId) const;
        S9sVariant nCpus(const int hostId) const;
        S9sVariant cpuUsagePercent(const int hostId) const;
        S9sVariant memTotal(const int hostId) const;
        S9sVariant memUsed(const int hostId) const;
        S9sVariant memFree(const int hostId) const;

        S9sVariant swapTotal(const int hostId) const;
        S9sVariant swapFree(const int hostId) const;

        S9sVariant netBytesPerSecond(const int hostId) const;
        S9sVariant rxBytesPerSecond(const int hostId) const;
        S9sVariant txBytesPerSecond(const int hostId) const;

        S9sVariant totalDiskBytes(const int hostId) const;
        S9sVariant freeDiskBytes(const int hostId) const;
        
        S9sString toString(
                const bool       syntaxHighlight,
                const S9sString &formatString) const;

    private:
        S9sVariantMap jobStatistics() const;
        S9sVariant sheetInfo(const S9sString &key) const;
};
