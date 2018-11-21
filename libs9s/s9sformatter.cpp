/* 
 * Copyright (C) 2016 severalnines.com
 */
#include "s9sformatter.h"

#include "S9sOptions"
#include "S9sFormat"
#include "S9sObject"
#include "S9sUser"
#include "S9sCluster"
#include "S9sNode"
#include "S9sServer"
#include "S9sContainer"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

#define BoolToHuman(boolVal) ((boolVal) ? 'y' : 'n')


bool
S9sFormatter::useSyntaxHighLight() const
{
    S9sOptions *options = S9sOptions::instance();
   
    return options->useSyntaxHighlight();
}

const char *
S9sFormatter::headerColorBegin() const
{
    if (useSyntaxHighLight())
        return TERM_BOLD;

    return "";
}

const char *
S9sFormatter::headerColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::userColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_ORANGE;

    return "";
}

const char *
S9sFormatter::userColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::groupColorBegin(
        const S9sString &groupName) const
{
    if (useSyntaxHighLight())
    {
        if (groupName == "0")
            return XTERM_COLOR_RED;
        else
            return XTERM_COLOR_CYAN;
    }

    return "";
}

const char *
S9sFormatter::groupColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::directoryColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_DIR;

    return "";
}

const char *
S9sFormatter::directoryColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::folderColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_FOLDER;

    return "";
}

const char *
S9sFormatter::folderColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::clusterStateColorBegin(
        const S9sString &state)
{
    if (useSyntaxHighLight())
    {
        if (state == "STARTED")
            return XTERM_COLOR_GREEN;
       else if (state == "FAILED" || state == "FAILURE")
            return XTERM_COLOR_RED;
        else
            return XTERM_COLOR_YELLOW;
    }
    
    return "";
}

const char *
S9sFormatter::clusterStateColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::jobStateColorBegin(
        const S9sString &state)
{
    if (useSyntaxHighLight())
    {
        if (state.startsWith("RUNNING"))
        {
            return XTERM_COLOR_GREEN;
        } else if (state == "FINISHED")
        {
            return XTERM_COLOR_GREEN;
        } else if (state == "FAILED")
        {
            return XTERM_COLOR_RED;
        }
    }

    return "";
}

const char *
S9sFormatter::jobStateColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::clusterColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_BLUE;

    return "";
}

const char *
S9sFormatter::clusterColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

S9sString 
S9sFormatter::bytesToHuman(
        ulonglong bytes) const
{
    S9sOptions *options = S9sOptions::instance();
    S9sString   retval;
    S9sVariant  variant = bytes;

    if (!options->humanReadable())
    {
        retval.sprintf("%'llu", variant.toULongLong());
    } else if (variant.toTBytes() > 1.0)
    {
        retval.sprintf("%.1fTB", variant.toTBytes());
    } else if (variant.toGBytes() >= 1.0) 
    {
        retval.sprintf("%.1fGB", variant.toGBytes());
    } else {
        retval.sprintf("%.1fMB", variant.toMBytes());
    }

    return retval;
}

const char *
S9sFormatter::ipColorBegin(
        const S9sString &ip) const
{
    if (useSyntaxHighLight() && ip.looksLikeIpAddress())
        return XTERM_COLOR_IP;
    else if (useSyntaxHighLight() && ip.empty())
        return XTERM_COLOR_IP;

    return "";
}

const char *
S9sFormatter::ipColorEnd(
        const S9sString &ip) const
{
    if (useSyntaxHighLight() && ip.looksLikeIpAddress())
        return TERM_NORMAL;
    else if (useSyntaxHighLight() && ip.empty())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::fileColorBegin(
        const S9sString &fileName) const
{
    if (useSyntaxHighLight())
    {
        if (fileName.endsWith(".gz"))
            return XTERM_COLOR_RED;
        else if (fileName.endsWith(".tar"))
            return XTERM_COLOR_ORANGE;
        else if (fileName.endsWith(".log"))
            return XTERM_COLOR_PURPLE;
        else if (fileName.endsWith(".cnf"))
            return XTERM_COLOR_LIGHT_PURPLE;
        else if (fileName.endsWith(".conf"))
            return XTERM_COLOR_LIGHT_PURPLE;
        else if (fileName.endsWith("/config"))
            return XTERM_COLOR_LIGHT_PURPLE;
        else if (fileName.endsWith(".ini"))
            return XTERM_COLOR_LIGHT_PURPLE;
        else if (fileName.endsWith(".pid"))
            return XTERM_COLOR_LIGHT_RED;
        else
            return XTERM_COLOR_11;
    }

    return "";
}

const char *
S9sFormatter::fileColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}


