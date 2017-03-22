//
//
//
#include "cmon/io.h"

function ellipsis(theString, theLength)
{
    if (theString.length() < theLength - 1)
        return theString;

    theString = theString.substr(0, theLength - 1) + "…";
    return theString;
}

function main()
{
    var hosts  = cluster::hosts();
    var nCores = 0;

    print("H:P:C   USER     SYS    IDLE   TEMP   LOAD MODEL");
    for (hostIdx = 0; hostIdx < hosts.size(); ++hostIdx)
    {
        var host = hosts[hostIdx];

        //print("*** hostIdx : ", hostIdx);
        //print("*** host    : ", host);
    
        stats = host.cpuInfo();
        //error(stats);
        for (idx = 0; idx < stats.size(); ++idx)
        {
            var info   = stats[idx];
            var model  = ellipsis(info["cpumodelname"], 37);
            var phys   = info["cpuphysicalid"];
            var cpuid  = info["cpuid"];
            var user   = info["user"];
            var sys    = info["sys"];
            var idle   = info["idle"];
            var temp   = info["cputemp"];
            var load   = info["loadavg1"];
            // FIXME: This is not right
            var hostId = hostIdx;

            //warning("info: ", info);
            
            print(hostId, ":", phys, ":", cpuid, " ",
                    user.toString(TwoDecimalPercent), " ",
                    sys.toString(TwoDecimalPercent), " ",
                    idle.toString(TwoDecimalPercent), " ",
                    temp.toString(Celsius), "  ",
                    load.toString(TwoDecimalNumber), " ",
                    model);

            ++nCores;
        }
    }

    print("Total ", nCores, " cores on ", hosts.size(), " hosts.");
    return true;
}
