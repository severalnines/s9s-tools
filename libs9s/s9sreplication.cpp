#include "s9sreplication.h"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sReplication::S9sReplication() :
    S9sObject()
{
    m_properties["class_name"] = "S9sReplication";
}

S9sReplication::S9sReplication(
        const S9sCluster &cluster,
        const S9sNode    &slave)
{
    m_cluster = cluster;
    m_slave   = slave;
}

S9sString 
S9sReplication::slaveHostName() const
{
    return m_slave.hostName();
}

S9sString
S9sReplication::masterHostName() const
{
    return m_slave.masterHost();
}

int
S9sReplication::slavePort() const
{
    return m_slave.port();
}

int
S9sReplication::masterPort() const
{
    return m_slave.masterPort();
}

S9sString
S9sReplication::slaveName() const
{
    S9sString retval;

    retval.sprintf("%s:%d", STR(slaveHostName()), slavePort());
    return retval;
}

S9sString
S9sReplication::masterName() const
{
    S9sString retval;

    retval.sprintf("%s:%d", STR(masterHostName()), masterPort());
    return retval;
}

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
