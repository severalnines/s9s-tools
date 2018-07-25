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
 * s9s-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with s9s-tools. If not, see <http://www.gnu.org/licenses/>.
 */
#include "s9sbackup.h"

#include <S9sUrl>
#include <S9sVariantMap>
#include <S9sRpcReply>
#include <S9sOptions>
#include <S9sDateTime>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sBackup::S9sBackup()
{
}
 
S9sBackup::S9sBackup(
        const S9sVariantMap &properties) :
    m_properties(properties)
{
}

S9sBackup::~S9sBackup()
{
}

S9sBackup &
S9sBackup::operator=(
        const S9sVariantMap &rhs)
{
    setProperties(rhs);
    
    return *this;
}

/**
 * \returns The S9sBackup converted to a variant map.
 */
const S9sVariantMap &
S9sBackup::toVariantMap() const
{
    return m_properties;
}

/**
 * \returns True if a property with the given key exists.
 */
bool
S9sBackup::hasProperty(
        const S9sString &key) const
{
    return m_properties.contains(key);
}

/**
 * \returns The value of the property with the given name or the empty
 *   S9sVariant object if the property is not set.
 */
S9sVariant
S9sBackup::property(
        const S9sString &name) const
{
    if (m_properties.contains(name))
        return m_properties.at(name);

    return S9sVariant();
}

/**
 * \param name The name of the property to set.
 * \param value The value of the property as a string.
 *
 * This function will investigate the value represented as a string. If it looks
 * like a boolean value (e.g. "true") then it will be converted to a boolean
 * value, if it looks like an integer (e.g. 42) it will be converted to an
 * integer. Then the property will be set accordingly.
 */
void
S9sBackup::setProperty(
        const S9sString &name,
        const S9sString &value)
{
    if (value.looksBoolean())
    {
        m_properties[name] = value.toBoolean();
    } else if (value.looksInteger())
    {
        m_properties[name] = value.toInt();
    } else {
        m_properties[name] = value;
    }
}

/**
 * \param properties The properties to be set as a name -> value mapping.
 *
 * Sets all the properties in one step. All the existing properties will be
 * deleted, then the new properties set.
 */
void
S9sBackup::setProperties(
        const S9sVariantMap &properties)
{
    m_properties = properties;
}

/**
 * \returns The host on which the backup was created.
 */
S9sString
S9sBackup::backupHost() const
{
    if (m_properties.contains("backup_host"))
        return m_properties.at("backup_host").toString();

    return S9sString();    
}

/**
 * \returns The host on which the backup was stored.
 */
S9sString
S9sBackup::storageHost() const
{
    if (m_properties.contains("storage_host"))
        return m_properties.at("storage_host").toString();

    return S9sString();    
}

S9sString
S9sBackup::title() const
{
    if (m_properties.contains("title"))
        return m_properties.at("title").toString();

    return S9sString();
}

/**
 * \returns The ID of the backup.
 */
int
S9sBackup::id() const
{
    if (m_properties.contains("id"))
        return m_properties.at("id").toInt();

    return 0;
}

/**
 * \returns The ID of the cluster.
 */
int
S9sBackup::clusterId() const
{
    if (m_properties.contains("cid"))
        return m_properties.at("cid").toInt();

    return 0;
}

/**
 * \returns The ID of the job that created the backup.
 */
int
S9sBackup::jobId() const
{
    if (m_properties.contains("job_id"))
        return m_properties.at("job_id").toInt();

    return 0;
}

/**
 * \returns The status of the backup as a string.
 */
S9sString
S9sBackup::status() const
{
    if (m_properties.contains("status"))
        return m_properties.at("status").toString().toUpper();

    return S9sString();    
}

const char *
S9sBackup::statusColorBegin(
        const bool syntaxHighlight)
{
    if (!syntaxHighlight)
        return "";

    if (status() == "COMPLETED")
        return XTERM_COLOR_GREEN;
    else if (status() == "RUNNING")
        return XTERM_COLOR_YELLOW;
    else if (status() == "PENDING")
        return XTERM_COLOR_YELLOW;
    else if (status() == "FAILED")
        return XTERM_COLOR_RED;
    
    return XTERM_COLOR_RED;
}

const char *
S9sBackup::statusColorEnd(
        const bool syntaxHighlight)
{
    if (!syntaxHighlight)
        return "";

    return TERM_NORMAL;
}

