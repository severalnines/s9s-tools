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
#pragma once

#include "s9sobject.h"
#include "s9svariantmap.h"
#include "s9surl.h"
#include "s9scluster.h"

/**
 * A class that represents a node/host/server. 
 */
class S9sContainer : public S9sObject
{
    public:
        S9sContainer();
        S9sContainer(const S9sContainer &orig);
        S9sContainer(const S9sVariantMap &properties);
        S9sContainer(const S9sString &stringRep);

        virtual ~S9sContainer();

        S9sContainer &operator=(const S9sVariantMap &rhs);

        virtual const S9sVariantMap &toVariantMap() const;

        virtual S9sString name() const;
        S9sString name(const int columns) const;

        S9sString alias() const;
        void setAlias(const S9sString &alias);

        virtual S9sString className() const;

        S9sString 
            toString(
                    const bool       syntaxHighlight,
                    const S9sString &formatString) const;

        S9sString hostname() const;

        S9sString ipAddress(
                const S9s::AddressType    addressType = S9s::AnyIpv4Address,
                const S9sString          &defaultValue = "") const;

        S9sString ipv4Addresses(
                const S9sString &separator = ", ",
                const S9sString &defaultValue = "-") const;

        S9sString parentServerName() const;
        void setParentServerName(const S9sString &value);

        S9sString state() const;
        virtual int stateAsChar() const;
        bool autoStart() const;

        S9sVariantMap subNet() const;
        
        S9sString subnetId(const S9sString &defaultValue = "") const;
        void setSubnetId(const S9sString &value);

        S9sString subnetVpcId(const S9sString &defaultValue = "") const;
        void setSubnetVpcId(const S9sString &value);
        S9sString subnetCidr(const S9sString &defaultValue = "") const;

        S9sString provider(const S9sString &defaultValue = "") const;
        void setProvider(const S9sString &providerName);

        S9sString image(const S9sString &defaultValue = "") const;
        void setImage(const S9sString &image);
        
        S9sString imageOsUser(const S9sString &defaultValue = "") const;
        void setImageOsUser(const S9sString &image);

        S9sString templateName(
                const S9sString  &defaultValue,
                bool              truncate = false) const;
        
        void setTemplate(const S9sString &templateName);
       
        S9sString region(const S9sString &defaultValue = "") const;
        void setRegion(const S9sString &image);

        S9sString type() const;
        double memoryLimitGBytes() const;
        S9sString configFile() const;
        S9sString rootFsPath() const;

        S9sString firewall(const S9sString &defaultValue = "-") const;
        S9sVariantList firewalls() const;
        S9sString firewalls(const S9sString &defaultValue) const;
        void setFirewalls(const S9sString &value);

        S9sVariantList volumes() const;
        void setVolumes(const S9sVariantList &volumes);
        
        uint nVolumes() const;
        int volumeGigaBytes(uint idx) const;
        S9sString volumeType(uint idx) const;

        S9sString osVersionString(const S9sString &defaultValue = "-") const;
        S9sString architecture(const S9sString &defaultValue = "-") const;

    private:
        S9sUrl           m_url;
};
