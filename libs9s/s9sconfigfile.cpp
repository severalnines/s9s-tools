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
#include "s9sconfigfile.h"
#include "s9sconfigfile_p.h"

#include <cstring>
#include <ctime>

#include "S9sFile"
#include "S9sVariantMap"

#define YY_EXTRA_TYPE S9sClusterConfigParseContext *
#include "config_parser.h"
#include "config_lexer.h"

//#include "haconfig_parser.h"
//#include "haconfig_lexer.h"

//#include "yaml_parser.h"
//#include "yaml_lexer.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

/******************************************************************************
 * S9sConfigAstNode implementation
 */
S9sConfigAstNode::S9sConfigAstNode(
        S9sConfigAstNode::NodeType nodeType,
        const char                  *origString) :
    m_syntax(S9s::GenericConfigSyntax),
    m_nodeType(nodeType),
    m_origString(origString),
    m_lineNumber(0),
    m_indent(0),
    m_child1(0),
    m_child2(0)
{
}

S9sConfigAstNode::S9sConfigAstNode(
        S9sConfigAstNode::NodeType nodeType,
        S9sConfigAstNode           *child1,
        S9sConfigAstNode           *child2) :
    m_syntax(S9s::GenericConfigSyntax),
    m_nodeType(nodeType),
    m_lineNumber(0),
    m_indent(0),
    m_child1(child1),
    m_child2(child2)
{
}

S9sConfigAstNode::~S9sConfigAstNode()
{
    if (m_child1)
    {
        delete m_child1;
        m_child1 = NULL;
    }

    if (m_child2)
    {
        delete m_child2;
        m_child2 = NULL;
    }
}

void
S9sConfigAstNode::setSyntax(
        const S9s::Syntax syntax)
{
    m_syntax = syntax;

    if (m_child1)
        m_child1->setSyntax(syntax);
    
    if (m_child2)
        m_child2->setSyntax(syntax);
}

void
S9sConfigAstNode::setChildren(
        S9sConfigAstNode  *child1,
        S9sConfigAstNode  *child2)
{
    if (m_child1)
        delete m_child1;

    if (m_child2)
        delete m_child2;

    m_child1 = child1;
    m_child2 = child2;
}

void
S9sConfigAstNode::setLineNumber(
        int lineNumber)
{
    m_lineNumber = lineNumber;

    if (m_child1)
        m_child1->setLineNumber(lineNumber);
    
    if (m_child2)
        m_child2->setLineNumber(lineNumber);
}

void
S9sConfigAstNode::setIndent(
        int indent)
{
    m_indent = indent;

    if (m_child1)
        m_child1->setIndent(indent);

    if (m_child2)
        m_child2->setIndent(indent);
}

const S9sString &
S9sConfigAstNode::origString() const
{
    return m_origString;
}

void 
S9sConfigAstNode::setType(
        S9sConfigAstNode::NodeType nodeType)
{
    m_nodeType = nodeType;
}

/**
 * If the node holds a file name this method will return it.
 */
S9sString 
S9sConfigAstNode::fileName() const
{
    if ((m_nodeType == Include || m_nodeType == IncludeDir) && m_child2)
        return m_child2->m_origString;

    return S9sString();
}

S9sString 
S9sConfigAstNode::leftValue() const
{
    if ((isAssignment() || isCommented()) && m_child1)
        return m_child1->m_origString;

    return S9sString();
}

S9sString 
S9sConfigAstNode::rightValue() const
{
    if (m_nodeType == Assignment && m_child2)
        return m_child2->m_origString;

    return S9sString();
}

void
S9sConfigAstNode::setRightValue(
        const S9sString &newValue) const
{
    if ((isAssignment() || isCommented()) && m_child2)
        m_child2->m_origString = newValue;
}

S9sString 
S9sConfigAstNode::sectionName() const
{
    if (m_nodeType == Section)
        return m_origString;

    return S9sString();
}

void
S9sConfigAstNode::build(
        S9sString &content)
{
    switch (m_syntax)
    {
        case S9s::GenericConfigSyntax:
        case S9s::MySqlConfigSyntax:
            buildMySqlConf(content);
            break;

        case S9s::HaProxyConfigSyntax:
            buildHaProxyConf(content);
            break;

        case S9s::YamlSyntax:
            buildYaml(content);
            break;

        case S9s::UnknownSyntax:
            // If the syntax if not known no sense to generate anything.
            break;
    }
}

void
S9sConfigAstNode::buildMySqlConf(
        S9sString &content)
{
    switch (m_nodeType)
    {
        case Include:
        case IncludeDir:
            if (m_child1)
                m_child1->build(content);

            //content += m_origString;
            content += " ";

            if (m_child2)
                m_child2->build(content);
        break;

        case Section:
            if (!m_origString.empty())
            {
                content += "[";
                content += m_origString;
                content += "]";
            }
        break;
        
        case Commented:
            // FIXME: This is not perfect, we do not know the original strings
            // of these two operators.
            content += "# ";

            if (m_child1)
                m_child1->build(content);

            //content += " = ";
            content += m_origString;

            if (m_child2)
                m_child2->build(content);
        break;

        default:
            if (m_child1)
                m_child1->build(content);

            content += m_origString;

            if (m_child2)
                m_child2->build(content);
    } 
}

