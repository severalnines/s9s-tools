function main(jobData)
{
    var cluster_id = jobData["effective_cluster_id"];

    print("   jobData: ", jobData);
    print("cluster_id: ", cluster_id);

    /*
     * Checking if the script indeed knows which cluster it is executed on.
     */
    if (cluster_id <= 0)
    {
        error("Cluster ID is ", cluster_id, ".");
        return false;
    }

    return true;
}
