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

S9sTreeNode
S9sBrowser::selectedNode() const
{
    return m_subTree.childNode(selectionIndex());
}

void
S9sBrowser::processKey(
        int key)
{
    if (!hasFocus())
        return;

    switch (key)
    {
        case S9S_KEY_ENTER:
            {
                S9sTreeNode node = selectedNode();

                if (node.name() == "..")
                {
                    m_path = S9sFile::dirname(m_path);
                    m_rootNode.subTree(m_path, m_subTree);
                    setSelectionIndex(0);
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
                    ::printf("\b");
                }
            }
            return;

        case 'd':
            m_isDebug = !m_isDebug;
            return;
    }

    S9sDisplayList::processKey(key);
}

/*
 * http://xahlee.info/comp/unicode_drawing_shapes.html
 */
void
S9sBrowser::printLine(
        int lineIndex)
{
    const char *normal = "\033[48;5;19m" "\033[1m\033[38;5;33m";
    const char *header = "\033[48;5;19m" "\033[1m\033[38;5;11m";
    const char *selection = "\033[1m\033[48;5;51m" "\033[2m\033[38;5;237m";
    int         column1;
    int         column2;
    int         column3;
    int         nChars = 0;
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
    
    header2Format.setCenterJustify();
    header2Format.setWidth(9);
    column2Format.setWidth(9);

    
    header3Format.setCenterJustify();
    header3Format.setWidth(9);
    column3Format.setWidth(9);
    
    header4Format.setCenterJustify();
    header4Format.setWidth(10);
    column4Format.setWidth(10);

    column1 = width() - 2 - 30;
    column2 = column1 + 10;
    column3 = column2 + 10;

    ::printf("%s", normal);
    if (lineIndex == 0)
    {
        ::printf("╔");
        ++nChars;
        
        while (nChars < width() - 1)
        {
            if (nChars == column1 || nChars == column2 || nChars == column3)
                ::printf("╤"); 
            else
                ::printf("═");

            ++nChars;
        }

        ::printf("╗");
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
        ::printf("╚");
        ++nChars;
        
        while (nChars < width() - 1)
        {
            ::printf("═");
            ++nChars;
        }

        ::printf("╝");
    } else if (lineIndex == height() - 3)
    {
        ::printf("╟");
        ++nChars;
        
        while (nChars < width() - 1)
        {
            if (nChars == column1 || nChars == column2 || nChars == column3)
                ::printf("┴"); 
            else
                ::printf("─");

            ++nChars;
        }

        ::printf("╢");
    } else if (lineIndex == height() - 2)
    {
        ::printf("║");
        ++nChars;
       
        ::printf("%s", STR(m_path));
        nChars += m_path.length();

        while (nChars < width() - 1)
        {
            ::printf(" ");
            ++nChars;
        }

        ::printf("║");
    } else {
        /*
         * The normal lines. 
         */
        int       listIndex = lineIndex - 2 + firstVisibleIndex();
        S9sString name;
        S9sString owner;
        S9sString group;
        S9sString mode;
        bool      selected;

        ensureSelectionVisible();

        if (listIndex < m_subTree.nChildren())
        {
            S9sTreeNode node = m_subTree.childNode(listIndex);
            
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

        selected = isSelected(listIndex) && hasFocus();

        ::printf("║");

        if (selected)
            ::printf("%s", selection);

        column1Format.printf(name);
        ::printf("│"); 
        
        column2Format.printf(owner);
        ::printf("│"); 
        
        column3Format.printf(group);
        ::printf("│"); 
        
        column4Format.printf(mode);
        
        if (selected)
            ::printf("%s%s", TERM_NORMAL, normal);

        ::printf("║");
    }
}
