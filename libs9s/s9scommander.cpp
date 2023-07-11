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
#include "s9scommander.h"
#include "s9sdatetime.h"
#include "s9smutexlocker.h"
#include "s9sentrydialog.h"
#include "s9squestiondialog.h"
#include "s9soptions.h"

#include <unistd.h>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sCommander::S9sCommander(
        S9sRpcClient            &client) : 
    S9sDisplay(true),
    m_client(client),
    m_rootNodeRecevied(0),
    m_communicating(false),
    m_reloadRequested(false),
    m_dialog(0),
    m_errorDialog(0),
    m_waitingForKeyPress(false)
{
    m_leftPanel  = &m_leftBrowser;
    m_rightPanel = &m_rightInfo;

    m_leftBrowser.setVisible(true);
    m_leftBrowser.setSelectionIndex(0);
    m_leftBrowser.setHasFocus(true);
    m_leftBrowser.setSelectionEnabled(true);

    m_rightBrowser.setVisible(false);
    m_rightBrowser.setSelectionIndex(0);
    m_rightBrowser.setHasFocus(false);
    m_rightBrowser.setSelectionEnabled(true);
    
    m_rightInfo.setVisible(true);
    m_rightInfo.setHasFocus(false);
    m_rightInfo.setInfoController(
            client.hostName(), client.port(), client.useTls());
    
    
    m_leftInfo.setVisible(false);
    m_leftInfo.setHasFocus(false);
    m_leftInfo.setInfoController(
            client.hostName(), client.port(), client.useTls());
    
    m_editor.setVisible(false);
    m_editor.setHasFocus(false);
}

S9sCommander::~S9sCommander()
{
}

S9sString 
S9sCommander::sourcePath() const
{
    S9sString retval;

    if (m_leftBrowser.hasFocus())
    {
        retval = m_leftBrowser.path();
    } else if (m_rightBrowser.hasFocus())
    {
        retval = m_rightBrowser.path();
    }

    return retval;
}

S9sString
S9sCommander::targetPath() const
{
    S9sString retval;

    if (m_leftBrowser.hasFocus() && m_rightBrowser.isVisible())
    {
        retval = m_rightBrowser.path();
    } else if (m_rightBrowser.hasFocus() && m_leftBrowser.isVisible())
    {
        retval = m_leftBrowser.path();
    }

    return retval;    
}

S9sString 
S9sCommander::sourceFullPath() const
{
    S9sString retval;

    if (m_leftBrowser.hasFocus())
    {
        retval = m_leftBrowser.selectedNodeFullPath();
    } else if (m_rightBrowser.hasFocus())
    {
        retval = m_rightBrowser.selectedNodeFullPath();
    }

    return retval;
}


void
S9sCommander::main()
{
    int          updateFreq = 10;
    start();
    updateTree();

        while (true)
        {
            bool updateRequested;
            bool authenticated;

            /*
             * We might get unauthenticated on the fly. While testing this
             * happens when you restart the controller, but there might be other
             * reasons too.
             */
            authenticated = m_client.isAuthenticated();
            //PRINT_LOG("   authenticated: %s", authenticated ? "yes" : "no");
            
            while (!authenticated)
            {
                PRINT_LOG("Not authenticated, need to do it.");
                m_client.maybeAuthenticate();

                authenticated = m_client.isAuthenticated();
                if (!authenticated)
                    usleep(3000000);
            }

            updateRequested = m_reloadRequested;

            if (time(NULL) - m_rootNodeRecevied > updateFreq || 
                    m_reloadRequested)
            {
                updateTree();
            }

            updateObject(updateRequested);
            usleep(300000);
        }    
}

/**
 * Reloads the tree from the controller and pushes it into the widgets.
 */
