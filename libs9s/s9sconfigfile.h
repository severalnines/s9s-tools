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
 * along with Foobar. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "S9sString"
#include "S9sVariant"
#include "S9sMap"
#include "S9sVector"
#include "S9sParseContext"

class S9sConfigAstNode;
class S9sConfigFilePrivate;

/**
 * Represents one piece of information that was found while parsing a config
 * file.
 */
class S9sConfigAstNode
{
    public:
        enum NodeType 
        {
            Keyword,
            Literal,
            Comment,
            Section,
            Assignment,
            Commented,
            Include,
            IncludeDir,
            Variable,
            NewLine,
            LiteralList
        };

        S9sConfigAstNode(
                S9sConfigAstNode::NodeType  nodeType,
                const char                  *origString);

        S9sConfigAstNode(
                S9sConfigAstNode::NodeType nodeType,
                S9sConfigAstNode           *child1,
                S9sConfigAstNode           *child2 = NULL);

        ~S9sConfigAstNode();

        void setSyntax(const S9s::Syntax syntax);

        void setChildren(
                S9sConfigAstNode  *child1,
                S9sConfigAstNode  *child2);

        int lineNumber() const { return m_lineNumber; };
        void setLineNumber(int lineNumber);
        
        int indent() const { return m_indent; };
        void setIndent(int indent);
        
        const S9sString &origString() const;
        void setType(S9sConfigAstNode::NodeType nodeType);
        S9sString fileName() const;
        S9sString leftValue() const;
        
        S9sString rightValue() const;
        void setRightValue(const S9sString &newValue) const;

        S9sString sectionName() const;

        static const char *nodeTypeToString(const NodeType type);
        void build(S9sString &content);
        void printDebug(int recursionLevel = 0) const;

        bool isNewLine() const { return m_nodeType == NewLine; };
        bool isInclude() const { return m_nodeType == Include; };
        bool isIncludeDir() const { return m_nodeType == IncludeDir; };
        bool isAssignment() const { return m_nodeType == Assignment; };
        bool isCommented() const { return m_nodeType == Commented; };
        bool isSection() const { return m_nodeType == Section; };

        static S9sConfigAstNode *
            assignment(
                    const S9sString &variableName,
                    const S9sString &varianleValue);

        static S9sConfigAstNode *newLine();

        static S9sConfigAstNode *
            section(
                    const S9sString &sectionName);

    private:
        S9sConfigAstNode(const S9sConfigAstNode&) {}
        S9sConfigAstNode &operator= (const S9sConfigAstNode&) {return *this;}

        void buildMySqlConf(S9sString &content);
        void buildHaProxyConf(S9sString &content);
        void buildYaml(S9sString &content);

    private:
        S9s::Syntax        m_syntax;
        NodeType           m_nodeType;
        S9sString          m_origString;
        int                m_lineNumber;
        int                m_indent;
        S9sConfigAstNode  *m_child1;
        S9sConfigAstNode  *m_child2;
};

/**
 * Class that holds all the necessery information while parsing a config file,
 * holds all the parsed data after.
 */
class S9sClusterConfigParseContext : public S9sParseContext
{
    public:
        S9sClusterConfigParseContext(const char *input, S9s::Syntax syntax);
        virtual ~S9sClusterConfigParseContext();
        

        void setValues(S9sVariantMap *values);

        virtual void reset();

        S9sMap<S9sString, int> includeFiles() const;
        S9sMap<S9sString, int> includeDirs() const;
        S9sMap<S9sString, int> variableNames() const;

        S9sVariantList collectVariables(
                const S9sString &variableName,
                const S9sString &filePath) const;
        
        bool changeVariable(
                const S9sString &sectionName,
                const S9sString &variableName,
                const S9sString &variableValue);
        
        bool changeVariable(
                const S9sString &variableName,
                const S9sString &variableValue);

        bool disableVariable(
                const S9sString &sectionName,
                const S9sString &variableName);
        
        bool disableVariable(
                const S9sString &variableName);

        bool removeVariable(
                const S9sString &sectionName,
                const S9sString &variableName);

        bool removeSection(
                const S9sString &sectionName);

        bool addVariable(
                const S9sString &sectionName,
                const S9sString &variableName,
                const S9sString &variableValue);

        bool hasVariable(
                const S9sString &sectionName,
                const S9sString &variableName,
                bool              includingDisabled);

        bool hasSection(const S9sString &sectionName);
        void append(S9sConfigAstNode *node);
        void build(S9sString &content);
        void printDebug() const;

        inline bool nameEqual(
                const S9sString &str1,
                const S9sString &str2) const;
        
        inline bool sectionEqual(
                const S9sString &str1,
                const S9sString &str2) const;

    private:
        S9s::Syntax                    m_syntax;
        S9sVector<S9sConfigAstNode *>  m_ast;
};
        
