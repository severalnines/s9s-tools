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
 * along with S9sTools. If not, see <http://www.gnu.org/licenses/>.
 */
#include "s9sstring.h"

#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <algorithm>

#include "S9sRegExp"

const S9sString S9sString::space = " ";
const S9sString S9sString::dash  = "-";


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
S9sString::operator=(
        const std::string &rhs)
{
    if (this == &rhs)
        return *this;

    std::string::operator=(rhs);
    return *this;
}

S9sString 
S9sString::operator*(
        const int rhs) const
{
    S9sString retval;

    if (rhs > 0)
    {
        for (int n = 0; n < rhs; ++n)
            retval += *this;
    }

    return retval;
}


/**
 * \returns the variant list contains the parts of the original string.
 *
 * Splits the string into words using ";, " as delimiters. These delimiters are
 * used in config files, so this method is made to process config files.
 *
 * This is an important function, we use this to split command line strings into
 * pieces so that the user need to type less by providing lists (e.g. host 
 * names) in a simple string.
 */
S9sVariantList
S9sString::split(
        const char *ifs) const
{
    S9sVariantList retval;

    if (empty())
        return retval;

    char *copy = strdup(this->c_str());
    char *x = copy;
    char *dd;

    dd = strtok(x, ifs);
    while (dd != NULL)
    {
        retval.push_back(dd);
        dd = strtok(NULL, ifs);
    }

    free(copy);
    return retval;
}

S9sString &
S9sString::appendWord(
        const S9sString &word)
{
    if (empty())
    {
        *this = word;
        return *this;
    }

    if (!word.empty())
    {
        if (!endsWith(" "))
            *this += " ";

        *this += word;
    }

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
 *
 * This function will convert the string to an integer. Should there be extra
 * characters after the integer part (e.g. "12.06") the first field will only be
 * considered.
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

#if 0
bool
S9sString::regMatch(
        const S9sString &regExp) const
{
    regex_t    preg;
    size_t     nmatch = 10;
    regmatch_t pmatch[11];
    int        retval;

    if (regcomp(&preg, STR(regExp), REG_EXTENDED) != 0) 
    {
        S9S_WARNING("ERROR in regular expression.");
        return false;
    }

    retval = regexec(&preg, this->c_str(), nmatch, pmatch, 0) == 0; 
    regfree(&preg);

    return retval != 0;
}

bool
S9sString::regMatch(
        const S9sString &regExp,
        S9sString       &matched) const
{
    regex_t    preg;
    size_t     nmatch = 2;
    regmatch_t pmatch[2];
    int        retval;

    matched.clear();
    if (regcomp(&preg, STR(regExp), REG_EXTENDED) != 0) 
    {
        S9S_WARNING("ERROR in regular expression.");
        return false;
    }

    retval = regexec(&preg, this->c_str(), nmatch, pmatch, 0) == 0; 
    if (retval != 0 && 
            pmatch[1].rm_so != -1 &&
            pmatch[1].rm_eo != -1)
    {
        matched = this->substr(
                pmatch[1].rm_so,
                pmatch[1].rm_eo - pmatch[1].rm_so);
    }

    regfree(&preg);

    return retval;
}

bool
S9sString::regMatch(
        const S9sString &regExp,
        S9sString       &matched1,
        S9sString       &matched2) const
{
    regex_t    preg;
    size_t     nmatch = 3;
    regmatch_t pmatch[3];
    int        retval;

    matched1.clear();
    matched2.clear();
    if (regcomp(&preg, STR(regExp), REG_EXTENDED) != 0) 
    {
        S9S_WARNING("ERROR in regular expression.");
        return false;
    }

    retval = regexec(&preg, this->c_str(), nmatch, pmatch, 0) == 0; 
    if (retval != 0 && 
            pmatch[1].rm_so != -1 &&
            pmatch[1].rm_eo != -1)
    {
        matched1 = this->substr(
                pmatch[1].rm_so,
                pmatch[1].rm_eo - pmatch[1].rm_so);
    }

    if (retval != 0 && 
            pmatch[2].rm_so != -1 &&
            pmatch[2].rm_eo != -1)
    {
        matched2 = this->substr(
                pmatch[2].rm_so,
                pmatch[2].rm_eo - pmatch[2].rm_so);
    }

    regfree(&preg);

    return retval;
}
#endif

void
S9sString::replace(
        size_t             pos, 
        size_t             len,
        const S9sString   &str)
{
    std::string::replace(pos, len, str);
}

void
S9sString::replace(
        S9sRegExp &regExp,
        S9sString  replacement)
{
    regExp.replace(*this, replacement);
}

/**
 * \returns the same string without the quotation marks at the beginning and at
 *   the end.
 *
 * This function checks if the *same* quotation mark can be found at both the
 * beginning and the end and removes them if so. Does not change the original
 * string but returns a new, modified string instead.
 */
S9sString
S9sString::unQuote() const
{
    S9sString retval = *this;

    if (retval.length() < 2)
        return retval;

    if ((retval[0] == '"' && retval[retval.length() - 1] == '"') ||
        (retval[0] == '\'' && retval[retval.length() - 1] == '\'')) 
    {
        retval = substr(1, retval.length() - 2);
    }

    return retval;
}

/**
 * \returns the same string converted to upper-case.
 */
S9sString
S9sString::toUpper() const
{
    S9sString retval = *this;

    std::transform(retval.begin(), retval.end(), retval.begin(), ::toupper);
    return retval;
}

/**
 * \returns the same string converted to lower-case.
 */
S9sString
S9sString::toLower() const
{
    S9sString retval = *this;

    std::transform(retval.begin(), retval.end(), retval.begin(), ::tolower);
    return retval;
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

/**
 * \returns true if a string is a representation of an integer number, and so
 *   the string can be converted to an integer
 */
bool
S9sString::looksInteger() const
{
    longlong  value;
    char      *endptr = NULL;

    if (empty())
        return false;

    value = strtoll(c_str(), &endptr, 10);
    if (endptr != NULL && *endptr != '\0')
        return false;

    if (value < INT_MIN || value > INT_MAX)
        return false;

    return true;

}

/**
 * A string looks like an ulonglong if it can be converted to a ulonglong, but
 * it can't be converted into an int.
 */
bool
S9sString::looksULongLong() const
{
    ulonglong  value;
    char      *endptr;

    if (empty())
        return false;

    // check if it can't be an unsigned
    if (startsWith("-"))
        return false;

    value = strtoull(c_str(), &endptr, 10);
    if (endptr != NULL && *endptr != '\0')
        return false;

    if (value <= INT_MAX)
        return false;

    return true;
}

S9sString
S9sString::pastTime(
        const time_t theTime)
{
    time_t    now = time(NULL);
    int       diff = now - theTime;
    S9sString retval;

    if (theTime == 0)
    {
        retval = "Never";
    } else if (diff == 0) 
    {
        retval = "Just now";
    } else {
        retval.sprintf("%d seconds ago", diff);
    }

    return retval;
}
