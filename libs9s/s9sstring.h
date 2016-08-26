/*
 * Copyright (C) 2011-2015 severalnines.com
 */
#pragma once

#include <string>
#include <stdarg.h>

#include "s9sglobal.h"

/**
 * A macro to convert a CmonString to C strings. This actually works with
 * std::strings also.
 */
#ifndef STR
#  define STR(_thestring) ((_thestring).c_str())
#endif

class S9sString : public std::string
{
    public:
        S9sString();
        S9sString(const char *str);
        S9sString(const std::string &str);

        S9sString &operator= (const S9sString &rhs);
        S9sString &operator= (const std::string &rhs);
        S9sString &operator= (const char *rhs);

        inline bool contains(char c) const;
        inline bool contains(const char *s) const;

        void sprintf(const char *formatString, ...);
        void vsprintf(const char *formatString, va_list arguments);

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

        void replace(const S9sString &from, const S9sString &to);
        void replace(size_t pos, size_t len, const S9sString &str);

        S9sString trim(const std::string &pWhitespace = " \t") const;

        bool looksInteger() const;
        bool looksULongLong() const;
};

typedef S9sString S9sFilePath;
typedef S9sString S9sFileName;

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


