function main()
{
    var hosts = cluster::hosts();
    for (idx = 0; idx < hosts.size(); ++idx)
    {
        host = hosts[idx];
        print(host.toMap());
    }

    print("Hello world!!!");
    return hosts.size();
}

