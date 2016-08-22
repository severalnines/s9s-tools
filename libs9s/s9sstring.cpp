/*
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "s9sstring.h"

#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

S9sString::S9sString() :
    std::string()
{
}

S9sString::S9sString(
        const char *str) :
    std::string()
{
    if (str == NULL)
        std::string::operator=("");
    else
        std::string::operator=(str);
}

S9sString::S9sString(
        const std::string &str) :
    std::string(str)
{
}

S9sString &
S9sString::operator= (
        const S9sString &rhs)
{
    std::string::operator=(rhs);

    return *this;
}

/**
 * This method is protected against segfaults when it received a NULL pointer
 * as argument. That's a very annoying issue with std::string, especially
 * considering that the mysql library is sending NULL pointers.
 */
S9sString &
S9sString::operator= (
        const char *rhs)
{
    if (rhs == NULL)
        std::string::operator=("");
    else
        std::string::operator=(rhs);

    return *this;
}

S9sString &
S9sString::operator= (
        const std::string &rhs)
{
    if (this == &rhs)
        return *this;

    std::string::operator=(rhs);
    return *this;
}

/**
 * \param formatString a standard printf() style formatstring.
 */
void
S9sString::sprintf(
        const char *formatString,
        ...)
{
    va_list arguments;

    va_start(arguments, formatString);
    vsprintf (formatString, arguments);
    va_end(arguments);
}

/**
 * \param formatString a standard printf() style formatstring.
 * \param arguments standard printf() style arguments.
 */
void
S9sString::vsprintf(
        const char *formatString,
        va_list     arguments)
{
    int     nPrinted;
    size_t  bufferLength = 512;
    char   *buffer = (char *) malloc(bufferLength);

    va_list argsCopy;
    va_copy(argsCopy, arguments);

    nPrinted = vsnprintf(buffer, bufferLength, formatString, arguments);

    if (nPrinted >= (int)bufferLength)
    {
        buffer = (char *) realloc(buffer, nPrinted + 1);
        bufferLength = nPrinted + 1;

        // must use the copy second time, as we already used the original
        vsnprintf(buffer, bufferLength, formatString, argsCopy);
    }

    va_end(argsCopy);

    *this = buffer;

    if (buffer)
        free(buffer);
}

/**
 * \param defaultVal the default value to return if the conversion fails
 * \returns the integer value of a string
 */
int
S9sString::toInt(
        const int defaultVal) const
{
    if (c_str() != NULL && !empty())
    {
        char *endptr;
        int   base = 0;
        long val = strtol(c_str(), &endptr, base);

        if (val <= INT_MAX && val >= INT_MIN)
            return val;
    }

    return defaultVal;
}

/**
 * \param defaultVal the default value to return if the conversion fails
 * \returns the integer value of a string
 */
ulonglong
S9sString::toULongLong(
        const ulonglong defaultVal) const
{
    if (c_str() != NULL && !empty())
        return strtoull(c_str(), NULL, 0);

    return defaultVal;
}

/**
 * \param defaultVal the value to return when the conversion fails.
 * \returns the string converted to a double.
 */
double
S9sString::toDouble(
        const double defaultVal) const
{
    if (c_str() != NULL)
        return atof(c_str());

    return defaultVal;
}

/**
 * \returns the value of the string converted to a boolean.
 *
 * This is the "official way" to convert a string to boolean. 
 */
bool
S9sString::toBoolean() const
{
    std::string trimmed = trim();

    if (trimmed.empty())
        return false;

    if (!strcasecmp(trimmed.c_str(), "yes") ||
        !strcasecmp(trimmed.c_str(), "true") ||
        !strcasecmp(trimmed.c_str(), "t"))
        return true;

    if (!strcasecmp(trimmed.c_str(), "on"))
        return true;

    if (atoi(trimmed.c_str()) != 0)
        return true;

    return false;
}

/**
 * \param from a string to find and remove
 * \param to a string to insert 
 *
 * Replaces all occurances of the a string with an other s tring.
 */
void
S9sString::replace(
        const S9sString &from,
        const S9sString &to)
{
    size_t start_pos = 0;

    while ((start_pos = find(from, start_pos)) != std::string::npos) 
    {
        std::string::replace(start_pos, from.length(), to);
        // In case 'to' contains 'from', like replacing 'x' with 'yx'
        start_pos += to.length(); 
    }
}

/**
 * \returns the string with the escaped special characters
 *
 * Replaces special characters with escaped version.
 */
S9sString
S9sString::escape() const
{
    S9sString retval;

    if (!contains('\'') && !contains('\"'))
    {
        retval = *this;
        return retval;
    }

    bool escaped = false;
    for (uint n = 0; n < length(); ++n)
    {
        char c = at(n);

        if (!escaped && (c == '\'' || c == '\"'))
            retval.append(1, '\\');

        if (c == '\\')
            escaped = !escaped;
        else
            escaped = false;

        retval.append(1, c); 
    }

    return retval;
}

/**
 * \returns the string with the replaced escape substrings.
 *
 * Replaces escaped character notations (e.g. \n or \t) with the binary value
 * (e.g. ASCII 10 or 9).
 */
S9sString
S9sString::unEscape() const
{
    S9sString retval;
   
    if (!contains('\\'))
    {
        retval = *this;
        return retval;
    }

    bool escaped = false;
    for (uint n = 0; n < length(); ++n)
    {
        char c = at(n);

        if (!escaped && c == '\\')
        {
            escaped = true;
            continue;
        }

        if (escaped)
        {
            if (c == '\"')
                retval.append(1, c);
            else if (c == '\\')
                retval.append(1, c);
            else if (c == 'n')
                retval.append(1, '\n');
            else if (c == 'r')
                retval.append(1, '\r');
            else if (c == 't')
                retval.append(1, '\t');
            else if (c == '/')
                retval.append(1, '/');
            else 
                retval.append(1, ' ');

            escaped = false;
            continue;
        }

        retval.append(1, c); 
    }

    return retval;
}

/**
 * \param str the substring to find.
 * \returns true if the string starts with the substring.
 */
bool
S9sString::startsWith(
        const char *str) const
{
    if (str == NULL)
        return false;

    return strncmp(c_str(), str, strlen(str)) == 0;
}

/**
 * \param ending the substring to find.
 * \returns true if the string ends with the substring.
 */
bool 
S9sString::endsWith(
        S9sString const &ending) const
{
    if (length() >= ending.length()) 
    {
        return 0 == compare(
                length() - ending.length(), ending.length(), ending);
    } 
      
    return false;
}

/**
 * \param pWhitespace the string with all the characters considered as white
 *   space characters.
 * \returns the string with the wite space characters removed from the beginning
 *   and the end.
 *
 * Remove (or strip) leading and trailing characters.
 */
S9sString 
S9sString::trim(
        const std::string &pWhitespace) const
{
    S9sString    retval;
    const size_t beginStr = find_first_not_of(pWhitespace);

    if (beginStr == std::string::npos)
    {
        // no content
        return retval;
    }

    const size_t endStr = find_last_not_of(pWhitespace);
    const size_t range = endStr - beginStr + 1;

    retval = substr(beginStr, range);
    return retval;
}