const char *
S9sFormatter::serverColorBegin() 
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_SERVER;

    return "";
}

const char *
S9sFormatter::serverColorEnd() 
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::hostStateColorBegin(
        const S9sString status) const
{
    if (useSyntaxHighLight())
    {
        if (status == "CmonHostRecovery" || status == "CmonHostShutDown")
        {
            return XTERM_COLOR_YELLOW;
        } else if (status == "CmonHostUnknown" || status == "CmonHostOffLine")
        {
            return XTERM_COLOR_RED;
        } else {
            return XTERM_COLOR_GREEN;
        }
    }

    return "";
}

const char *
S9sFormatter::hostStateColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::containerColorBegin(
        int stateAsChar) const
{
    if (useSyntaxHighLight())
    {
        if (stateAsChar == 't')
            return XTERM_COLOR_RED;
        else if (stateAsChar == 's')
            return XTERM_COLOR_RED;
        else if (stateAsChar == '?')
            return XTERM_COLOR_RED;
        else if (stateAsChar == 'q')
            return XTERM_COLOR_YELLOW;

        return XTERM_COLOR_NODE;
    }

    return "";
}

const char *
S9sFormatter::containerColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::typeColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_GREEN;

    return "";
}

const char *
S9sFormatter::typeColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::greyColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_DARK_GRAY;

    return "";
}

const char *
S9sFormatter::greyColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}


S9sString 
S9sFormatter::mBytesToHuman(
        ulonglong mBytes) const
{
    return bytesToHuman(mBytes * (1024ull * 1024ull));
}

S9sString 
S9sFormatter::kiloBytesToHuman(
        ulonglong kBytes) const
{
    return bytesToHuman(kBytes * 1024ull);
}

S9sString
S9sFormatter::percent(
        const ulonglong total,
        const ulonglong part) const
{
    S9sString retval;
    double    percent;
    
    if (total == 0ull)
        percent = 100.0;
    else
        percent = 100.0 * ((double)part / (double)total);

    retval.sprintf("%.1f%%", percent);
    return retval;
}

void 
S9sFormatter::printObjectStat(
        const S9sObject    &object) const
{
    //S9sOptions *options = S9sOptions::instance();
    //int         terminalWidth = options->terminalWidth();
    const char *greyBegin = greyColorBegin();
    const char *greyEnd   = greyColorEnd();
    
    //
    // "    Name: 192.168.0.128"
    //
    printf("%s    Name:%s ", greyBegin, greyEnd);
    // FIXME: the color should depend on the class
    printf("%s", clusterColorBegin());
    printf("%-32s ", STR(object.name()));
    printf("%s", clusterColorEnd());
    
    printf("\n");
   
    //
    // "CDT path: /ft_ndb_6776"
    //
    printf("%sCDT path:%s ", greyBegin, greyEnd);
    printf("%s", folderColorBegin());
    printf("%-32s ", STR(object.cdtPath()));
    printf("%s", folderColorEnd());
    printf("\n");
    
    //
    // "   Class: CmonNdbHost                          Owner: pipas/testgroup"
    //
    printf("%s   Class:%s ", greyBegin, greyEnd);
    printf("%s%-33s%s ", 
            typeColorBegin(), 
            STR(object.className()), 
            typeColorEnd());
    
    printf("%s   Owner:%s ", greyBegin, greyEnd);
    printf("%s%s%s/%s%s%s ", 
            userColorBegin(), STR(object.ownerName()), userColorEnd(),
            groupColorBegin(object.groupOwnerName()), 
            STR(object.groupOwnerName()), 
            groupColorEnd());
    
    printf("\n");
    
    //
    // "      ID: -                                      ACL: rwxrw----"
    //
    printf("%s      ID:%s ", greyBegin, greyEnd);
    printf("%-38s", STR(object.id("-")));

    printf("%s ACL:%s ", greyBegin, greyEnd);
    printf("%s", STR(object.aclShortString()));

    printf("\n");
}

