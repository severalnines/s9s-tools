/*
 * Severalnines Tools
 * Copyright (C) 2017-2018 Severalnines AB
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
#include "s9sdbgrowthreport.h"
#include "s9soptions.h"
#include "s9sdatetime.h"
#include "s9sdebug.h"

/**
 * Report's constructor.
 */
S9sDbGrowthReport::S9sDbGrowthReport()
{
    initialize();
}

/**
 *  Initializing the member variables.
 */
void S9sDbGrowthReport::initialize()
{
    m_options                      = S9sOptions::instance();
    m_syntaxHighlight              = m_options->useSyntaxHighlight();
    m_hasDbName                    = m_options->hasDbSchemaName();
    m_hasDate                      = m_options->hasDbSchemaDate();
    m_dbNameOption                 = m_options->dBSchemaName();
    m_groupByDate                  = !m_hasDate && !m_hasDbName;
    m_groupByDbName                = m_hasDate && !m_hasDbName;
    m_dbNameFilterPreConditions    = m_options->hasDbSchemaName() && !m_dbNameOption.empty();
    /*
     * The flag for detecting the data being filtered with DB name.
     */
    m_dbNameFilterDataFound        = false;
    /*
     * Total lines in the report.
     */
    m_nLines                       = 0;
    /*
    * Time in the past by 31 days.
    */
    const uint secondsAgoFor31Days = 31 * 24 * 3600;
    const S9sDateTime
            &currentDateTime       = S9sDateTime::currentDateTime();
    m_dateTimeDaysAgo              = S9sDateTime((currentDateTime - secondsAgoFor31Days)/1000);
    if(m_hasDate)
        m_dateOption.parseDateFormat(m_options->dBSchemaDate());
    m_dataReportList.clear();
}

/**
 * Prints the DbGrowth list in its long and detailed format.
 * @param data
 */
void S9sDbGrowthReport::printReport(const S9sVariant &data)
{
    initialize();
    const S9sVariantList &dataList = data.toVariantList();

    S9sFormat            tablesRowsFormat;
    S9sFormat            dataSizeFormat;
    S9sFormat            indexSizeFormat;
    S9sFormat            totalSizeFormat;

    /*
     * Preparing the data for the report: flattening and sorting the list of data source.
     */
    prepareData(dataList);
    /*
     * Printing the header.
     */
    if (!m_options->isNoHeaderRequested() && m_nLines > 0)
    {
        printf("%s", headerColorBegin());
        m_dateFormat.printHeader("DATE");
        if(!m_groupByDate)
        {
            m_dbNameFormat.printHeader("DB_NAME");
            if (!m_groupByDbName)
            {
                m_tableNameFormat.printHeader("TABLE_NAME");
            }
            tablesRowsFormat.printHeader("TABLES_ROWS");
        }
        dataSizeFormat.printHeader("DATA_SIZE");
        indexSizeFormat.printHeader("INDEX_SIZE");
        totalSizeFormat.printHeader("TOTAL_SIZE");
        printf("%s", headerColorEnd());

        printf("\n");
    }

    /*
     * Sorting the report data list by total size in descending order.
     * If the size is the same for two results, the sorting is made
     * by date in descending order within the size group.
     */
    sort(m_dataReportList.begin(), m_dataReportList.end(),
         compareDataByTotalSizeAndDate);

    /*
     * Setting up the colors highlighting
     */
    const char *colorBegin      = "";
    const char *colorEnd        = "";
    const char *groupColorBegin = "";
    const char *groupColorEnd   = "";

    if (m_syntaxHighlight)
    {
        colorBegin      = XTERM_COLOR_ORANGE;
        colorEnd        = TERM_NORMAL;
        groupColorBegin = XTERM_COLOR_CYAN;
        groupColorEnd   = TERM_NORMAL;
    }

    /*
     * Printing the data
     */
    for (uint idxData = 0; idxData < m_dataReportList.size(); ++idxData)
    {
        S9sVariantMap dataMap = m_dataReportList[idxData].toVariantMap();

        printf("%s", groupColorBegin);
        m_dateFormat.printf(dataMap["date"].toString());
        printf("%s", groupColorEnd);

        if(!m_groupByDate)
        {
            printf("%s", colorBegin);
            m_dbNameFormat.printf(dataMap["db_name"].toString());
            printf("%s", colorEnd);

            printf("%s", colorBegin);
            if (!m_groupByDbName)
            {
                m_tableNameFormat.printf(dataMap["table_name"].toString());
            }
            tablesRowsFormat.printf(dataMap["tables_rows"].toULongLong());
        }
        printf("%s", colorEnd);

        printf("%s", groupColorBegin);
        dataSizeFormat.printf(dataMap["data_size"].toULongLong());
        indexSizeFormat.printf(dataMap["index_size"].toULongLong());
        totalSizeFormat.printf(dataMap["total_size"].toULongLong());
        printf("%s", groupColorEnd);

        ::printf("\n");
    }

}

