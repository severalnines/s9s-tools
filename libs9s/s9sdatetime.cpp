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
#include "s9sdatetime.h"
#include "S9sVariant"

#include <stdio.h>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

#define SECONDS_IN_ONE_MINUTE 60
#define SECONDS_IN_ONE_HOUR (60 * 60)
#define SECONDS_IN_ONE_DAY (24 * 60 * 60)

static const char *shortMonthNames[] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
    "Oct", "Nov", "Dec", NULL
};

/**
 * This constructor will create an object that holds no date or time, that is
 * invalid.
 */
S9sDateTime::S9sDateTime()
{
    m_timeSpec.tv_sec  = -1;
    m_timeSpec.tv_nsec = 0;
}

S9sDateTime::S9sDateTime(
        time_t theTime)
{
    m_timeSpec.tv_sec  = theTime;
    m_timeSpec.tv_nsec = 0;
}

S9sDateTime::S9sDateTime(
        const S9sDateTime &dt)
{
    m_timeSpec.tv_sec  = dt.m_timeSpec.tv_sec;
    m_timeSpec.tv_nsec = dt.m_timeSpec.tv_nsec;
}

S9sDateTime::~S9sDateTime()
{
}

S9sDateTime 
S9sDateTime::operator+(
        const int seconds) const
{
    S9sDateTime retval = *this;

    retval += seconds;
    return retval;
}

S9sDateTime &
S9sDateTime::operator+=(
        const int seconds)
{
    m_timeSpec.tv_sec  += seconds;

    return *this;
}

S9sDateTime &
S9sDateTime::operator-=(
        const int seconds)
{
    m_timeSpec.tv_sec  -= seconds;

    return *this;
}

/**
 * \param rhs The right-hand-side of the operator.
 *
 * Normal assignment operator that copies the date & time.
 */
S9sDateTime &
S9sDateTime::operator=(
        const S9sDateTime &rhs)
{
    m_timeSpec.tv_sec  = rhs.m_timeSpec.tv_sec;
    m_timeSpec.tv_nsec = rhs.m_timeSpec.tv_nsec;

    return *this;
}

/**
 * \param rhs The right-hand-side of the operator.
 */
bool
S9sDateTime::operator< (
        const time_t &rhs) const
{
    if (rhs < 0 || m_timeSpec.tv_sec < 0)
        return false;

    return m_timeSpec.tv_sec < rhs;
}

/**
 * \param rhs The right-hand-side of the operator.
 */
bool
S9sDateTime::operator> (
        const time_t &rhs) const
{
    if (rhs < 0 || m_timeSpec.tv_sec < 0)
        return false;

    return m_timeSpec.tv_sec > rhs;
}

/**
 * \param rhs The right-hand-side of the operator.
 */
bool
S9sDateTime::operator> (
        const S9sDateTime &rhs) const
{
    if (m_timeSpec.tv_sec < 0)
        return false;

    if (m_timeSpec.tv_sec > rhs.m_timeSpec.tv_sec)
        return true;

    if (m_timeSpec.tv_sec == rhs.m_timeSpec.tv_sec)
        return m_timeSpec.tv_nsec > rhs.m_timeSpec.tv_nsec;

    return false;
}


/**
 * \param rhs The right-hand-side of the operator.
 *
 * Operator to compare two S9sDateTime class objects.
 */
bool
S9sDateTime::operator== (
        const S9sDateTime &rhs) const
{
    return 
        m_timeSpec.tv_sec == rhs.m_timeSpec.tv_sec && 
        m_timeSpec.tv_nsec == rhs.m_timeSpec.tv_nsec; 
}

/**
 * \param rhs The right-hand-side of the operator.
 *
 * Operator to compare two S9sDateTime class objects.
 */
bool
S9sDateTime::operator< (
        const S9sDateTime &rhs) const
{
    if (m_timeSpec.tv_sec < rhs.m_timeSpec.tv_sec)
        return true;

    if (m_timeSpec.tv_sec == rhs.m_timeSpec.tv_sec &&
            m_timeSpec.tv_nsec < rhs.m_timeSpec.tv_nsec)
    {
        return true;
    }

    return false;
}

/**
 * \param rhs The right-hand-side of the operator.
 *
 * Operator to compare two S9sDateTime class objects.
 */