void
S9sCommander::updateTree()
{
    S9sRpcReply      getTreeReply;

    // Updating the screen.
    m_mutex.lock();
    m_rightInfo.setInfoRequestName("getTree");
    m_leftInfo.setInfoRequestName("getTree");
    m_mutex.unlock();

    m_communicating   = true;
    m_reloadRequested = false;

    m_networkMutex.lock();
    m_client.getTree(true);
    getTreeReply = m_client.reply();
    m_networkMutex.unlock();
    
    // Updating the screen.
    m_mutex.lock();
    m_rightInfo.setInfoRequestName("");
    m_leftInfo.setInfoRequestName("");
    m_rightInfo.setInfoLastReply(getTreeReply);
    m_leftInfo.setInfoLastReply(getTreeReply);
    
    m_leftInfo.setInfoController(
            m_client.hostName(), m_client.port(), m_client.useTls());

    m_rightInfo.setInfoController(
            m_client.hostName(), m_client.port(), m_client.useTls());

    if (getTreeReply.isOk())
    {
        m_rootNode = getTreeReply.tree();
        m_leftBrowser.setCdt(m_rootNode);
        m_rightBrowser.setCdt(m_rootNode);
        m_rootNodeRecevied = time(NULL);
    }

    m_communicating = false;

    if (m_dialog != NULL)
    {
        if (m_dialog->userData("delayedClose").toBoolean())
        {
            delete m_dialog;
            m_dialog = NULL;
        }
    }

    m_mutex.unlock(); 
}

bool
S9sCommander::renameMove(
        const S9sString sourcePath,
        const S9sString targetPath)
{
    S9sMutexLocker   locker(m_networkMutex);
    S9sRpcReply      reply;
    bool             success;

    PRINT_LOG("Renaming/moving an entry.");
    PRINT_LOG(" sourcePath: %s", STR(sourcePath));
    PRINT_LOG(" targetPath: %s", STR(targetPath));

    m_communicating   = true;
    if (targetPath.contains("/"))
        m_client.moveInTree(sourcePath, targetPath);
    else
        m_client.rename(sourcePath, targetPath);

    reply = m_client.reply();

    success = reply.isOk();
    if (!success)
    { 
        showErrorDialog(reply.errorString());
    } else {
        m_reloadRequested = true;
    }

    return success;
}

bool
S9sCommander::createFolder(
        const S9sString fullPath)
{
    S9sMutexLocker   locker(m_networkMutex);
    S9sRpcReply      reply;
    bool             success;
   
    PRINT_LOG("Creating a folder.");

    m_communicating   = true;
    m_client.mkdir(fullPath);
    reply = m_client.reply();

    success = reply.isOk();
    if (!success)
    { 
        showErrorDialog(reply.errorString());
    } else {
        m_reloadRequested = true;
    }

    return success;
}

bool
S9sCommander::createFile(
        const S9sString fullPath)
{
    S9sMutexLocker   locker(m_networkMutex);
    S9sRpcReply      reply;
    bool             success;

    PRINT_LOG("Creating a folder.");
    m_communicating   = true;
    m_client.mkfile(fullPath);
    reply = m_client.reply();

    success = reply.isOk();
    if (!success)
    { 
        showErrorDialog(reply.errorString());
    } else {
        m_reloadRequested = true;
    }

    return success;
}

bool
S9sCommander::deleteEntry(
        const S9sString fullPath)
{
    S9sMutexLocker   locker(m_networkMutex);
    S9sRpcReply      reply;
    bool             success;
       
    m_communicating   = true;
    m_client.deleteFromTree(fullPath);
    reply = m_client.reply();

    success = reply.isOk();
    if (!success)
    { 
        showErrorDialog(reply.errorString());
    } else {
        m_reloadRequested = true;
    }

    return success;
}

bool
S9sCommander::saveContent(
        const S9sString fullPath, 
        const S9sString content)
{
    S9sMutexLocker   locker(m_networkMutex);
    S9sRpcReply      reply;
    bool             success;

    PRINT_LOG("Saving CDT file '%s'.", STR(fullPath));
    m_communicating   = true;
    m_client.setContent(fullPath, content);
    reply = m_client.reply();

    success = reply.isOk();
    if (!success)
    { 
        showErrorDialog(reply.errorString());
    } else {
        m_reloadRequested = true;
        m_leftInfo.invalidateObject();
        m_rightInfo.invalidateObject();
    }

    return success;
}

bool
S9sCommander::loadObject(
        const S9sString &path,
        S9sVariantMap   &object)
{
    S9sRpcReply    reply;

    object.clear();

    m_networkMutex.lock();
    m_client.getObject(path);
    reply = m_client.reply();
    m_networkMutex.unlock();

    if (reply.isOk())
    {
        object = reply.getObject();
        return true;
    }

    return false;
}

