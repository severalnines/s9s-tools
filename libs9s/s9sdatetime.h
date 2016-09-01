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
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <ctime>

#include "S9sGlobal"
#include "S9sString"
#include "S9sVariantMap"

/**
 * Helper macro to print date & time in debug and log messages.
 */
#define S9S_TIME_T(_value) \
    STR(S9sDateTime((_value)).toString(S9sDateTime::MySqlLogFileFormat))

#define DAYS_TO_SECONDS(_days) ((_days) * 24 * 60 * 60)
#define WEEKS_TO_SECONDS(_weeks) ((_weeks) * 7 * 24 * 60 * 60)
#define MONTHS_TO_SECONDS(_months) ((_months) * 31 * 24 * 60 * 60)
#define YEARS_TO_SECONDS(_years) ((_years) * 365 * 24 * 60 * 60)

/**
 * Basic class to represntent a date-time value.
 */
class S9sDateTime
{
    public:
        enum DateTimeFormat
        {
            /** A format that can be used in file names. */
            FileNameFormat,
            /** Short format, only the date part e.g. "140210" */
            ShortDayFormat,
            /** Format that is used in logs, e.g. "Feb 10 10:04:18" */
            LogFileFormat,
            /** Format used in MySQL logs, e.g. "2014-03-17 10:15:12" */
            MySqlLogFileFormat,
            /** MySQL uses this in log files, e.g "140415  0:44:42" */
            MySqlShortLogFormat,
            /** Format used in MySQL logs, e.g. "2014-03-17" */
            MySqlLogFileDateFormat,
            /** MySQL uses this in log files, e.g "140415" */
            MySqlShortLogDateFormat,
            /** hh:mm */
            ShortTimeFormat,
            /** hh:mm:ss */
            LongTimeFormat,
            /** Locale specific. */
            ShortDateFormat,
            /** Locale specific. */
            LocalDateTimeFormat,
            /** Used for e-mail sending */
            EmailDateTimeFormat,
            /** "2015-11-19T04:46:01.000Z" */
            TzDateTimeFormat,
        };

        S9sDateTime();
        S9sDateTime(time_t theTime);
        S9sDateTime(const S9sDateTime &dt);

        virtual ~S9sDateTime();

        S9sDateTime &operator+=(const int seconds);
        S9sDateTime &operator-=(const int seconds);
        S9sDateTime operator+(const int seconds) const;
        S9sDateTime &operator=(const S9sDateTime &rhs);
        bool operator< (const time_t &rhs) const;
        bool operator> (const time_t &rhs) const;
        bool operator> (const S9sDateTime &rhs) const;
        bool operator>= (const time_t &rhs) const;
        bool operator>= (const S9sDateTime &rhs) const;
        bool operator== (const S9sDateTime &rhs) const;
        bool operator<  (const S9sDateTime &rhs) const;
        bool operator<= (const S9sDateTime &rhs) const;
        longlong operator-(const S9sDateTime &rhs) const;

        time_t toTimeT() const;
        static S9sDateTime currentDateTime();
        ulong ellapsedSeconds() const;
        ulong epochTime() const { return m_timeSpec.tv_sec; };

        virtual S9sString toString() const;

        virtual S9sString toString(
                const S9sString &formatString) const;

        S9sString toString(
                S9sDateTime::DateTimeFormat format) const;

        int second() const;
        int minute() const;
        int hour(const bool GMT = false) const;
        int month() const;
        int day() const;
        int year() const;
        int weekday() const;
        int yearday() const;
        int weekNumber() const;

        static int currentWeekNumber();
        static int previousWeekNumber();
        static time_t weekStart();

        static S9sString secondsToUiString(int seconds);

        bool parse(const S9sString &input, int *length = NULL);

        bool parseLogFileFormat(
                const S9sString &input,
                int *length = NULL);

        bool parseMySqlLogFileFormat(
                const S9sString &input,
                int *length = NULL);

        bool parseMySqlShortLogFormat(
                const S9sString &input,
                int *length = NULL);
        
        bool parseMySqlShortLogFormatNoLeadZero(
                const S9sString &input,
                int *length = NULL);
        
        bool parseTzFormat(
                const S9sString &input,
                int *length = NULL);

        static S9sString timeZoneName();
        static int timeZone();
        static int dayLight();
        static S9sString elapsedTime(time_t timeDiff);

        S9sVariantMap toVariantMap() const;
        void setFromVariantMap(S9sVariantMap theMap);

        static bool parseIso8601(const S9sString &input, double &value);

    private:
        struct timespec m_timeSpec;
};
