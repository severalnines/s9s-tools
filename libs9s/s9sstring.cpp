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
#include "s9sstring.h"

#include <strings.h>
#include <string.h>
#include <iostream>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "S9sRegExp"

// Let's read in 16KB chunks
#define READ_BUFFER_SIZE 16384

const S9sString S9sString::space = " ";
const S9sString S9sString::dash  = "-";

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"


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

S9sString &
S9sString::operator+=(
        const std::string &rhs)
{
    std::string::operator+=(rhs);
    return *this;
}

S9sString &
S9sString::operator+=(
        const char rhs)
{
    std::string::operator+=(rhs);
    return *this;
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
    vsprintf(formatString, arguments);
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
    if (!empty() && c_str() != NULL)
    {
        S9sString myCopy = *this;
        myCopy.replace(".", decimalSeparator());
        
        return atof(STR(myCopy));
    }

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
    {
        return true;
    }

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

S9sString 
S9sString::baseName() const
{
    S9sString retval = *this;

    size_t lastsep = retval.find_last_of("/");
    if (lastsep != std::string::npos)
        retval = retval.substr(lastsep + 1);

    return retval;
}

bool
S9sString::looksBoolean() const
{
    std::string trimmed = trim();

    if (trimmed.empty())
        return false;

    if (!strcasecmp(trimmed.c_str(), "yes") ||
        !strcasecmp(trimmed.c_str(), "no"))
    {
        return true;
    }

    if (!strcasecmp(trimmed.c_str(), "true") ||
        !strcasecmp(trimmed.c_str(), "false"))
    {
        return true;
    }

    return false;
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
S9sString::decimalSeparator()
{
    S9sString tmp;
    S9sString retval;

    tmp.sprintf("%3.1f", 0.0f);
    retval += tmp[1];

    return retval;
}

S9sString 
S9sString::html2ansi(
        const S9sString &input)
{
    S9sString s           = input;
    S9sString origString;

do_again:
    origString = s;
    s.replace("<em style='color: #c66211;'>",     XTERM_COLOR_3);
    s.replace("<em style='color: #75599b;'>",     XTERM_COLOR_3);
    s.replace("<strong style='color: #110679;'>", XTERM_COLOR_16);
    s.replace("<strong style='color: #59a449;'>", XTERM_COLOR_9);
    s.replace("<em style='color: #007e18;'>",     XTERM_COLOR_4);
    s.replace("<em style='color: #7415f6;'>",     XTERM_COLOR_5);
    s.replace("<em style='color: #1abc9c;'>",     XTERM_COLOR_6);
    s.replace("<em style='color: #d35400;'>",     XTERM_COLOR_7);
    s.replace("<em style='color: #c0392b;'>",     XTERM_COLOR_8);
    s.replace("<em style='color: #0b33b5;'>",     XTERM_COLOR_BLUE);
    s.replace("<em style='color: #34495e;'>",     XTERM_COLOR_CYAN);
    s.replace("<em style='color: #f3990b;'>",     XTERM_COLOR_7);
    s.replace("<em style='color: #c49854;'>",     XTERM_COLOR_7);
    s.replace("<em style='color: #877d0f;'>",     XTERM_COLOR_7);
    s.replace("<strong style='color: red;'>",     XTERM_COLOR_RED);

    //s.replace("", );
    s.replace("</em>",       TERM_NORMAL);
    s.replace("</strong>",   TERM_NORMAL);

    // Replacing all the other colors. This code is originally created to be
    // used with a palette, but I am not sure if we should modify the palette,
    // so it is kinda unfinished here.
    S9sRegExp regexp2("<em style=.color:[^;]+;.>",      "i");
    S9sRegExp regexp3("<strong style=.color:[^;]+;.>",  "i");
    
    s.replace(regexp2, XTERM_COLOR_ORANGE);
    s.replace(regexp3, XTERM_COLOR_8);

    s.replace("<BR/>", "\n");
    s.replace("<br/>", "\n");

    if (origString != s)
        goto do_again;

    return s;
}

S9sString 
S9sString::html2text(
        const S9sString &input)
{
    S9sString s           = input;
    S9sString origString;

do_again:
    origString = s;
    s.replace("<em style='color: #c66211;'>",     "");
    s.replace("<em style='color: #75599b;'>",     "");
    s.replace("<strong style='color: #110679;'>", "");
    s.replace("<strong style='color: #59a449;'>", "");
    s.replace("<em style='color: #007e18;'>",     "");
    s.replace("<em style='color: #7415f6;'>",     "");
    s.replace("<em style='color: #1abc9c;'>",     "");
    s.replace("<em style='color: #d35400;'>",     "");
    s.replace("<em style='color: #c0392b;'>",     "");
    s.replace("<em style='color: #0b33b5;'>",     "");
    s.replace("<em style='color: #34495e;'>",     "");
    s.replace("<em style='color: #f3990b;'>",     "");
    s.replace("<em style='color: #c49854;'>",     "");
    s.replace("<strong style='color: red;'>",     "");

    //s.replace("", );
    s.replace("</em>",       "");
    s.replace("</strong>",   "");

    // Replacing all the other colors. This code is originally created to be
    // used with a palette, but I am not sure if we should modify the palette,
    // so it is kinda unfinished here.
    S9sRegExp regexp1("<em style=.color:[^;]+;.>",      "i");
    S9sRegExp regexp2("<strong style=.color:[^;]+;.>",  "i");


    s.replace(regexp1, "");
    s.replace(regexp2, "");

    s.replace("<BR/>", "\n");
    s.replace("<br/>", "\n");

    if (origString != s)
        goto do_again;

    return s;
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

/**
 * Converts the number of seconds to a human readable string representing
 * uptime, e.g. "00:06:38" or "4 days 17:24:11"
 */
S9sString
S9sString::uptime(
        ulonglong seconds)
{
    S9sString retval;
    int       days;
    int       hours;
    int       minutes;
    
    days     = seconds / (60 * 60 * 24);
    seconds -= days * (60 * 60 * 24);

    hours    = seconds / (60 * 60);
    seconds -= hours * (60 * 60);

    minutes  = seconds / 60;
    seconds -= minutes * 60;

    if (days > 1)
    {
        retval.sprintf("%d days %02d:%02d:%02d", days, hours, minutes, seconds);
    } else if (days > 0)
    {
        retval.sprintf("%d day %02d:%02d:%02d", days, hours, minutes, seconds);
    } else {
        retval.sprintf("%02d:%02d:%02d", hours, minutes, seconds);
    }

    return retval;
}

/**
 * Reads all the lines from the standard input and stores it in an S9sString
 * string.
 */
S9sString 
S9sString::readStdIn()
{
    S9sString retval;

    for (std::string line; std::getline(std::cin, line);) 
    {
        retval += line;
        retval += '\n';
    }

    return retval;
}

/**
 * Same as read(3), but this repeats the request if it was interrupted by a
 * signal.
 */
static ssize_t
safeRead (
    int fileDescriptor, 
    void *buffer, 
    size_t bufferSize)
{
    ssize_t retval;
  
    do {
        retval = ::read(fileDescriptor, buffer, bufferSize);
    } while (retval == -1 && errno == EINTR);

    return retval;
}

/**
 * \param fileName The name of the file to read.
 * \param content The place where the content of the file will be placed.
 * \param errorString The place where the error message will be placed if
 *   something went wrong.
 * \returns True if everything is ok.
 *
 * Reads the entire file into a string.
 */
bool
S9sString::readFile(
        const S9sString &fileName,
        S9sString       &content,
        S9sString       &errorString)
{
    int      fileDescriptor;
    bool     retval = false;

    fileDescriptor = open(STR(fileName), O_RDONLY); 
    if (fileDescriptor < 0)
    {
        errorString.sprintf(
                "Error opening '%s' for reading: %m", STR(fileName));
        return false;
    }

    // content should be empty
    content.clear();

    char *buffer = new char[READ_BUFFER_SIZE];
    if (buffer == 0) 
    {
        errorString.sprintf("can't allocate memory");
        return false;
    }

    retval = true;
    ssize_t readBytes = 0;
    do
    {
        readBytes = safeRead(fileDescriptor, buffer, READ_BUFFER_SIZE);

        if (readBytes > 0) 
        {
            content += std::string(buffer, (size_t) readBytes);
        } else if (readBytes < 0) 
        {
            // reading failed
            errorString.sprintf("read error: %m");
            retval = false;
            break;
        }
    } while (readBytes > 0);

    delete[] buffer;
    ::close(fileDescriptor);

    return retval;
}

/**
 * Same as write(3), but this repeats the write request if it was interrupted by
 * a signal.
 */
static ssize_t
safeWrite (
    int         fileDescriptor, 
    const void *buffer, 
    size_t      nBytes)
{
    ssize_t retval;
  
    do {
        retval = ::write(fileDescriptor, buffer, nBytes);
    } while (retval == -1 && errno == EINTR);

    return retval;
}

/**
 * \param fileName The name of the file to read.
 * \param content The place where the content of the file will be placed.
 * \param errorString The place where the error message will be placed if
 *   something went wrong.
 * \returns True if everything is ok.
 *
 * Reads the entire file into a string.
 */
bool
S9sString::writeFile(
        const S9sString     &fileName,
        S9sString           &content,
        S9sString           &errorString)
{
    mode_t   mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int      fileDescriptor;
    ssize_t  nBytes;
    int      errorCode;

    fileDescriptor = open(STR(fileName), 
            O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fileDescriptor < 0)
    {
        errorString.sprintf(
                "Error opening '%s' for writing: %m",
                STR(fileName));
        return false;
    }

    nBytes = ::safeWrite(fileDescriptor, STR(content), content.size());
    if (nBytes < (ssize_t) content.size())
    {
        errorString.sprintf(
                "Error writing file '%s': %m", 
                STR(fileName));
        ::close(fileDescriptor);
        return false;
    }

    errorCode = ::close(fileDescriptor);
    if (errorCode != 0)
    {
        errorString.sprintf(
                "Error closing file '%s': %m", 
                STR(fileName));
        return false;
    }

    return true;
}

std::string
S9sString::buildPath(
        const std::string &path1,
        const std::string &path2)
{
    std::string retval;
    bool        needSeparator = 
        !path1.empty() && 
        path1[path1.length() - 1] != DIRSEPARATOR &&
        !path2.empty() && 
        path2[0] != DIRSEPARATOR;

    bool        removeSeparator = 
        !path1.empty() && 
        path1[path1.length() - 1] == DIRSEPARATOR &&
        !path2.empty() && 
        path2[0] == DIRSEPARATOR;

    retval += path1;

    if (removeSeparator)
        retval.resize(retval.size () - 1);

    if (needSeparator)
        retval += DIRSEPARATOR;

    retval += path2;

    return retval;
}

std::string
S9sString::buildPath(
        const std::string &path1,
        const std::string &path2,
        const std::string &path3)
{
    std::string first = buildPath(path1, path2);

    return buildPath(first, path3);
}
