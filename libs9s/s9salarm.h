/*
 * Severalnines Tools
 * Copyright (C) 2016-218 Severalnines AB
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

/**
 * A class that represents a node/host/server. 
 */
class S9sAlarm : public S9sObject
{
    public:
        S9sAlarm();
        S9sAlarm(const S9sAlarm &orig);
        explicit S9sAlarm(const S9sVariantMap &properties);

        virtual ~S9sAlarm() = default;

        S9sAlarm &operator=(const S9sVariantMap &rhs);

        const S9sVariantMap &toVariantMap() const override;

        S9sString title() const;
        int alarmId() const;
        int clusterId() const;
        S9sString typeName(const S9sString &defaultValue = "");
        S9sString componentName(const S9sString &defaultValue = "");
        S9sString severityName(const S9sString &defaultValue = "");
        S9sString hostName(const S9sString &defaultValue = "-");

        int counter() const;
        int ignoredCounter() const;
        bool isIgnored() const;

        const char *severityColorBegin(const bool syntaxHighlight);
        const char *severityColorEnd(const bool syntaxHighlight);

};

