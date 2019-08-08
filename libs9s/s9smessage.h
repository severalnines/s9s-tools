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
#include <S9sDateTime>

/**
 * Represents human readable messages that are in the log or in the job message
 * log.
 *
 * Here is how a message coming from the log looks like:
 * \code{.js}
 * {
 *   "class_name": "CmonLogMessage",
 *   "component": "ClusterConfiguration",
 *   "created": "2019-08-08T06:45:00.728Z",
 *   "log_class": "LogMessage",
 *   "log_id": 133,
 *   "log_origins": {
 *     "sender_binary": "cmon",
 *     "sender_file": "cmonhostmanager.cpp",
 *     "sender_line": 619,
 *     "sender_pid": 40692,
 *     "tv_nsec": 728473605,
 *     "tv_sec": 1565246700
 *   },
 *   "log_specifics": {
 *     "cluster_id": 1,
 *     "message_text": "Registering CmonMySqlHost: 192.168.0.76:3306"
 *   },
 *   "severity": "LOG_DEBUG"
 * }
 * \endcode
 *
 * And here is a message that comes from a job:
 * \code{.js}
 * {
 *   "class_name": "CmonJobMessage",
 *   "created": "2019-08-08T06:33:34.000Z",
 *   "file_name": "Communication.cpp",
 *   "job_id": 1,
 *   "line_number": 6665,
 *   "message_id": 6,
 *   "message_status": "JOB_SUCCESS",
 *   "message_text": "Checking ssh/sudo on 3 hosts."
 * }
 * \endcode
 */
class S9sMessage
{
    public:
        S9sMessage();
        S9sMessage(const S9sVariantMap &properties);

        virtual ~S9sMessage();
        
        S9sMessage &operator=(const S9sVariantMap &rhs);

        const S9sVariantMap &toVariantMap() const;

        
        bool hasFileName() const;
        S9sString fileName() const;

        #ifdef LOG_FUNCNAMES_TO_JOBLOG
        bool hasFunctionName() const;
        S9sString functionName() const;
        #endif

        bool hasLineNumber() const;
        int lineNumber() const;

        int clusterId() const;
        int messageId() const;
        int jobId() const;
        S9sString logClass() const;


        S9sDateTime created() const;
        S9sString severity() const;

        void wrap();
        S9sString message() const;

        bool isError() const;

        S9sString toString() const;
        S9sString toString(
                const bool       syntaxHighlight,
                const S9sString &formatString) const;

        S9sString termColorString() const;

    private:
        S9sVariantMap    m_properties;
};