void
S9sCommander::updateObject(
        bool updateRequested)
{
    S9sString path;
    bool      needToRefresh;

    if (m_rightInfo.isVisible())
    {
        path = m_leftBrowser.selectedNodeFullPath();

        needToRefresh = 
            path != m_rightInfo.objectPath() ||
            m_rightInfo.needsUpdate();

        if (time(NULL) - m_rightInfo.objectSetTime() > 15)
            needToRefresh = true;
        
        if (updateRequested)
            needToRefresh = true;

        if (needToRefresh)
            updateObject(path, m_rightInfo);
    }
    
    if (m_leftInfo.isVisible())
    {
        path = m_rightBrowser.selectedNodeFullPath();

        needToRefresh = 
            path != m_leftInfo.objectPath() ||
            m_leftInfo.needsUpdate();

        if (time(NULL) - m_leftInfo.objectSetTime() > 15)
            needToRefresh = true;

        if (updateRequested)
            needToRefresh = true;

        if (needToRefresh)
            updateObject(path, m_leftInfo);
    }
    
    if (m_editor.isVisible() && m_editor.isReadonly())
        updateObject(path, m_editor);
}

void
S9sCommander::updateObject(
        const S9sString &path,
        S9sInfoPanel    &target)
{
    S9sVariantMap  theMap;
    S9sRpcReply    reply;
   
    if (path.empty())
        return;

    if (m_editor.isVisible())
        return;

    /*
     *
     */
    m_mutex.lock();
    target.setInfoRequestName("getObject");
    m_mutex.unlock();

    /*
     *
     */
    m_networkMutex.lock();
    m_client.getObject(path);
    reply = m_client.reply();
    m_networkMutex.unlock();
    theMap = reply.getObject();
   
    /*
     *
     */
    m_mutex.lock();
    target.setInfoRequestName("");
    target.setInfoLastReply(reply);

    if (reply.isOk())
        target.setInfoObject(path, theMap);
    else
        target.setInfoObject(path, reply);
   
    m_mutex.unlock();
}

void
S9sCommander::updateObject(
        const S9sString &path,
        S9sEditor       &target)
{
    S9sMutexLocker locker(m_mutex);

    S9sVariantMap  theMap;
    S9sRpcReply    reply;
   
    if (path.empty())
        return;

    if (target.isVisible() && target.isReadonly())
    {
        int timePassed = time(NULL) - target.objectSetTime();

        if (timePassed > 2)
        {
            S9sString path = target.objectPath();
            
            m_networkMutex.lock();
            m_client.getObject(path);
            reply = m_client.reply();
            m_networkMutex.unlock();

            if (reply.isOk())
            {
                target.setObject(path, reply.getObject());
            }
        }

        return;
    }    
}

/**
 * \returns True if the program should continue refreshing the screen, false to
 *   exit.
 */
bool 
S9sCommander::refreshScreen()
{
    if (m_waitingForKeyPress)
        return true;
    
    startScreen();
    printHeader();

    if (m_editor.isVisible())
    {
        m_editor.setSize(width(), height() - 2);
        m_editor.setLocation(0, 2);
    }

    m_leftBrowser.setSize(width() / 2, height() - 2);
    m_leftBrowser.setLocation(0, 2);

    m_rightBrowser.setSize(width() / 2, height() - 2);
    m_rightBrowser.setLocation(width() / 2 + 1, 2);
    
    m_leftInfo.setSize(width() / 2, height() - 2);
    m_rightInfo.setSize(width() / 2, height() - 2);

    m_rightInfo.setInfoNode(m_leftBrowser.selectedNode());
    m_leftInfo.setInfoNode(m_rightBrowser.selectedNode());

    for (int idx = 0u; idx < height() - 2; ++idx)
    {
        if (m_editor.isVisible())
        {
            m_editor.printLine(idx);
        } else {
            if (m_leftBrowser.isVisible())
                m_leftBrowser.printLine(idx);
            else if (m_leftInfo.isVisible())
                m_leftInfo.printLine(idx);

            if (m_rightBrowser.isVisible())
                m_rightBrowser.printLine(idx);
            else if (m_rightInfo.isVisible())
                m_rightInfo.printLine(idx);
        }

        printNewLine();

    }

    printFooter();

    if (m_dialog != NULL)
        m_dialog->refreshScreen();

    if (m_errorDialog != NULL)
        m_errorDialog->refreshScreen();

    if (m_editor.isVisible())
        m_editor.showCursor();

    return true;
}

