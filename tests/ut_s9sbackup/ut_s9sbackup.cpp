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
#include "ut_s9sbackup.h"

#include "S9sBackup"
#include "S9sVariantMap"
#include "S9sRpcClient"
#include "S9sOptions"

#define DEBUG
#define WARNING
#include "s9sdebug.h"

static const char *backupJson1 = 
"{\n"
"    'backup': [ \n"
"    {\n"
"        'db': 'all',\n"
"        'files': [ \n"
"        {\n"
"            'class_name': 'CmonBackupFile',\n"
"            'created': '2017-06-09T09:15:19.000Z',\n"
"            'hash': 'md5:f670d738bcb318fa8d50a5b5f899c5bb',\n"
"            'path': 'pg_dump_2017-06-09_111515.sql.gz',\n"
"            'size': 856,\n"
"            'type': 'full'\n"
"        } ],\n"
"        'start_time': '2017-06-09T09:15:19.000Z'\n"
"    } ],\n"
"    'backup_host': '192.168.1.134',\n"
"    'chain_up': 0,\n"
"    'cid': 1,\n"
"    'class_name': 'CmonBackupRecord',\n"
"    'compressed': true,\n"
"    'config': \n"
"    {\n"
"        'backupDir': '/tmp',\n"
"        'backupHost': '192.168.1.134',\n"
"        'backupMethod': '',\n"
"        'backupToIndividualFiles': false,\n"
"        'backup_failover': false,\n"
"        'backup_failover_host': '',\n"
"        'ccStorage': true,\n"
"        'compression': true,\n"
"        'createdBy': 'pipas',\n"
"        'description': 'null',\n"
"        'includeDatabases': '',\n"
"        'netcat_port': 9999,\n"
"        'origBackupDir': '/tmp',\n"
"        'scheduleId': 0,\n"
"        'set_gtid_purged_off': true,\n"
"        'storageHost': '',\n"
"        'throttle_rate_iops': 0,\n"
"        'throttle_rate_netbw': 0,\n"
"        'usePigz': false,\n"
"        'wsrep_desync': false,\n"
"        'xtrabackupParallellism': 1,\n"
"        'xtrabackup_locks': false\n"
"    },\n"
"    'created': '2017-06-09T09:15:15.000Z',\n"
"    'created_by': '',\n"
"    'description': '',\n"
"    'finished': '2017-06-09T09:15:19.108Z',\n"
"    'id': 2,\n"
"    'job_id': 4,\n"
"    'log_file': '',\n"
"    'lsn': 0,\n"
"    'method': 'pgdump',\n"
"    'parent_id': 0,\n"
"    'root_dir': '/tmp/BACKUP-2',\n"
"    'schedule_id': 0,\n"
"    'status': 'Completed',\n"
"    'storage_host': '192.168.1.127',\n"
"    'total_datadir_size': 72663848,\n"
"    'verified': \n"
"    {\n"
"        'message': '',\n"
"        'status': 'Unverified',\n"
"        'verified_time': '1969-12-31T23:59:59.000Z'\n"
"    }\n"
"}\n"
;


UtS9sBackup::UtS9sBackup()
{
}

UtS9sBackup::~UtS9sBackup()
{
}

bool
UtS9sBackup::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testCreate,          retval);
    PERFORM_TEST(testSetProperties,   retval);
    PERFORM_TEST(testAssign,          retval);

    return retval;
}

/**
 * Creating nodes from simple (non JSon) strings and checking if the properties
 * are set.
 */
bool
UtS9sBackup::testCreate()
{

    return true;
}

/**
 * Testing the S9sBackup::setProperties() method. 
 */
bool
UtS9sBackup::testSetProperties()
{
    S9sVariantMap theMap;
    S9sBackup     theBackup;

    S9S_VERIFY(theMap.parse(backupJson1));
    theBackup.setProperties(theMap);

    S9S_COMPARE(theBackup.backupHost(),       "192.168.1.134");
    S9S_COMPARE(theBackup.id(),               2);
    S9S_COMPARE(theBackup.clusterId(),        1);
    S9S_COMPARE(theBackup.status(),           "COMPLETED");
    S9S_COMPARE(theBackup.rootDir(),          "/tmp/BACKUP-2");
    S9S_COMPARE(theBackup.owner(),            "pipas");
    S9S_COMPARE(theBackup.nBackups(),         1);
    S9S_COMPARE(theBackup.nFiles(0),          1);
    S9S_COMPARE(theBackup.filePath(0, 0), "pg_dump_2017-06-09_111515.sql.gz");
    S9S_COMPARE(theBackup.fileSize(0, 0),     856);
    S9S_COMPARE(theBackup.fileCreated(0, 0),   "2017-06-09T09:15:19.000Z");

    return true;
}

/**
 * Testing the S9sBackup::operator=(const S9sVariantMap &) operator.
 */
bool
UtS9sBackup::testAssign()
{
    S9sVariantMap theMap;
    S9sBackup     theBackup;

    S9S_VERIFY(theMap.parse(backupJson1));
    theBackup = theMap;

    S9S_COMPARE(theBackup.backupHost(),       "192.168.1.134");
    S9S_COMPARE(theBackup.id(),               2);
    S9S_COMPARE(theBackup.clusterId(),        1);
    S9S_COMPARE(theBackup.status(),           "COMPLETED");
    S9S_COMPARE(theBackup.rootDir(),          "/tmp/BACKUP-2");
    S9S_COMPARE(theBackup.owner(),            "pipas");
    S9S_COMPARE(theBackup.nBackups(),         1);
    S9S_COMPARE(theBackup.nFiles(0),          1);
    S9S_COMPARE(theBackup.filePath(0, 0), "pg_dump_2017-06-09_111515.sql.gz");
    S9S_COMPARE(theBackup.fileSize(0, 0),     856);
    S9S_COMPARE(theBackup.fileCreated(0, 0),   "2017-06-09T09:15:19.000Z");

    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sBackup)