/**
 * Possible values are "Unverified"...
 */
S9sString
S9sBackup::verificationStatus() const
{
    S9sVariantMap verificationMap;

    if (m_properties.contains("verified"))
        verificationMap = m_properties.at("verified").toVariantMap();

    return verificationMap["status"].toString();
}

bool
S9sBackup::encrypted() const
{
    bool retval = false;

    if (m_properties.contains("encrypted"))
        retval = m_properties.at("encrypted").toBoolean();

    return retval;
}



/**
 * \returns The date and time when the backup creation was started as it is in
 *   the JSon reply.
 */
S9sVariant
S9sBackup::begin() const
{
    if (m_properties.contains("created"))
        return m_properties.at("created");

    return S9sVariant();
}

/**
 * \returns The date and time when the backup creation was started formatted as
 *   the command line options are set.
 */
S9sString
S9sBackup::beginAsString() const
{
    S9sOptions  *options   = S9sOptions::instance();
    S9sString    rawString = begin().toString();
    S9sDateTime  created;
    S9sString    retval;

    if (!created.parse(rawString))
        return "-";

    retval = options->formatDateTime(created);
    return retval;
}

/**
 * \returns The date and time when the backup creation was finished as it is in
 *   the JSon reply.
 */
S9sVariant
S9sBackup::end() const
{
    if (m_properties.contains("finished"))
        return m_properties.at("finished");

    return S9sVariant();
}

/**
 * \returns The date and time when the backup creation was finished formatted as
 *   the command line options was set.
 */
S9sString
S9sBackup::endAsString() const
{
    S9sOptions  *options   = S9sOptions::instance();
    S9sString    rawString = end().toString();
    S9sDateTime  created;
    S9sString    retval;

    if (!created.parse(rawString))
        return "-";

    retval = options->formatDateTime(created);
    return retval;
}

/**
 * \returns The absolute path of the root directory where these backup files are
 * placed.
 */
S9sString
S9sBackup::rootDir() const
{
    if (m_properties.contains("root_dir"))
        return m_properties.at("root_dir").toString();

    return S9sString();    
}

/**
 * \returns The name of the user that is created the configuration.
 */
S9sString
S9sBackup::configOwner() const
{
    return configValue("createdBy").toString();
}

/**
 * \returns The configured backup host.
 */
S9sString
S9sBackup::configBackupHost() const
{
    return configValue("backupHost").toString();
}

/**
 * \returns The configured backup method or '-' if the backup method is not
 *   configured.
 */
S9sString
S9sBackup::configMethod() const
{
    S9sString retval = configValue("backupMethod").toString();

    if (retval.empty())
        retval = "-";

    return retval;
}

/**
 * \returns The configured description or '-' if the backup description is not
 *   configured.
 */
S9sString
S9sBackup::configDescription() const
{
    S9sString retval = configValue("description").toString();

    if (retval.empty())
        retval = "-";

    return retval;
}

/**
 * \returns True if the archive file(s) are compressed.
 */
bool
S9sBackup::isCompressed() const
{
    if (m_properties.contains("compressed"))
        return m_properties.at("compressed").toBoolean();

    return false;
}

/**
 * \returns How many backups the object contains, how many backups was created.
 */
int 
S9sBackup::nBackups() const
{
    if (m_properties.contains("backup"))
        return m_properties.at("backup").size();

    return 0;
}

S9sString
S9sBackup::databaseNamesAsString(
        const int        backupIndex,
        const S9sString &delimiter) const
{
    S9sVariantMap   theMap = backupMap(backupIndex);
    S9sVariantList  theList;
    S9sString       retval;

    if (theMap.contains("database_names"))
        theList = theMap.at("database_names").toVariantList();

    for (uint idx = 0u; idx < theList.size(); ++idx)
    {
        if (!retval.empty())
            retval += delimiter;

        retval += theList[idx].toString();
    }

    return retval;
}

/**
 * \returns How many files the given backup contains.
 */
int 
S9sBackup::nFiles(
        const int backupIndex) const
{
    S9sVariantMap theMap = backupMap(backupIndex);

    if (theMap.contains("files"))
        return theMap.at("files").size();

    return 0;
}

/**
 * \returns The file name of the archive where the backup is stored.
 */