/**
 * \param key The code of the pressed key.
 *
 * This function is called when the user pressed a key on the keyboard.
 */
void
S9sCommander::processKey(
        int key)
{
    PRINT_LOG("S9sCommander::processKey():");
    PRINT_LOG("*** key: %0x", key);

    #if 0
    // 
    // This is not working in a terminal emulator, not to mention on an apple
    // computer. This was an attempt to read the state of the Shift key.
    //
    char shift_state = 6;
    if (ioctl(0, TIOCLINUX, &shift_state) < 0) 
    {
        PRINT_LOG("ioctl TIOCLINUX 6 (get shift state) error: %m");
    } else {
        PRINT_LOG("%02x", shift_state);
    }
    #endif

    if (m_waitingForKeyPress)
    {
        setConioTerminalMode(true, true);
        m_waitingForKeyPress = false;
        return;
    }
    
    if (m_errorDialog != NULL)
    {
        m_errorDialog->processKey(key);
        if (m_errorDialog->isCancelPressed() || m_errorDialog->isOkPressed())
        {
            delete m_errorDialog;
            m_errorDialog = NULL;
        }
    
        return;
    }

    if (m_dialog != NULL)
    {
        m_dialog->processKey(key);

        PRINT_LOG(" isOkPressed: %s", 
                m_dialog->isOkPressed() ? "yes" : "no");

        if (m_dialog->isCancelPressed())
        {
            delete m_dialog;
            m_dialog = NULL;
        } else if (m_dialog->isOkPressed())
        {
            bool success = false;

            if (m_dialog->userData("type") == "moveFile")
            {
                S9sString sourcePath;
                S9sString targetPath;

                sourcePath = m_dialog->userData("sourcePath").toString();
                targetPath = m_dialog->text();
                
                success = renameMove(sourcePath, targetPath); 
            } else if (m_dialog->userData("type") == "createFolder")
            {
                S9sString folderName = m_dialog->text();
                S9sString parentFolderName = sourcePath();

                success = createFolder(parentFolderName + "/" + folderName);
            } else if (m_dialog->userData("type") == "createFile")
            {
                S9sString folderName = m_dialog->text();
                S9sString parentFolderName = sourcePath();

                success = createFile(parentFolderName + "/" + folderName);
            } else if (m_dialog->userData("type") == "deleteEntry")
            {
                S9sString path = m_dialog->userData("objectPath").toString();

                success = deleteEntry(path);
            } else {
                PRINT_LOG("Unhandled dialog type.");
                PRINT_LOG(" type: '%s'", 
                        STR(m_dialog->userData("type").toString()));
            }

            if (m_dialog)
                m_dialog->setIsOkPressed(false);

            if (success)
            {
                if (m_dialog)
                {
                    #if 1
                    delete m_dialog;
                    m_dialog = NULL;
                    #else
                    m_dialog->setUserData("delayedClose", true);
                    #endif
                }
            }
        }

        return;
    } else if (m_editor.isVisible() && 
            key != S9S_KEY_ESC && 
            key != S9S_KEY_F10)
    {
        m_editor.processKey(key);
        if (m_editor.isSaveRequested())
        {
            saveContent(m_editor.path(), m_editor.content());
            m_editor.setSaveRequested(false);
        }

        return;
    }

    switch (key)
    {

        case '\t':
            // Tab switches the focus between the left and right panel.
            if (m_leftBrowser.hasFocus())
            {
                m_leftBrowser.setHasFocus(false);
                m_leftInfo.setHasFocus(false);
                m_rightBrowser.setHasFocus(true);
                m_rightInfo.setHasFocus(true);
            } else {
                m_leftBrowser.setHasFocus(true);
                m_leftInfo.setHasFocus(true);
                m_rightBrowser.setHasFocus(false);
                m_rightInfo.setHasFocus(false);
            }
            return;

        case 'r':
        case 'R':
            if (m_rightInfo.isVisible())
            {
                m_rightBrowser.setVisible(true);
                m_rightInfo.setVisible(false);
            } else {
                m_rightBrowser.setVisible(false);
                m_rightInfo.setVisible(true);
            }
            break;
        
        case 'l':
        case 'L':
            if (m_leftInfo.isVisible())
            {
                m_leftBrowser.setVisible(true);
                m_leftInfo.setVisible(false);
            } else {
                m_leftBrowser.setVisible(false);
                m_leftInfo.setVisible(true);
            }
            break;

        case 'j':
        case 'J':
            m_leftInfo.setShowJson(!m_leftInfo.showJson());
            m_rightInfo.setShowJson(!m_rightInfo.showJson());
            break;

        case 'd':
        case 'D':
            // Turning on and off the debug mode.
            m_viewDebug = !m_viewDebug;
            break;

        case S9S_KEY_F3:
            if (!sourceFullPath().empty())
            {
                S9sString      path = sourceFullPath();
                S9sVariantMap  object;
               
                loadObject(path, object);

                m_editor.setObject(path, object);
                m_editor.setVisible(true);
                m_editor.setHasFocus(true);
                m_editor.setIsReadOnly(true);
            }
            break;

        case S9S_KEY_F4:
            if (!sourceFullPath().empty())
            {
                S9sString      path = sourceFullPath();
                S9sVariantMap  object;
               
                loadObject(path, object);

                m_editor.setObject(path, object);
                m_editor.setVisible(true);
                m_editor.setHasFocus(true);
                m_editor.setIsReadOnly(false);
            }

            break;
        
        case S9S_KEY_SHIFT_F4:
            if (m_dialog == NULL)
            {
                PRINT_LOG("Creating make file dialog.");
                m_dialog = new S9sEntryDialog(this);
                m_dialog->setTitle("Create File");
                m_dialog->setMessage("Enter file name:");
                m_dialog->setUserData("type", "createFile");
                m_dialog->setSize(60, 6);
            }
            break;
        
        case S9S_KEY_F6:
            if (m_dialog == NULL)
            {
                S9sString      sourceFilePath = sourceFullPath();
                S9sString      targetDirPath = targetPath();

                PRINT_LOG("Creating rename/move file dialog.");
                PRINT_LOG("  sourcePath: '%s'", STR(sourceFilePath));
                PRINT_LOG("  targetPath: '%s'", STR(targetDirPath));
                
                m_dialog = new S9sEntryDialog(this);
                m_dialog->setTitle("Rename/move File");
                m_dialog->setMessage("Enter file name or path:");
                m_dialog->setUserData("type", "moveFile");
                m_dialog->setUserData("sourcePath", sourceFilePath);
                m_dialog->setText(targetDirPath);
                m_dialog->setSize(60, 6);
            }
            break;

        case S9S_KEY_F7:
            if (m_dialog == NULL)
            {
                PRINT_LOG("Creating mkdir dialog.");
                m_dialog = new S9sEntryDialog(this);
                m_dialog->setTitle("Create Folder");
                m_dialog->setMessage("Enter folder name:");
                m_dialog->setUserData("type", "createFolder");
                m_dialog->setSize(60, 6);
            }
            break;

        case S9S_KEY_F8:
            PRINT_LOG("F8");
            if (m_dialog == NULL)
            {
                S9sString fullPath = sourceFullPath();
                S9sString baseName = S9sFile::basename(fullPath);
                S9sString message;

                PRINT_LOG("Creating delete dialog.");
                message.sprintf(
                        "Delete CDT entry\n"
                        "\"%s\"?",
                        STR(baseName));

                PRINT_LOG("*** fullPath: %s", STR(fullPath));

                m_dialog = new S9sQuestionDialog(this);
                m_dialog->setTitle("Delete");
                m_dialog->setMessage(message);
                m_dialog->setUserData("type", "deleteEntry");
                m_dialog->setUserData("objectPath", fullPath);
                m_dialog->setSize(40, 6);
            }
            break;
        
        case S9S_KEY_F10:
        case 'q':
            if (m_editor.isVisible())
            {
                m_editor.setVisible(false);
                m_editor.setHasFocus(false);
            } else {
                exit(0);
            }
            break;

        case S9S_KEY_ESC:
            if (m_dialog != NULL)
            {
                delete m_dialog;
                m_dialog = NULL;
            } else if (m_editor.isVisible())
            {
                m_editor.setVisible(false);
                m_editor.setHasFocus(false);
            }

            break;
    }

    if (m_leftBrowser.hasFocus() && m_leftBrowser.isVisible())
    {
        m_leftBrowser.processKey(key); 

        if (!m_leftBrowser.activatedNodeFullPath().empty())
        {
            S9sString path = m_leftBrowser.activatedNodeFullPath();

            entryActivated(path, m_leftBrowser.activatedNode());
            m_leftBrowser.resetActivatedStatus();
        }
    } else if (m_rightBrowser.hasFocus() && m_rightBrowser.isVisible())
    {
        m_rightBrowser.processKey(key);
        
        if (!m_rightBrowser.activatedNodeFullPath().empty())
        {
            S9sString path = m_rightBrowser.activatedNodeFullPath();

            entryActivated(path, m_rightBrowser.activatedNode());
            m_rightBrowser.resetActivatedStatus();
        }
    } else if (m_leftInfo.hasFocus() && m_leftInfo.isVisible())
    {
        m_leftInfo.processKey(key); 
    } else if (m_rightInfo.hasFocus() && m_rightInfo.isVisible())
    {
        m_rightInfo.processKey(key);
    }
}

