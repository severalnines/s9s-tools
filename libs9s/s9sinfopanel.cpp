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

#include "s9sdisplay.h"
#include "s9suser.h"
#include "s9scluster.h"
#include "s9scontainer.h"
#include "s9snode.h"
#include "s9sserver.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sInfoPanel::S9sInfoPanel() :
    S9sWidget(),
    m_showJson(false),
    m_objectSetTime(0),
    m_objectInvalid(true),
    m_previewLineOffset(0)
{
}

S9sInfoPanel::~S9sInfoPanel()
{
}

void 
S9sInfoPanel::processKey(
        int key)
{
    switch (key)
    {
        case S9S_KEY_DOWN:
            ++m_previewLineOffset;
            break;

        case S9S_KEY_UP:
            --m_previewLineOffset;
            break;

        case S9S_KEY_PGDN:
            break;

        case S9S_KEY_PGUP:
            break;
    }

    if (m_previewLineOffset < 0)
        m_previewLineOffset = 0;

    if (m_previewLineOffset > (int) m_previewLines.size() - height() + 8)
        m_previewLineOffset = (int) m_previewLines.size() - height() + 8;
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

/**
 * \param path The full CDT path of the object.
 * \param object The object itself in serialized format.
 *
 * Sets the object that is shown in the view.
 */
void
S9sInfoPanel::setInfoObject(
        const S9sString     &path,
        const S9sVariantMap &object)
{
    bool isObjectChanged = path != m_objectPath;

    m_objectPath    = path;
    m_object        = object;
    m_objectSetTime = time(NULL);
    m_objectInvalid = false;

    if (isObjectChanged)
        m_previewLineOffset = 0;

    m_previewLines.clear();
}

/**
 * This method should be called when the shown object is updated and theinfo
 * panel shows the old object. When this method is called the info panel starts
 * showing the "waiting for preview" message.
 */
void
S9sInfoPanel::invalidateObject()
{
    m_objectInvalid = true;
}

/**
 * \returns True if the shown object needs to be set/refreshed, the info panel
 *   shows the "waiting for preview" message.
 */
bool
S9sInfoPanel::needsUpdate() const
{
    if (m_objectInvalid)
        return true;

    if (m_objectPath != m_node.fullPath())
        return true;

    return false;
}

S9sString
S9sInfoPanel::objectPath() const
{
    return m_objectPath;
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
S9sInfoPanel::setInfoLastReply(
        const S9sRpcReply &reply)
{
    m_lastReply = reply;
}

void
S9sInfoPanel::setInfoNode(
        const S9sTreeNode &node)
{
    m_node = node;
}

time_t
S9sInfoPanel::objectSetTime() const
{
    return m_objectSetTime;
}

void
S9sInfoPanel::setShowJson(
        bool showJson)
{
    if (m_showJson == showJson)
        return;

    m_showJson = showJson;
    m_previewLineOffset = 0;

    m_previewLines.clear();
}

bool
S9sInfoPanel::showJson() const
{
    return m_showJson;
}

void
S9sInfoPanel::printLine(
        int lineIndex)
{
    const char *normal = "\033[48;5;19m" "\033[1m\033[38;5;33m";
    //const char *header = "\033[48;5;19m" "\033[1m\033[38;5;11m";
    const char *selection = "\033[1m\033[48;5;51m" "\033[2m\033[38;5;237m";

    m_nChars = 0;
    ::printf("%s", normal);
    if (lineIndex == 0)
    {
        // The top frame line.
        printChar("╔");
        printChar("═", width() - 1);
        printChar("╗");
    } else if (lineIndex == height() - 1)
    {
        // Last line, frame.
        printChar("╚");
        
        while (m_nChars < width() - 1)
            printChar("═");

        printChar("╝");
    } else if (lineIndex == 1) 
    {
        S9sString tmp;

        printChar("║");
        printNameValue("Controller", controllerUrl());
        printChar(" ", width() - 1);
        printChar("║");
    } else if (lineIndex == 2) 
    {
        S9sString tmp;

        printChar("║");
        
        if (!m_requestName.empty())
        {
            printNameValue("Request", m_requestName);
        } else if (!m_lastReply.requestStatusAsString().empty())
        {
            printNameValue("Reply", m_lastReply.requestStatusAsString());
        }
         
        printChar(" ", width() - 1);
        printChar("║");
    } else if (lineIndex == 3)
    {
        printChar("╟");
        printChar("─", width() - 1);
        printChar("╢");
    } else if (lineIndex == 4) 
    {
        S9sString tmp;

        printChar("║");
        
        if (!m_node.name().empty())
            printNameValue("Name", m_node.name());
        
        printChar(" ", width() - 1);
        printChar("║");
    } else if (lineIndex == 5) 
    {
        S9sString tmp;

        printChar("║");
        
        if (!m_node.name().empty())
            printNameValue("Type", m_node.typeName());
        
        printChar(" ", width() - 1);
        printChar("║");
    } else if (lineIndex == 6) 
    {
        S9sString tmp;

        printChar("║");
        
        if (!m_node.spec().empty())
        {
            printNameValue("Spec", m_node.spec());
        } else if (m_node.isDevice())
        {
            printNameValue("Device", m_node.sizeString());
        }
        
        printChar(" ", width() - 1);
        printChar("║");
    } else if (lineIndex == 7)
    {
        S9sString  title = " Preview ";
        int        titleStart;

        //
        // The separator on the top of the title area.
        //
        printChar("╟");

        titleStart = (width() - 2 - title.length()) / 2;
        if (titleStart >= 0)
        {
            printChar("─", titleStart);
            
            if (hasFocus())
                ::printf("%s", selection);

            printString(title);
            
            if (hasFocus())
                ::printf("%s%s", TERM_NORMAL, normal);
        }

        printChar("─", width() - 1);
        printChar("╢");
    } else if (lineIndex >= 8 && lineIndex < height() - 1)
    {
        printLinePreview(lineIndex - 8);
        #if 0
        printChar("║");
        printString("  No preview available.");
        printChar(" ", width() - 1);
        printChar("║");
        #endif
    } else {
        printChar("║");
        printChar(" ", width() - 1);
        printChar("║");
    }
}

void
S9sInfoPanel::printLinePreview(
        int lineIndex)
{
    if (m_node.name() == "..")
    {
        // This is the "up-dir". We don't have this on the controller.
        S9sString      text  = m_node.toVariantMap().toString();
        S9sVariantList lines = text.split("\n");

        printChar("║");

        if (lineIndex >= 0 && lineIndex < (int)lines.size())
            printString(lines[lineIndex].toString());

        printChar(" ", width() - 1);
        printChar("║");
    } else if (m_node.name().empty())
    {
        // No node set.
        printChar("║");
        printChar(" ", width() - 1);
        printChar("║");
    } else if (m_objectPath != m_node.fullPath() || m_objectInvalid)
    {
        // The node is set, but this is an old one, we are waiting for the
        // information about the node.
        //PRINT_LOG("       m_objectPath: '%s'", STR(m_objectPath));
        //PRINT_LOG("  m_node.fullPath(): '%s'", STR(m_node.fullPath()));
        if (lineIndex == 0)
        {
            printChar("║");
            printString(" Waiting for preview.");
            printChar(" ", width() - 1);
            printChar("║");
        } else {
            printChar("║");
            printChar(" ", width() - 1);
            printChar("║");
        }
    } else if (m_object.contains("request_status"))
    {
        printLinePreviewReply(lineIndex);
    } else {
        printLinePreviewCached(lineIndex);
    }
}

void
S9sInfoPanel::printLinePreviewReply(
        int lineIndex)
{
    if (lineIndex == 0)
    {
        printChar("║");
        printChar(" ");

        if (m_object.contains("error_string"))
            printString(m_object.at("error_string").toString());

        printChar(" ", width() - 1);
        printChar("║");
    } else {
        printChar("║");
        printChar(" ", width() - 1);
        printChar("║");
    }
}

void
S9sInfoPanel::ensurePreviewLinesCached()
{
    // A non-empty cache is always a valid cache, we invalidate it by deleting
    // the lines.
    if (!m_previewLines.empty())
        return;

    if (showJson())
    {
        S9sString      text  = m_object.toString();
        m_previewLines = text.split("\n");
    } else if (
            m_object.contains("class_name") &&
            m_object.contains("type") &&
            m_object["class_name"] == "CmonCdtEntry" &&
            m_object["type"] == "CmonCdtFile")
    {
        S9sString text = m_object["content"].toString();
        m_previewLines = text.split("\n", true);
    } else if (
            m_object.contains("class_name") &&
            m_object["class_name"] == "CmonClusterInfo")
    {
        S9sCluster cluster(m_object);
        S9sString  text;

        text = cluster.toString(false,
                "       Name: <b>%N</b>\n"
                "   CDT path: <b>%P</b>\n"
                "       Type: <b>%-12T</b> Vendor: <b>%V</b>\n"
                "      State: <b>%S\n"
                "     Status: <b>%M\n"
                "         ID: <b>%I\n"
                "     Config: '<b>%C</b>'\n"
                );
        m_previewLines = text.split("\n");
    } else if (
            m_object.contains("class_name") &&
            (m_object["class_name"] == "CmonLxcServer" ||
             m_object["class_name"] == "CmonCloudServer"))
    {
        S9sServer  server(m_object);
        S9sString  text;

        text = server.toString(false,
                "       Name: <b>%-12N</b>\n"
                "   CDT path: <b>%h</b>\n"
                "      Class: <b>%z</b>\n"
                "         IP: <b>%A</b>\n"
                "     Status: <b>%-12S</b>\n"
                "       Type: <b>%-12T</b> Version: <b>%V</b>\n"
                "      Model: <b>%m</b>\n"
                "      State: <b>%S\n"
                "     Status: <b>%M\n"
                "         ID: <b>%I\n"
                );
        m_previewLines = text.split("\n");
    } else if (
            m_object.contains("class_name") &&
            (m_object["class_name"] == "CmonHost" ||
             m_object["class_name"] == "CmonController" ||
             m_object["class_name"].toString().contains("Host")))
    {
        S9sNode    node(m_object);
        S9sString  text;

        text = node.toString(false,
                "       Name: <b>%-12N</b>   Port: <b>%P</b>\n"
                "   CDT path: <b>%h</b>\n"
                "      Class: <b>%z</b>\n"
                "         IP: <b>%A</b>\n"
                "     Status: <b>%-12S</b>   Role: <b>%R</b>\n"
                "       Type: <b>%-12T</b> Vendor: <b>%V</b>\n"
                "      State: <b>%-12S</b>    PID: <b>%p</b>\n"
                "     Status: <b>%M\n"
                "         ID: <b>%I\n"
                "     Config: '<b>%C</b>'\n"
                "   Data Dir: '<b>%D</b>'\n"
                "   Log File: '<b>%g</b>'\n"
                "   PID File: '<b>%d</b>'\n"
                );
        m_previewLines = text.split("\n");
    } else if (
            m_object.contains("class_name") &&
            m_object["class_name"] == "CmonContainer")
    {
        S9sContainer container(m_object);
        S9sString    text;

        text = container.toString(false,
                "       Name: <b>%N</b>\n"
                "      Class: <b>%z</b>\n"
                "      Cloud: <b>%c</b>\n"
                "       IpV4: <b>%-12A</b>  PrivateIp: <b>%a</b>\n"
                "       Type: <b>%-12T</b>\n"
                "      State: <b>%S</b>\n"
                "         ID: <b>%I</b>\n"
                "     Subnet: <b>%U</b>\n"
                "     Server: <b>%P</b>\n"
                "     Config: '<b>%C</b>'\n"
                );
        m_previewLines = text.split("\n");
    } else if (
            m_object.contains("class_name") &&
            m_object["class_name"] == "CmonUser")
    {
        S9sUser   user(m_object);
        S9sString text;

        text = user.toString(false,
                "       Name: <b>%N</b>\n"
                "   CDT path: <b>%P</b>\n"
                "         ID: <b>%I</b>\n"
                "   Fullname: <b>%F</b>\n"
                "      Email: <b>%M</b>\n"
                "     Groups: <b>%G</b>\n"
                );

        m_previewLines = text.split("\n");
    } else if (m_object.contains("class_name") &&
            m_object["class_name"] == "CmonCdtEntry")
    {
        S9sString text;

        text = m_object.toString(false,
                "       Name: <b>${name}</b>\n"
                "   CDT path: <b>${cdt_path}</b>\n"
                "       Type: <b>${type}</b>\n"
                "      Owner: <b>${owner_user_name}</b>/<b>"
                "${owner_group_name}</b>\n");
        
        m_previewLines = text.split("\n");
    } else {
        S9sString      text  = m_object.toString();

        m_previewLines = text.split("\n");
    }
}

/**
 * The panels preview lines are cached for speed. This method is printing one of
 * the cached lines as requested.
 */
void
S9sInfoPanel::printLinePreviewCached(
        int lineIndex)
{
    ensurePreviewLinesCached();

    /*
     * Printing the line of the preview we are supposed to print.
     */
    printChar("║");

    lineIndex += m_previewLineOffset;
    if (lineIndex >= 0 && lineIndex < (int)m_previewLines.size())
        printString(m_previewLines[lineIndex].toString());

    printChar(" ", width() - 1);
    printChar("║");
}

void
S9sInfoPanel::printLinePreviewJson(
        int lineIndex)
{
        S9sString      text  = m_object.toString();
        S9sVariantList lines = text.split("\n");

        printChar("║");

        if (lineIndex >= 0 && lineIndex < (int)lines.size())
            printString(lines[lineIndex].toString());

        printChar(" ", width() - 1);
        printChar("║");
}

void
S9sInfoPanel::printLinePreviewJson(
        int lineIndex,
        S9sRpcReply &reply)
{
        S9sString      text  = reply.toString();
        S9sVariantList lines = text.split("\n");

        printChar("║");

        if (lineIndex >= 0 && lineIndex < (int)lines.size())
            printString(lines[lineIndex].toString());

        printChar(" ", width() - 1);
        printChar("║");
}

void
S9sInfoPanel::printString(
        const S9sString &theString)
{
    const char *normal = TERM_NORMAL "\033[48;5;19m" "\033[1m\033[38;5;33m";
    const char *bold = "\033[48;5;19m" "\033[1m\033[38;5;11m";
    S9sString   asciiString = theString;
    S9sString   colorString = theString;
    int         availableChars = width() - m_nChars - 1;
    
    if (availableChars <= 0)
        return;

    asciiString.replace("<b>",  "");
    asciiString.replace("</b>", "");
    
    colorString.replace("<b>",  bold);
    colorString.replace("</b>", normal);

    if ((int)asciiString.length() > availableChars)
    {
        asciiString.resize(availableChars);
        ::printf("%s", STR(asciiString));
    } else {
        ::printf("%s", STR(colorString));
        ::printf("%s", normal);
    }

    m_nChars += asciiString.length();
}

void
S9sInfoPanel::printNameValue(
        const S9sString &name,
        const S9sString &value)
{
    const char *normal = "\033[48;5;19m" "\033[1m\033[38;5;33m";
    const char *header = "\033[48;5;19m" "\033[1m\033[38;5;11m";
    S9sString   tmp;

    tmp.sprintf("%11s: ", STR(name));
    ::printf("%s", STR(tmp));
    m_nChars += tmp.length();
   
    ::printf("%s", header);
    ::printf("%s", STR(value));
    ::printf("%s", normal);
    m_nChars += value.length();
}

void
S9sInfoPanel::printChar(
        int c)
{
    ::printf("%c", c);
    ++m_nChars;
}

void
S9sInfoPanel::printChar(
        const char *c)
{
    ::printf("%s", c);
    ++m_nChars;
}

void
S9sInfoPanel::printChar(
        const char *c,
        const int   lastColumn)
{
    while (m_nChars < lastColumn)
    {
        ::printf("%s", c);
        ++m_nChars;
    }
}