/**
 * Compares the results by total_size and date in sorting
 * @param a
 * @param b
 * @return
 */
bool S9sDbGrowthReport::compareDataByTotalSizeAndDate(const S9sVariant &a,
                                                      const S9sVariant &b)
{
    S9sVariantMap aMap = a.toVariantMap();
    S9sVariantMap bMap = b.toVariantMap();

    bool result = false;
    if(aMap["total_size"].toULongLong() > bMap["total_size"].toULongLong())
    {
        result = true;
    }
    else if(aMap["total_size"].toULongLong() == bMap["total_size"].toULongLong())
    {
        result = aMap["date"].toString() > bMap["date"].toString();
    }
    return result;
}

/**
 * Method prepares data for the DbGrowthReport: sorts by name and date and provides grouping.
 * @param dataList
 * @return
 */
void
S9sDbGrowthReport::prepareData(const S9sVariantList &dataList)
{
    S9sVariantMap dataGroupMap;
    filterDataWithGrouping(dataList, dataGroupMap);

    if(m_groupByDate)
    {
        for(S9sString key : dataGroupMap.keys())
        {
            S9sVariantMap  dataMap = dataGroupMap[key].toVariantMap();
            S9sString dateCreated = dataMap["date_created"].toString();
            ulonglong dataSize = dataMap["data_size"].toULongLong();
            ulonglong indexSize = dataMap["index_size"].toULongLong();

            setDataMap(dataMap, dateCreated, "", "", 0, dataSize, indexSize);
            m_dataReportList << dataMap;
            m_nLines++;
        }
    }
    if(m_groupByDbName)
    {
        for(S9sString dbKey : dataGroupMap.keys()) {
            S9sVariantMap dbMap = dataGroupMap[dbKey].toVariantMap();
            for (S9sString dbName: dbMap.keys()) {
                S9sVariantMap dataMap = dbMap[dbName].toVariantMap();
                S9sString dateCreated = dataMap["date_created"].toString();
                ulonglong dataSize = dataMap["data_size"].toULongLong();
                ulonglong indexSize = dataMap["index_size"].toULongLong();
                ulonglong rowCount = dataMap["row_count"].toULongLong();

                setDataMap(dataMap, dateCreated, dbName, "", rowCount, dataSize, indexSize);
                m_dataReportList << dataMap;
                m_nLines++;
            }
        }
    }
}

/**
 * Filters data with given criteria and groups the results
 * @param dataList
 * @param dataListFlatten
 * @param dataGroupMap
 */
