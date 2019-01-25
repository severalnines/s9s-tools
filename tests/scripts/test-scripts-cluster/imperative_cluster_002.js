/*
 * A test created to check the hosts. This obviously needs more checks.
 */
function main()
{
    var hosts = cluster::hosts();
    var map;
    var retval = true;

    for (idx = 0; idx < hosts.size(); ++idx)
    {
        map        = hosts[idx].toMap();
        className  = map["class_name"];
        role       = map["role"];
        port       = map["port"];
        hostStatus = map["hoststatus"];

        //print("map: ", hosts[idx].toMap());
        print("");
        print("         hostName(): ", hosts[idx].hostName());
        print("     dataHostName(): ", hosts[idx].dataHostName());
        print(" internalHostName(): ", hosts[idx].internalHostName());
        print("             port(): ", hosts[idx].port());
        print("        connected(): ", hosts[idx].connected());
        print("         class_name: ", className);
        print("               role: ", role);
        print("               port: ", port);
        print("         hoststatus: ", hostStatus);
        
        if (role == "controller")
        {
            if (port != 9555)
            {
                error("The port is not ok.");
                retval = false;
            }
        } else if (role == "none")
        {
            if (port != 3306)
            {
                error("The port is not ok.");
                retval = false;
            }
        } else {
            error("The role '", role, "' is invalid.");
            return false;
        }

        if (hostStatus != "CmonHostOnline")
        {
            error("Host status is not ok.");
            retval = false;
        }
    }

    return retval;
}