bool
S9sDateTime::operator<= (
        const S9sDateTime &rhs) const
{
    if (m_timeSpec.tv_sec < rhs.m_timeSpec.tv_sec)
        return true;

    if (m_timeSpec.tv_sec == rhs.m_timeSpec.tv_sec)
        return m_timeSpec.tv_nsec <= rhs.m_timeSpec.tv_nsec;

    return false;
}

/**
 * \param rhs The right-hand-side of the operator.
 *
 */
bool
S9sDateTime::operator>= (
        const time_t &rhs) const
{
    if (rhs < 0 || m_timeSpec.tv_sec < 0)
        return false;

    return m_timeSpec.tv_sec >= rhs;
}

/**
 * \param rhs The right-hand-side of the operator.
 *
 */
bool
S9sDateTime::operator>= (
        const S9sDateTime &rhs) const
{
    if (m_timeSpec.tv_sec < 0)
        return false;

    if (m_timeSpec.tv_sec > rhs.m_timeSpec.tv_sec)
        return true;

    if (m_timeSpec.tv_sec == rhs.m_timeSpec.tv_sec)
        return m_timeSpec.tv_nsec >= rhs.m_timeSpec.tv_nsec;

    return false;
}

/**
 * \param rhs The right-hand-side of the operator.
 * \returns the time difference in milliseconds
 */
longlong
S9sDateTime::operator-(
        const S9sDateTime &rhs) const
{
    longlong retval = 1000 * (m_timeSpec.tv_sec - rhs.m_timeSpec.tv_sec);

    retval += (m_timeSpec.tv_nsec - rhs.m_timeSpec.tv_nsec) / 1000000;
    return retval;
}

/**
 * Converts the date&time object to a standard time_t type.
 */
time_t
S9sDateTime::toTimeT() const
{
    return m_timeSpec.tv_sec;
}

S9sDateTime
S9sDateTime::currentDateTime()
{
    S9sDateTime retval;

    if (::clock_gettime(CLOCK_REALTIME, &retval.m_timeSpec) != 0)
    {
        S9S_WARNING ("clock_gettime failure: %m");
    }

    return retval;
}

ulong
S9sDateTime::ellapsedSeconds() const
{
    return time(NULL) - m_timeSpec.tv_sec;
}

int
S9sDateTime::second() const
{
    struct tm *lt = ::localtime(&m_timeSpec.tv_sec);

    return lt->tm_sec;
}

int
S9sDateTime::minute() const
{
    struct tm *lt = ::localtime(&m_timeSpec.tv_sec);

    return lt->tm_min;
}

int
S9sDateTime::hour(
        const bool GMT) const
{
    if (GMT)
    {
        struct tm *gmtTime = ::gmtime(&m_timeSpec.tv_sec);
        return gmtTime->tm_hour;
    }

    struct tm *lt = ::localtime(&m_timeSpec.tv_sec);
    return lt->tm_hour;
}

/**
 * \returns The ISO 8601:1988 week number as a decimal number (1-53).
 *
 */
int
S9sDateTime::weekNumber() const
{
    struct tm    *lt = ::localtime(&m_timeSpec.tv_sec);
    char          buffer[80];
    S9sString     tmp;

    strftime(buffer, sizeof(buffer), "%V", lt);
    tmp = buffer;
    return tmp.toInt();
}

/**
 * \returns the month in the year between 1 and 12
 *
 */
int
S9sDateTime::month() const
{
    struct tm *lt = ::localtime(&m_timeSpec.tv_sec);

    return lt->tm_mon + 1;
}

/**
 * \returns the day in the month between 1 and 31
 *
 */
int
S9sDateTime::day() const
{
    struct tm *lt = ::localtime(&m_timeSpec.tv_sec);

    return lt->tm_mday;
}

/**
 * \returns the year of the date like 2014 (none of that 104 nonsense)
 *
 */
int
S9sDateTime::year() const
{
    struct tm *lt = ::localtime(&m_timeSpec.tv_sec);

    return 1900 + lt->tm_year;
}

/**
 * Sunday = 1, Monday = 2,... Saturday = 7
 */
int
S9sDateTime::weekday() const
{
    struct tm *lt = ::localtime(&m_timeSpec.tv_sec);

    // tm_wday: Sunday = 0, Monday = 1... Saturday = 6
    return lt->tm_wday + 1;
}

/**
 * Jan 01 = 1 , Dec 31  = 365 or 366 if leap year
 */
