function main()
{
    var clusterInfo;
    var className;
/*
    print("   cluster::name() = ", cluster::name());
    print("     cluster::id() = ", cluster::id());
    print("  cluster::state() = ", cluster::state());
    print("   cluster::type() = ", cluster::type());
    print(" cluster::vendor() = ", cluster::vendor());
*/

    clusterInfo = CmonClusterInfo::getClusterInfo();
    names       = clusterInfo.keys();
    //className   = clusterInfo["class_name"];

    print("  clusterInfo: ", clusterInfo);
    print("        names: ", names);
    //print("        class: ", className);
}

