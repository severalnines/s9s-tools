#! /usr/bin/cmonexec -- 

enum ViewFormat
{
    NormalView,
    LongView,
    TimeView,
    OsView
};

var scriptName;
var viewFormat  = NormalView;
var omitHeaders = false;

//
//
//
function statusToString(number)
{
    switch (number)
    {
        case 0:
            return "starting";

        case 10:
            return "connected";
    }

    return "???" + number;
}

function printHostLong(host)
{
    var hostId    = host["hostId"];
    var hostName  = host["hostname"];
    var ip        = host["ip"];
    var nodeType  = host["nodetype"];
    var role      = host["role"];
    var stat      = host["status"];
    var connected = host["connected"];
    var sshfail   = host["sshfailcount"];
    var read      = host["readonly"];
    var galera    = host["isGalera"];
    var port      = host["port"];
    var myhostname;

    if (port.toInt() > 0)
        myhostname = hostName + ":" + port;
    else 
        myhostname = hostName;

    print(
            hostId.toString().rightAlign(3), " ",
            statusToString(stat).rightAlign(10), " ",
            connected ? 'C' : '-',
            read ? 'R' : 'W',
            sshfail > 0 ? '!' : 'S', 
            galera ? 'G' : '-', " ",

            nodeType.leftAlign(12), " ",
            role.leftAlign(10), " ",
            ip.leftAlign(12), " ",
            myhostname);
}

function printHostNormal(host)
{
    var hostName  = host["hostname"];

    print(hostName);
}

function printHostTime(host)
{
    var hostId    = host["hostId"];
    var hostName  = host["hostname"];
    var nodeType  = host["nodetype"];
    var role      = host["role"];
    var stat      = host["status"];
    var connected = host["connected"];
    var sshfail   = host["sshfailcount"];
    var read      = host["readonly"];
    var galera    = host["isGalera"];
    var wallclock = host["wallclock"];
    var timeStamp = host["wallclocktimestamp"];
    var age       = (CmonDateTime::currentDateTime() - timeStamp).toInt();
    var current   = new CmonDateTime(wallclock + age);

    print(
            hostId.toString().rightAlign(3), " ",
            statusToString(stat).rightAlign(10), " ",
            connected ? 'C' : '-',
            read ? 'R' : 'W',
            sshfail > 0 ? '!' : 'S', 
            galera ? 'G' : '-', " ",

            nodeType.leftAlign(12), " ",
            role.leftAlign(10), " ",
            current.toString(), " ",
            hostName);
}

function printHostOs(host)
{
    var hostId    = host["hostId"];
    var hostName  = host["hostname"];
    var ip        = host["ip"];
    var nodeType  = host["nodetype"];
    var role      = host["role"];
    var stat      = host["status"];
    var connected = host["connected"];
    var sshfail   = host["sshfailcount"];
    var read      = host["readonly"];
    var galera    = host["isGalera"];
    var port      = host["port"];
    var os        = host["distribution"];
    var name      = os["name"];
    var codename  = os["codename"];
    var release   = os["release"];
    var osInfo;

    //print(host);
    //print("*** codename : ", codename);
    //print("*** release  : ", release);
    if (!name.empty() && !release.empty() && !codename.empty())
        osInfo = name + " " + release + " (" + codename + ")";
    else if (!name.empty() && !release.empty())
        osInfo = name + " " + release;
    else if (!name.empty())
        osInfo = name;
    else 
        osInfo = "";

    print(
            hostId.toString().rightAlign(3), " ",
            statusToString(stat).rightAlign(10), " ",
            connected ? 'C' : '-',
            read ? 'R' : 'W',
            sshfail > 0 ? '!' : 'S', 
            galera ? 'G' : '-', " ",

            nodeType.leftAlign(12), " ",
            role.leftAlign(10), " ",
            hostName, " ",
            osInfo);
}

function printHelpAndExit()
{
    print("Usage:");
    print("  ", scriptName, " [OPTIONS]...");
    print("Where options are:");
    print("  --help      Print this help and exit.");
    print("  -l, --long  Print detailed list.");
    print("  --noheader  Do not print header lines.");
    print("  --time      Print the wallclack time of the hosts.");
    print("  --os        Print the operating system information of the hosts.");
    print("");
    exit(0);
}

function processArguments(argList)
{
    var arg0 = new CmonFile(argList[0]);

    scriptName = arg0.baseName();

    for (idx = 0; idx < argList.size(); ++idx)
    {
        if (argList[idx] == "--long" || argList[idx] == "-l")
            viewFormat = LongView;
        else if (argList[idx] == "--time")
            viewFormat = TimeView;
        else if (argList[idx] == "--os")
            viewFormat = OsView;
        else if (argList[idx] == "--help")
            printHelpAndExit();
        else if (argList[idx] == "--noheader")
            omitHeaders = true;
    }
}

function printHeader()
{
    if (omitHeaders)
        return;

    switch (viewFormat)
    {
        case LongView:
            print(" ID    STATUS  STAT TYPE         "
                    "ROLE          IP        HOSTNAME");
            break;

        case TimeView:
            print(" ID    STATUS  STAT TYPE         "
                    "ROLE          DATE&TIME    HOSTNAME");
            break;
        
        case OsView:
            print(" ID    STATUS  STAT TYPE         "
                    "ROLE       HOSTNAME  OS");
            break;
    }
}

function main()
{
    var hosts  = cluster::hosts();
    var nCores = 0;

    processArguments(arguments);
    printHeader();

    for (hostIdx = 0; hostIdx < hosts.size(); ++hostIdx)
    {
        var host      = hosts[hostIdx].toMap();

        //warning(host);
        switch (viewFormat)
        {
            case NormalView:
                printHostNormal(host);
                break;
            
            case LongView:
                printHostLong(host);
                break;

            case TimeView:
                printHostTime(host);
                break;
            
            case OsView:
                printHostOs(host);
                break;
        }
    }

    if (viewFormat != NormalView)
    {
        print("Total ", hosts.size(), " host(s).");
        print("");
    }

    return 0;
}