void
S9sDbGrowthReport::filterDataWithGrouping(const S9sVariantList &dataList,
                                          S9sVariantMap        &dataGroupMap)
{

    for (uint idxData = 0; idxData < dataList.size(); ++idxData)
    {
        S9sVariantMap     dbGrowthMap  = dataList[idxData].toVariantMap();
        const S9sVariantList &dbsList  = dbGrowthMap["dbs"].toVariantList();
        const S9sString &dateCreated   = dbGrowthMap["created"].toString();

        S9sDateTime dataDate;
        dataDate.parseDbGrowthDataFormat(
                dateCreated + " " +
                 dbGrowthMap["year"].toString());

        m_dataDateWithoutTime.setDate(dataDate.year(),
                                    dataDate.yearday(),
                                    dataDate.month(),
                                    dataDate.day());

        bool dateFilterIsValid = false;

        if(m_options->hasDbSchemaDate() && !m_options->dBSchemaDate().empty())
        {
            dateFilterIsValid = m_dataDateWithoutTime == m_dateOption;
        }
        else
        {
            dateFilterIsValid = dataDate >= m_dateTimeDaysAgo;
        }

        if(!dateFilterIsValid)
            continue;


        m_dbNameFilterDataFound = false;
        applyFiltersAndGroups(dbsList, dataGroupMap, dateCreated);
        /*
         * In case if the DB name filter was set and no data found, the record
         * that contains the whole dbs array should be skipped from the report.
         */
        if(m_dbNameFilterPreConditions && !m_dbNameFilterDataFound)
            continue;

        /*
         * If a dbs array is empty and no filtering applied, the record with
         * a date and empty db information should be added to the report.
         */
        if(dbsList.empty())
        {
            S9sVariantMap  dataMap;
            setDataMap(dataMap, dateCreated, "", "");
            m_dataReportList << dataMap;
            m_nLines++;
        }
        m_dateFormat.widen(dateCreated);
    }
}

/**
 * Iterates through the db data, filters the results by date, by db name
 * and saves the output to the flattened list
 * @param dbsList
 * @param dataGroupMap
 */
void
S9sDbGrowthReport::applyFiltersAndGroups(const S9sVariantList &dbsList,
                                         S9sVariantMap        &dataGroupMap,
                                         const S9sString      &dateCreated)
{
    /*
     * The flag for filtering pre-conditions with DB name.
     */
    for(uint idxDbs = 0; idxDbs < dbsList.size(); ++idxDbs)
    {
        S9sVariantMap   dbsMap  = dbsList[idxDbs].toVariantMap();
        m_dbName = dbsMap["db_name"].toString();

        if ( m_dbNameFilterPreConditions &&
             !m_dbName.toUpper().startsWith(m_dbNameOption.toUpper().c_str()))
        {
            continue;
        }
        m_dbNameFilterDataFound = true;

        if(m_groupByDate)
        {
            calculateGroupByDate(dataGroupMap, dbsMap, dateCreated);
            continue;
        }

        m_dbNameFormat.widen(m_dbName);

        if(m_groupByDbName)
        {
            calculateGroupByDbName(dataGroupMap, dbsMap, dateCreated);
            continue;
        }

        const S9sVariantList &tablesList = dbsMap["tables"].toVariantList();
        /*
         * If tables were not found, adding the upper-level information
         */
        if(tablesList.empty())
        {
            S9sVariantMap dataMap;
            setDataMap(dataMap, dateCreated, m_dbName, "",
                       dbsMap["row_count"].toULongLong(),
                       dbsMap["data_size"].toULongLong(),
                       dbsMap["index_size"].toULongLong());
            m_dataReportList << dataMap;
            m_nLines++;
        }
        /*
         * Filling up the detailed report, which includes tables' names and their data
         */
        collectTablesData(tablesList, dateCreated);
    }
}

/**
 * Collects tables data and puts them to the flattened list.
 * @param tablesList
 */
void
S9sDbGrowthReport::collectTablesData(const S9sVariantList &tablesList,
                                     const S9sString      &dateCreated)
{
    for(uint idxTables = 0; idxTables < tablesList.size(); ++idxTables) {
        S9sVariantMap   tableMap   = tablesList[idxTables].toVariantMap();
        const S9sString &tableName = tableMap["table_name"].toString();

        m_tableNameFormat.widen(tableName);

        S9sVariantMap  dataMap;
        setDataMap(dataMap, dateCreated, m_dbName,
                   tableName,
                   tableMap["row_count"].toULongLong(),
                   tableMap["data_size"].toULongLong(),
                   tableMap["index_size"].toULongLong());
        m_dataReportList << dataMap;
        m_nLines++;
    }
}

/**
 * Sets the data map that is being used in a final flattened list
 * @param map
 * @param date
 * @param dbName
 * @param tableName
 * @param tablesRows
 * @param dataSize
 * @param indexSize
 */
