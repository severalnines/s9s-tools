/*
 * Severalnines Tools
 * Copyright (C) 2020  Severalnines AB
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
#include "s9spkginfo.h"
#include "S9sContainer"
#include "S9sRegExp"
#include "S9sFormatter"
#include "S9sRpcReply"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sPkgInfo::S9sPkgInfo() :
    S9sObject()
{
    m_properties["class_name"] = "CmonPkgInfo";
}

S9sPkgInfo::S9sPkgInfo(
        const S9sPkgInfo &orig) :
    S9sObject(orig)
{
}

S9sPkgInfo::S9sPkgInfo(
        const S9sVariantMap &properties) :
    S9sObject(properties)
{
    if (!m_properties.contains("class_name"))
        m_properties["class_name"] = "CmonPkgInfo";
}

S9sPkgInfo::~S9sPkgInfo()
{
}

S9sPkgInfo &
S9sPkgInfo::operator=(
        const S9sVariantMap &rhs)
{
    setProperties(rhs);
    
    return *this;
}

S9sString 
S9sPkgInfo::className() const
{
    return property("class_name").toString();
}

/**
 * \param syntaxHighlight Controls if the string will have colors or not.
 * \param formatString The formatstring with markup.
 * \returns The string representation according to the format string.
 *
 * Converts the PkgInfo class to a string using a special format string
 * that may contain field names of PkgInfo properties.
 */
S9sString
S9sPkgInfo::toString(
        const bool       syntaxHighlight,
        const S9sString &formatString) const
{
    S9sString    retval;
    S9sString    tmp;
    char         c;
    S9sString    partFormat;
    bool         percent      = false;
    bool         escaped      = false;

    for (uint n = 0; n < formatString.size(); ++n)
    {
        c = formatString[n];
       
        if (c == '%' && !percent)
        {
            percent    = true;
            partFormat = "%";
            continue;
        } else if (c == '\\' && !escaped)
        {
            escaped = true;
            continue;
        }

        if (escaped)
        {
            switch (c)
            {
                case '\"':
                    retval += '\"';
                    break;

                case '\\':
                    retval += '\\';
                    break;
       
                case 'a':
                    retval += '\a';
                    break;

                case 'b':
                    retval += '\b';
                    break;

                case 'e':
                    retval += '\027';
                    break;

                case 'n':
                    retval += '\n';
                    break;

                case 'r':
                    retval += '\r';
                    break;

                case 't':
                    retval += '\t';
                    break;
            }
        } else if (percent)
        {
            switch (c)
            {
                case 'z':
                    // The class name.
                    partFormat += "s";

                    if (syntaxHighlight)
                        retval += XTERM_COLOR_GREEN;

                    retval.aprintf(STR(partFormat), STR(className()));

                    if (syntaxHighlight)
                        retval += TERM_NORMAL;
                    break;

                case 'N':
                    // The name of the package.
                    partFormat += "s";
                    retval.aprintf(STR(partFormat), STR(name()));
                    break;

                case 'T':
                    // The type of the server.
                    partFormat += "s";
                    retval.aprintf(STR(partFormat), STR(hostClassName()));
                    break;

                case 'H':
                    // The name of the host having the package.

                    if (syntaxHighlight)
                        retval += XTERM_COLOR_BLUE;

                    partFormat += "s";
                    retval.aprintf(STR(partFormat), STR(hostName()));

                    if (syntaxHighlight)
                        retval += TERM_NORMAL;
                    break;

                case 'D':
                // DateTime of last check for available versions
                    partFormat += "s";
                    retval.aprintf(STR(partFormat),
                        STR(lastUpdated().toString(S9sDateTime::CompactFormat)));
                    break;

                case 'v':
                    // The installed version.
                    partFormat += "s";
                    retval.aprintf(STR(partFormat), STR(installedVersion()));
                    break;

                case 'V':
                    // The available version.
                    partFormat += "s";
                    retval.aprintf(STR(partFormat), STR(availableVersion()));
                    break;

                case '%':
                    retval += '%';
                    break;

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case '-':
                case '+':
                case '.':
                case '\'':
                    partFormat += c;
                    continue;
            }
        } else {
            retval += c;
        }

        percent      = false;
        escaped      = false;
    }

    return retval;
}


S9sString 
S9sPkgInfo::name() const
{
    return property("package_name").toString();
}

S9sString 
S9sPkgInfo::hostClassName() const
{
    return property("host_class_name").toString();
}

S9sString
S9sPkgInfo::hostName() const
{
    return property("host_name").toString();
}

S9sDateTime
S9sPkgInfo::lastUpdated() const
{
    S9sDateTime retval;

    retval.parse( property("last_updated").toString() );

    return retval;
}

S9sString
S9sPkgInfo::installedVersion() const
{
    return property("installed_version").toString();
}

S9sString
S9sPkgInfo::availableVersion() const
{
    return property("available_version").toString();
}

const char *
S9sPkgInfo::colorBegin(
        bool    useSyntaxHighLight) const
{
    const char *retval = "";

    if (useSyntaxHighLight)
    {
        retval = XTERM_COLOR_GREEN;
    }

    return retval;
}

const char *
S9sPkgInfo::colorEnd(
        bool    useSyntaxHighLight) const
{
    return useSyntaxHighLight ? TERM_NORMAL : "";
}

bool 
S9sPkgInfo::compareByName(
        const S9sPkgInfo &server1,
        const S9sPkgInfo &server2)
{
    if ( server1.hostName() == server2.hostName() )
    {
        return server1.name() < server2.name();
    } else
        return server1.hostName() < server2.hostName();
}


