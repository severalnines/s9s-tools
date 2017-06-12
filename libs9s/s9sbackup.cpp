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
 * \returns The status of the backup as a string.
 */
S9sString
S9sBackup::status() const
{
    if (m_properties.contains("status"))
        return m_properties.at("status").toString().toUpper();

    return S9sString();    
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
 * \returns The name of the user that created these backups.
 */
S9sString
S9sBackup::owner() const
{
    return configValue("createdBy").toString();
}

int 
S9sBackup::nBackups() const
{
    if (m_properties.contains("backup"))
        return m_properties.at("backup").size();

    return 0;
}

int 
S9sBackup::nFiles(
        const int backupIndex) const
{
    S9sVariantMap theMap = backupMap(backupIndex);

    if (theMap.contains("files"))
        return theMap.at("files").size();

    return 0;
}

S9sString
S9sBackup::filePath(
        const int backupIndex,
        const int fileIndex)
{
    S9sVariantMap theFileMap = fileMap(backupIndex, fileIndex);

    if (theFileMap.contains("path"))
        return theFileMap.at("path").toString();

    return S9sString();
}

S9sVariant
S9sBackup::fileSize(
        const int backupIndex,
        const int fileIndex)
{
    S9sVariantMap theFileMap = fileMap(backupIndex, fileIndex);

    if (theFileMap.contains("size"))
        return theFileMap.at("size");

    return S9sVariant();
}

S9sVariant
S9sBackup::fileCreated(
        const int backupIndex,
        const int fileIndex)
{
    S9sVariantMap theFileMap = fileMap(backupIndex, fileIndex);

    if (theFileMap.contains("created"))
        return theFileMap.at("created");

    return S9sVariant();
}

S9sVariantMap
S9sBackup::fileMap(
        const int backupIndex,
        const int fileIndex)
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