void 
S9sFormatter::printUserStat(
        const S9sUser &user) const
{
    S9sOptions *options = S9sOptions::instance();
    int         terminalWidth = options->terminalWidth();
    const char *greyBegin = greyColorBegin();
    const char *greyEnd   = greyColorEnd();
    S9sString   title;
    
    //
    // The title that is in inverse. 
    //
    if (!user.fullName().empty())
        title.sprintf("%s", STR(user.fullName()));
    else
        title.sprintf("%s", STR(user.userName()));

    printf("%s", TERM_INVERSE);
    printf("%s", STR(title));
    for (int n = title.length(); n < terminalWidth; ++n)
        printf(" ");
    printf("%s", TERM_NORMAL);
   
    printObjectStat(user);
 
    //
    // "Fullname: László Pere                  Email: laszlo@severalnines.com"
    //
    printf("%sFullname:%s ", greyBegin, greyEnd);
    printf("%-28s ", STR(user.fullName("-")));
    printf("\n");
   
    //
    //
    //
    printf("%s   Email:%s ", greyBegin, greyEnd);
    printf("%s ", STR(user.emailAddress("-")));
    printf("\n");
    
    //
    //
    //
    printf("%sDisabled:%s ", greyBegin, greyEnd);
    printf("%s", user.isDisabled() ? "yes" : "no");
    printf("\n");

    
    //
    // " Suspend: no                   Failed logins: 0"
    //
    printf("%s Suspend:%s ", greyBegin, greyEnd);
    printf("%-19s ", user.isSuspended() ? "yes" : "no");
    
    printf("%s         Failed logins:%s ", greyBegin, greyEnd);
    printf("%d", user.nFailedLogins());
    printf("\n");
    
    //
    // "  Groups: users"
    //
    printf("%s  Groups:%s ", greyBegin, greyEnd);
    printf("%-30s ", STR(user.groupNames(", ")));
    printf("\n");

    //
    // " Created: 2017-10-26T12:08:55.945Z"
    //
    printf("%s Created:%s ", greyBegin, greyEnd);
    printf("%-30s ", STR(user.createdString("-")));
    printf("\n");

    //
    // "   Login: 2017-10-26T12:10:37.762Z"
    //
    printf("%s   Login:%s ", greyBegin, greyEnd);
    printf("%-30s ", STR(user.lastLoginString("-")));
    printf("\n");
    
    //
    // 
    //
    printf("%s Failure:%s ", greyBegin, greyEnd);
    printf("%-24s ", STR(user.failedLoginString("-")));
    
    
    printf("\n\n");
}

/**
 * Prints one host in the "stat" format, the format that print every single
 * detail. Well, a lot of details anyway.
 */