int
S9sDateTime::yearday() const
{
    struct tm *lt = ::localtime(&m_timeSpec.tv_sec);
    
    // tm_wday: Sunday = 0, Monday = 1... Saturday = 6
    return lt->tm_yday + 1;
}

int
S9sDateTime::currentWeekNumber()
{
    S9sDateTime  dt = currentDateTime();
    struct tm    *lt = ::localtime(&dt.m_timeSpec.tv_sec);
    char          buffer[80];
    S9sString    tmp;

    strftime(buffer, sizeof(buffer), "%V", lt);
    tmp = buffer;

    return tmp.toInt();
}

int
S9sDateTime::previousWeekNumber()
{
    S9sDateTime  dt = time(NULL) - WEEKS_TO_SECONDS(1);
    struct tm    *lt = ::localtime(&dt.m_timeSpec.tv_sec);
    char          buffer[80];
    S9sString    tmp;

    strftime(buffer, sizeof(buffer), "%V", lt);
    tmp = buffer;

    return tmp.toInt();
}

/**
 * \returns The first second of the current 8601:1988 week.
 */
time_t
S9sDateTime::weekStart()
{
    time_t       start = time(NULL);
    S9sDateTime dt;
    int          theWeek;
    int          thisWeek;
    int          delta = WEEKS_TO_SECONDS(1) / 2;

    dt      = start;
    theWeek = dt.weekNumber();

successive_again:
    for (;;)
    {
        dt       = start - delta;
        thisWeek = dt.weekNumber();

        if (thisWeek != theWeek)
            break;

        start   -= delta;
    }

    if (delta > 1)
    {
        delta /= 2;
        goto successive_again;
    }

    return start;
}

S9sString
S9sDateTime::toString() const
{
    return toString(TzDateTimeFormat);
}

/**
 * Converts the date&time to a string.
 */
S9sString
S9sDateTime::toString(
        S9sDateTime::DateTimeFormat format) const
{
    struct tm *lt = ::localtime(&m_timeSpec.tv_sec);
    S9sString retval;

    switch (format)
    {
        case FileNameFormat:
            retval.sprintf("%04d-%02d-%02d_%02d%02d%02d",
                    lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                    lt->tm_hour, lt->tm_min, lt->tm_sec);
            break;

        case ShortDayFormat:

            retval.sprintf("%d%02d%02d",
                    lt->tm_year - 100, lt->tm_mon + 1, lt->tm_mday);
            break;

        case ShortDateFormat:
            {
                char buffer[80];
                strftime(buffer, sizeof(buffer), "%x", lt);
                retval = buffer;
            }
            break;

        case LogFileFormat:
            {
                char buffer[80];
                strftime(buffer, sizeof(buffer), "%b %d %H:%M:%S", lt);
                retval = buffer;
            }
            break;

        case MySqlLogFileFormat:
            retval.sprintf("%04d-%02d-%02d %02d:%02d:%02d",
                    lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                    lt->tm_hour, lt->tm_min, lt->tm_sec);
            break;

        case CompactFormat:
            if (isToday())
            {
                char buffer[80];

                strftime(buffer, sizeof(buffer), "%H:%M:%S", lt);
                retval = buffer;
            } else {
                retval.sprintf("%04d-%02d-%02d %02d:%02d:%02d",
                        lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                        lt->tm_hour, lt->tm_min, lt->tm_sec);
            }
            break;

        case MySqlShortLogFormat:
            retval.sprintf("%2d%02d%02d %2d:%02d:%02d",
                    lt->tm_year - 100, lt->tm_mon + 1, lt->tm_mday,
                    lt->tm_hour, lt->tm_min, lt->tm_sec);
            break;

        case MySqlLogFileDateFormat:
            retval.sprintf("%04d-%02d-%02d",
                   lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday);

            break;

        case MySqlShortLogDateFormat:
            retval.sprintf("%2d%02d%02d",
                    lt->tm_year - 100, lt->tm_mon + 1, lt->tm_mday);
            break;

        case ShortTimeFormat:
            {
                char buffer[80];
                strftime(buffer, sizeof(buffer), "%H:%M", lt);
                retval = buffer;
            }
            break;

        case LongTimeFormat:
            {
                char buffer[80];
                strftime(buffer, sizeof(buffer), "%H:%M:%S", lt);
                retval = buffer;
            }
            break;

        case LocalDateTimeFormat:
            {
                char buffer[80];
                strftime(buffer, sizeof(buffer), "%c", lt);
                retval = buffer;
            }
            break;

        case EmailDateTimeFormat:
            {
                char buffer[120];
                strftime(buffer, sizeof(buffer), 
                        "%a, %d %b %Y %H:%M:%S %z", lt);

                retval = buffer;
            }
            break;

        case TzDateTimeFormat:
            {
                char       buffer[120];
                size_t     size = sizeof(buffer);
                S9sString millisecs;

                // 
                // Here we change to GTMT and format that.
                // 
                struct tm *lt = ::gmtime(&m_timeSpec.tv_sec);
                    
                strftime(buffer, size, "%Y-%m-%dT%H:%M:%S", lt);
                millisecs.sprintf(".%03d", m_timeSpec.tv_nsec / 1000000);

                retval  = buffer;
                retval += millisecs;
                retval += "Z";
            }
            break;

    }
    return retval;
}

