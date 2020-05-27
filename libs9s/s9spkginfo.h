/*
 * Severalnines Tools
 * Copyright (C) 2020  Severalnines AB
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
#include "S9sDateTime"

/**
 * A class that represents a hardware server.
 */
class S9sPkgInfo : public S9sObject
{
    public:
        S9sPkgInfo();
        S9sPkgInfo(const S9sPkgInfo &orig);
        S9sPkgInfo(const S9sVariantMap &properties);

        virtual ~S9sPkgInfo();

        S9sPkgInfo &operator=(const S9sVariantMap &rhs);
        
        virtual S9sString className() const;
        
        S9sString 
            toString(
                const bool       syntaxHighlight,
                const S9sString &formatString) const;

        virtual S9sString name() const;
        S9sString packageType() const;
        S9sString hostName() const;
        S9sDateTime lastUpdated() const;
        S9sString installedVersion() const;
        S9sString availableVersion() const;

        const char *colorBegin(bool useSyntaxHighLight) const;
        const char *colorEnd(bool useSyntaxHighLight) const;

        static bool compareByName(
                const S9sPkgInfo &server1,
                const S9sPkgInfo &server2);
};