void
S9sFormatter::printNodeStat(
        const S9sCluster &cluster,
        const S9sNode    &node) const
{
    S9sOptions *options = S9sOptions::instance();
    int         terminalWidth = options->terminalWidth();
    const char *greyBegin = greyColorBegin();
    const char *greyEnd   = greyColorEnd();
    S9sString   title;
    S9sString   slavesAsString;
    S9sString   message;
    
    //
    // The title line that is in inverse. 
    //
    if (node.port() > 0)
        title.sprintf(" %s:%d ", STR(node.name()), node.port());
    else
        title.sprintf(" %s ", STR(node.name()));

    printf("%s", TERM_INVERSE/*headerColorBegin()*/);
    printf("%s", STR(title));
    for (int n = title.length(); n < terminalWidth; ++n)
        printf(" ");
    printf("%s", TERM_NORMAL /*headerColorEnd()*/);
    printf("\n");

    printObjectStat(node);
    
    //
    //
    //
    printf("%s      IP:%s ", greyBegin, greyEnd);
    printf("%-27s ", STR(node.ipAddress()));
    //printf("\n");
    
    printf("          %sPort:%s ", greyBegin, greyEnd);
    if (node.hasPort())
    printf("%d ", node.port());
    printf("\n");
    
    // 
    // 
    //
    printf("%s   Alias:%s ", greyBegin, greyEnd);
    printf("%-34s", STR(node.alias("-")));
    //printf("\n");
    
    printf("%s Cluster:%s ", greyBegin, greyEnd);
    printf("%s%s%s (%d) ", 
            clusterColorBegin(), 
            STR(cluster.name()), 
            clusterColorEnd(),
            cluster.clusterId());
    printf("\n");
      
    //
    // "   Class: CmonPostgreSqlHost         Type: postgres"
    //
    printf("%s   Class:%s ", greyBegin, greyEnd);
    printf("%s%-35s%s ", 
            typeColorBegin(), 
            STR(node.className()), 
            typeColorEnd());
    
    printf("%s  Type:%s ", greyBegin, greyEnd);
    printf("%s", STR(node.nodeType()));
    printf("\n");
   
    //
    //
    //
    printf("%s  Status:%s ", greyBegin, greyEnd);
    printf("%-35s", STR(node.hostStatus()));
    //printf("\n");
    
    printf("   %sRole:%s ", greyBegin, greyEnd);
    printf("%s", STR(node.role()));
    printf("\n");
    
    //
    //
    //
    printf("%s      OS:%s ", greyBegin, greyEnd);
    printf("%-35s", STR(node.osVersionString()));

    printf("%s Access:%s ", greyBegin, greyEnd);
    printf("%s", node.readOnly() ? "read-only" : "read-write");

    printf("\n");
    
    //
    printf("%s   VM ID:%s ", greyBegin, greyEnd);
    printf("%s", STR(node.containerId("-")));
    printf("\n");

    //  Version: 1.4.7
    printf("%s Version:%s ", greyBegin, greyEnd);
    printf("%s", STR(node.version()));
    printf("\n");


    // A line for the human readable message.
    message = node.message();
    if (message.empty())
        message = "-";
    printf("%s Message:%s ", greyBegin, greyEnd);
    printf("%s", STR(message));
    printf("\n");
   
    slavesAsString = node.slavesAsString();
    if (!slavesAsString.empty())
    {
        printf("%s  Slaves:%s ", greyBegin, greyEnd);
        printf("%s", STR(slavesAsString));
        printf("\n");
    }

    /*
     * Last seen time and SSH fail count.
     */
    printf("%sLastSeen:%s ", greyBegin, greyEnd);
    printf("%-38s", STR(S9sString::pastTime(node.lastSeen())));
    //printf("\n");
    
    printf("%s SSH:%s ", greyBegin, greyEnd);
    printf("%d ", node.sshFailCount());
    printf("%sfail(s)%s ", greyBegin, greyEnd);

    printf("\n");

    //
    // A line of switches.
    //
    printf("%s Connect:%s %c ", 
            greyBegin, greyEnd, 
            BoolToHuman(node.readOnly()));

    printf("%sMaintenance:%s %c ", 
            greyBegin, greyEnd, 
            BoolToHuman(node.isMaintenanceActive()));
    
    printf("%sManaged:%s %c ", 
            greyBegin, greyEnd, 
            BoolToHuman(node.managed()));
    
    printf("%sRecovery:%s %c ", 
            greyBegin, greyEnd, 
            BoolToHuman(node.nodeAutoRecovery()));

    printf("%sSkip DNS:%s %c ", 
            greyBegin, greyEnd, 
            BoolToHuman(node.skipNameResolve()));
    
    printf("%sSuperReadOnly:%s %c ", 
            greyBegin, greyEnd, 
            BoolToHuman(node.superReadOnly()));

    
    printf("\n");
    
    //
    //
    //
    if (node.pid() > 0)
    {
        printf("%s     Pid:%s %d", 
                greyBegin, greyEnd, 
                node.pid());
    } else {
        printf("%s     PID:%s -", greyBegin, greyEnd);
    }
    
    printf("  %sUptime:%s %s", 
            greyBegin, greyEnd, 
            STR(S9sString::uptime(node.uptime())));
    
    printf("\n");

    //
    // Lines of various files.
    //
    printf("%s  Config:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            fileColorBegin(node.configFile()),
            STR(node.configFile()),
            fileColorEnd());
    printf("\n");
    
    printf("%s LogFile:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            fileColorBegin(node.logFile()),
            STR(node.logFile()),
            fileColorEnd());
    printf("\n");

    printf("%s PidFile:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            fileColorBegin(node.pidFile()),
            STR(node.pidFile()),
            fileColorEnd());
    printf("\n");
    
    printf("%s DataDir:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            XTERM_COLOR_BLUE,
            STR(node.dataDir()),
            TERM_NORMAL);

    printf("\n");

    printBackendServersSubList(node);
}

/**
 * \param server The server to print.
 *
 * Prints one server in stat format, a format we use when the --stat command
 * line option is provided.
 */