/**
 * \param button The mouse button code.
 * \param x The x coordinate measured in characters.
 * \param y The y coordinate measured in characters.
 * \returns True if the mouse event is processed and should not considered by
 *   other widgets.
 */
bool 
S9sCommander::processButton(
        uint button, 
        uint x, 
        uint y)
{
    if (y == 1)
    {
        if (x >= 25 && x <= 27)
        {
            // The reload/abort button.
            if (m_communicating)
            {
                m_communicating = false;
            } else {
                m_reloadRequested = true;
            }

            return true;
        }
    }

    if (m_leftBrowser.processButton(button, x, y))
    {
        if (m_leftBrowser.hasFocus())
        {
            m_rightBrowser.setHasFocus(false);
            m_rightInfo.setHasFocus(false);
            m_leftInfo.setHasFocus(true);
        }
        return true;
    } else if (m_rightBrowser.processButton(button, x, y))
    {
        if (m_rightBrowser.hasFocus())
        {
            m_leftBrowser.setHasFocus(false);
            m_leftInfo.setHasFocus(false);
            m_rightInfo.setHasFocus(true);
        }
        return true;
    }

    return S9sDisplay::processButton(button, x, y);
}

void
S9sCommander::printHeader()
{
    S9sDateTime dt = S9sDateTime::currentDateTime();
    S9sString   title = "S9S";

    ::printf("%s%-12s%s ", 
            TERM_SCREEN_TITLE_BOLD, 
            STR(title), 
            TERM_SCREEN_TITLE);

    ::printf("%c ", rotatingCharacter());
    ::printf("%s ", STR(dt.toString(S9sDateTime::LongTimeFormat)));

    // Printing the network activity character.
    if (m_communicating || m_reloadRequested)
        ::printf("❌ ");
    else
        ::printf("⟳ ");

    if (m_viewDebug)
    {
        ::printf("0x%02x ",      lastKeyCode());
        ::printf("%02dx%02d ",   width(), height());
        ::printf("%02d:%03d,%03d ", m_lastButton, m_lastX, m_lastY);
    }

    printNewLine();
}

