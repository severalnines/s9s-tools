#include "s9sreplication.h"

#include "S9sFormatter"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sReplication::S9sReplication() :
    S9sObject()
{
    m_properties["class_name"] = "S9sReplication";
}

/**
 * \param cluster The cluster that holds the slave.
 * \param slave The slave node of a replication link.
 */
S9sReplication::S9sReplication(
        const S9sCluster &cluster,
        const S9sNode    &slave)
{
    m_properties["class_name"] = "S9sReplication";

    m_cluster = cluster;
    m_slave   = slave;
}

/**
 * \returns True if the properties of the slave set in the object indeed
 *   suggests that this is a slave and has a master host.
 */
bool
S9sReplication::isValid() const
{
    S9sString role = m_slave.role();

    return 
        role != "controller" && 
        role != "master" &&
        !masterHostName().empty();

}

/**
 * \returns The name of the slave host.
 */
S9sString 
S9sReplication::slaveHostName() const
{
    return m_slave.hostName();
}

/**
 * \returns The name of the master host if this is a valid replication.
 */
S9sString
S9sReplication::masterHostName() const
{
    return m_slave.masterHost();
}

/**
 * \returns The port of the slave node.
 */
int
S9sReplication::slavePort() const
{
    return m_slave.port();
}

/**
 * \returns The port number of the master if the replication is valid.
 */
int
S9sReplication::masterPort() const
{
    return m_slave.masterPort();
}

/**
 * \returns The host name of the slave node and the port of the slave node.
 */
S9sString
S9sReplication::slaveName() const
{
    S9sString retval;

    retval.sprintf("%s:%d", STR(slaveHostName()), slavePort());
    return retval;
}

/**
 * \returns The host name of the master node and the port of the master node
 *   if the replication is valid.
 */
S9sString
S9sReplication::masterName() const
{
    S9sString retval;

    retval.sprintf("%s:%d", STR(masterHostName()), masterPort());
    return retval;
}

/**
 * \returns The short version of the status for the slave host (e.g. "Online").
 */
S9sString
S9sReplication::slaveStatusShort() const
{
    return m_slave.hostStatusShort();
}

/**
 * \returns The "slave_io_state" property of the host that describes the slave
 *   status in a human readable format.
 */
S9sString
S9sReplication::slaveMessage() const
{
    S9sVariantMap map = slaveInfo();
    return map["slave_io_state"].toString();
}

int
S9sReplication::secondsBehindMaster() const
{
    S9sVariantMap map = slaveInfo();
    return map["slave_io_state"].toInt();
}

/**
 * \returns Dunno, some property of the slave that describes where it is in the
 * master's log?
 */
S9sString
S9sReplication::slavePosition() const
{
    S9sVariantMap map = slaveInfo();

    // This is for mysql.
    if (map.contains("exec_master_log_pos"))
        return map.at("exec_master_log_pos").toString();

    // This is for postgresql
    if (map.contains("replay_location"))
        return map.at("replay_location").toString();

    return S9sString();
}

S9sString
S9sReplication::masterPosition() const
{
    S9sVariantMap map = masterInfo();
    return map["position"].toString();
}

/**
 * \param syntaxHighlight Controls if the string will have colors or not.
 * \param formatString The formatstring with markup.
 * \returns The string representation according to the format string.
 *
 * Converts the object to a string using a special format string that may
 * contain field names of properties of the object.
 */
S9sString
S9sReplication::toString(
        const bool       syntaxHighlight,
        const S9sString &formatString) const
{
    S9sFormatter formatter;
    S9sString    retval;
    S9sString    tmp, part;
    char         c;
    S9sString    partFormat;
    bool         percent      = false;
    bool         escaped      = false;

    for (uint n = 0; n < formatString.size(); ++n)
    {
        c = formatString[n];
       
        if (c == '%' && !percent)
        {
            percent    = true;
            partFormat = "%";
            continue;
        } else if (c == '\\' && !escaped)
        {
            escaped = true;
            continue;
        }

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
                case 'c':
                    // The cluster ID of the slave.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), m_cluster.clusterId());
                    retval += tmp;
                    break;
                
                case 'd':
                    // Seconds behind master.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), secondsBehindMaster());
                    retval += tmp;
                    break;

                case 'h':
                    // The host name of the slave.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(slaveHostName()));
                    retval += tmp;
                    break;
                
                case 'H':
                    // The host name of the master.
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(masterHostName()));
                    retval += tmp;
                    break;
                
                case 'o':
                    // The slave position
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(slavePosition()));
                    retval += tmp;
                    break;
                
                case 'O':
                    // The master position
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(masterPosition()));
                    retval += tmp;
                    break;
                
                case 'p':
                    // The port of the slave.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), slavePort());
                    retval += tmp;
                    break;
                
                case 'P':
                    // The port of the master.
                    partFormat += 'd';
                    tmp.sprintf(STR(partFormat), masterPort());
                    retval += tmp;
                    break;
                
                case 's':
                    // The short version of the slave status.
                    partFormat += 's';

                    part = slaveStatusShort();
                    tmp.sprintf(STR(partFormat), STR(part));

                    if (syntaxHighlight)
                        retval += formatter.hostStateColorBegin(part);

                    retval += tmp;

                    if (syntaxHighlight)
                        retval += formatter.hostStateColorEnd();
                    
                    break;
                
                case 'm':
                    // The slave status message "slave_io_state".
                    partFormat += 's';
                    tmp.sprintf(STR(partFormat), STR(slaveMessage()));
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

        percent      = false;
        escaped      = false;
    }

    return retval;
}