/**
 * This is the familiar formatted toString() method we use for all the objects
 * registered as meta values. The libc however has its own date&time formatting
 * function which we re-use here, so we don't have to implement our own.
 */
S9sString
S9sDateTime::toString(
        const S9sString &formatString) const
{
    // FIXME: finite buffer size!
    size_t     bufferSize = 1024;
    char       buffer[bufferSize];
    struct tm *lt = ::localtime(&m_timeSpec.tv_sec);

    ::strftime(buffer, bufferSize, STR(formatString), lt);
    return S9sString(buffer);
}


/**
 * \returns the name of the time zone used by the computer that runs the
 *   program
 */
S9sString
S9sDateTime::timeZoneName()
{
    tzset();
    return S9sString(tzname[0]);
}

/**
 * \returns how many seconds must be added to the local time to get UTC
 * (Coordinated Universal Time). CET for example is 1 hour ahead of UTC and so
 * the return value is -3600.
 */
int
S9sDateTime::timeZone()
{
    tzset();
    return (int) timezone;
}

/**
 * \returns how many seconds must be added to the local time because of the
 * daylight saving time
 */
int
S9sDateTime::dayLight()
{
    tzset();
    return daylight * 60 * 60;
}

/**
 * \param input the input line to be parsed
 * \param length the place where the method returns how many characters are
 *   parsed or NULL
 * \returns true if the string is successfully parsed
 *
 */
bool
S9sDateTime::parse(
        const S9sString &input,
        int              *length)
{
    bool retval = false;
    if (parseLogFileFormat(input, length))
        retval = true;
    else if (parseMySqlLogFileFormat(input, length))
        retval = true;
    else if (parseMySqlShortLogFormat(input, length))
        retval = true;
    else if (parseMySqlShortLogFormatNoLeadZero(input, length))
        retval = true;
    else if (parseTzFormat(input, length))
        retval = true;

    S9S_DEBUG("***  input : %s", STR(input));
    S9S_DEBUG("*** retval : %s", retval ? "true" : "false");
    S9S_DEBUG("***  value : %s", STR(toString()));
    return retval;
}

S9sString
S9sDateTime::secondsToUiString(
        int seconds)
{
    S9sString retval;
    int        days, hours, minutes;

    days = seconds / SECONDS_IN_ONE_DAY;
    seconds -= days * SECONDS_IN_ONE_DAY;

    hours = seconds / SECONDS_IN_ONE_HOUR;
    seconds -= hours * SECONDS_IN_ONE_HOUR;

    minutes = seconds / SECONDS_IN_ONE_MINUTE;
    seconds -= minutes * SECONDS_IN_ONE_MINUTE;

    if (days == 1 && hours == 0 && minutes == 0 && seconds == 0)
    {
        retval = "one day";
    } else if (days > 0)
    {
        retval = "more than one day";
    } else if (hours == 1 && minutes == 0 && seconds == 0)
    {
        retval = "one hour";
    } else if (hours > 1 && minutes == 0 && seconds == 0)
    {
        retval.sprintf("%d hours", hours);
    } else if (hours == 1)
    {
        retval = "more than one hour";
    } else if (hours > 1)
    {
        retval.sprintf("more than %d hours", hours);
    } else if (minutes > 1 && seconds == 0)
    {
        retval.sprintf("%d minutes", minutes);
    } else if (minutes == 1 && seconds == 0)
    {
        retval = "one minute";
    } else if (minutes > 1)
    {
        retval.sprintf("more than %d minutes", minutes);
    } else if (minutes == 1)
    {
        retval = "more than one minute";
    } else if (seconds == 1)
    {
        retval.sprintf("one second");
    } else
    {
        retval.sprintf("%d seconds", seconds);
    }

    return retval;
}