void
S9sCommander::printFooter()
{
    const char     *inverse = TERM_NORMAL "\033[46m" "\033[38;5;232m";
    const char     *normal  = TERM_NORMAL;
    S9sVariantList  labels;
    S9sString       format;
    int             fieldSize;

    for (;m_lineCounter < height() - 1; ++m_lineCounter)
    {
        ::printf("%s", TERM_ERASE_EOL);
        ::printf("\n\r");
        ::printf("%s", TERM_ERASE_EOL);
    } 

    fieldSize = (width() / 10) - 2;
    if (fieldSize < 6)
        fieldSize = 6;

    format.sprintf("%%s%%2u%%s%%-%ds%%s", fieldSize);
    if (m_editor.isVisible() && !m_editor.isReadonly())
    {
        labels << 
            "" << "Save" << "" <<
            "" << "" << "" <<
            "" << "" << "" <<
            "Quit";
    } else if (m_editor.isVisible())
    {
        labels << 
            "" << "" << "" <<
            "" << "" << "" <<
            "" << "" << "" <<
            "Quit";
    } else {
        labels << 
            "" << "" << "View" <<
            "Edit" << "" << "" <<
            "MkDir" << "Delete" << "" <<
            "Quit";
    }

    for (uint idx = 0u; idx < labels.size(); ++idx)
    {
        ::printf(STR(format), 
                normal, idx + 1, inverse, 
                STR(labels[idx].toString()), normal);
    }

    ::printf("%s", TERM_ERASE_EOL);
    ::printf("%s", TERM_NORMAL);
    ::fflush(stdout);
}

