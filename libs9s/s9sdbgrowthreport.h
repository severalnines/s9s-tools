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
#pragma once

#include "S9sVariantMap"
#include "S9sFormat"
#include "S9sObject"
#include "S9sFormatter"
#include "S9sDateTime"

class S9sFormat;
class S9sOptions;
class S9sDateTime;

/**
 * A dedicated class for building and printing the detailed DbGrowth report.
 */
class S9sDbGrowthReport
{
    public:
        S9sDbGrowthReport();

        void printReport(const S9sVariant &data);

    private:
        void initialize();

        void prepareData(const S9sVariantList &dataList);

        void filterDataWithGrouping(const S9sVariantList &dataList,
                                    S9sVariantMap &dataGroupMap);

        void setDataMap(S9sVariantMap& map,
                        const S9sString& date,
                        const S9sString& dbName,
                        const S9sString& tableName,
                        ulonglong   tablesRows = 0,
                        ulonglong   dataSize   = 0,
                        ulonglong   indexSize  = 0);

        void calculateGroupByDate(S9sVariantMap&    dataGroupMap,
                                  S9sVariantMap&    dbsMap,
                                  const S9sString   &dateCreated);

        void calculateGroupByDbName(S9sVariantMap&    dataGroupMap,
                                    S9sVariantMap&    dbsMap,
                                    const S9sString   &dateCreated);

        static bool compareDataByTotalSizeAndDate(const S9sVariant &a,
                                                  const S9sVariant &b);

        void applyFiltersAndGroups(const S9sVariantList &dbsList,
                                   S9sVariantMap        &dataGroupMap,
                                   const S9sString      &dateCreated);

        void collectTablesData(const S9sVariantList &tablesList,
                               const S9sString      &dateCreated);

        const char *headerColorBegin() const;
        const char *headerColorEnd() const;

    private:
        S9sOptions    *m_options;
        S9sString      m_dbNameOption;
        S9sDateTime    m_dateOption;
        S9sDateTime    m_dateTimeDaysAgo;
        S9sDateTime    m_dataDateWithoutTime;
        S9sString      m_dbName;
        S9sFormat      m_dateFormat;
        S9sFormat      m_dbNameFormat;
        S9sFormat      m_tableNameFormat;
        S9sVariantList m_dataReportList;
        uint           m_nLines;
        bool           m_syntaxHighlight;
        bool           m_hasDbName;
        bool           m_hasDate;
        bool           m_groupByDate;
        bool           m_groupByDbName;
        bool           m_dbNameFilterPreConditions;
        bool           m_dbNameFilterDataFound;
};