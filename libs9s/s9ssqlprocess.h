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
 * A class that represents a query running inside an SQL server, an SQL process.
 *
 * This is how a postgresql process looks like:
 * \code{.js}
 * {
 *   "appName": "psql",
 *   "backendPid": 27963,
 *   "backendStart": "2020-03-09 07:20:30.160186+00",
 *   "class_name": "CmonPostgreSqlDbProcess",
 *   "client": "t7500.homelab.local:45918",
 *   "databaseName": "pipas1_48325",
 *   "elapsedTime": "00:00:00.003684",
 *   "hostId": 2,
 *   "hostname": "192.168.0.227",
 *   "query": "INSERT INTO test_table(NAME, VALUE) VALUES('README.md', 10);",
 *   "queryStart": "2020-03-09 07:20:30.164062+00",
 *   "reportTs": 1583738430,
 *   "state": "active",
 *   "userName": "pipas1_48325",
 *   "waiting": "",
 *   "xactStart": "2020-03-09 07:20:30.164062+00"
 * },
 * \endcode
 *
 * And here is how an SQL process that comes from a MySQL cluster looks like:
 *
 * \code{.js}
 * {
 *   "blocked_by_trx_id": "",
 *   "client": "192.168.0.127:49016",
 *   "command": "Query",
 *   "currentTime": 1583740689,
 *   "db": "pipas1_45463",
 *   "duration": 0,
 *   "host": "192.168.0.127:49016",
 *   "hostId": 1,
 *   "hostname": "192.168.0.227",
 *   "info": "INSERT INTO test_table(NAME, VALUE) VALUES('2018d', 10)",
 *   "innodb_status": "",
 *   "innodb_trx_id": "",
 *   "instance": "192.168.0.227:3306",
 *   "lastseen": 0,
 *   "message": "",
 *   "mysql_trx_id": 1119,
 *   "pid": 1119,
 *   "query": "INSERT INTO test_table(NAME, VALUE) VALUES('2018d', 10)",
 *   "reportTs": 1583740689,
 *   "sql": "",
 *   "state": "Commit",
 *   "time": 0,
 *   "user": "pipas1_45463"
 * }
 * \endcode
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
        
        static bool 
            compareSqlProcessByTime(
                    const S9sSqlProcess &a,
                    const S9sSqlProcess &b);

        virtual S9sString className() const;
        int pid() const;
        S9sString command() const;
        S9sString userName(const S9sString &defaultValue = "-") const;
        int time() const;
        S9sString instance() const;
        S9sString hostName() const;
        S9sString client(const S9sString &defaultValue = "-") const;
        S9sString query(const S9sString &defaultValue = "-") const;
};