void
S9sConfigAstNode::buildHaProxyConf(
        S9sString &content)
{
    switch (m_nodeType)
    {
        case Include:
        case IncludeDir:
            S9S_WARNING("Include");
            if (m_child1)
                m_child1->buildHaProxyConf(content);

            //content += m_origString;
            content += " ";

            if (m_child2)
                m_child2->buildHaProxyConf(content);

            break;

        case Section:
            S9S_WARNING("Section");
            if (!m_origString.empty())
            {
                content += m_origString;
            }
            
            if (m_child1)
            {
                content += " ";
                m_child1->buildHaProxyConf(content);
            }

            break;
        
        case Commented:
            S9S_WARNING("Commented");
            // FIXME: This is not perfect, we do not know the original strings
            // of these two operators.
            content += "# ";

            if (m_child1)
                m_child1->buildHaProxyConf(content);

            //content += " = ";
            content += m_origString;

            if (m_child2)
                m_child2->buildHaProxyConf(content);
            
            break;

        case Assignment:
            S9S_WARNING("Assignment");
            if (m_child1)
            {
                content += "    ";
                m_child1->buildHaProxyConf(content);
            }

            if (m_child2)
            {
                content += " ";
                m_child2->buildHaProxyConf(content);
            }

            break;

        case LiteralList:
            S9S_WARNING("LiteralList");
            if (m_child1)
                m_child1->build(content);

            if (m_child2)
            {
                content += " ";
                m_child2->buildHaProxyConf(content);
            }

            break;

        default:
            S9S_WARNING("default: %s", nodeTypeToString(m_nodeType));
            if (m_child1)
                m_child1->buildHaProxyConf(content);

            content += m_origString;

            if (m_child2)
                m_child2->buildHaProxyConf(content);
    } 
}

void
S9sConfigAstNode::buildYaml(
        S9sString &content)
{
    switch (m_nodeType)
    {
        case Include:
        case IncludeDir:
            if (m_child1)
                m_child1->build(content);

            //content += m_origString;
            content += " ";

            if (m_child2)
                m_child2->build(content);
        break;

        case Section:
            for (int n = 0; n < indent(); ++n)
                content += " ";

            content += m_origString;
            content += ":\n";
            break;
        
        case Commented:
            // FIXME: This is not perfect, we do not know the original strings
            // of these two operators.
            content += "# ";

            if (m_child1)
                m_child1->build(content);

            //content += " = ";
            content += m_origString;

            if (m_child2)
                m_child2->build(content);
        
            break;

        case Assignment:
            for (int n = 0; n < indent(); ++n)
                content += " ";
                
            m_child1->build(content);
            content += m_origString;
            m_child2->build(content);
        
            break;

        default:
            if (m_child1)
                m_child1->build(content);

            content += m_origString;

            if (m_child2)
                m_child2->build(content);
    } 
}

void
S9sConfigAstNode::printDebug(
        int recursionLevel) const
{
    S9sString codePiece = m_origString;

    codePiece.replace("\n", "\\n");
    codePiece.replace("\r", "\\r");

    if (recursionLevel == 0)
    {
        printf("%04d:%03d %-14s ", 
                m_lineNumber, 
                indent(),
                nodeTypeToString(m_nodeType));
    } else {
        printf("%04d:%03d ", 
                m_lineNumber,
                indent());

        for (int n = 0; n < recursionLevel; ++n)
            printf("     ");

        printf("%-14s ", nodeTypeToString(m_nodeType));
    }

    printf("'%s'", STR(codePiece));
    printf("\n");

    if (m_child1)
        m_child1->printDebug(recursionLevel + 1);
    
    if (m_child2)
        m_child2->printDebug(recursionLevel + 1);
}

const char *
S9sConfigAstNode::nodeTypeToString(
        const NodeType type)
{
    switch (type)
    {
        case Keyword:
            return "Keyword";

        case Literal:
            return "Literal";

        case Comment:
            return "Comment";

        case Section:
            return "Section";

        case Assignment:
            return "Assignment";
        
        case Commented:
            return "Commented";

        case Include:
            return "Include";
        
        case IncludeDir:
            return "IncludeDir";
        
        case Variable:
            return "Variable";
        
        case NewLine:
            return "NewLine";

        case LiteralList:
            return "LiteralList";
    }

    return "Invalid";
}

S9sConfigAstNode *
S9sConfigAstNode::assignment(
        const S9sString &variableName,
        const S9sString &variableValue)
{
    S9sConfigAstNode *retval;

    retval = new S9sConfigAstNode(Assignment, "=");
    retval->m_child1 = new S9sConfigAstNode(Variable, STR(variableName));
    retval->m_child2 = new S9sConfigAstNode(Literal, STR(variableValue));
    return retval;
}

S9sConfigAstNode *
S9sConfigAstNode::newLine()
{
    return new S9sConfigAstNode(NewLine, "\n");
}

S9sConfigAstNode *
S9sConfigAstNode::section(
        const S9sString &sectionName)
{
    return new S9sConfigAstNode(Section, STR(sectionName));
}

/******************************************************************************
 * S9sClusterConfigParseContext implementation
 */
S9sClusterConfigParseContext::S9sClusterConfigParseContext(
        const char       *input,
        S9s::Syntax       syntax) :
    S9sParseContext(input),
    m_syntax(syntax)
{
}

S9sClusterConfigParseContext::~S9sClusterConfigParseContext()
{
    reset();
}

/**
 * Private function called before parsing, resets the context.
 */
void
S9sClusterConfigParseContext::reset()
{
    for (uint idx = 0; idx < m_ast.size(); ++idx)
        delete m_ast[idx];

    m_ast.clear();
}

/**
 * \returns the file names found as include files
 *
 * The file names are represented as they found, if the file holds an include
 * with a relative path it will be presented as a relativa path. The caller must
 * know where the parsed file was to interpret a relative path and convert it to
 * an absolute path.
 */
S9sMap<S9sString, int> 
S9sClusterConfigParseContext::includeFiles() const
{
    S9sMap<S9sString, int> retval;

    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        if (!m_ast[idx]->isInclude())
            continue;

        retval[m_ast[idx]->fileName()] = m_ast[idx]->lineNumber();
    }

    return retval;
}

S9sMap<S9sString, int> 
S9sClusterConfigParseContext::includeDirs() const
{
    S9sMap<S9sString, int> retval;

    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        if (!m_ast[idx]->isIncludeDir())
            continue;

        retval[m_ast[idx]->fileName()] = m_ast[idx]->lineNumber();
    }

    return retval;
}

