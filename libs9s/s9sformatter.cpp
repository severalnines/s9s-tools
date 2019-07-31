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
        const S9sString &state) const
{
    if (useSyntaxHighLight())
    {
        if (state == "STARTED" || state == "RUNNING")
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
        ulonglong bytes)
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
S9sFormatter::serverColorBegin(
        int stateAsChar) const
{
    if (useSyntaxHighLight())
    {
        switch (stateAsChar)
        {
            case '?':
            case 'r':
                return XTERM_COLOR_YELLOW;

            case 'l':
            case 'f':
            case '-':
                return XTERM_COLOR_RED;

            case 'o':
            default:
                return XTERM_COLOR_SERVER;
        }
    }

    return "";
}

const char *
S9sFormatter::serverColorEnd() const
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

const char *
S9sFormatter::objectColorBegin(
        const S9sObject    &object) const
{
    if (useSyntaxHighLight())
    {
        S9sString className = object.className();

        if (className == "CmonContainer")
        {
            return containerColorBegin(object.stateAsChar());
        } else if (className == "CmonLxcServer" || 
                className == "CmonCloudServer")
        {
            return serverColorBegin(object.stateAsChar());
        } else if (className == "CmonUser")
        {
            return userColorBegin();
        }
    } else {
        return "";
    }

    return "";
}

const char *
S9sFormatter::objectColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}


S9sString 
S9sFormatter::mBytesToHuman(
        ulonglong mBytes)
{
    return bytesToHuman(mBytes * (1024ull * 1024ull));
}

S9sString 
S9sFormatter::kiloBytesToHuman(
        ulonglong kBytes)
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
    printf("%s", objectColorBegin(object));
    printf("%-32s ", STR(object.name()));
    printf("%s", objectColorEnd());
    
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
    printf("\n");
   
    printObjectStat(user);
 
    //
    // "Fullname: László Pere                  Email: laszlo@severalnines.com"
    //
    printf("%sFullname:%s ", greyBegin, greyEnd);
    printf("%-28s ", STR(user.fullName("-")));
    printf("\n");
   
    //
    // "   Email: -"
    //
    printf("%s   Email:%s ", greyBegin, greyEnd);
    printf("%s ", STR(user.emailAddress("-")));
    printf("\n");
    
    //
    //
    //
    printf("%s   DName:%s ", greyBegin, greyEnd);
    printf("%s ", STR(user.distinguishedName("-")));
    printf("\n");
    
    //
    //
    //
    printf("%s  Origin:%s ", greyBegin, greyEnd);
    printf("%s ", STR(user.origin("-")));
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
    
    
    printf("\n");
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
    ::printf("%s      IP:%s ", greyBegin, greyEnd);
    ::printf("%s", ipColorBegin(node.ipAddress()));
    ::printf("%-27s ", STR(node.ipAddress()));
    ::printf("%s", ipColorEnd());
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
     
    #if 0
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
    #endif

    //
    //
    //
    ::printf("%s  Status:%s ", greyBegin, greyEnd);
    ::printf("%s", hostStateColorBegin(node.hostStatus()));
    ::printf("%-35s", STR(node.hostStatus()));
    ::printf("%s", hostStateColorEnd());
    
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

/**
 * \param controller The controller to print.
 *
 * Prints one controller in stat format, a format we use when the --stat command
 * line option is provided.
 */
