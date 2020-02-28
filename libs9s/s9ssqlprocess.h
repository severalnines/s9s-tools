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

/*
 * A class that represents a query running inside an SQL server, an SQL process.
 */
class S9sSqlProcess : public S9sObject
{
    public:
        S9sSqlProcess();
        S9sSqlProcess(const S9sSqlProcess &orig);
        S9sSqlProcess(const S9sVariantMap &properties);

        virtual ~S9sSqlProcess();

        S9sSqlProcess &operator=(const S9sVariantMap &rhs);

        static bool 
            compareSqlProcess(
                    const S9sSqlProcess &a,
                    const S9sSqlProcess &b);

        virtual S9sString className() const;
        int pid() const;
        S9sString command() const;
        S9sString userName() const;
        int time() const;
        S9sString instance() const;
        S9sString hostName() const;
        S9sString query(const S9sString &defaultValue) const;
};

