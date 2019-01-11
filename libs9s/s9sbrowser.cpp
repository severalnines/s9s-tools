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
#include "s9sbrowser.h"
#include "S9sFormat"
#include "S9sDisplay"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sBrowser::S9sBrowser() :
    S9sDisplayList(),
    m_path("/"),
    m_isDebug(false)
{
    setHeaderHeight(2);
    setFooterHeight(3);
}

S9sBrowser::~S9sBrowser()
{
}

void
S9sBrowser::setCdt(
        const S9sTreeNode &node)
{
    bool success;

    m_rootNode = node;
    
    success = m_rootNode.subTree(m_path, m_subTree);
    if (!success)
    {
        m_subTree = m_rootNode;
        m_path    = "/";
    }

    setNumberOfItems(m_subTree.nChildren());
}

S9sString 
S9sBrowser::path() const
{
    return m_path;
}

/**
 * \returns The node that is representing the entry highlighted on the screen.
 */
S9sTreeNode
S9sBrowser::selectedNode() const
{
    return m_subTree.childNode(selectionIndex());
}

S9sString
S9sBrowser::selectedNodeFullPath() const
{
    S9sString retval;

    if (m_path.empty() || m_name.empty())
        return retval;

    return S9sFile::buildPath(m_path, m_name);
}

S9sTreeNode
S9sBrowser::activatedNode() const
{
    return m_activatedNode;
}

S9sString
S9sBrowser::activatedNodeFullPath() const
{
    return m_acivatedPath;
}

void
S9sBrowser::resetActivatedStatus()
{
    m_acivatedPath  = "";
    m_activatedNode = S9sTreeNode();
}

void
S9sBrowser::setSelectionIndexByName(
        const S9sString &name)
{
    const S9sVector<S9sTreeNode> &childNodes = m_subTree.childNodes();
    int   index = 0;

    for (uint idx = 0u; idx < childNodes.size(); ++idx)
    {
        if (childNodes[idx].name() != name)
            continue;

        index = idx;
        break;
    }

    setSelectionIndex(index);
}

void
S9sBrowser::processKey(
        int key)
{
    if (!hasFocus())
        return;

    resetActivatedStatus();
    
    switch (key)
    {
        case S9S_KEY_ENTER:
            {
                S9sTreeNode node = selectedNode();

                if (node.name() == "..")
                {
                    S9sString parentBasename;

                    s9s_log("Up dir...");

                    parentBasename = S9sFile::basename(m_path);
                    m_path = S9sFile::dirname(m_path);

                    m_rootNode.subTree(m_path, m_subTree);
                    setSelectionIndexByName(parentBasename);

                    setNumberOfItems(m_subTree.nChildren());
                } else if (node.nChildren() > 0)
                {
                    if (!m_path.endsWith("/"))
                        m_path += "/";

                    m_path += node.name();
                    m_rootNode.subTree(m_path, m_subTree);
                    setSelectionIndex(0);
                    setNumberOfItems(m_subTree.nChildren());
                } else {
                    m_acivatedPath  = selectedNodeFullPath();
                    m_activatedNode = selectedNode();
                }
            }
            return;

        case 'd':
            m_isDebug = !m_isDebug;
            return;
    }

    S9sDisplayList::processKey(key);
}

#if 0
bool 
S9sBrowser::processButton(
        uint button, 
        uint x, 
        uint y)
{
}
#endif

/*
 * http://xahlee.info/comp/unicode_drawing_shapes.html
 */