void
S9sFormatter::printControllerStat(
        const S9sServer &controller) const
{
    S9sOptions    *options = S9sOptions::instance();
    int            terminalWidth = options->terminalWidth();
    const char    *greyBegin = greyColorBegin();
    const char    *greyEnd   = greyColorEnd();
    S9sString      title;

    //
    // The title line that is in inverse. 
    //
    if (controller.hostName() == controller.ipAddress())
    {
        title = controller.hostName();
    } else {
        title.sprintf("%s (%s)", 
                STR(controller.hostName()), STR(controller.ipAddress()));
    }

    printf("%s", TERM_INVERSE);
    printf("%s", STR(title));
    for (int n = title.length(); n < terminalWidth; ++n)
        printf(" ");
    printf("%s", TERM_NORMAL);
    printf("\n");

    printObjectStat(controller);

    //
    // "      IP: 192.168.1.4"
    //
    printf("%s      IP:%s ", greyBegin, greyEnd);
    printf("%s%-33s%s ", 
            ipColorBegin(controller.ipAddress()),
            STR(controller.ipAddress()),
            ipColorEnd());

    printf("%s    Port:%s ", greyBegin, greyEnd);
    printf("%d ", controller.port());

    printf("\n");
    
    //
    // "  Status: CmonHostOnline                        Role: follower"
    //
    printf("%s  Status:%s ", greyBegin, greyEnd);
    printf("%s%-35s%s ", 
            hostStateColorBegin(controller.hostStatus()),
            STR(controller.hostStatus()),
            hostStateColorEnd());

    printf("  %sRole:%s ", greyBegin, greyEnd);
    printf("%s", STR(controller.role()));
    printf("\n");
    
    //
    // ""
    //
    //printf("%s      OS:%s ", greyBegin, greyEnd);
    //printf("%-24s", STR(controller.osVersionString("-")));
    //printf("\n");
    
    //
    // "     PID: 51836 "
    //
    printf("%s     PID:%s ", greyBegin, greyEnd);
    printf("%-6d", controller.pid());
    printf("\n");

    // 
    // "   Alias: ''                        Owner: pipas/users" 
    //
    printf("%s   Alias:%s ", greyBegin, greyEnd);
    printf("%-16s ", STR(controller.alias("-")));
    printf("\n");
    
    //
    // "  Config: 'configs/FtFull.conf'"
    //
    printf("%s  Config:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            fileColorBegin(controller.configFile()),
            STR(controller.configFile()),
            fileColorEnd());

    printf("\n");
    
    //
    // " DataDir: '/tmp/cmon/controller3/var/lib/cmon'"
    //
    printf("%s DataDir:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            XTERM_COLOR_BLUE,
            STR(controller.dataDir()),
            TERM_NORMAL);
    printf("\n");
    
    //
    // " LogFile: './cmon-ft-install.log'"
    //
    printf("%s LogFile:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            fileColorBegin(controller.logFile()),
            STR(controller.logFile()),
            fileColorEnd());
    printf("\n");

    
    printf("\n");
}

/**
 * Prints one cluster in "stat" format, a very detailed format that is
 * used when the --stat command line option is provided.
 */
void
S9sFormatter::printClusterStat(
        const S9sCluster &cluster) const
{
    S9sOptions *options = S9sOptions::instance();
    int         terminalWidth = options->terminalWidth();
    const char *greyBegin = greyColorBegin();
    const char *greyEnd   = greyColorEnd();
    S9sString   title;

    //
    // The title that is in inverse. 
    //
    title.sprintf(" %s ", STR(cluster.name()));

    ::printf("%s", TERM_INVERSE);
    ::printf("%s", STR(title));
    
    for (int n = title.length(); n < terminalWidth; ++n)
        ::printf(" ");

    ::printf("\n");
    ::printf("%s", TERM_NORMAL);
   
    printObjectStat(cluster);

    //
    // 
    //
    printf("%s  Status:%s ", greyBegin, greyEnd);
    printf("%s%s%s ", 
            clusterStateColorBegin(cluster.state()), 
            STR(cluster.state()),
            clusterStateColorEnd());
    printf("\n");
    
    printf("%s    Type:%s ", greyBegin, greyEnd);
    printf("%-32s ", STR(cluster.clusterType()));
    
    printf("%s   Vendor:%s ", greyBegin, greyEnd);
    printf("%s", STR(cluster.vendorAndVersion()));
    printf("\n");
    
    printf("%s  Status:%s ", greyBegin, greyEnd);
    printf("%s", STR(cluster.statusText()));
    printf("\n");

    //
    // Counting the alarms.
    //
    printf("%s  Alarms:%s ", greyBegin, greyEnd);
    printf("%2d ", cluster.alarmsCritical());
    printf("%scrit %s ", greyBegin, greyEnd);
    printf("%2d ", cluster.alarmsWarning());
    printf("%swarn %s ", greyBegin, greyEnd);
    printf("\n");

    //
    // Counting the jobs.
    //
    printf("%s    Jobs:%s ", greyBegin, greyEnd);
    printf("%2d ", cluster.jobsAborted());
    printf("%sabort%s ", greyBegin, greyEnd);
    printf("%2d ", cluster.jobsDefined());
    printf("%sdefnd%s ", greyBegin, greyEnd);
    printf("%2d ", cluster.jobsDequeued());
    printf("%sdequd%s ", greyBegin, greyEnd);
    printf("%2d ", cluster.jobsFailed());
    printf("%sfaild%s ", greyBegin, greyEnd);
    printf("%2d ", cluster.jobsFinished());
    printf("%sfinsd%s ", greyBegin, greyEnd);
    printf("%2d ", cluster.jobsRunning());
    printf("%srunng%s ", greyBegin, greyEnd);
    printf("\n");
    
    //
    // Lines of various files.
    //
    printf("%s  Config:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            fileColorBegin(cluster.configFile()),
            STR(cluster.configFile()),
            fileColorEnd());

    printf("\n");

    printf("%s LogFile:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            fileColorBegin(cluster.logFile()),
            STR(cluster.logFile()),
            fileColorEnd());
    printf("\n");
    printf("\n");

    printHostTable(cluster);
    printReplicationTable(cluster);
}