void
S9sFormatter::printServerStat(
        const S9sServer &server) const
{
    S9sOptions    *options = S9sOptions::instance();
    int            terminalWidth = options->terminalWidth();
    const char    *greyBegin = greyColorBegin();
    const char    *greyEnd   = greyColorEnd();
    S9sString      title;
    S9sVariantList processorNames = server.processorNames();
    S9sVariantList nicNames       = server.nicNames();
    S9sVariantList bankNames      = server.memoryBankNames();
    S9sVariantList diskNames      = server.diskNames();
    S9sVariantList containers;

    //
    // The title line that is in inverse. 
    //
    if (server.hostName() == server.ipAddress())
    {
        title = server.hostName();
    } else {
        title.sprintf("%s (%s)", 
                STR(server.hostName()), STR(server.ipAddress()));
    }

    printf("%s", TERM_INVERSE);
    printf("%s", STR(title));
    for (int n = title.length(); n < terminalWidth; ++n)
        printf(" ");
    printf("%s", TERM_NORMAL);
    printf("\n");

    printObjectStat(server);

    //
    // "      IP: 192.168.1.4"
    //
    printf("%s      IP:%s ", greyBegin, greyEnd);
    printf("%s%-33s%s ", 
            ipColorBegin(server.ipAddress()),
            STR(server.ipAddress()),
            ipColorEnd());

    printf("%sProtocol:%s ", greyBegin, greyEnd);
    printf("%-25s ", STR(server.protocol()));

    printf("\n");
    
    //
    //
    //
    printf("%s  Status:%s ", greyBegin, greyEnd);
    printf("%s%-24s%s ", 
            hostStateColorBegin(server.hostStatus()),
            STR(server.hostStatus()),
            hostStateColorEnd());

    printf("\n");
    
    //
    // ""
    //
    printf("%s      OS:%s ", greyBegin, greyEnd);
    printf("%-24s", STR(server.osVersionString("-")));
    printf("\n");
    

    // 
    // "   Alias: ''                        Owner: pipas/users" 
    //
    printf("%s   Alias:%s ", greyBegin, greyEnd);
    printf("%-16s ", STR(server.alias("-")));
    //printf("\n");
    
    printf("\n");
    
    //
    // "   Model: SUN FIRE X4170 SERVER (4583256-1)"
    //
    printf("%s   Model:%s ", greyBegin, greyEnd);
    printf("%-16s ", STR(server.model("-")));
    printf("\n");

    //
    //
    //
    printf("%s Summary:%s ", greyBegin, greyEnd);
    printf("%2d VMs", server.nContainers());
    printf(", %.0fGB RAM", server.totalMemoryGBytes());
    printf(", %d CPUs", server.nCpus());
    printf(", %d cores", server.nCores());
    printf(", %d threads", server.nThreads());
    printf("\n");

    //
    //
    //
    printf("%s  Limits:%s ", greyBegin, greyEnd);
    printf("%s/%s VMs", 
            STR(server.nContainersMaxString()),
            STR(server.nRunningContainersMaxString()));
    printf("\n");

    //
    // "  CPU(s): 2 x Intel(R) Xeon(R) CPU L5520 @ 2.27GHz"
    //
    for (uint idx = 0u; idx < processorNames.size(); ++idx)
    {
        if (idx == 0u)
        {
            printf("%s  CPU(s):%s ", greyBegin, greyEnd);
        } else {
            printf("          ");
        }

        printf("%s\n", STR(processorNames[idx].toString()));
    }

    //
    // "  NIC(s): 4 x 82575EB Gigabit Network Connection"
    //
    for (uint idx = 0u; idx < nicNames.size(); ++idx)
    {
        if (idx == 0u)
        {
            printf("%s  NIC(s):%s ", greyBegin, greyEnd);
        } else {
            printf("          ");
        }

        printf("%s\n", STR(nicNames[idx].toString()));
    }
    
    //
    // "   Banks: 16 x DIMM 800 MHz (1.2 ns)"
    //
    for (uint idx = 0u; idx < bankNames.size(); ++idx)
    {
        if (idx == 0u)
        {
            printf("%s   Banks:%s ", greyBegin, greyEnd);
        } else {
            printf("          ");
        }

        printf("%s\n", STR(bankNames[idx].toString()));
    }

    //
    // "   Disks:  4 x FUJITSU MBE2147RC"
    //
    for (uint idx = 0u; idx < diskNames.size(); ++idx)
    {
        if (idx == 0u)
        {
            printf("%s   Disks:%s ", greyBegin, greyEnd);
        } else {
            printf("          ");
        }

        printf("%s\n", STR(diskNames[idx].toString()));
    }
        
    //
    //
    //
    printf("\n");
    containers = server.containers();
    printContainersCompact(containers);

    printf("\n");
}