void 
S9sCommander::showErrorDialog(
        const S9sString &errorString)
{
    if (m_errorDialog != NULL)
        delete m_errorDialog;

    m_errorDialog = new S9sQuestionDialog(this);
    m_errorDialog->setTitle("Error");
    m_errorDialog->setMessage(errorString);
    m_errorDialog->setUserData("type", "errorDialog");
    m_errorDialog->setSize(60, 6);
}

/**
 * \param path The full CDT of the entry.
 * \param node The node representing the entry.
 *
 * This method is called when a user has pressed the enter key on an entry other
 * than an entry with child nodes.
 */
void
S9sCommander::entryActivated(
        const S9sString   &path,
        const S9sTreeNode &node)
{
    PRINT_LOG("Activated '%s'.", STR(path));
    PRINT_LOG("     isFile: %s", node.isFile() ? "true" : "false");
    PRINT_LOG(" executable: %s", node.isExecutable() ? "true" : "false");

    if (node.isFile() && node.isExecutable())
    {
        S9sMutexLocker   locker(m_networkMutex);
        S9sRpcReply      reply;
        bool             success;

        reset_terminal_mode();

        m_client.executeCdtEntry(path);
        reply   = m_client.reply();
        success = reply.isOk();

        PRINT_LOG("  success: %s\n", success ? "true" : "true");
        
        waitForJobWithLog(0, reply.jobId(), m_client);
        //sleep(10);
        //setConioTerminalMode(true, true);
        m_waitingForKeyPress = true;
        ::printf("\n*** Press any key to continue. ***\n");
        fflush(stdout);
    }
}

void 
S9sCommander::waitForJobWithLog(
        const int     clusterId,
        const int     jobId, 
        S9sRpcClient &client)
{
    S9sOptions    *options         = S9sOptions::instance();
    S9sVariantMap  job;
    S9sRpcReply    reply;
    bool           success, finished;
    int            nLogsPrinted = 0;
    int            nEntries;
    int            nFailures = 0;
    int            nAuthentications = 0;

    for (;;)
    {
        /*
         * Requested at most 300 log messages. If we have more we will print
         * them later in the next round.
         */
        success = client.getJobLog(jobId, 300, nLogsPrinted);
        if (success)
        {
            reply     = client.reply();
            success   = reply.isOk();

            if (reply.isAuthRequired())
            {
                if (nAuthentications > 3)
                    break;

                success = client.authenticate();
                ++nAuthentications;
            } else {
                nAuthentications = 0;
            }
        }

        /*
         * If we have errors we count them, if we have more errors than we care
         * to abide we exit.
         */
        if (success)
        { 
            nFailures = 0;
        } else {
            bool messagePrinted = false;

            if (!reply.errorString().empty())
            {
                PRINT_ERROR("%s", STR(reply.errorString()));
                messagePrinted = true;
            }

            if (!client.errorString().empty())
            {
                PRINT_ERROR("%s", STR(client.errorString()));
                messagePrinted = true;
            }

            if (!messagePrinted)
            {
                PRINT_ERROR("Error while getting job log.");
            }


            //printf("\n\n%s\n", STR(reply.toString()));
            ++nFailures;
            if (nFailures > 3)
                break;

            continue;
        }

        /*
         * Printing the log messages.
         */
        nEntries = reply["messages"].toVariantList().size();
        if (nEntries > 0)
            reply.printJobLogBrief("");

        nLogsPrinted += nEntries;

        job = reply["job"].toVariantMap();
        if (job["status"] == "FAILED")
            options->setExitStatus(S9sOptions::JobFailed);

        finished = 
            job["status"] == "ABORTED"   ||
            job["status"] == "FINISHED"  ||
            job["status"] == "FAILED";
        
        fflush(stdout);
        if (finished)
            break;
        
        sleep(1);
    }

    printf("\n");
}