void
S9sBrowser::printLine(
        int lineIndex)
{
    const char *normal     = TERM_NORMAL "\033[48;5;19m" "\033[38;5;33m";
    const char *folder     = TERM_NORMAL "\033[48;5;19m" XTERM_COLOR_FOLDER;
    const char *user       = TERM_NORMAL "\033[48;5;19m" XTERM_COLOR_USER;
    const char *groupColor = TERM_NORMAL "\033[48;5;19m" "\033[2m\033[38;5;3m";
    const char *deviceColor = TERM_NORMAL "\033[48;5;19m" "\033[38;5;198m";
    const char *execColor  = TERM_NORMAL "\033[48;5;19m" "\033[32m";
    const char *file       = TERM_NORMAL "\033[48;5;19m" "\033[38;5;199m";
    const char *cluster    = TERM_NORMAL "\033[48;5;19m" "\033[2m\033[38;5;197m";
    const char *hostColor  = TERM_NORMAL "\033[48;5;19m" "\033[38;5;46m";
    const char *header     = "\033[48;5;19m" "\033[1m\033[38;5;11m";

    const char *selection = "\033[1m\033[48;5;51m" "\033[2m\033[38;5;237m";
    int         column1;
    int         column2;
    int         column3;
    S9sFormat   header1Format(header, normal);
    S9sFormat   header2Format(header, normal);
    S9sFormat   header3Format(header, normal);
    S9sFormat   header4Format(header, normal);
    
    S9sFormat   column1Format;
    S9sFormat   column2Format;
    S9sFormat   column3Format;
    S9sFormat   column4Format;

    header1Format.setCenterJustify();
    header1Format.setWidth(width() - 2 - 31);
    column1Format.setWidth(width() - 2 - 31);
    column1Format.setEllipsize();
    
    header2Format.setCenterJustify();
    header2Format.setWidth(9);
    column2Format.setWidth(9);
    column2Format.setEllipsize();

    
    header3Format.setCenterJustify();
    header3Format.setWidth(9);
    column3Format.setWidth(9);
    column3Format.setEllipsize();
    
    header4Format.setCenterJustify();
    header4Format.setWidth(10);
    column4Format.setWidth(10);
    column4Format.setEllipsize();

    column1 = width() - 2 - 30;
    column2 = column1 + 10;
    column3 = column2 + 10;

    m_nChars = 0;
    ::printf("%s", normal);
    if (lineIndex == 0)
    {
        printChar("╔");

        printString(" " + m_path + " ");

        while (m_nChars < width() - 1)
        {
            if (m_nChars == column1 || 
                    m_nChars == column2 || m_nChars == column3)
                ::printf("╤"); 
            else
                ::printf("═");

            ++m_nChars;
        }

        printChar("╗");
    } else if (lineIndex == 1) 
    {
        ::printf("║");
   
        header1Format.printf("Name");
        ::printf("│"); 
        
        header2Format.printf("User");
        ::printf("│"); 
        
        header3Format.printf("Group");
        ::printf("│"); 
        
        header4Format.printf("Mode");

        ::printf("║");
    } else if (lineIndex == height() - 1)
    {
        // Last line, frame.
        printChar("╚");
        printChar("═", width() - 1);
        printChar("╝");
    } else if (lineIndex == height() - 3)
    {
        printChar("╟");
        
        while (m_nChars < width() - 1)
        {
            if (m_nChars == column1 || m_nChars == column2 || m_nChars == column3)
                ::printf("┴"); 
            else
                ::printf("─");

            ++m_nChars;
        }

        printChar("╢");
    } else if (lineIndex == height() - 2)
    {
        //
        // The single line text on the bottom of the panel.
        //
        printChar("║");
        printString(" ");
        printString(m_name);
        printChar(" ", width() - 1);
        printChar("║");
    } else {
        /*
         * The normal lines, showing data.
         */
        S9sTreeNode node;
        int         listIndex = lineIndex - 2 + firstVisibleIndex();
        S9sString   name;
        S9sString   owner;
        S9sString   group;
        S9sString   mode;
        bool        selected;

        ensureSelectionVisible();
        
        selected = isSelected(listIndex) && hasFocus();

        if (listIndex < m_subTree.nChildren())
        {
            node = m_subTree.childNode(listIndex);
            
            if (selected)
                m_name = node.name();

            if (m_isDebug)
            {
                name.sprintf("%d %d %s", 
                        selectionIndex(), 
                        listHeight(),
                        STR(node.name()));
            } else {
                name = node.name();
            }

            if (node.name() != "..")
            {
                owner = node.ownerUserName();
                group = node.ownerGroupName();
                mode  = aclStringToUiString(node.acl());
            }
        }


        ::printf("║");

        if (selected)
            ::printf("%s", selection);
        else if (node.isFolder())
            ::printf("%s", folder);
        else if (node.isDevice())
            ::printf("%s", deviceColor);
        else if (node.isFile() && node.isExecutable())
            ::printf("%s", execColor);
        else if (false && node.isUser())
            ::printf("%s", user);
        else if (false && node.isGroup())
            ::printf("%s", groupColor);
        else if (false && node.isFile())
            ::printf("%s", file);
        else if (false && node.isCluster())
            ::printf("%s", cluster);
        else if (false && node.isNode())
            ::printf("%s", hostColor);

        column1Format.printf(name);
        
        if (selected)
            ::printf("%s%s", TERM_NORMAL, selection);
        else
            ::printf("%s%s", TERM_NORMAL, normal);

        ::printf("│"); 
        
        column2Format.printf(owner);
        ::printf("│"); 
        
        column3Format.printf(group);
        ::printf("│"); 
        
        column4Format.printf(mode);
        
        //if (selected)
        ::printf("%s%s", TERM_NORMAL, normal);

        ::printf("║");
    }
}

void
S9sBrowser::printString(
        const S9sString &theString)
{
    S9sString  myString = theString;
    int        availableChars = width() - m_nChars - 1;
    
    if (availableChars <= 0)
        return;

    if ((int)theString.length() > availableChars)
        myString.resize(availableChars);

    ::printf("%s", STR(myString));
    m_nChars += myString.length();
}

void
S9sBrowser::printChar(
        int c)
{
    ::printf("%c", c);
    ++m_nChars;
}

void
S9sBrowser::printChar(
        const char *c)
{
    ::printf("%s", c);
    ++m_nChars;
}

void
S9sBrowser::printChar(
        const char *c,
        const int   lastColumn)
{
    while (m_nChars < lastColumn)
    {
        ::printf("%s", c);
        ++m_nChars;
    }
}
