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
#pragma once

#include "S9sVariantMap"

/**
 * A class that represents a backup. 
 */
class S9sBackup
{
    public:
        S9sBackup();
        S9sBackup(const S9sVariantMap &properties);

        virtual ~S9sBackup();

        S9sBackup &operator=(const S9sVariantMap &rhs);

        bool hasProperty(const S9sString &key) const;
        S9sVariant property(const S9sString &name) const;
        void setProperty(const S9sString &name, const S9sString &value);
        
        const S9sVariantMap &toVariantMap() const;
        void setProperties(const S9sVariantMap &properties);

        /*
         * Getters for simple properties.
         */
        S9sString backupHost() const;
        S9sString storageHost() const;
        int id() const;
        int clusterId() const;
        int jobId() const;
        S9sString status() const;
        S9sVariant begin() const;
        S9sString beginAsString() const;
        S9sVariant end() const;
        S9sString endAsString() const;
        S9sString rootDir() const;
        bool isCompressed() const;
        S9sString method() const;
        S9sString description() const;

        /*
         * Properties by actual backups and files.
         */
        int nBackups() const;

        S9sString databaseNamesAsString(
                const int        backupIndex,
                const S9sString &delimiter) const;

        int nFiles(const int backupIndex) const;

        S9sString fileName(
                const int backupIndex,
                const int fileIndex) const;

        S9sString filePath(
                const int backupIndex,
                const int fileIndex) const;

        S9sVariant fileSize(
                const int backupIndex,
                const int fileIndex) const;

        S9sVariant fileCreated(
                const int backupIndex,
                const int fileIndex) const;

        S9sString fileCreatedString(
                const int backupIndex,
                const int fileIndex) const;
        
        /*
         * The configuration values.
         */
        S9sString configOwner() const;
        S9sString configBackupHost() const;
        S9sString configMethod() const;
        S9sString configDescription() const;

        S9sString toString(
                const int        backupIndex,
                const int        fileIndex,
                const bool       syntaxHighlight,
                const S9sString &formatString) const;

    private:
        S9sVariant configValue(const S9sString &key) const;
        S9sVariant config() const;
        S9sVariantMap backupMap(const int backupIndex) const;
        S9sVariantMap fileMap(const int backupIndex, const int fileIndex) const;

    private:
        S9sVariantMap    m_properties;
};