void 
S9sFormatter::printContainerStat(
        const S9sContainer &container) const
{
    S9sOptions *options = S9sOptions::instance();
    S9s::AddressType addressType = options->addressType();
    int         terminalWidth = options->terminalWidth();
    const char *greyBegin = greyColorBegin();
    const char *greyEnd   = greyColorEnd();
    S9sString   title;
    S9sString   tmpString;
    S9sString   tmp;

    //
    // The title that is in inverse. 
    //
    if (container.ipAddress(addressType).empty())
    {
        title.sprintf(" %s ", STR(container.alias()));
    } else {
        title.sprintf(" %s (%s)", 
                STR(container.alias()), 
                STR(container.ipAddress(addressType)));
    }

    printf("%s", TERM_INVERSE);
    printf("%s", STR(title));
    for (int n = title.length(); n < terminalWidth; ++n)
        printf(" ");
    printf("%s", TERM_NORMAL);
    
    printObjectStat(container);


    //
    // "    IPv4: 54.93.99.244                          Type: cmon-cloud"
    //
    printf("%s    IPv4:%s ", greyBegin, greyEnd);
    printf("%-37s", STR(container.ipv4Addresses()));

    printf("%s Type:%s ", greyBegin, greyEnd);
    printf("%s", STR(container.type()));

    printf("\n");
    
    //
    //
    //
    tmp = container.ipAddress(S9s::PublicIpv4Address, "-");
    printf("%sPublicIp:%s ", greyBegin, greyEnd);
    printf("%s%-33s%s", ipColorBegin(tmp), STR(tmp), ipColorEnd(tmp));

    tmp = container.ipAddress(S9s::PrivateIpv4Address, "-");
    printf("%sPrivateIp:%s ", greyBegin, greyEnd);
    printf("%s%s%s", ipColorBegin(tmp), STR(tmp), ipColorEnd(tmp));

    printf("\n");
    
    //
    // "  Server: core1                                State: RUNNING"
    //
    printf("%s  Server:%s ", greyBegin, greyEnd);
    printf("%s", serverColorBegin());
    printf("%-33s ", STR(container.parentServerName()));
    printf("%s", serverColorEnd());
    
    printf("%s   State:%s ", greyBegin, greyEnd);
    printf("%s%s%s ", 
            clusterStateColorBegin(container.state()), 
            STR(container.state()),
            clusterStateColorEnd());

    printf("\n");

     
    //
    // "   Cloud: az                                  Region: Southeast Asia"
    //
    printf("%s   Cloud:%s ", greyBegin, greyEnd);
    printf("%-34s", STR(container.provider("-")));

    printf("%s  Region:%s ", greyBegin, greyEnd);
    printf("%s", STR(container.region("-")));

    printf("\n");

    //
    // "  Subnet: subnet-6a1d1c12                       CIDR: 172.31.0.0/20"
    //
    printf("%s  Subnet:%s ", greyBegin, greyEnd);
    printf("%-34s", STR(container.subnetId("-")));
    
    printf("%s    CIDR:%s ", greyBegin, greyEnd);
    printf("%s", STR(container.subnetCidr("-")));

    printf("\n");
    
    //
    //
    //
    printf("%s  VPC ID:%s ", greyBegin, greyEnd);
    printf("%-34s", STR(container.subnetVpcId("-")));

    printf("\n");
    
    //
    //
    //
    printf("%sFirewall:%s ", greyBegin, greyEnd);
    printf("%-34s", STR(container.firewalls("-")));

    printf("\n");

    //
    //
    //
    printf("%sTemplate:%s ", greyBegin, greyEnd);
    printf("%-36s", STR(container.templateName("-")));

    printf("%s Image:%s ", greyBegin, greyEnd);
    printf("%s", STR(container.image("-")));

    printf("\n");

    //
    // "      OS: ubuntu 16.04 xenial                  Arch: x86_64"
    //
    printf("%s      OS:%s ", greyBegin, greyEnd);
    printf("%-36s", STR(container.osVersionString()));
    
    printf("%s  Arch:%s ", greyBegin, greyEnd);
    printf("%s ", STR(container.architecture()));

    printf("\n");
    
    //
    //
    //
    printf("%s   Start:%s ", greyBegin, greyEnd);
    printf("%s",  container.autoStart() ? "y" : "n");
    printf("\n");
    
    //
    //
    //
    printf("%s  Limits:%s ", greyBegin, greyEnd);

    tmpString = "";
    if (container.memoryLimitGBytes() > 0)
        tmpString.aprintf("%.0fGB RAM",  container.memoryLimitGBytes());

    for (uint idx = 0u; idx < container.nVolumes(); ++idx)
    {
        if (!tmpString.empty())
            printf(", ");
        
        ::printf("%dGB %s", 
                container.volumeGigaBytes(idx), 
                STR(container.volumeType(idx).toUpper()));
    }

    ::printf("%s", STR(tmpString));
    ::printf("\n");


    //
    // "  Config: '/var/lib/lxc/www/config'"
    //
    printf("%s  Config:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            fileColorBegin(container.configFile()),
            STR(container.configFile()),
            fileColorEnd());
    
    printf("\n");
    
    //
    //
    //
    printf("%s Root FS:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            XTERM_COLOR_BLUE,
            STR(container.rootFsPath()),
            TERM_NORMAL);

    printf("\n");
    printf("\n");
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
    S9sFormat      providerFormat;
    S9sFormat      aliasFormat;
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
        S9sString          provider  = container.provider("-");
        S9sString          user      = container.ownerName();
        S9sString          group     = container.groupOwnerName();
        S9sString          alias     = container.alias();
        S9sString          ipAddress = container.ipAddress(addressType, "-");

        providerFormat.widen(provider);
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
    
    providerFormat.widen("CLOUD");
    userFormat.widen("OWNER");
    groupFormat.widen("GROUP");
    ipFormat.widen("IP ADDRESS");
    aliasFormat.widen("NAME");

    printf("%s", headerColorBegin());
    printf("%s", STR(indent));
    printf("S ");
    providerFormat.printf("CLOUD", false);
    userFormat.printf("OWNER", false);
    groupFormat.printf("GROUP", false);
    ipFormat.printf("IP ADDRESS", false);
    aliasFormat.printf("NAME", false);
    printf("%s", headerColorEnd());
    printf("\n");


    for (uint idx = 0u; idx < containers.size(); ++idx)
    {
        const S9sContainer container = containers[idx].toContainer();
        S9sString          provider  = container.provider("-");
        S9sString          user      = container.ownerName();
        S9sString          group     = container.groupOwnerName();
        S9sString          alias     = container.alias();
        S9sString          ipAddress = container.ipAddress(addressType, "-");
        
        printf("%s", STR(indent));
        printf("%c ", container.stateAsChar());

        providerFormat.printf(provider);
        userFormat.printf(user);
        groupFormat.printf(group);
        ipFormat.printf(ipAddress);

        ::printf("%s", containerColorBegin(container.stateAsChar()));
        aliasFormat.printf(alias);
        ::printf("%s", containerColorEnd());

        printf("\n");
    }
}