void
S9sDbGrowthReport::setDataMap(S9sVariantMap   &map,
                              const S9sString &date,
                              const S9sString &dbName,
                              const S9sString &tableName,
                              ulonglong       tablesRows,
                              ulonglong       dataSize,
                              ulonglong       indexSize)
{
    map["date"] = date;
    map["db_name"] = dbName;
    map["table_name"] = tableName;
    map["tables_rows"] = tablesRows;
    map["data_size"] = dataSize;
    map["index_size"] = indexSize;
    map["total_size"] = dataSize + indexSize;
}

/**
 * Groups the results by date
 * @param dataGroupMap
 * @param dbsMap
 */
void
S9sDbGrowthReport::calculateGroupByDate(S9sVariantMap&    dataGroupMap,
                                        S9sVariantMap&    dbsMap,
                                        const S9sString   &dateCreated)
{
    ulonglong dataSize = 0;
    ulonglong indexSize = 0;
    S9sVariantMap  dataMap;
    if(dataGroupMap.contains(m_dataDateWithoutTime.toString()))
    {
        dataMap = dataGroupMap[m_dataDateWithoutTime.toString()].toVariantMap();
        dataSize = dataMap["data_size"].toULongLong();
        indexSize = dataMap["index_size"].toULongLong();
    }
    else
    {
        dataMap["date_created"] = dateCreated;
        dataMap["data_size"] = 0;
        dataMap["index_size"] = 0;
    }
    dataSize += dbsMap["data_size"].toULongLong();
    indexSize += dbsMap["index_size"].toULongLong();
    dataMap["data_size"]  = dataSize;
    dataMap["index_size"] = indexSize;
    dataGroupMap[m_dataDateWithoutTime.toString()] = dataMap;
}

/**
 * Groups the results by Db name
 * @param dataGroupMap
 * @param dbsMap
 */
void
S9sDbGrowthReport::calculateGroupByDbName(S9sVariantMap   &dataGroupMap,
                                          S9sVariantMap   &dbsMap,
                                          const S9sString &dateCreated)
{
    ulonglong dataSize = 0;
    ulonglong indexSize = 0;
    ulonglong rowCount = 0;
    S9sVariantMap  dbMap;
    S9sVariantMap  dataMap;
    if(dataGroupMap.contains(m_dataDateWithoutTime.toString()))
    {
        dbMap = dataGroupMap[m_dataDateWithoutTime.toString()].toVariantMap();
        if(dbMap.contains(m_dbName))
        {
            dataMap = dbMap[m_dbName].toVariantMap();
        }
        else
        {
            dataMap["date_created"] = dateCreated;
            dataMap["data_size"] = 0;
            dataMap["index_size"] = 0;
            dataMap["row_count"] = 0;
            dbMap[m_dbName] = dataMap;
        }
        dataSize = dataMap["data_size"].toULongLong();
        indexSize = dataMap["index_size"].toULongLong();
        rowCount = dataMap["row_count"].toULongLong();
    }
    else
    {
        dataMap["date_created"] = dateCreated;
        dataMap["data_size"] = 0;
        dataMap["index_size"] = 0;
        dataMap["row_count"] = 0;
        dbMap[m_dbName] = dataMap;
    }
    dataSize += dbsMap["data_size"].toULongLong();
    indexSize += dbsMap["index_size"].toULongLong();
    rowCount += dbsMap["row_count"].toULongLong();
    dataMap["data_size"]  = dataSize;
    dataMap["index_size"] = indexSize;
    dataMap["row_count"]  = rowCount;
    dbMap[m_dbName] = dataMap;
    dataGroupMap[m_dataDateWithoutTime.toString()] = dbMap;
}

/**
 * Starts the header to be printed with bold font.
 * @return
 */
const char *
S9sDbGrowthReport::headerColorBegin() const
{
    if (m_options->useSyntaxHighlight())
        return TERM_BOLD;
    return "";
}

/**
 * Stops the header to be printed with bold font.
 * @return
 */
const char *
S9sDbGrowthReport::headerColorEnd() const
{
    if (m_options->useSyntaxHighlight())
        return TERM_NORMAL;
    return "";
}