/**
 * Collects and returns the variable names.
 */
S9sMap<S9sString, int> 
S9sClusterConfigParseContext::variableNames() const
{
    S9sMap<S9sString, int> retval;

    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        if (!m_ast[idx]->isAssignment())
            continue;

        retval[m_ast[idx]->leftValue()] = m_ast[idx]->lineNumber();
    }

    return retval;
}

/**
 * Collects and returns the list of variables. If the variable name is provided
 * returns only those variables that have the same name.
 */
S9sVariantList
S9sClusterConfigParseContext::collectVariables(
        const S9sString &variableName,
        const S9sString &filePath) const
{
    S9sVariantList retval;
    S9sString      sectionName;
    
    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        bool       isEqual;
        S9sString  thisName;

        if (m_ast[idx]->isSection())
            sectionName = m_ast[idx]->sectionName();

        if (!m_ast[idx]->isAssignment())
            continue;

        thisName = m_ast[idx]->leftValue();
        isEqual  = nameEqual(m_ast[idx]->leftValue(), variableName);

        if (!variableName.empty() && !isEqual)
            continue;

        S9sVariantMap theMap;
        theMap["variablename"] = thisName;
        theMap["linenumber"]   = m_ast[idx]->lineNumber();
        theMap["value"]        = m_ast[idx]->rightValue();
        theMap["filepath"]     = filePath;
        theMap["section"]      = sectionName;

        retval << S9sVariant(theMap);
    }

    return retval;
}

/**
 * \param sectionName the name of the section where we change the value or an
 *   empty string to change the value only in the global section
 * \returns true if the variable is found and the value has been changed.
 *
 * Changes a variable value. If the variable is commented-out it will be
 * re-enabled. 
 */
bool
S9sClusterConfigParseContext::changeVariable(
        const S9sString &sectionName,
        const S9sString &variableName,
        const S9sString &variableValue)
{
    S9sString  currentSection;
    bool       retval = false;

    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        S9sConfigAstNode  *node = m_ast[idx];
        bool               isEqual;

        if (node->isSection())
            currentSection = node->sectionName();

        if (!sectionEqual(currentSection, sectionName))
            continue;

        isEqual = nameEqual(node->leftValue(), variableName);
        if (node->isAssignment() && isEqual)
        {
            // We found the variable, it is there.
            node->setRightValue(variableValue);
            retval = true;
        } else if (node->isCommented() && isEqual)
        {
            // We found the variable and it is there, but it is commented out.
            node->setRightValue(variableValue);
            node->setType(S9sConfigAstNode::Assignment);
            retval = true;
        }
    }

    return retval;
}

/**
 * Overloaded version of the method that ignores the section name and changes
 * every occurences of the variable assignment where it finds it. If there are
 * multiple occurences all of them will be changed.
 */
bool
S9sClusterConfigParseContext::changeVariable(
        const S9sString &variableName,
        const S9sString &variableValue)
{
    bool       retval = false;

    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        S9sConfigAstNode *node = m_ast[idx];
        bool              isEqual;

        isEqual = nameEqual(node->leftValue(), variableName);
        if (node->isAssignment() && isEqual)
        {
            node->setRightValue(variableValue);
            retval = true;
        } else if (node->isCommented() && isEqual)
        {
            node->setRightValue(variableValue);
            node->setType(S9sConfigAstNode::Assignment);
            retval = true;
        }
    }

    return retval;
}

bool
S9sClusterConfigParseContext::disableVariable(
        const S9sString &sectionName,
        const S9sString &variableName)
{
    S9sString currentSection;
    bool       retval = false;

    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        S9sConfigAstNode *node = m_ast[idx];
        bool isEqual;

        if (node->isSection())
            currentSection = node->sectionName();

        if (!sectionEqual(currentSection, sectionName))
            continue;

        isEqual = nameEqual(node->leftValue(), variableName);
        if (node->isAssignment() && isEqual)
        {
            node->setType(S9sConfigAstNode::Commented);
            retval = true;
        } else if (node->isCommented() && isEqual)
        {
            // Already disabled.
            retval = true;
        }
    }

    return retval;
}

bool
S9sClusterConfigParseContext::disableVariable(
        const S9sString &variableName)
{
    bool       retval = false;

    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        S9sConfigAstNode *node = m_ast[idx];
        bool isEqual;

        isEqual = nameEqual(node->leftValue(), variableName);
        if (node->isAssignment() && isEqual)
        {
            node->setType(S9sConfigAstNode::Commented);
            retval = true;
        } else if (node->isCommented() && isEqual)
        {
            // Already disabled.
            retval = true;
        }
    }

    return retval;
}

bool
S9sClusterConfigParseContext::removeVariable(
        const S9sString &sectionName,
        const S9sString &variableName)
{
    S9sString  currentSection;
    uint       index = 0u;
    bool       found = false;

    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        S9sConfigAstNode *node = m_ast[idx];
        bool              isEqual;

        if (node->isSection())
            currentSection = node->sectionName();

        if (!sectionEqual(currentSection, sectionName))
            continue;

        isEqual = nameEqual(node->leftValue(), variableName);
        if (isEqual && (node->isAssignment() || node->isCommented()))
        {
            // save the 'index' so we found the item to be removed
            found = true;
            index = idx;

            break;
        }
    }

    // ok lets remove the specified variable then
    if (found && m_ast.size() > index)
    {
        delete m_ast[index];
        m_ast.erase (m_ast.begin() + index);
    }

    return true;
}