/**
 * \param slave The slave host to be used as filter. Most probably from the
 *   --slave command line option.
 * \returns True if the slave of the replication is the same as the argument or
 *   the argument is a host not specified (a host that has no hostname).
 *
 * We use this to filter replication links by the slave host while printing 
 * the list.
 */
bool
S9sReplication::matchSlave(
        const S9sNode &slave)
{
    bool retval = true;

    if (slave.hostName().empty())
    {
        retval = true;
    } else if (slaveHostName() != slave.hostName())
    {
        retval = false;
    } else if (slave.port() > 0 && slavePort() != slave.port())
    {
        return false;
    }

    return retval;
}

/**
 * \param master The master host to be used as filter. Most probably from the
 *   --master command line option.
 * \returns True if the master of the replication is the same as the argument or
 *   the argument is a host not specified (a host that has no hostname).
 */
bool
S9sReplication::matchMaster(
        const S9sNode &master)
{
    bool retval = true;

    if (master.hostName().empty())
    {
        retval = true;
    } else if (masterHostName() != master.hostName())
    {
        retval = false;
    } else if (master.port() > 0 && masterPort() != master.port())
    {
        return false;
    }

    return retval;
}

/**
 *
 * \code{.js}
 * "replication_slave": {
 *   "exec_master_log_pos": "1575",
 *   "executed_gtid_set": "33296d7e-b815-11e9-933b-00163ec3c3c4:1-6",
 *   "last_io_errno": "0",
 *   "last_io_error": "",
 *   "last_io_error_timestamp": "",
 *   "last_sql_errno": "0",
 *   "last_sql_error": "",
 *   "last_sql_error_timestamp": "",
 *   "linkstatus": 1,
 *   "master_host": "192.168.0.76",
 *   "master_log_file": "binlog.000001",
 *   "master_port": "3306",
 *   "master_server_id": "1001",
 *   "master_uuid": "33296d7e-b815-11e9-933b-00163ec3c3c4",
 *   "read_master_log_pos": "1575",
 *   "relay_master_log_file": "binlog.000001",
 *   "retrieved_gtid_set": "33296d7e-b815-11e9-933b-00163ec3c3c4:1-6",
 *   "seconds_behind_master": "0",
 *   "semisync_status": "ON",
 *   "slave_io_running": "Yes",
 *   "slave_io_state": "Waiting for master to send event",
 *   "slave_sql_running": "Yes",
 *   "slave_sql_state": "Slave has read all relay log; waiting for the slave I/O thread to update it",
 *   "sqldelay": 0,
 *   "status": "Waiting for master to send event<br/>Slave has read all relay log; waiting for the slave I/O thread to update it",
 *   "using_gtid": "ON"
 * }
 * \endcode
 */
S9sVariantMap
S9sReplication::slaveInfo() const
{
    S9sVariantMap retval;
    S9sVariantMap tmp;

    tmp = m_slave.toVariantMap();
    retval = tmp["replication_slave"].toVariantMap();
    return retval;
}

/**
 *
 * \code{.js}
 * "replication_master": {
 *   "binlog_do_db": "",
 *   "binlog_ignore_db": "",
 *   "exec_gtid": "33296d7e-b815-11e9-933b-00163ec3c3c4:1-6",
 *   "file": "binlog.000001",
 *   "position": "1575",
 *   "semisync_status": "OFF"
 * }
 * \endcode
 */
S9sVariantMap
S9sReplication::masterInfo() const
{
    S9sNode       master = node(masterHostName(), masterPort());
    S9sVariantMap retval;
    S9sVariantMap tmp;

    tmp = master.toVariantMap();
    retval = tmp["replication_master"].toVariantMap();
    return retval;
}

S9sNode
S9sReplication::node(
        const S9sString &hostName,
        const int        port) const
{
    S9sVector<S9sNode> nodes = m_cluster.nodes();

    for (uint idx = 0u; idx < nodes.size(); ++idx)
    {
        const S9sNode &node = nodes[idx];

        if (node.hostName() == hostName && node.port() == port)
            return node;
    }

    return S9sNode();
}

