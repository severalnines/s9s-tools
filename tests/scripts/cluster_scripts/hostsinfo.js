function main()
{
    hosts = cluster::hosts();

    for (idx = 0; idx < hosts.size(); ++idx)
    {
        print("");
        print("         hostName(): ", hosts[idx].hostName());
        print("     dataHostName(): ", hosts[idx].dataHostName());
        print(" internalHostName(): ", hosts[idx].internalHostName());
        print("             port(): ", hosts[idx].port());
        print("        connected(): ", hosts[idx].connected());
        print(" sqlStatusVariables(): ", hosts[idx].sqlStatusVariables());
    }
}
