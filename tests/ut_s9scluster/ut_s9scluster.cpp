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
#include "ut_s9scluster.h"

#include "S9sCluster"
#include "S9sVariantMap"
#include "S9sRpcClient"
#include "S9sOptions"

#define DEBUG
#define WARNING
#include "s9sdebug.h"

static const char *clusterJson1 = 
"{\n"
"    'alarm_statistics': \n"
"    {\n"
"        'class_name': 'CmonAlarmStatistics',\n"
"        'cluster_id': 1,\n"
"        'critical': 2,\n"
"        'warning': 0\n"
"    },\n"
"    'class_name': 'CmonClusterInfo',\n"
"    'cluster_auto_recovery': true,\n"
"    'cluster_id': 1,\n"
"    'cluster_name': 'ft_postgresql_19203',\n"
"    'cluster_type': 'POSTGRESQL_SINGLE',\n"
"    'configuration_file': '/tmp/cmon_1.cnf',\n"
"    'group_owner': \n"
"    {\n"
"        'class_name': 'CmonGroup',\n"
"        'group_id': 2,\n"
"        'group_name': 'users'\n"
"    },\n"
"    'hosts': [ \n"
"    {\n"
"        'class_name': 'CmonPostgreSqlHost',\n"
"        'clusterid': 1,\n"
"        'configfile': [ '/etc/postgresql/9.6/main/postgresql.conf' ],\n"
"        'connected': true,\n"
"        'data_directory': '/var/lib/postgresql/9.6/main',\n"
"        'datadir': '/var/lib/postgresql/9.6/main',\n"
"        'description': '',\n"
"        'distribution': \n"
"        {\n"
"            'codename': 'xenial',\n"
"            'name': 'ubuntu',\n"
"            'release': '16.04',\n"
"            'type': 'debian'\n"
"        },\n"
"        'hostId': 1,\n"
"        'hostname': '192.168.1.129',\n"
"        'hoststatus': 'CmonHostOnline',\n"
"        'hot_standby': true,\n"
"        'ip': '192.168.1.129',\n"
"        'lastseen': 1496744170,\n"
"        'logfile': '/var/log/postgresql/postgresql-9.6-main.log',\n"
"        'maintenance_mode_active': false,\n"
"        'message': 'Up and running',\n"
"        'nodetype': 'postgres',\n"
"        'pid': 4296,\n"
"        'pingstatus': 0,\n"
"        'pingtime': 0,\n"
"        'port': 8089,\n"
"        'readonly': false,\n"
"        'received_location': '0/1530D70',\n"
"        'replay_location': '0/1530D70',\n"
"        'role': 'master',\n"
"        'sshfailcount': 0,\n"
"        'ssl_certs': \n"
"        {\n"
"            'server': \n"
"            {\n"
"                'ca': '',\n"
"                'id': 0,\n"
"                'key': '/etc/ssl/private/ssl-cert-snakeoil.key',\n"
"                'path': '/etc/ssl/certs/ssl-cert-snakeoil.pem'\n"
"            }\n"
"        },\n"
"        'timestamp': 1496744170,\n"
"        'unique_id': 1,\n"
"        'uptime': 337989,\n"
"        'version': '9.6.3',\n"
"        'wallclock': 1496744155,\n"
"        'wallclocktimestamp': 1496744135\n"
"    }, \n"
"    {\n"
"        'class_name': 'CmonHost',\n"
"        'clusterid': 1,\n"
"        'configfile': '/tmp/cmon_1.cnf',\n"
"        'connected': true,\n"
"        'distribution': \n"
"        {\n"
"            'codename': 'trusty',\n"
"            'name': 'ubuntu',\n"
"            'release': '14.04',\n"
"            'type': 'debian'\n"
"        },\n"
"        'hostId': 2,\n"
"        'hostname': '192.168.1.127',\n"
"        'hoststatus': 'CmonHostOnline',\n"
"        'ip': '192.168.1.127',\n"
"        'lastseen': 1496744133,\n"
"        'logfile': '/tmp/cmon_1.log',\n"
"        'maintenance_mode_active': false,\n"
"        'message': 'Up and running',\n"
"        'nodetype': 'controller',\n"
"        'pid': 18459,\n"
"        'pingstatus': 0,\n"
"        'pingtime': 0,\n"
"        'port': 9555,\n"
"        'role': 'controller',\n"
"        'timestamp': 1496744135,\n"
"        'unique_id': 2,\n"
"        'uptime': 338737,\n"
"        'version': '1.4.2',\n"
"        'wallclock': 1496744135,\n"
"        'wallclocktimestamp': 1496744135\n"
"    } ],\n"
"    'info': \n"
"    {\n"
"        'cluster.status': 2,\n"
"        'cluster.statustext': 'Cluster started.',\n"
"        'cmon.domainname': '',\n"
"        'cmon.hostname': 't7500',\n"
"        'cmon.running': true,\n"
"        'cmon.starttime': 1496406208,\n"
"        'cmon.uptime': 337967,\n"
"        'conf.backup_retention': 31,\n"
"        'conf.clusterid': 1,\n"
"        'conf.clustername': 'ft_postgresql_19203',\n"
"        'conf.clustertype': 5,\n"
"        'conf.configfile': '/tmp/cmon_1.cnf',\n"
"        'conf.hostname': '192.168.1.127',\n"
"        'conf.os': 'debian',\n"
"        'conf.statustext': 'Configuration loaded.',\n"
"        'host.1.connected': true,\n"
"        'host.1.cpu_io_wait_percent': 1.60776,\n"
"        'host.1.cpu_steal_percent': 0,\n"
"        'host.1.cpu_usage_percent': 5.08688,\n"
"        'host.1.cpucores': 16,\n"
"        'host.1.cpuinfo': [ \n"
"        {\n"
"            'class_name': 'CmonCpuInfo',\n"
"            'cpucores': 4,\n"
"            'cpumaxmhz': 2.268e+06,\n"
"            'cpumhz': 1600,\n"
"            'cpumodel': 'Intel(R) Xeon(R) CPU           L5520  @ 2.27GHz',\n"
"            'cputemp': 54.5,\n"
"            'hostid': 1,\n"
"            'physical_cpu_id': 0,\n"
"            'siblings': 8,\n"
"            'vendor': 'GenuineIntel'\n"
"        }, \n"
"        {\n"
"            'class_name': 'CmonCpuInfo',\n"
"            'cpucores': 4,\n"
"            'cpumaxmhz': 2.268e+06,\n"
"            'cpumhz': 1600,\n"
"            'cpumodel': 'Intel(R) Xeon(R) CPU           L5520  @ 2.27GHz',\n"
"            'cputemp': 54.5,\n"
"            'hostid': 1,\n"
"            'physical_cpu_id': 1,\n"
"            'siblings': 8,\n"
"            'vendor': 'GenuineIntel'\n"
"        } ],\n"
"        'host.1.cpumaxmhz': 2268,\n"
"        'host.1.cpumhz': 1600,\n"
"        'host.1.cpumodel': 'Intel(R) Xeon(R) CPU        L5520  @ 2.27GHz',\n"
"        'host.1.cputemp': 54.5,\n"
"        'host.1.devices': [ '/dev/mapper/core1--vg-root' ],\n"
"        'host.1.free_disk_bytes': 186433818624,\n"
"        'host.1.hostname': '192.168.1.129',\n"
"        'host.1.interfaces': [ 'eth0' ],\n"
"        'host.1.ip': '192.168.1.129',\n"
"        'host.1.membuffer': 0,\n"
"        'host.1.memcached': 41,\n"
"        'host.1.memfree': 14935744,\n"
"        'host.1.memtotal': 16777216,\n"
"        'host.1.network_interfaces': [ \n"
"        {\n"
"            'interface_name': 'eth0',\n"
"            'rx_bytes_per_sec': 2474.26,\n"
"            'tx_bytes_per_sec': 4532.81\n"
"        } ],\n"
"        'host.1.pingdelay': -1,\n"
"        'host.1.pingstatustext': \"Creating ICMP socket (to ping '192.168.1.129') failed: Operation not permitted.\",\n"
"        'host.1.port': 8089,\n"
"        'host.1.rx_bytes_per_second': 2474.26,\n"
"        'host.1.swapfree': 0,\n"
"        'host.1.swaptotal': 0,\n"
"        'host.1.total_disk_bytes': 208033853440,\n"
"        'host.1.tx_bytes_per_second': 4532.81,\n"
"        'host.1.uptime': 338273,\n"
"        'host.1.wallclock': 1496744155,\n"
"        'host.1.wallclocksampled': 1496744135,\n"
"        'host.2.class_name': 'controller',\n"
"        'host.2.connected': true,\n"
"        'host.2.cpu_io_wait_percent': 1.7426,\n"
"        'host.2.cpu_steal_percent': 0,\n"
"        'host.2.cpu_usage_percent': 147.845,\n"
"        'host.2.cpucores': 24,\n"
"        'host.2.cpuinfo': [ \n"
"        {\n"
"            'class_name': 'CmonCpuInfo',\n"
"            'cpucores': 6,\n"
"            'cpumaxmhz': 2.661e+06,\n"
"            'cpumhz': 1596,\n"
"            'cpumodel': 'Intel(R) Xeon(R) CPU           X5650  @ 2.67GHz',\n"
"            'cputemp': 0,\n"
"            'hostid': 2,\n"
"            'physical_cpu_id': 0,\n"
"            'siblings': 12,\n"
"            'vendor': 'GenuineIntel'\n"
"        }, \n"
"        {\n"
"            'class_name': 'CmonCpuInfo',\n"
"            'cpucores': 6,\n"
"            'cpumaxmhz': 2.661e+06,\n"
"            'cpumhz': 1596,\n"
"            'cpumodel': 'Intel(R) Xeon(R) CPU           X5650  @ 2.67GHz',\n"
"            'cputemp': 0,\n"
"            'hostid': 2,\n"
"            'physical_cpu_id': 1,\n"
"            'siblings': 12,\n"
"            'vendor': 'GenuineIntel'\n"
"        } ],\n"
"        'host.2.cpumaxmhz': 2661,\n"
"        'host.2.cpumhz': 1596,\n"
"        'host.2.cpumodel': 'Intel(R) Xeon(R) CPU         X5650  @ 2.67GHz',\n"
"        'host.2.cputemp': 0,\n"
"        'host.2.devices': [ '/dev/sda1' ],\n"
"        'host.2.free_disk_bytes': 1396676083712,\n"
"        'host.2.hostname': '192.168.1.127',\n"
"        'host.2.interfaces': [ 'eth0' ],\n"
"        'host.2.ip': '192.168.1.127',\n"
"        'host.2.membuffer': 428764,\n"
"        'host.2.memcached': 35153412,\n"
"        'host.2.memfree': 4377200,\n"
"        'host.2.memtotal': 49453276,\n"
"        'host.2.network_interfaces': [ \n"
"        {\n"
"            'interface_name': 'eth0',\n"
"            'rx_bytes_per_sec': 57331,\n"
"            'tx_bytes_per_sec': 5996.88\n"
"        } ],\n"
"        'host.2.pingdelay': -1,\n"
"        'host.2.pingstatustext': \"Creating ICMP socket (to ping '192.168.1.127') failed: Operation not permitted.\",\n"
"        'host.2.port': 9555,\n"
"        'host.2.rx_bytes_per_second': 57331,\n"
"        'host.2.swapfree': 0,\n"
"        'host.2.swaptotal': 0,\n"
"        'host.2.total_disk_bytes': 2125259440128,\n"
"        'host.2.tx_bytes_per_second': 5996.88,\n"
"        'host.2.uptime': 790772,\n"
"        'host.2.version': '1.4.2',\n"
"        'host.2.wallclock': 1496744135,\n"
"        'host.2.wallclocksampled': 1496744135,\n"
"        'license.expires': -1,\n"
"        'license.status': false,\n"
"        'license.statustext': 'No license found.',\n"
"        'mail.statustext': 'Created.',\n"
"        'netStat.1.eth0.rxBytes': 1505133999,\n"
"        'netStat.1.eth0.txBytes': 899854836,\n"
"        'netStat.2.eth0.rxBytes': 65120542031,\n"
"        'netStat.2.eth0.txBytes': 172177249308\n"
"    },\n"
"    'job_statistics': \n"
"    {\n"
"        'by_state': \n"
"        {\n"
"            'ABORTED': 0,\n"
"            'DEFINED': 0,\n"
"            'DEQUEUED': 0,\n"
"            'FAILED': 0,\n"
"            'FINISHED': 4,\n"
"            'RUNNING': 0\n"
"        },\n"
"        'class_name': 'CmonJobStatistics',\n"
"        'cluster_id': 1\n"
"    },\n"
"    'log_file': '/tmp/cmon_1.log',\n"
"    'maintenance_mode_active': false,\n"
"    'managed': true,\n"
"    'node_auto_recovery': true,\n"
"    'owner': \n"
"    {\n"
"        'class_name': 'CmonUser',\n"
"        'email_address': '',\n"
"        'groups': [ \n"
"        {\n"
"            'class_name': 'CmonGroup',\n"
"            'group_id': 2,\n"
"            'group_name': 'users'\n"
"        } ],\n"
"        'user_id': 3,\n"
"        'user_name': 'pipas'\n"
"    },\n"
"    'state': 'STARTED',\n"
"    'status_text': 'All nodes are operational.',\n"
"    'vendor': 'postgres',\n"
"    'version': '9.6'\n"
"}\n"
;


UtS9sCluster::UtS9sCluster()
{
}

UtS9sCluster::~UtS9sCluster()
{
}

bool
UtS9sCluster::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testCreate,          retval);
    PERFORM_TEST(testAssign,          retval);

    return retval;
}

/**
 *
 */
bool
UtS9sCluster::testCreate()
{
    S9sCluster cluster;

    S9S_COMPARE(cluster.name(),      "");
    S9S_COMPARE(cluster.ownerName(), "");
    return true;
}

/**
 *
 */
bool
UtS9sCluster::testAssign()
{
    S9sVariantMap theMap;
    S9sCluster    theCluster;

    S9S_VERIFY(theMap.parse(clusterJson1));
    theCluster = theMap;

    S9S_COMPARE(theCluster.className(), "CmonClusterInfo");
    S9S_COMPARE(theCluster.name(),      "ft_postgresql_19203");

    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sCluster)

