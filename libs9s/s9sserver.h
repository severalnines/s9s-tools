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
 * A class that represents a hardware server.
 */
class S9sServer : public S9sObject
{
    public:
        S9sServer();
        S9sServer(const S9sServer &orig);
        S9sServer(const S9sVariantMap &properties);

        virtual ~S9sServer();

        S9sServer &operator=(const S9sVariantMap &rhs);
        
        virtual S9sString className() const;

        virtual S9sString name() const;
        virtual S9sString id(const S9sString &defaultValue = "") const;
        S9sString hostName() const;
        S9sString alias(const S9sString &defaultValue = "") const;
        S9sString message(const S9sString &defaultValue = "") const;
        S9sString version(const S9sString &defaultValue = "") const;
        S9sString ipAddress(const S9sString &defaultValue = "") const;
        S9sString protocol() const;
        S9sString status() const;
        S9sString hostStatus() const;

        S9sVariantList subnets() const;
        int nSubnets() const;
        S9sString subnetCidr(const int idx) const;
        S9sString subnetRegion(const int idx) const;
        S9sString subnetProvider(const int idx) const;
        S9sString subnetId(const int idx) const;
        S9sString subnetVpcId(const int idx) const;

        S9sVariantList templates() const;
        int nTemplates() const;
        S9sString templateName(const int idx) const;
        S9sString templateRegion(const int idx) const;
        S9sString templateProvider(const int idx) const;


        const char *colorBegin(bool useSyntaxHighLight) const;
        const char *colorEnd(bool useSyntaxHighLight) const;


        int nContainers() const;
        int nContainersMax() const;
        S9sString nContainersMaxString() const;
        int nRunningContainersMax() const;
        S9sString nRunningContainersMaxString() const;

        S9sString ownerName() const;
        S9sString groupOwnerName() const;
        S9sString model(const S9sString &defaultValue = "") const;

        S9sString osVersionString(const S9sString &defaultValue = "") const;
        S9sVariantList processorNames() const;
        S9sVariantList nicNames() const;
        S9sVariantList memoryBankNames() const;
        S9sVariantList diskNames() const;

        S9sVariantList containers() const;
        double totalMemoryGBytes() const;
        int nCpus() const;
        int nCores() const;
        int nThreads() const;
};