bool
S9sClusterConfigParseContext::removeSection(
        const S9sString &sectionName)
{
    S9sString currentSection;

    // we delete everything from this section
    // beginning until the next section is found
    uint       startFrom = 0u;
    uint       deleteCnt = 0u;

    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        S9sConfigAstNode *node = m_ast[idx];

        if (node->isSection())
        {
            currentSection = node->sectionName();

            if (sectionEqual (currentSection, sectionName))
            {
                // ok, so this sections starts now
                startFrom = idx;
            }
        }

        if (sectionEqual(currentSection, sectionName))
        {
            // we are inside the 'sectionName' so we must remove this line
            deleteCnt++;
        }
    }

    // ok lets remove the whole section then
    if (deleteCnt > 0u && m_ast.size() >= (startFrom + deleteCnt))
    {
        for (uint idx = startFrom; idx < (startFrom+deleteCnt); ++idx)
            delete m_ast[idx];

        m_ast.erase (m_ast.begin() + startFrom,
                m_ast.begin() + (startFrom + deleteCnt));
    }

    return true;
}

/**
 * Adds a variable to the given section. If the section is not found in the file
 * the section will be added before the variable.
 */
bool
S9sClusterConfigParseContext::addVariable(
        const S9sString &sectionName,
        const S9sString &variableName,
        const S9sString &variableValue)
{
    S9sString  currentSection;
    int        lastCandidate = -1;

    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        S9sConfigAstNode *node = m_ast[idx];

        if (node->isSection())
            currentSection = node->sectionName();

        if (!sectionEqual(currentSection, sectionName))
            continue;

        if (node->isAssignment() || node->isSection())
        {
            lastCandidate = idx;
        }
    }

    if (lastCandidate < 0)
    {
        // section without name must be handled specially,
        // lets insert then the new value to the beginning
        // of the file
        if (sectionName.empty ()) {
            lastCandidate = 0;
        } else {
            m_ast << S9sConfigAstNode::newLine();
            m_ast << S9sConfigAstNode::section(sectionName);
        
            lastCandidate = m_ast.size() - 1;
        }
    }


    if (lastCandidate > -1)
    {
        if (lastCandidate + 1 < (int) m_ast.size() &&
                m_ast[lastCandidate + 1]->isNewLine())
        {
            ++lastCandidate;
        } else {
            ++lastCandidate;
            m_ast.insert(
                    m_ast.begin() + lastCandidate, 
                    S9sConfigAstNode::newLine());
        }

        /*
         * Creating and inserting a new assignment sub-tree.
         */
        lastCandidate++;
        m_ast.insert(
                m_ast.begin() + lastCandidate,
                S9sConfigAstNode::assignment(variableName, variableValue));
            
        /*
         * Adding a new line node.
         */
        ++lastCandidate;
        m_ast.insert(
                m_ast.begin() + lastCandidate, 
                S9sConfigAstNode::newLine());
    }

    return true;
}

/**
 * \param sectionName the name of the section to check or an empty string to
 *   check the global variables
 * \param includingDisabled if it is true the commented-out variables are also
 *   checked
 * \returns true if the given section has a variable with the given name
 */
bool
S9sClusterConfigParseContext::hasVariable(
        const S9sString &sectionName,
        const S9sString &variableName,
        bool              includingDisabled)
{
    S9sMap<int, S9sVariantList> pathToIndent;
    S9sVariantList  path;
    int            currentIndent = 0;
    S9sString      currentSection;
    bool           retval = false;

    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        S9sConfigAstNode *node = m_ast[idx];
        bool isEqual;

        if (node->isNewLine())
        {
            continue;
        } if (node->isSection())
        {
            currentSection = node->sectionName();

            if (path.empty() || node->indent() > currentIndent)
            {
                currentIndent = node->indent();
                pathToIndent[currentIndent] = path;
                path << currentSection;
            } else if (node->indent() < currentIndent)
            {
                path = pathToIndent[node->indent()];
                currentIndent = node->indent();
            }
        
            S9S_DEBUG("-----------");
            S9S_DEBUG("SECTION");
            S9S_DEBUG("  node     : '%s'", STR(node->origString()));
            S9S_DEBUG("  indent   : %d", node->indent());
            S9S_DEBUG("  path     : '%s'", STR(path.toString('.')));
        } else if (node->isAssignment()) 
        {
            S9sVariantList fullPath = path;

            fullPath << node->leftValue();

            S9S_DEBUG("-----------");
            S9S_DEBUG("ASSIGNMENT");
            S9S_DEBUG("  name     : '%s'", STR(node->leftValue()));
            S9S_DEBUG("  fullPath : %s", STR(fullPath.toString('.')));
            S9S_DEBUG("  indent   : %d", node->indent());
            S9S_DEBUG("  path     : '%s'", STR(path.toString('.')));
        }
        

        if (!sectionEqual(currentSection, sectionName))
            continue;

        isEqual = nameEqual(node->leftValue(), variableName);
        if (node->isAssignment() && isEqual)
        {
            retval = true;
            break;
        } else if (includingDisabled &&
                node->isCommented() && isEqual)
        {
            retval = true;
            break;
        }
    }

    return retval;
}

/**
 * \returns true if the file has a section with the given name
 */
bool
S9sClusterConfigParseContext::hasSection(
        const S9sString &sectionName)
{
    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        S9sConfigAstNode *node = m_ast[idx];

        if (!node->isSection())
            continue;

        if (sectionEqual(node->sectionName(), sectionName))
            return true;
    }

    return false;
}

/**
 * This method is called by the parser to store parsed items when they are
 * found.
 *
 * This will steal the node object.
 */
void
S9sClusterConfigParseContext::append(
        S9sConfigAstNode *node)
{
    node->setLineNumber(lineNumber());
    node->setSyntax(m_syntax);

    if (node->isNewLine())
        incrementLineNumber();

    m_ast << node;
}

/**
 * Takes the internal representation of a parsed configuration file and builds
 * the lines that can be used as an actual configuration file. So modification
 * of a configuration file takes three steps: 1) parsing 2) modification and 3)
 * building.
 */
void 
S9sClusterConfigParseContext::build(
        S9sString &content)
{
    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        m_ast[idx]->build(content);
    }
}

/**
 * Prints the internal representation of the parsed data.
 */