inline bool 
S9sClusterConfigParseContext::nameEqual(
        const S9sString &str1,
        const S9sString &str2) const
{
    return str1 == str2;
}
        
inline bool 
S9sClusterConfigParseContext::sectionEqual(
        const S9sString &str1,
        const S9sString &str2) const
{
    return str1 == str2;
}

/**
 * A class that represents one cluster configuration file (e.g. a MySql
 * configuration file). This is not the S9s configuration file although the
 * syntax might be similar or even the same.
 *
 * S9sConfigFile uses implicit sharing. We need this for we want to put
 * S9sConfigFile objects into containers while we store pointers inside.
 */
class S9sConfigFile 
{
    public:
        S9sConfigFile();
        S9sConfigFile(S9s::Syntax syntax);
        S9sConfigFile(const S9sConfigFile &orig);
        virtual ~S9sConfigFile();

        S9sConfigFile &operator= (const S9sConfigFile &rhs);

        bool save(S9sString &errorString);

        void setIncludeLevel(int value);
        int includeLevel() const;

        void setName(const S9sString &name);
        S9sString name() const;

        void setFileName(const S9sString &fileName);
        const S9sString &fileName() const;

        bool sourceFileExists() const;

        void setContent(const S9sString &content);
        const S9sString &content() const;

        void setSize(int size);
        int size() const;

        void setCrc(ulonglong crc);
        ulonglong crc() const;

        S9sString crcStr() const;

        bool parse();
        bool parse(const char *source);
        bool parseSourceFile();

        void setPath(const S9sString &path);
        S9sString path() const;
        
        void collectIncludeFiles(S9sVariantList &includeFileNames) const;
        void collectIncludeDirs(S9sVariantList &includeDirNames) const;
        void collectVariableNames(S9sVariantList &variableNames) const;
        
        S9sVariantList 
            collectVariables(
                    const S9sString &variableName) const;

        void appendSearchGroup(
                const S9sString &groupName);

        S9sString 
            variableValue(
                    const S9sString &variableName) const;

        S9sString 
            variableValue(
                const S9sString &sectionName,
                const S9sString &variableName) const;

        S9sString
            variableValue(
                const S9sString &sectionName,
                const S9sString &variableName,
                const S9sString &defaultValue) const;

        S9sVariantList 
            variableValueAsStringList(
                    const S9sString &variableName);

        bool changeVariable(
                const S9sString &sectionName,
                const S9sString &variableName,
                const S9sString &variableValue);
        
        bool changeVariable(
                const S9sString &variableName,
                const S9sString &variableValue);

        bool disableVariable(
                const S9sString &sectionName,
                const S9sString &variableName);
        
        bool disableVariable(
                const S9sString &variableName);

        bool removeVariable(
                const S9sString &sectionName,
                const S9sString &variableName);

        bool removeSection(
                const S9sString &sectionName);

        bool addVariable(
                const S9sString &sectionName,
                const S9sString &variableName,
                const S9sString &variableValue);

        bool addVariable(
                const S9sString &variableName,
                const S9sString &variableValue);

        bool hasVariable(
                const S9sString &sectionName,
                const S9sString &variableName,
                bool              includingDisabled = false);

        bool hasSection(const S9sString &sectionName);
        S9sString errorString() const;
        
        void build(S9sString &content);
        void printDebug() const;

        // misc methods
        bool hasChange() const;
        void setHasChange (bool hasChange = true);

        ulonglong timeStamp() const;
        void setTimeStamp(const ulonglong timeStamp = 0ull);

    private:
        S9sConfigFile (const S9sConfigAstNode&) {}
        S9sConfigFile &operator= (const S9sConfigAstNode&) {return *this;}

    private:
        S9sConfigFilePrivate *m_priv;

    friend class UtS9sConfigFile;
};

extern int config_parse(S9sClusterConfigParseContext &context);
extern int haconfig_parse(S9sClusterConfigParseContext &context);
extern int yaml_parse(S9sClusterConfigParseContext &context);

/**
 * Represents a set of config files that are related to each other in some way.
 */
class S9sConfigFileSet : public S9sVector<S9sConfigFile>
{
    public:
        bool contains(const S9sString &filePath);

        bool parse();
        void collectIncludeFiles(S9sVariantList &includeFileNames) const;
        S9sVariantList collectVariables(const S9sString &variableName) const;

        S9sString variableValue(
                const S9sString &sectionName,
                const S9sString &variableName) const;

        void changeVariable(
                const S9sString &section,
                const S9sString &variableName,
                const S9sString &value);

        void disableVariable(
                const S9sString &sectionName,
                const S9sString &variableName);

        void removeSection(
                const S9sString &sectionName);

        void toVariantMap(S9sVariantMap &map);

        const S9sVariantList &errors() const { return m_errorStrings; };
        S9sConfigFile &appendNewFile(S9s::Syntax syntax);

    private:
        S9sVariantList    m_errorStrings;
};