void 
S9sFormatter::printReplicationTable(
        const S9sCluster &cluster) const
{
    S9sOptions        *options = S9sOptions::instance();
    int                terminalWidth = options->terminalWidth();
    int                tableWidth;
    S9sString          indent;
    S9sVector<S9sNode> nodes = cluster.nodes();
    int                nLines = 0;
    S9sFormat          slaveNameFormat;
    S9sFormat          masterNameFormat;
    S9sFormat          masterClusterFormat;
    S9sFormat          linkStatusFormat;

    for (uint idx = 0u; idx < nodes.size(); ++idx)
    {
        const S9sNode &node           = nodes[idx];
        S9sString      role           = node.role();
        S9sString      hostName       = node.hostName();
        int            port           = node.port();
        S9sString      masterHostname = node.masterHost();
        int            masterPort     = node.masterPort();
        S9sString      masterCluster  = "?";
        S9sString      linkStatus     = "???";
        S9sString      masterName;
        S9sString      slaveName;


        if (role == "controller")
            continue;
        
        if (role == "master")
            continue;

        if (masterHostname.empty())
            continue;

        masterName.sprintf("%s:%d", STR(masterHostname), masterPort);
        slaveName.sprintf("%s:%d", STR(hostName), port);

        //::printf("%12s ", STR(role));
        slaveNameFormat.widen(slaveName);
        masterNameFormat.widen(masterName);
        masterClusterFormat.widen(masterCluster);
        linkStatusFormat.widen(linkStatus);
        
        ++nLines;
    }

    if (nLines == 0)
        return;

    slaveNameFormat.widen("SLAVE");
    masterNameFormat.widen("MASTER");
    masterClusterFormat.widen("MASTER_CLUSTER");
    linkStatusFormat.widen("STATUS");

    tableWidth = 
        slaveNameFormat.realWidth() + masterNameFormat.realWidth() +
        masterClusterFormat.realWidth() + linkStatusFormat.realWidth();

    if (terminalWidth - tableWidth > 0)
        indent = S9sString(" ") * ((terminalWidth - tableWidth) / 2);
   
    /*
     * Printing the header.
     */
    printf("%s", headerColorBegin());
    printf("%s", STR(indent));
        
    slaveNameFormat.printf("SLAVE");
    masterNameFormat.printf("MASTER");
    masterClusterFormat.printf("MASTER_CLUSTER");
    linkStatusFormat.printf("STATUS");

    printf("%s", headerColorEnd());
    printf("\n");

    /*
     *
     */
    for (uint idx = 0u; idx < nodes.size(); ++idx)
    {
        const S9sNode &node           = nodes[idx];
        S9sString      role           = node.role();
        S9sString      hostName       = node.hostName();
        int            port           = node.port();
        S9sString      masterHostname = node.masterHost();
        int            masterPort     = node.masterPort();
        S9sString      masterCluster  = "?";
        S9sString      linkStatus     = "???";
        S9sString      masterName;
        S9sString      slaveName;


        if (role == "controller")
            continue;
        
        if (role == "master")
            continue;

        if (masterHostname.empty())
            continue;

        masterName.sprintf("%s:%d", STR(masterHostname), masterPort);
        slaveName.sprintf("%s:%d", STR(hostName), port);

        ::printf("%s", STR(indent));
        slaveNameFormat.printf(slaveName);
        masterNameFormat.printf(masterName);
        masterClusterFormat.printf(masterCluster);
        linkStatusFormat.printf(linkStatus);
        ::printf("\n");
    }

    ::printf("\n");
}

