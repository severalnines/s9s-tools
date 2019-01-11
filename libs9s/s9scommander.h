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
#pragma once

#include "S9sDisplay"
#include "S9sRpcClient"
#include "S9sTreeNode"
#include "S9sBrowser"
#include "S9sInfoPanel"
#include "S9sEditor"

class S9sDialog;

class S9sCommander :
    public S9sDisplay
{
    public:
        S9sCommander(S9sRpcClient &client);
        virtual ~S9sCommander();
        
        S9sString sourcePath() const;
        S9sString sourceFullPath() const;

        S9sString targetPath() const;

        virtual void main();
        virtual void processKey(int key);
        virtual bool processButton(uint button, uint x, uint y);        

    protected:
        virtual bool refreshScreen();
        virtual void printHeader();
        virtual void printFooter();

        void updateTree();

        void entryActivated(
                const S9sString   &path,
                const S9sTreeNode &node);

        void waitForJobWithLog(
                const int     clusterId,
                const int     jobId, 
                S9sRpcClient &client);

        bool renameMove(
                const S9sString sourcePath,
                const S9sString targetPath);

        bool createFolder(const S9sString fullPath);
        bool createFile(const S9sString fullPath);
        bool deleteEntry(const S9sString fullPath);
        bool saveContent(const S9sString fullPath, const S9sString content);
        bool loadObject(const S9sString &fullPath, S9sVariantMap &object);

        void updateObject(bool updateRequested);
        void updateObject(const S9sString &path, S9sInfoPanel &target);
        void updateObject(const S9sString &path, S9sEditor &target);


    protected:
        void showErrorDialog(const S9sString &errorString);
        
    private:
        S9sRpcClient    &m_client;
        S9sMutex         m_networkMutex;        
        
        S9sWidget       *m_leftPanel;
        S9sWidget       *m_rightPanel;

        S9sBrowser       m_leftBrowser;
        S9sInfoPanel     m_leftInfo;
        S9sBrowser       m_rightBrowser;
        S9sInfoPanel     m_rightInfo;

        S9sTreeNode      m_rootNode;
        time_t           m_rootNodeRecevied;
        bool             m_communicating;
        bool             m_reloadRequested;
        bool             m_viewDebug;
        
        S9sDialog       *m_dialog;
        S9sDialog       *m_errorDialog;
        S9sEditor        m_editor;
};

