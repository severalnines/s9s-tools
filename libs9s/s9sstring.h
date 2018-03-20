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

#include <string>
#include <stdarg.h>

#include "s9sglobal.h"

#define DIRSEPARATOR '/'

/**
 * A macro to convert a S9sString to C strings. This actually works with
 * std::strings also.
 */
#ifndef STR
#  define STR(_thestring) ((_thestring).c_str())
#endif

#ifndef STR_BOOL
#  define STR_BOOL(_thebool) (_thebool ? "true" : "false")
#endif

class S9sRegExp;
class S9sVariantList;

class S9sString : public std::string
{
    public:
        S9sString();
        S9sString(const char *str);
        S9sString(const std::string &str);

        S9sString &operator=(const S9sString &rhs);
        S9sString &operator=(const std::string &rhs);
        S9sString &operator=(const char *rhs);
        S9sString  operator*(const int rhs) const;
        S9sString &operator+=(const std::string &rhs);
        S9sString &operator+=(const char rhs);

        inline bool contains(char c) const;
        inline bool contains(const char *s) const;

        int terminalLength() const;

        void sprintf(const char *formatString, ...);
        void vsprintf(const char *formatString, va_list arguments);
        void aprintf(const char *formatString, ...);

        S9sVariantList split(const char *ifs = ";, ") const;
        S9sString &appendWord(const S9sString &word);

        int toInt(const int defaultVal = 0) const;
        ulonglong toULongLong(const ulonglong defaultVal = 0ull) const;
        double toDouble(const double defaultVal = 0.0) const;
        bool toBoolean() const;
      
        S9sString unQuote() const;

        S9sString escape() const;
        S9sString unEscape() const;

        S9sString toUpper() const;
        S9sString toLower() const;

        bool startsWith(const char *str) const;
        bool endsWith(S9sString const &ending) const;

#if 0
        bool regMatch(const S9sString &regExp) const;
        bool regMatch(const S9sString &regExp, S9sString &matched) const;

        bool regMatch(
                const S9sString   &regExp, 
                S9sString         &matched1, 
                S9sString         &matched2) const;
#endif
        void replace(const S9sString &from, const S9sString &to);
        void replace(size_t pos, size_t len, const S9sString &str);
        void replace(S9sRegExp &regExp, S9sString replacement);

        S9sString trim(const std::string &pWhitespace = " \t") const;
        S9sString baseName() const;

        bool looksBoolean() const;
        bool looksInteger() const;
        bool looksULongLong() const;

        static S9sString decimalSeparator();

        static S9sString html2ansi(const S9sString &input);
        static S9sString html2text(const S9sString &input);

        static S9sString pastTime(const time_t theTime);
        static S9sString uptime(ulonglong seconds);

        static S9sString readStdIn();
        
        static bool readFile(
                const S9sString     &fileName,
                S9sString           &content,
                S9sString           &errorString);

        static bool writeFile(
                const S9sString     &fileName,
                S9sString           &content,
                S9sString           &errorString);
    
        static std::string
            buildPath(
                    const std::string &path1,
                    const std::string &path2);
    
        static std::string
            buildPath(
                    const std::string &path1,
                    const std::string &path2,
                    const std::string &path3);

        static const S9sString space;
        static const S9sString dash;
};

typedef S9sString S9sFilePath;
typedef S9sString S9sFileName;
typedef S9sString S9sDirName;

inline bool 
S9sString::contains(char c) const
{
    return find(c) != std::string::npos;
}

inline bool 
S9sString::contains(const char *s) const
{
    return find(s) != std::string::npos;
}