void 
S9sFormatter::printHostTable(
        const S9sCluster &cluster) const
{
    S9sOptions    *options = S9sOptions::instance();
    int            terminalWidth = options->terminalWidth();
    S9sVariantList hostIds = cluster.hostIds();
    S9sFormat      hostNameFormat;
    S9sFormat      coresFormat;
    S9sFormat      memTotalFormat;
    S9sFormat      memUsedFormat;
    S9sFormat      cpuUsageFormat;
    S9sFormat      totalDiskFormat;
    S9sFormat      freeDiskFormat;
    S9sFormat      labelFormat;
    S9sFormat      swapTotalFormat;
    S9sFormat      swapFreeFormat;
    S9sFormat      rxSpeedFormat;
    S9sFormat      txSpeedFormat;
    int            tableWidth;
    S9sString      indent;

    memUsedFormat.setRightJustify();
    cpuUsageFormat.setRightJustify();
    rxSpeedFormat.setRightJustify();
    txSpeedFormat.setRightJustify();

    for (uint idx = 0u; idx < hostIds.size(); ++idx)
    {
        int        hostId    = hostIds[idx].toInt();
        S9sString  hostName  = cluster.hostName(hostId);
        S9sVariant nCores    = cluster.nCpuCores(hostId);
        S9sVariant memTotal  = cluster.memTotal(hostId);
        S9sVariant memUsed   = cluster.memUsed(hostId);
        S9sVariant cpuUsage  = cluster.cpuUsagePercent(hostId);
        S9sVariant totalDisk = cluster.totalDiskBytes(hostId);
        S9sVariant freeDisk  = cluster.freeDiskBytes(hostId);
        S9sVariant swapTotal = cluster.swapTotal(hostId);
        S9sVariant swapFree  = cluster.swapFree(hostId);
        S9sVariant rxSpeed   = cluster.rxBytesPerSecond(hostId);
        S9sVariant txSpeed   = cluster.txBytesPerSecond(hostId);
        

        hostNameFormat.widen(hostName);

        coresFormat.widen(nCores.toInt());
        cpuUsageFormat.widen(cpuUsage.toString(S9sVariant::IntegerNumber)+"%");

        memTotalFormat.widen(memTotal.toString(S9sVariant::BytesShort));
        memUsedFormat.widen(memUsed.toString(S9sVariant::BytesShort));

        totalDiskFormat.widen(totalDisk.toString(S9sVariant::BytesShort));
        freeDiskFormat.widen(freeDisk.toString(S9sVariant::BytesShort));

        swapTotalFormat.widen(swapTotal.toString(S9sVariant::BytesShort));
        swapFreeFormat.widen(swapFree.toString(S9sVariant::BytesShort));

        rxSpeedFormat.widen(rxSpeed.toString(S9sVariant::BytesPerSecShort));
        txSpeedFormat.widen(txSpeed.toString(S9sVariant::BytesPerSecShort));
    }
    
    tableWidth = 
        hostNameFormat.realWidth() + coresFormat.realWidth() +
        cpuUsageFormat.realWidth() + 
        memTotalFormat.realWidth() + memUsedFormat.realWidth() +
        swapTotalFormat.realWidth() + 
        totalDiskFormat.realWidth() + swapFreeFormat.realWidth() +
        freeDiskFormat.realWidth() + labelFormat.realWidth() +
        rxSpeedFormat.realWidth()  + txSpeedFormat.realWidth();

    if (terminalWidth - tableWidth > 0)
        indent = S9sString(" ") * ((terminalWidth - tableWidth) / 2);

    hostNameFormat.widen("HOSTNAME");

    printf("%s", headerColorBegin());
    printf("%s", STR(indent));

    hostNameFormat.printf("HOSTNAME");
    
    labelFormat = coresFormat + cpuUsageFormat;
    labelFormat.setCenterJustify();
    labelFormat.printf("CPU");

    labelFormat = memTotalFormat + memUsedFormat;
    labelFormat.setCenterJustify();
    labelFormat.printf("MEMORY");
    
    labelFormat = swapTotalFormat + swapFreeFormat;
    labelFormat.setCenterJustify();
    labelFormat.printf("SWAP"); 

    labelFormat = totalDiskFormat + freeDiskFormat;
    labelFormat.setCenterJustify();
    labelFormat.printf("DISK"); 
    
    labelFormat = rxSpeedFormat + txSpeedFormat;
    labelFormat.setCenterJustify();
    labelFormat.printf("NICs"); 

    printf("%s", headerColorEnd());
    printf("\n");

        
    for (uint idx = 0u; idx < hostIds.size(); ++idx)
    {
        int        hostId    = hostIds[idx].toInt();
        S9sString  hostName  = cluster.hostName(hostId);
        S9sVariant nCores    = cluster.nCpuCores(hostId);
        S9sVariant memTotal  = cluster.memTotal(hostId);
        S9sVariant memUsed   = cluster.memUsed(hostId);
        S9sVariant cpuUsage  = cluster.cpuUsagePercent(hostId);
        S9sVariant totalDisk = cluster.totalDiskBytes(hostId);
        S9sVariant freeDisk  = cluster.freeDiskBytes(hostId);
        S9sVariant swapTotal = cluster.swapTotal(hostId);
        S9sVariant swapFree  = cluster.swapFree(hostId);
        S9sVariant rxSpeed   = cluster.rxBytesPerSecond(hostId);
        S9sVariant txSpeed   = cluster.txBytesPerSecond(hostId);

        printf("%s", STR(indent));

        hostNameFormat.printf(hostName);
        coresFormat.printf(nCores.toInt());
        cpuUsageFormat.printf(
                cpuUsage.toString(S9sVariant::IntegerNumber) + "%");

        memTotalFormat.printf(memTotal.toString(S9sVariant::BytesShort));
        memUsedFormat.printf(memUsed.toString(S9sVariant::BytesShort));
        
        swapTotalFormat.printf(swapTotal.toString(S9sVariant::BytesShort));
        swapFreeFormat.printf(swapFree.toString(S9sVariant::BytesShort));
        
        totalDiskFormat.printf(totalDisk.toString(S9sVariant::BytesShort));
        freeDiskFormat.printf(freeDisk.toString(S9sVariant::BytesShort));
        
        rxSpeedFormat.printf(rxSpeed.toString(S9sVariant::BytesPerSecShort));
        txSpeedFormat.printf(txSpeed.toString(S9sVariant::BytesPerSecShort));
        printf("\n");
    }

    printf("\n");
}