void
S9sClusterConfigParseContext::printDebug() const
{
    for (uint idx = 0; idx < m_ast.size(); ++idx)
    {
        //if (m_ast[idx]->isNewLine())
        //    continue;

        m_ast[idx]->printDebug();
    }
}

/******************************************************************************
 * S9sConfigFile implementation.
 */

/**
 * The constructor that creates a S9sConfigFile with defaults.
 */
S9sConfigFile::S9sConfigFile() :
    m_priv(new S9sConfigFilePrivate)
{
}

/**
 * A constructor that creates a S9sConfigFile and sets what syntax should it
 * expect. The HaProxy config file for example has a syntax that is very 
 * different from a MySQL config file and should be parsed using a different
 * grammar.
 */
S9sConfigFile::S9sConfigFile(
        S9s::Syntax syntax) :
    m_priv(new S9sConfigFilePrivate)
{
    m_priv->m_syntax = syntax;
}
        
S9sConfigFile::S9sConfigFile(
        const S9sConfigFile &orig)
{
	m_priv = orig.m_priv;

	if (m_priv) 
		m_priv->ref();
}

S9sConfigFile::~S9sConfigFile()
{
	if (m_priv && m_priv->unRef() == 0)
    {
        delete m_priv;
        m_priv = 0;
	}     
}

S9sConfigFile &
S9sConfigFile::operator= (
		const S9sConfigFile &rhs)
{
	if (this == &rhs)
		return *this;

	if (m_priv && m_priv->unRef() == 0)
    {
        delete m_priv;
        m_priv = 0;
	}

	m_priv = rhs.m_priv;
	if (m_priv) 
    {
		m_priv->ref ();
	}

	return *this;
}

bool 
S9sConfigFile::save(
        S9sString &errorString)
{
    if (m_priv->filename.empty())
    {
        errorString = "No filename has specified.";
        return false;
    }

    S9sString content;
    build(content);

    S9sFile file(m_priv->filename);
    if (! file.writeTxtFile(content))
    {
        errorString = file.errorString();
        return false;
    }

    return true;
}

/**
 * Include level for a file that is loaded by the application is 0. Config files
 * that are included from this file has the include level 1, files that included
 * from files with include level 1 has iclude level 2 and so on.
 */
void
S9sConfigFile::setIncludeLevel(
        int value) 
{
    m_priv->m_includeLevel = value;
}

/**
 * \returns the include level set by the setIncludeLevel()
 */
int 
S9sConfigFile::includeLevel() const
{
    return m_priv->m_includeLevel;
}

void
S9sConfigFile::setSize(
        int size) 
{
    m_priv->m_size = size;
}

/**
 * Sets the file path for the object. This file name is used in error messages
 * and is used as a data source later.
 */
void
S9sConfigFile::setFileName(
        const S9sString &fileName)
{
    m_priv->filename = fileName;
}

const S9sString &
S9sConfigFile::fileName() const
{
    return m_priv->filename;
}

/**
 * This function checks if the file with the name/path set using the 
 * setFileName() method exists.
 */
bool
S9sConfigFile::sourceFileExists() const
{
    if (m_priv->filename.empty())
        return false;

    S9sFile file(m_priv->filename);

    return file.exists();
}

void
S9sConfigFile::setName(
        const S9sString &name)
{
    m_priv->name = name;
}

S9sString
S9sConfigFile::name() const
{
    return m_priv->name;
}


int 
S9sConfigFile::size() const
{
    return m_priv->m_size;
}

void
S9sConfigFile::setCrc(
        ulonglong   crc)
{
    m_priv->m_crc = crc;
}

ulonglong
S9sConfigFile::crc() const
{
    return m_priv->m_crc;
}

S9sString
S9sConfigFile::crcStr() const
{
    S9sString retval;
    retval.sprintf ("%08llx", m_priv->m_crc);
    return retval;
}

void
S9sConfigFile::setPath(
        const S9sString &path)
{
    m_priv->m_fullpath = path;
}

S9sString
S9sConfigFile::path() const
{
    return m_priv->m_fullpath;
}

bool
S9sConfigFile::hasChange() const
{
    return m_priv->m_hasChange;
}

void
S9sConfigFile::setHasChange(
        bool hasChange)
{
    m_priv->m_hasChange = hasChange;
}

ulonglong
S9sConfigFile::timeStamp() const 
{
    return m_priv->m_timeStamp;
}

void
S9sConfigFile::setTimeStamp (
        const ulonglong     timeStamp)
{
    if (timeStamp == 0ull)
        m_priv->m_timeStamp = (ulonglong) time(0);
    else
        m_priv->m_timeStamp = timeStamp;
}

void
S9sConfigFile::setContent(
        const S9sString &content)
{
    m_priv->m_content = content;

    // FIXME: this is might be not the right place to do this
    if (m_priv->m_size <= 0)
        setSize(m_priv->m_content.size());
}

const S9sString &
S9sConfigFile::content() const
{
    return m_priv->m_content;
}

/**
 * Collects all the variable name from the parsed data.
 */
S9sVariantList
S9sConfigFile::collectVariables(
        const S9sString &variableName) const
{
    S9sVariantList retval;

    if (m_priv->m_parseContext)
        retval = m_priv->m_parseContext->collectVariables(
                variableName, m_priv->m_fullpath);

    return retval;
}

void
S9sConfigFile::appendSearchGroup(
        const S9sString &groupName)
{
    m_priv->m_searchGroups << groupName;
}

/**
 * \returns the variable value as a string if the variable can be found in any
 *   of the search groups, returns the empty string if not.
 *
 * The search groups (sections) are searched in order, so if the variable is
 * defined in multiple sections always the one found first is returned. If
 * search groups are not specified the first value defined in the file will be
 * returned regardless its section name.
 */
