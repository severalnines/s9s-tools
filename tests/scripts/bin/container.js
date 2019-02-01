function ipOfContainer(map)
{
    var network = map["network"];

    if (network["public_ip"].size() > 0)
        return network["public_ip"][0];
    else if (network["private_ip"].size() > 0)
        return network["private_ip"][0];

    return "-";
}

function main(jobData)
{
    var containers = CmonServer::containers();
    var retval = true;
    
    for (idx = 0; idx < containers.size(); ++idx)
    {
        theMap  = containers[idx].toMap();
        
        //print("theMap: ", theMap);
        statusString = theMap["status"];
        ownerName    = theMap["owner_user_name"];
        groupName    = theMap["owner_group_name"];
        hostName     = theMap["hostname"];
        ipAddress    = ipOfContainer(theMap);

        print(
                statusString.leftAlign(8), " ",
                ownerName.leftAlign(8),    " ",
                groupName.leftAlign(8),    " ",
                hostName.leftAlign(24),    " ",
                ipAddress.leftAlign(16),   " " 
                );

        if (ownerName == "" || groupName == "")
        {
            error("Owner/group name empty.");
            retval = false;
        }

        if (hostName == "")
        {
            error("Name of the container is unknown.");
            retval = false;
        }
    }

    return retval;
}