/**
 * \returns true if the string is parsed using the MySqlLogFileFormat.
 *
 * This method assumes the string starts with a date time in a
 * MySqlLogFileFormat (e.g. "2014-03-17 10:15:12") and will parse the values if
 * so.
 *
 * These parsing methods need to be fast, so we are using only low level stuff
 * here.
 */
bool
S9sDateTime::parseMySqlLogFileFormat(
        const S9sString &input,
        int              *length)
{
    bool retval = false;
    int  year;
    int  month;
    int  monthDay;
    int  hour;
    int  minute;
    int  second;

    if (input.length() < 19)
        return retval;

    // parsing the year
    if (!isdigit(input[0]) ||
            !isdigit(input[1]) ||
            !isdigit(input[2]) ||
            !isdigit(input[3]))
        return retval;

    if (input[4] != '-' && input[4] != '/')
        return retval;

    year = 1000 * (input[0] - '0') +
        100 * (input[1] - '0') +
        10 * (input[2] - '0') +
        (input[3] - '0');

    // the month
    if (!isdigit(input[5]) || !isdigit(input[6]))
        return false;

    if (input[7] != '-' && input[7] != '/')
        return false;

    month = 10 * (input[5] - '0') + (input[6] - '0');

    // The day
    if (!isdigit(input[8]) ||
            !isdigit(input[9]) ||
            input[10] != ' ')
        return false;

    monthDay = 10 * (input[8] - '0') + (input[9] - '0');

    // The hour
    if (!isdigit(input[11]) ||
            !isdigit(input[12]) ||
            input[13] != ':')
        return false;

    hour = 10 * (input[11] - '0') + (input[12] - '0');

    // The minute
    if (!isdigit(input[14]) ||
            !isdigit(input[15]) ||
            input[16] != ':')
        return false;

    minute = 10 * (input[14] - '0') + (input[15] - '0');

    // The second
    if (!isdigit(input[17]) ||
            !isdigit(input[18]))
        return false;

    second = 10 * (input[17] - '0') + (input[18] - '0');

    //
    // Transforming and checking.
    //
    struct tm builtTime;
    time_t    theTime;

    builtTime.tm_year  = year - 1900;
    builtTime.tm_mon   = month - 1;
    builtTime.tm_mday  = monthDay;
    builtTime.tm_hour  = hour;
    builtTime.tm_min   = minute;
    builtTime.tm_sec   = second;
    builtTime.tm_isdst = -1;

    theTime = mktime(&builtTime);
    if (theTime >= 0)
    {
        m_timeSpec.tv_sec  = theTime;
        m_timeSpec.tv_nsec = 0;
        retval = true;
    }

    if (retval && length != NULL)
        *length = 19;

    return retval;
}

/**
 * \param input The input line to be parsed
 * \param length The place where the method returns how many characters are
 *   parsed or NULL
 * \returns true if the string is parsed using the MySqlShortLogFormat.
 *
 * This method assumes the string starts with a date time in a
 * MySqlShortLogFormat (e.g. "140415  0:44:42") and will parse the values if so.
 * The parsing will be done in a case-insensitive manner.
 *
 * These parsing methods need to be fast, so we are using only low level stuff
 * here.
 */