void
S9sFormatter::printBackendServersSubList(
        const S9sNode &node) const
{
    if (node.hasBackendServers())
    {
        S9sOptions    *options = S9sOptions::instance();
        int            terminalWidth = options->terminalWidth();
        S9sFormat      hostNameFormat(ipColorBegin(), ipColorEnd());
        S9sFormat      portFormat;
        S9sFormat      statusFormat;
        S9sFormat      commentFormat;
        int            tableWidth;
        S9sString      indent;

        /*
         *
         */
        hostNameFormat.widen("NAME");
        portFormat.widen("PORT");
        statusFormat.widen("STATUS");
        commentFormat.widen("COMMENT");
        
        for (uint idx = 0u; idx < node.numberOfBackendServers(); ++idx)
        {
            S9sString hostName = node.backendServerName(idx);
            int       port     = node.backendServerPort(idx);
            S9sString status   = node.backendServerStatus(idx);
            S9sString comment  = node.backendServerComment(idx);

            hostNameFormat.widen(hostName);
            portFormat.widen(port);
            statusFormat.widen(status);
            commentFormat.widen(comment);
        }

        tableWidth = 3 +
            hostNameFormat.realWidth() + portFormat.realWidth() +
            statusFormat.realWidth()   + commentFormat.realWidth();
    
        if (terminalWidth - tableWidth > 0)
            indent = S9sString(" ") * ((terminalWidth - tableWidth) / 2);

        printf("\n");
        
        printf("%s", headerColorBegin());
        printf("%s", STR(indent));
        hostNameFormat.printf("NAME", false);
        portFormat.printf("PORT", false);
        statusFormat.printf("STATUS", false);
        commentFormat.printf("COMMENT", false);
        printf("%s", headerColorEnd());
        printf("\n");

        for (uint idx = 0u; idx < node.numberOfBackendServers(); ++idx)
        {
            S9sString hostName = node.backendServerName(idx);
            int       port     = node.backendServerPort(idx);
            S9sString status   = node.backendServerStatus(idx);
            S9sString comment  = node.backendServerComment(idx);

            printf("%s", STR(indent));
            hostNameFormat.printf(hostName);
            portFormat.printf(port);
            statusFormat.printf(status);
            commentFormat.printf(comment);

            printf("\n");
        }
            
        printf("\n");
    }
}

void
S9sFormatter::printContainersCompact(
        const S9sVariantList &containers) const
{
    S9sOptions    *options = S9sOptions::instance();
    S9s::AddressType addressType = options->addressType();
    int            terminalWidth = options->terminalWidth();
    S9sFormat      aliasFormat(containerColorBegin(), containerColorEnd());
    S9sFormat      ipFormat(ipColorBegin(), ipColorEnd());
    S9sFormat      userFormat(userColorBegin(), userColorEnd());
    S9sFormat      groupFormat(groupColorBegin(), groupColorEnd());
    S9sString      indent;
    int            tableWidth;

    if (containers.empty())
        return;

    for (uint idx = 0u; idx < containers.size(); ++idx)
    {
        const S9sContainer container = containers[idx].toContainer();
        S9sString          user      = container.ownerName();
        S9sString          group     = container.groupOwnerName();
        S9sString          alias     = container.alias();
        S9sString          ipAddress = container.ipAddress(addressType, "-");

        userFormat.widen(user);
        groupFormat.widen(group);
        aliasFormat.widen(alias);
        ipFormat.widen(ipAddress);
    }
    
    tableWidth = 
        3 +
        aliasFormat.realWidth()  + ipFormat.realWidth() +
        userFormat.realWidth()   + groupFormat.realWidth();
    
    if (terminalWidth - tableWidth > 0)
        indent = S9sString(" ") * ((terminalWidth - tableWidth) / 2);

    printf("%s", headerColorBegin());
    printf("%s", STR(indent));
    printf("S ");
    userFormat.printf("OWNER", false);
    groupFormat.printf("GROUP", false);
    ipFormat.printf("IP ADDRESS", false);
    aliasFormat.printf("NAME", false);
    printf("%s", headerColorEnd());
    printf("\n");


    for (uint idx = 0u; idx < containers.size(); ++idx)
    {
        const S9sContainer container = containers[idx].toContainer();
        S9sString          user      = container.ownerName();
        S9sString          group     = container.groupOwnerName();
        S9sString          alias     = container.alias();
        S9sString          ipAddress = container.ipAddress(addressType, "-");
        
        printf("%s", STR(indent));
        printf("%c ", container.stateAsChar());

        userFormat.printf(user);
        groupFormat.printf(group);
        ipFormat.printf(ipAddress);
        aliasFormat.printf(alias);
        printf("\n");
    }
}

