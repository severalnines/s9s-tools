/*
 * Checking the CmonClusterInfo we get about the cluster running the scripts.
 */
function main()
{
    var clusterInfo;
    var theMap;
    var className;
    var clusterId;
    var clusterType;
    var retval = true;

    clusterInfo = CmonClusterInfo::getClusterInfo();
    theMap      = clusterInfo.toMap();
    className   = theMap["class_name"];
    clusterId   = theMap["cluster_id"];
    clusterType = theMap["cluster_type"];

    print("        class: ", className);
    print("   cluster_id: ", clusterId);
    print(" cluster_type: ", clusterType);

    if (className != "CmonClusterInfo")
    {
        error("Class name is not ok.");
        retval = false;
    }

    if (clusterId <= 0)
    {
        error("Cluster ID is not ok.");
        retval = false;
    }

    if (clusterType != "GALERA")
    {
        error("Cluster type is not ok.");
        retval = false;
    }

    return retval;
}