S9sString
S9sConfigFile::variableValue(
        const S9sString &variableName) const
{
    S9sVariantList variables;
    S9sString      retval;

    variables = collectVariables(variableName);
    if (m_priv->m_searchGroups.empty())
    {
        for (uint idx = 0; idx < variables.size(); ++idx)
        {
            S9sString group = variables[idx]["section"].toString();
    
            //if (!hasSearchGroup(group))
            //    continue;

            retval = variables[idx]["value"].toString();
            break;
        }
    } else {
        for (uint idx = 0u; idx < m_priv->m_searchGroups.size(); ++idx)
        {
            S9sString searchGroup = m_priv->m_searchGroups[idx].toString();

            for (uint idx = 0; idx < variables.size(); ++idx)
            {
                S9sString group = variables[idx]["section"].toString();
    
                if (searchGroup != group)
                    continue;

                return variables[idx]["value"].toString();
            }
        }
    }

    return retval.unQuote();
}

/**
 * \returns the value of the given variable but only if it exists in the
 *   specified section.
 *
 * Please note that if the variable is not defined in the given search group the
 * empty string will be returned.
 */
S9sString
S9sConfigFile::variableValue(
        const S9sString &sectionName,
        const S9sString &variableName) const
{
    S9sVariantList variables;
    S9sString      retval;

    variables = collectVariables(variableName);
    for (uint idx = 0; idx < variables.size(); ++idx)
    {
        S9sString group = variables[idx]["section"].toString();

        if (group != sectionName)
            continue;

        retval = variables[idx]["value"].toString();
        break;
    }

    return retval;
}

/**
 * \returns the value of the specified name in the specified section or in the
 *   search groups or the specified default value.
 */
S9sString
S9sConfigFile::variableValue(
        const S9sString &sectionName,
        const S9sString &variableName,
        const S9sString &defaultValue) const
{
    S9sString retval;

    retval = variableValue(sectionName, variableName);

    if (retval.empty())
        retval = variableValue(variableName);
        
    if (retval.empty())
        retval = defaultValue;
    
    return retval;
}


/**
 * The same as the variableValue() method but this one returns the value as a
 * string list. The list of strings can be separated by ';' or ',' in the
 * configuration file.
 */
S9sVariantList
S9sConfigFile::variableValueAsStringList(
        const S9sString &variableName)
{
    S9sString tmp = variableValue(variableName);

    if (tmp.empty())
    {
        return S9sVariantList();
    }
     
    return tmp.split();
}

/**
 * \returns true if the file has a section with the given name
 */
bool 
S9sConfigFile::hasSection(
        const S9sString &sectionName)
{
    if (m_priv->m_parseContext)
        return m_priv->m_parseContext->hasSection(sectionName);

    return false;
}

/**
 * \param sectionName the name of the section where we change the value or an
 *   empty string to change the value only in the global section
 * \returns true if the variable is found and the value has been changed.
 *
 * Changes a variable value. If the variable is commented-out it will be
 * re-enabled. 
 */
bool
S9sConfigFile::changeVariable(
        const S9sString &sectionName,
        const S9sString &variableName,
        const S9sString &variableValue)
{
    if (m_priv->m_parseContext)
        return m_priv->m_parseContext->changeVariable(
                sectionName, variableName, variableValue);

    return false;
}

/**
 * Overloaded version of the method that ignores the section name and changes
 * every occurences of the variable assignment where it finds it. If there are
 * multiple occurences all of them will be changed.
 */
bool
S9sConfigFile::changeVariable(
        const S9sString &variableName,
        const S9sString &variableValue)
{
    if (m_priv->m_parseContext)
        return m_priv->m_parseContext->changeVariable(
                variableName, variableValue);

    return false;
}

bool
S9sConfigFile::disableVariable(
        const S9sString &sectionName,
        const S9sString &variableName)
{
    if (m_priv->m_parseContext)
        return m_priv->m_parseContext->disableVariable(
                sectionName, variableName);

    return false;
}

bool
S9sConfigFile::disableVariable(
        const S9sString &variableName)
{
    if (m_priv->m_parseContext)
        return m_priv->m_parseContext->disableVariable(variableName);

    return false;
}

bool
S9sConfigFile::removeVariable(
        const S9sString &sectionName,
        const S9sString &variableName)
{
    if (m_priv->m_parseContext)
        return m_priv->m_parseContext->removeVariable(
                sectionName, variableName);

    return false;
}

bool
S9sConfigFile::removeSection(
        const S9sString &sectionName)
{
    if (m_priv->m_parseContext)
        return m_priv->m_parseContext->removeSection(sectionName);

    return false;
}


/**
 * Adds a variable to the given section. If the section is not found in the file
 * the section will be added before the variable.
 */
bool
S9sConfigFile::addVariable(
        const S9sString &sectionName,
        const S9sString &variableName,
        const S9sString &variableValue)
{
    if (m_priv->m_parseContext)
        return m_priv->m_parseContext->addVariable(
                sectionName, variableName, variableValue);

    return false;
}

bool
S9sConfigFile::addVariable(
        const S9sString &variableName,
        const S9sString &variableValue)
{
    if (m_priv->m_parseContext)
        return m_priv->m_parseContext->addVariable(
                S9sString(), variableName, variableValue);

    return false;
}

/**
 * \param sectionName the name of the section to check or an empty string to
 *   check the global variables
 * \param includingDisabled if it is true the commented-out variables are also
 *   checked
 * \returns true if the given section has a variable with the given name
 */
bool
S9sConfigFile::hasVariable(
        const S9sString &sectionName,
        const S9sString &variableName,
        bool              includingDisabled)
{
    if (m_priv->m_parseContext)
    {
        return m_priv->m_parseContext->hasVariable(
                sectionName, variableName, includingDisabled);
    }

    return false;
}

bool
S9sConfigFile::setVariable(
        const S9sString &sectionName,
        const S9sString &variableName,
        const S9sString &variableValue)
{
    bool retval = false;

    if (hasVariable(sectionName, variableName))
    {
        retval = changeVariable(sectionName, variableName, variableValue);
    } else if (hasVariable("", variableName))
    {
        retval = changeVariable(variableName, variableValue);
    } else {
        retval = addVariable(sectionName, variableName, variableValue);
    }

    return retval;
}