S9sString
S9sBackup::fileName(
        const int backupIndex,
        const int fileIndex) const
{
    S9sVariantMap theFileMap = fileMap(backupIndex, fileIndex);

    if (theFileMap.contains("path"))
        return theFileMap.at("path").toString();

    return S9sString();
}

S9sString
S9sBackup::filePath(
        const int backupIndex,
        const int fileIndex) const
{
    S9sString retval;

    retval = rootDir();

    if (!retval.empty() && !retval.endsWith("/"))
        retval += "/";

    retval += fileName(backupIndex, fileIndex);
    return retval;
}

/**
 * \returns The size of the backup file measured in bytes.
 */
S9sVariant
S9sBackup::fileSize(
        const int backupIndex,
        const int fileIndex) const
{
    S9sVariantMap theFileMap = fileMap(backupIndex, fileIndex);

    if (theFileMap.contains("size"))
        return theFileMap.at("size");

    return S9sVariant();
}

/**
 * \returns The "created" field of the file.
 *
 * This function will return the "created" property of the file as it is found
 * in the JSon reply from the controller, so most probably as a date and time
 * string.
 */
S9sVariant
S9sBackup::fileCreated(
        const int backupIndex,
        const int fileIndex) const
{
    S9sVariantMap theFileMap = fileMap(backupIndex, fileIndex);

    if (theFileMap.contains("created"))
        return theFileMap.at("created");

    return S9sVariant();
}

/**
 * \returns The last modification date and time of the archive file.
 */
S9sString
S9sBackup::fileCreatedString(
        const int backupIndex,
        const int fileIndex) const
{
    S9sOptions  *options = S9sOptions::instance();
    S9sString    rawString = fileCreated(backupIndex, fileIndex).toString();
    S9sDateTime  created;
    S9sString    retval;

    if (!created.parse(rawString))
        return "-";

    retval = options->formatDateTime(created);
    return retval;
}

/**
 * \returns The string representing the method which is used to create the
 *   backup.
 */
S9sString
S9sBackup::method() const
{
    if (m_properties.contains("method"))
        return m_properties.at("method").toString();

    return S9sString();
}

/**
 * \returns The description of the backup.
 */
S9sString
S9sBackup::description() const
{
    S9sString retval;

    if (m_properties.contains("description"))
        retval = m_properties.at("description").toString();

    if (retval.empty())
        retval = S9sString("-");

    return retval;
}


/**
 * \param syntaxHighlight Controls if the string will have colors or not.
 * \param formatString The formatstring with markup.
 * \returns The string representation according to the format string.
 *
 * Converts the message to a string using a special format string that may
 * contain field names of message properties.
 */
