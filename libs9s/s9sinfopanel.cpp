/*
 * Severalnines Tools
 * Copyright (C) 2016-2018 Severalnines AB
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
#include "s9sinfopanel.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sInfoPanel::S9sInfoPanel() :
    S9sWidget()
{
}

S9sInfoPanel::~S9sInfoPanel()
{
}


void
S9sInfoPanel::setInfoController(
        const S9sString &hostName,
        const int        port,
        const bool       useTls)
{
    m_hostName = hostName;
    m_port     = port;
    m_useTls   = useTls;
}

void
S9sInfoPanel::setInfoRequestName(
        const S9sString  requestName)
{
    m_requestName = requestName;
}

S9sString
S9sInfoPanel::controllerUrl() const
{
    S9sString retval;

    retval.sprintf(
            "%s://%s:%d",
            m_useTls ? "https" : "http",
            STR(m_hostName),
            m_port);

    return retval;
}

void
S9sInfoPanel::printLine(
        int lineIndex)
{
    const char *normal = "\033[48;5;19m" "\033[1m\033[38;5;33m";
    const char *header = "\033[48;5;19m" "\033[1m\033[38;5;11m";
    //const char *selection = "\033[1m\033[48;5;51m" "\033[2m\033[38;5;237m";
    int         nChars = 0;

    ::printf("%s", normal);
    if (lineIndex == 0)
    {
        ::printf("╔");
        ++nChars;
        
        while (nChars < width() - 1)
        {
            ::printf("═");
            ++nChars;
        }

        ::printf("╗");
    } else if (lineIndex == height() - 1)
    {
        // Last line, frame.
        ::printf("╚");
        ++nChars;
        
        while (nChars < width() - 1)
        {
            ::printf("═");
            ++nChars;
        }

        ::printf("╝");
    } else if (lineIndex == 1) 
    {
        S9sString tmp;

        ::printf("║");
        ++nChars;
        
        tmp = "  Controller: ";
        ::printf("%s", STR(tmp));
        nChars += tmp.length();
   
        tmp = controllerUrl();
        ::printf("%s", header);
        ::printf("%s", STR(tmp));
        ::printf("%s", normal);
        nChars += tmp.length();

        while (nChars < width() - 1)
        {
            ::printf(" ");
            ++nChars;
        }

        ::printf("║");
    } else if (lineIndex == 2) 
    {
        S9sString tmp;

        ::printf("║");
        ++nChars;
        
        if (!m_requestName.empty())
        {
            tmp = "     Request: ";
            ::printf("%s", STR(tmp));
            nChars += tmp.length();
   
            tmp = m_requestName;
            ::printf("%s", header);
            ::printf("%s", STR(tmp));
            ::printf("%s", normal);
            nChars += tmp.length();
        }
         
        while (nChars < width() - 1)
        {
            ::printf(" ");
            ++nChars;
        }

        ::printf("║");
    } else {
        ::printf("║");
        ++nChars;
   
        while (nChars < width() - 1)
        {
            ::printf(" ");
            ++nChars;
        }

        ::printf("║");

    }
}