/**
 * \returns true if the parsing was succesful, false otherwise
 *
 * This function will parse the configuration that was set with the setContent()
 * before.
 */
bool
S9sConfigFile::parse()
{
    int retval;
    bool success = false;

    if (m_priv->m_parseContext)
        delete m_priv->m_parseContext;

    /* 
     * Setting up.
     */
    m_priv->m_parseContext = 
        new S9sClusterConfigParseContext(
                STR(m_priv->m_content), 
                m_priv->m_syntax);

    m_priv->m_parseContext->reset();

    switch (m_priv->m_syntax)
    {
        case S9s::GenericConfigSyntax:
        case S9s::MySqlConfigSyntax:
            /* Init. */
            config_lex_init(&m_priv->m_parseContext->m_flex_scanner);
            config_set_extra(
                    m_priv->m_parseContext, 
                    m_priv->m_parseContext->m_flex_scanner);

            /* Doing the parsing. */
            retval = config_parse(*m_priv->m_parseContext);
            success = retval == 0;

            /* Finalizing. */
            config_lex_destroy(m_priv->m_parseContext->m_flex_scanner);
            break;

        case S9s::HaProxyConfigSyntax:
#if 0
            /* Init. */
            haconfig_lex_init(&m_priv->m_parseContext->m_flex_scanner);
            haconfig_set_extra(
                    m_priv->m_parseContext, 
                    m_priv->m_parseContext->m_flex_scanner);

            /* Doing the parsing. */
            retval = haconfig_parse(*m_priv->m_parseContext);
            success = retval == 0;

            /* Finalizing. */
            haconfig_lex_destroy(m_priv->m_parseContext->m_flex_scanner);
#endif
            break;
        
        case S9s::YamlSyntax:
#if 0
            /* Init. */
            yaml_lex_init(&m_priv->m_parseContext->m_flex_scanner);
            yaml_set_extra(
                    m_priv->m_parseContext, 
                    m_priv->m_parseContext->m_flex_scanner);

            /* Doing the parsing. */
            retval = yaml_parse(*m_priv->m_parseContext);
            success = retval == 0;

            /* Finalizing. */
            yaml_lex_destroy(m_priv->m_parseContext->m_flex_scanner);
#endif
            break;

        case S9s::UnknownSyntax:
#if 0
            // Here unknown syntax simply means autodetect.
            m_priv->m_syntax = S9s::MySqlConfigSyntax;
            if (parse())
                return true;

            m_priv->m_syntax = S9s::HaProxyConfigSyntax;
            if (parse())
                return true;
           
            m_priv->m_syntax = S9s::YamlSyntax;
            if (parse())
                return true;

            // ok, we failed, here we return to the most probable syntax to get
            // proper error messages.
            m_priv->m_syntax = S9s::MySqlConfigSyntax;
            return parse();
#endif
            break;
    }

    m_priv->m_parseContext->m_flex_scanner = NULL;
    if (success)
    {
        S9S_DEBUG("ERROR: %s", STR(m_priv->m_parseContext->errorString()));
    }

    return success;
}

/**
 * Convenience function to set the content and call the parser.
 */
bool
S9sConfigFile::parse(
        const char *source)
{
    setContent(source);
    return parse();
}

/**
 * This function will try to parse the source file set by calling the 
 * setFileName() function.
 */
bool
S9sConfigFile::parseSourceFile()
{
    S9sFile   file(m_priv->filename);
    S9sString content;

    file.readTxtFile(content);
    return parse(STR(content));
}

/**
 * Collects the include files found while parsing the file. This method will not
 * clean the list, it will check if the include file is already there, so
 * calling this method on multiple S9sConfigFile objects with the same list
 * will collect all the include files while eliminating duplicates.
 *
 * All the file names are going to be stored with absolute path. For this the
 * method will use the base directory taken from the string set by the 
 * setPath() function.
 */
void
S9sConfigFile::collectIncludeFiles(
        S9sVariantList &includeFileNames) const
{
    if (!m_priv->m_parseContext)
        return;

    S9sVector<S9sString> tmp = m_priv->m_parseContext->includeFiles().keys();
    S9sString dirName = S9sFile::dirname(m_priv->m_fullpath);

    for (uint idx = 0; idx < tmp.size(); ++idx)
    {
        S9sString includeString = tmp[idx];

        if (!S9sFile::isAbsolutePath(includeString))
            includeString = S9sFile::buildPath(dirName, includeString);

        if (includeFileNames.contains(includeString))
            continue;

        includeFileNames << includeString;
    }
}

void
S9sConfigFile::collectIncludeDirs(
        S9sVariantList &includeDirNames) const
{
    if (!m_priv->m_parseContext)
        return;

    S9sVector<S9sString> tmp = m_priv->m_parseContext->includeDirs().keys();
    S9sString dirName = S9sFile::dirname(m_priv->m_fullpath);

    for (uint idx = 0; idx < tmp.size(); ++idx)
    {
        S9sString includeString = tmp[idx];

        if (!S9sFile::isAbsolutePath(includeString))
            includeString = S9sFile::buildPath(dirName, includeString);

        if (includeDirNames.contains(includeString))
            continue;

        includeDirNames << includeString;
    }
}

/**
 * This function will collect the names of all the variables found in the config
 * file. The names will be added to the list and then the list is sorted.
 */
void
S9sConfigFile::collectVariableNames(
        S9sVariantList &variableNames) const
{
    if (!m_priv->m_parseContext)
        return;

    S9sVector<S9sString> tmp = m_priv->m_parseContext->variableNames().keys();
    for (uint idx = 0; idx < tmp.size(); ++idx)
    {
        S9sString variableName = tmp[idx];

        if (variableNames.contains(variableName))
            continue;

        variableNames << variableName;
    }

    variableNames.sort();
}