bool
S9sDateTime::parseMySqlShortLogFormat(
        const S9sString &input,
        int              *length)
{
    bool retval = false;
    int  year;
    int  month;
    int  monthDay;
    int  hour;
    int  minute;
    int  second;

    if (input.length() < 15)
        return retval;

    // parsing the year
    if (!isdigit(input[0]) || !isdigit(input[1]))
        return retval;

    year = 10 * (input[0] - '0') + (input[1] - '0') + 2000;

    // the month
    if (!isdigit(input[2]) || !isdigit(input[3]))
        return false;

    month = 10 * (input[2] - '0') + (input[3] - '0');

    // The day
    if (!isdigit(input[4]) || !isdigit(input[5]))
        return false;

    monthDay = 10 * (input[4] - '0') + (input[5] - '0');

    // The hour
    if ((!isdigit(input[7]) && input[7] != ' ') ||
            !isdigit(input[8]) ||
            input[9] != ':')
        return false;

    hour = input[8] - '0';
    if (input[7] != ' ')
        hour += 10 * (input[7] - '0');

    // The minute
    if (!isdigit(input[10]) ||
            !isdigit(input[11]) ||
            input[12] != ':')
        return false;

    minute = 10 * (input[10] - '0') + (input[11] - '0');

    // The second
    if (!isdigit(input[13]) ||
            !isdigit(input[14]))
        return false;

    second = 10 * (input[13] - '0') + (input[14] - '0');

    //
    // Transforming and checking.
    //
    struct tm builtTime;
    time_t    theTime;

    builtTime.tm_year  = year - 1900;
    builtTime.tm_mon   = month - 1;
    builtTime.tm_mday  = monthDay;
    builtTime.tm_hour  = hour;
    builtTime.tm_min   = minute;
    builtTime.tm_sec   = second;
    builtTime.tm_isdst = -1;

    theTime = mktime(&builtTime);
    if (theTime >= 0)
    {
        m_timeSpec.tv_sec  = theTime;
        m_timeSpec.tv_nsec = 0;
        retval = true;
    }

    if (retval && length != NULL)
        *length = 15;

    return retval;
}

/**
 *         7
 * "130516 9:36:25"
 */
bool
S9sDateTime::parseMySqlShortLogFormatNoLeadZero(
        const S9sString &input,
        int              *length)
{
    bool retval = false;
    int  year;
    int  month;
    int  monthDay;
    int  hour;
    int  minute;
    int  second;

    if (input.length() < 14)
        return retval;

    // parsing the year
    if (!isdigit(input[0]) || !isdigit(input[1]))
        return retval;

    year = 10 * (input[0] - '0') + (input[1] - '0') + 2000;

    // the month
    if (!isdigit(input[2]) || !isdigit(input[3]))
        return false;

    month = 10 * (input[2] - '0') + (input[3] - '0');

    // The day
    if (!isdigit(input[4]) || !isdigit(input[5]))
        return false;

    monthDay = 10 * (input[4] - '0') + (input[5] - '0');

    // The hour
    if (!isdigit(input[7]) || input[8] != ':')
        return false;

    hour = input[7] - '0';

    // The minute
    if (!isdigit(input[9]) ||
            !isdigit(input[10]) ||
            input[11] != ':')
        return false;

    minute = 10 * (input[9] - '0') + (input[10] - '0');

    // The second
    if (!isdigit(input[12]) ||
            !isdigit(input[13]))
        return false;

    second = 10 * (input[12] - '0') + (input[13] - '0');

    //
    // Transforming and checking.
    //
    struct tm builtTime;
    time_t    theTime;

    builtTime.tm_year  = year - 1900;
    builtTime.tm_mon   = month - 1;
    builtTime.tm_mday  = monthDay;
    builtTime.tm_hour  = hour;
    builtTime.tm_min   = minute;
    builtTime.tm_sec   = second;
    builtTime.tm_isdst = -1;

    theTime = mktime(&builtTime);
    if (theTime >= 0)
    {
        m_timeSpec.tv_sec  = theTime;
        m_timeSpec.tv_nsec = 0;
        retval = true;
    }

    if (retval && length != NULL)
        *length = 14;

    return retval;
}

/**
 * Example: "2016-06-06T11:47:39.500Z"
 * FIXME: The time zone information is not parsed here (except Z). There could
 * be +hh:mm or -hh:mm for example.
 */