S9sString
S9sBackup::toString(
        const int        backupIndex,
        const int        fileIndex,
        const bool       syntaxHighlight,
        const S9sString &formatString) const
{
    S9sString    retval;
    S9sString    tmp;
    char         c;
    S9sString    partFormat;
    bool         percent        = false;
    bool         escaped        = false;
    bool         modifierConfig = false;

    S9S_WARNING("syntaxHighlight : %s", syntaxHighlight ? "true" : "false");
    for (uint n = 0; n < formatString.size(); ++n)
    {
        c = formatString[n];
       
        if (c == '%' && !percent)
        {
            percent    = true;
            partFormat = "%";
            continue;
        } else if (percent && c == 'c')
        {
            modifierConfig = true;
            continue;
        } else if (c == '\\')
        {
            escaped = true;
            continue;
        }

        // FIXME: just to avoid unused veriable message.
        if (modifierConfig)
            modifierConfig = modifierConfig;

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
                case 'B':
                    // The time when the backup creation was started.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(beginAsString()));
                    retval += tmp;
                    break;

                case 'C':
                    // The file creation date and time.
                    partFormat += 's';

                    tmp.sprintf(
                            STR(partFormat), 
                            STR(fileCreatedString(backupIndex, fileIndex)));

                    retval += tmp;
                    break;
               
                case 'd':
                    // The list of databases.
                    partFormat += 's';
                    tmp.sprintf(
                            STR(partFormat), 
                            STR(databaseNamesAsString(backupIndex)));

                    retval += tmp;
                    break;
                    
                case 'D':
                    // The description.
                    partFormat += 's';

                    if (modifierConfig)
                        tmp.sprintf(STR(partFormat), STR(configDescription()));
                    else
                        tmp.sprintf(STR(partFormat), STR(description()));

                    retval += tmp;
                    break;
                
                case 'e':
                    // The encryption status.
                    partFormat += 's';

                    tmp.sprintf(STR(partFormat), 
                            encrypted() ? "ENCRYPTED" : "UNENCRYPTED");
                    
                    retval += tmp;
                    break;

                case 'E':
                    // The time when the backup creation was finished.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(endAsString()));
                    retval += tmp;
                    break;
                
                case 'F':
                    // The file name.
                    partFormat += 's';

                    if (syntaxHighlight)
                    {
                        retval += S9sRpcReply::fileColorBegin(
                                fileName(backupIndex, fileIndex));
                    }

                    tmp.sprintf(
                            STR(partFormat), 
                            STR(fileName(backupIndex, fileIndex)));
                    
                    retval += tmp;

                    if (syntaxHighlight)
                        retval += S9sRpcReply::fileColorEnd();
                    
                    break;

                case 'H':
                    // The backup host.
                    partFormat += 's';

                    if (modifierConfig)
                        tmp.sprintf(STR(partFormat), STR(configBackupHost()));
                    else
                        tmp.sprintf(STR(partFormat), STR(backupHost()));

                    retval += tmp;
                    break;
                
                case 'I':
                    // The numerical ID of the backup.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), id());
                    retval += tmp;
                    break;
                
                case 'i':
                    // The cluster ID of the backup.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), clusterId());
                    retval += tmp;
                    break;
               
                case 'J':
                    // The ID of the job.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), jobId());
                    retval += tmp;
                    break;

                case 'M':
                    // The backup method.
                    partFormat += 's';

                    if (modifierConfig)
                        tmp.sprintf(STR(partFormat), STR(configMethod()));
                    else
                        tmp.sprintf(STR(partFormat), STR(method()));

                    retval += tmp;
                    break;
                
                case 'O':
                    // The owner.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(configOwner()));
                    retval += tmp;
                    break;

                case 'P':
                    // The file name.
                    partFormat += 's';

                    if (syntaxHighlight)
                    {
                        retval += S9sRpcReply::fileColorBegin(
                                fileName(backupIndex, fileIndex));
                    }

                    tmp.sprintf(
                            STR(partFormat), 
                            STR(filePath(backupIndex, fileIndex)));
                    
                    retval += tmp;

                    if (syntaxHighlight)
                        retval += S9sRpcReply::fileColorEnd();
                    
                    break;
                
                case 'R':
                    // The root directory of the backup.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(rootDir()));
                    retval += tmp;
                    break;
                
                case 'S':
                    // The storage host. 
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(storageHost()));
                    retval += tmp;
                    break;
                
                case 's':
                    // The storage host. 
                    partFormat += "llu";
                    tmp.sprintf(
                            STR(partFormat), 
                            fileSize(backupIndex, fileIndex).toULongLong());
                    retval += tmp;
                    break;
            
                case 't':
                    // The storage host. 
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(title()));
                    retval += tmp;
                    break;
                
                case 'v':
                    // The verification status.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(verificationStatus()));
                    retval += tmp;
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

        percent        = false;
        escaped        = false;
        modifierConfig = false;
    }

    return retval;
}

S9sVariantMap
S9sBackup::fileMap(
        const int backupIndex,
        const int fileIndex) const
{
    S9sVariantMap  theBackupMap = backupMap(backupIndex);
    S9sVariantList theFileList;

    if (theBackupMap.contains("files"))
        theFileList = theBackupMap.at("files").toVariantList();

    if (fileIndex >= 0 && fileIndex < (int) theFileList.size())
        return theFileList[fileIndex].toVariantMap();

    return S9sVariantMap();
}

S9sVariantMap
S9sBackup::backupMap(
        const int backupIndex) const
{
    S9sVariant backups;

    if (m_properties.contains("backup"))
        backups = m_properties.at("backup");

    if (backupIndex >= 0 && backupIndex < backups.size())
        return backups[backupIndex].toVariantMap();
    
    return S9sVariantMap();
}


S9sVariant
S9sBackup::configValue(
        const S9sString &key) const
{
    S9sVariantMap configMap = config().toVariantMap();

    return configMap[key];
}

S9sVariant
S9sBackup::config() const
{
    if (m_properties.contains("config"))
        return m_properties.at("config");

    return S9sVariantMap();
}

