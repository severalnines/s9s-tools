//
// This is actually a demonstration of some array access to statistical data.
//
#include "cmon/io.h"

function printUtil(host, startTime, endTime)
{
    var list = host.memoryStats(startTime, endTime);

    if (list.size() == 0)
    {
        error("No memory stats found.");
        return false;
    }

    var array = list.toArray("ramtotal,ramfree,swaptotal,swapfree,created");

    print("");
    print("    RAMTOTAL     RAMFREE   SWAPTOTAL    SWAPFREE     CREATED");
    print("------------------------------------------------------------");
    for (column = 0; column < array.columns(); ++column)
    {
        /*
        for (format = 0; format < 21; ++format)
        {
            print(
                    "format: ", format, 
                    " result: ", array[4, column].toString(format));
        }
        */

        print(
                array[0, column].toString(19).rightAlign(12),
                array[1, column].toString(19).rightAlign(12),
                array[2, column].toString(19).rightAlign(12),
                array[3, column].toString(19).rightAlign(12),
                array[4, column].toString(10).rightAlign(12)
                );

        if (array[0, column] == 0) 
            return false;
    }
    
    print("------------------------------------------------------------");
    print("Host: ", host.hostName());
    print("");

    return true;
}

function main()
{
    var endTime   = CmonDateTime::currentDateTime();
    var startTime = endTime - 60;
    var hosts     = cluster::hosts();
    var success   = true;

    for (idx = 0; idx < hosts.size(); ++idx)
    {
        // The test config file contains this host for some reason, we just skip
        // it.
        if (hosts[idx].hostName() == "127.0.0.2")
            continue;

        success = printUtil(hosts[idx], startTime, endTime);

        if (!success)
            break;
    }

    return success;
}