/**
 * \returns the human readable format error description
 */
S9sString
S9sConfigFile::errorString() const
{
    if (m_priv->m_parseContext)
        return m_priv->m_parseContext->errorString();

    return S9sString();
}
        
/**
 * Takes the internal representation of a parsed configuration file and builds
 * the lines that can be used as an actual configuration file. So modification
 * of a configuration file takes three steps: 1) parsing 2) modification and 3)
 * building.
 */
void 
S9sConfigFile::build(
        S9sString &content)
{
    content.clear();

    if (m_priv->m_parseContext)
        m_priv->m_parseContext->build(content);
}

void
S9sConfigFile::printDebug() const
{
    printf("\n");
    printf("--- S9sConfigFile ------\n");
    if (m_priv->m_parseContext)
        m_priv->m_parseContext->printDebug();
    fflush(stdout);
}

/******************************************************************************
 * S9sConfigFileSet implementation
 */

/**
 * \returns true if the file with the given file path is already in the set.
 */
bool 
S9sConfigFileSet::contains(
        const S9sString &filePath)
{
    for (uint idx = 0u; idx < this->size(); ++idx)
        if (at(idx).path() == filePath)
            return true;

    return false;
}

/**
 * Convenience function to parse all the files in a set.
 */
bool
S9sConfigFileSet::parse()
{
    bool retval = true;

    m_errorStrings.clear();

    for (uint idx = 0; idx < size(); ++idx)
    {
        bool success;

        success = at(idx).parse();
        if (!success)
        {
            S9sString errorString;

            errorString.sprintf("Error in file '%s': %s.",
                    STR(at(idx).path()), 
                    STR(at(idx).errorString()));

            m_errorStrings << errorString;
            retval = false;
        }
    }

    return retval;
}

/**
 * Collects the names of the include files in the set.
 */
void 
S9sConfigFileSet::collectIncludeFiles(
        S9sVariantList &includeFileNames) const
{
    includeFileNames.clear();
    
    for (uint idx = 0; idx < size(); ++idx)
        at(idx).collectIncludeFiles(includeFileNames);
}

/**
 * The same variable can be set in multiple sections or even in multiple files,
 * so this method is able to return multiple veriables with the same name. In
 * the return variable every item is a S9sVariant that holds a S9sMap that
 * contains various keys represeinting the variable. Currently the keys are:
 * "variablename", "linenumber", "value", "filepath", "section".
 */
S9sVariantList
S9sConfigFileSet::collectVariables(
        const S9sString &variableName) const
{
    S9sVariantList retval;
    
    for (uint idx = 0; idx < size(); ++idx)
    {
        S9sVariantList thisList;

        thisList = at(idx).collectVariables(variableName);

        if (!thisList.empty())
            retval.append(thisList);
    }

    return retval;
}

/**
 * Returns the value of a given variable in a given section.
 */
S9sString 
S9sConfigFileSet::variableValue(
        const S9sString &sectionName,
        const S9sString &variableName) const
{
    S9sVariantList variables;
    S9sString      retval;

    variables = collectVariables(variableName);
    for (uint idx = 0; idx < variables.size(); ++idx)
    {
        S9sString group = variables[idx]["section"].toString();

        if (sectionName != group)
            continue;

        retval = variables[idx]["value"].toString();
        break;
    }

    return retval;
}


/**
 * Changes the value for a given variable. If the variable is "commented out" it
 * will be enabled again.
 */
void
S9sConfigFileSet::changeVariable(
        const S9sString &section,
        const S9sString &variableName,
        const S9sString &value)
{
    for (uint idx = 0u; idx < size(); ++idx)
    {
        S9sConfigFile &configFile = at(idx);

        if (!configFile.hasVariable(section, variableName, true))
            continue;

        /*
         * If we are here the variable is found (even defined or commented out),
         * so we can change the value, uncomment it if necessary, and that's
         * all.
         */
        configFile.changeVariable(section, variableName, value);
        return;
    }

    /*
     * If we are here the variable was not found, so we need to add it.
     */
    if (size() > 0u)
        at(0u).addVariable(section, variableName, value);
}

/*
 * Makes the variable "commented out".
 */
void
S9sConfigFileSet::disableVariable(
        const S9sString &sectionName,
        const S9sString &variableName)
{
    for (uint idx = 0u; idx < size(); ++idx)
    {
        at(idx).disableVariable(sectionName, variableName);
    }
}

/**
 * Removes a whole section
 */
void
S9sConfigFileSet::removeSection(
        const S9sString    &sectionName)
{
    for (uint idx = 0u; idx < size(); ++idx)
    {
        at(idx).removeSection (sectionName);
    }
}

/**
 * Puts the values into a variant map. This method is created to send the whole
 * set to the UI, so it only holds the sections, names and values (no file
 * names, line numbers etc.).
 \code
{
    "project": 
    {
        "tags": "first_tag;second_tag"
    }
}
 \endcode
 */
void
S9sConfigFileSet::toVariantMap(
        S9sVariantMap &map) 
{
    for (uint idx = 0u; idx < size(); ++idx)
    {
        S9sVariantList variables = collectVariables("");

        for (uint idx = 0u; idx < variables.size(); ++idx)
        {
            S9sVariantMap       variable = variables[idx].toVariantMap();
            const S9sString     section  = variable["section"].toString();
            const S9sString     name     = variable["variablename"].toString();
            const S9sString     value    = variable["value"].toString();

            if (!map.contains(section))
                map[section] = S9sVariantMap();

            map[section][name] = value;
        }
    }
}

/**
 * Appends a new file to the file set, returns a reference to this file.
 */
S9sConfigFile &
S9sConfigFileSet::appendNewFile(
        S9s::Syntax syntax)
{
    S9sConfigFile file(syntax);

    push_back(file);
    return back();
}