bool
S9sDateTime::parseTzFormat(
        const S9sString &input,
        int              *length)
{
    bool retval = false;
    int  year;
    int  month;
    int  monthDay;
    int  hour;
    int  minute;
    int  second;
    int  fields;
    int  timezone = 0;
    int  milliseconds;

    fields = sscanf(STR(input), "%4d-%02d-%02dT%02d:%02d:%02d.%dZ", 
            &year, &month, &monthDay,
            &hour, &minute, &second,
            &milliseconds);

    if (fields != 7)
    {
        return false;
    }

    S9S_WARNING("*** fields       : %d", fields);
    S9S_WARNING("*** year         : %d", year);
    S9S_WARNING("*** month        : %d", month);
    S9S_WARNING("*** monthDay     : %d", monthDay);
    S9S_WARNING("*** hour         : %d", hour);
    S9S_WARNING("*** minute       : %d", minute);
    S9S_WARNING("*** second       : %d", second);
    S9S_WARNING("*** milliseconds : %d", milliseconds);
    S9S_WARNING("*** timezone     : %d", timezone);

    //
    // Transforming and checking.
    //
    struct tm builtTime;
    time_t    theTime;

    builtTime.tm_year  = year - 1900;
    builtTime.tm_mon   = month - 1;
    builtTime.tm_mday  = monthDay;
    builtTime.tm_hour  = hour;
    builtTime.tm_min   = minute;
    builtTime.tm_sec   = second;
    builtTime.tm_isdst = -1;

    theTime = mktime(&builtTime);
    if (theTime >= 0)
    {
        m_timeSpec.tv_sec  = theTime;
        m_timeSpec.tv_nsec = milliseconds * 1000000;
        retval = true;
    
        timezone /= 100;
        timezone *= 60 * 60;
        
        S9S_WARNING("*** timezone     : %d", timezone);
        S9S_WARNING("*** timeZone()   : %d", timeZone());
        S9S_WARNING("*** dayLight()   : %d", dayLight());
        S9S_WARNING("*** mktime()     : %s", S9S_TIME_T(m_timeSpec.tv_sec));

        m_timeSpec.tv_sec += timezone;
        m_timeSpec.tv_sec -= timeZone();

        
        if (builtTime.tm_isdst)
            m_timeSpec.tv_sec += dayLight();
    }

    if (retval && length != NULL)
        *length = 26;

    return retval;
}

S9sString 
S9sDateTime::elapsedTime(
        time_t elapsedSeconds)
{
    int        hours, minutes, seconds;
    S9sString retval;

    hours = elapsedSeconds / (60 * 60);
    elapsedSeconds -= hours * 60 * 60;

    minutes = elapsedSeconds / 60;
    elapsedSeconds -= minutes * 60;

    seconds = elapsedSeconds;

    retval.sprintf("%02d:%02d:%02d", hours, minutes, seconds);

    return retval;
}


/**
 * \param input the input line to be parsed
 * \length the place where the method returns how many characters are parsed or
 *   NULL
 * \returns true if the string is parsed using the LogFileFormat.
 *
 * This method assumes the string starts with a date time in a LogFileFormat
 * (e.g. "May 14 14:59:21") and will parse the values if so. The parsing will be
 * done in a case-insensitive manner.
 *
 * These parsing methods need to be fast, so we are using only low level stuff
 * here.
 */
bool
S9sDateTime::parseLogFileFormat(
        const S9sString &input,
        int              *length)
{
    S9sString s = input.toLower();
    bool       retval = false;

    int        month    = -1;
    int        monthDay = -1;
    int        hour     = -1;
    int        minute   = -1;
    int        second   = -1;

    if (s.length() < 15)
        return retval;

    // Parsing the month name.
    for (int idx = 0; shortMonthNames[idx] != NULL; ++idx)
    {
        S9sString monthName = shortMonthNames[idx];

        if (s.startsWith(STR(monthName.toLower())))
        {
            month = idx;
            break;
        }
    }

    if (month < 0)
        return retval;

    if (s[3] != ' ')
        return retval;

    // The day of the month: one or two digit decimal number.
    if (
        (!isdigit(s[4]) && s[4] != ' ') ||
        !isdigit(s[5]) || s[6] != ' ')
        return retval;

    monthDay = (s[5] - '0');
    if (s[4] != ' ')
        monthDay += 10 * (s[4] - '0');

    // The hour
    if (!isdigit(s[7]) || !isdigit(s[8]) || s[9] != ':')
        return retval;

    hour = 10 * (s[7] - '0') + (s[8] - '0');

    // The minute
    if (!isdigit(s[10]) || !isdigit(s[11]) || s[12] != ':')
        return retval;

    minute = 10 * (s[10] - '0') + (s[11] - '0');

    // The second
    if (!isdigit(s[13]) || !isdigit(s[14]))
        return retval;

    second = 10 * (s[13] - '0') + (s[14] - '0');

    //
    // Transforming and checking
    //
    S9sDateTime now = S9sDateTime::currentDateTime();
    struct tm builtTime;
    time_t    theTime;

    builtTime.tm_year  = now.year() - 1900;
    builtTime.tm_mon   = month;
    builtTime.tm_mday  = monthDay;
    builtTime.tm_hour  = hour;
    builtTime.tm_min   = minute;
    builtTime.tm_sec   = second;
    builtTime.tm_isdst = -1;

    theTime = mktime(&builtTime);
    if (theTime >= 0)
    {
        m_timeSpec.tv_sec  = theTime;
        m_timeSpec.tv_nsec = 0;
        retval = true;
    }

    if (retval && length != NULL)
        *length = 15;

    return retval;
}

