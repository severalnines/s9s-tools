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

#include "s9sobject.h"

/**
 * A class that represents a running OS process.
 */
class S9sProcess : public S9sObject
{
    public:
        S9sProcess();
        S9sProcess(const S9sProcess &orig);
        S9sProcess(const S9sVariantMap &properties);

        virtual ~S9sProcess();

        S9sProcess &operator=(const S9sVariantMap &rhs);
        
        virtual S9sString className() const;
        int pid() const;
        S9sString userName() const;
        S9sString hostName() const;
        int priority() const;

        ulonglong virtMem() const;
        S9sString virtMem(const char *ignored) const;

        longlong resMem() const;
        S9sString resMem(const char *ignored) const;

        double cpuUsage() const;
        S9sString cpuUsage(const char *ignored) const;

        double memUsage() const;
        S9sString memUsage(const char *ignored) const;

        S9sString executable() const;
        S9sString state() const;
};