S9sVariantMap
S9sDateTime::toVariantMap() const
{
    S9sVariantMap theMap;
    
    theMap["tv_sec"]  = (ulonglong) m_timeSpec.tv_sec;
    theMap["tv_nsec"] = (ulonglong) m_timeSpec.tv_nsec;

    return theMap;
}

void
S9sDateTime::setFromVariantMap(
        S9sVariantMap theMap)
{
    m_timeSpec.tv_sec  = theMap["tv_sec"].toULongLong();
    m_timeSpec.tv_nsec = theMap["tv_nsec"].toInt();
}

/**
 * Static function to parse ISO 8601 format string that represent time
 * durations.
 */
enum Iso8601State
{
    ExpectP,
    ExpectNumber,
    ExpectDecimal,
    ExpectSuffix,
    MayEnd
};

/**
 * \param input the string to parse
 * \param value the place where the method returns he value in seconds
 * \returns true if the input successfuly parsed, false on an error
 *
 * This will parse ISO 8601 duration strings like "PT15.5S" or "P1Y1M1DT15.5S".
 *
 * * Here is a little calculator to generate ISO 8601 format strings and back:
 * http://www.ostyn.com/standards/scorm/samples/ISOTimeForSCORM.htm
 */
bool 
S9sDateTime::parseIso8601(
        const S9sString &input, 
        double           &value)
{
    int          n = 0;
    S9sString    number;
    Iso8601State state = ExpectP;
    bool         hasT = false;

    value = 0;
    if (input.empty())
        return false;

    for (;;)
    {
        char c;

        c = input[n];

        switch (state)
        {
            case ExpectP:
                if (c == 'P')
                {
                    state = ExpectNumber;
                    n++;
                } else {
                    return false;
                }
                break;

            case ExpectNumber:
                if (c == 'T')
                {
                    if (hasT)
                        return false;

                    hasT = true;
                    ++n;
                } else if (c >= '0' && c <= '9')
                {
                    number += c;
                    ++n;
                } else if (c == '.')
                {
                    state = ExpectDecimal;
                    number += c;
                    ++n;
                } else {
                    state = ExpectSuffix;
                }
                break;

            case ExpectDecimal:
                if (c >= '0' && c <= '9')
                {
                    number += c;
                    ++n;
                } else {
                    state = ExpectSuffix;
                }
                break;

            case ExpectSuffix:
                if (c == 'S')
                {
                    value   += number.toDouble();
                    number   = "";

                    state = MayEnd;
                    ++n;
                } else if (c == 'M')
                {
                    if (hasT)
                        value += number.toDouble() * 60.0;
                    else 
                        value += number.toDouble() * 2629800.0;

                    number   = "";

                    state = MayEnd;
                    ++n;
                } else if (c == 'H')
                {
                    value   += number.toDouble() * 60.0 * 60.0;
                    number   = "";

                    state = MayEnd;
                    ++n;
                } else if (c == 'D')
                {
                    value   += number.toDouble() * 60.0 * 60.0 * 24.0;
                    number   = "";

                    state = MayEnd;
                    ++n;
                } else if (c == 'Y')
                {
                    value   += number.toDouble() * 31557600.0;
                    number   = "";

                    state = MayEnd;
                    ++n;
                }
                break;

            case MayEnd:
                if (c == '\0')
                    return true;

                state = ExpectNumber;
                break;
        }
    }

    return true;
}

double
S9sDateTime::milliseconds(
        const S9sDateTime &time1,
        const S9sDateTime &time2)
{
    double retval;
    double millisec;

    retval = 
        (double) time1.m_timeSpec.tv_sec -
        (double) time2.m_timeSpec.tv_sec;

    millisec = 
        (double) time1.m_timeSpec.tv_nsec - 
        (double) time2.m_timeSpec.tv_nsec;

    millisec /= 1000000.0;

    return retval * 1000.0 + millisec;
}

bool
S9sDateTime::isToday() const
{
    S9sDateTime today = S9sDateTime::currentDateTime();

    return 
        today.year()  == year() &&
        today.month() == month() &&
        today.day()   == day();
}
