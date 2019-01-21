function main()
{
    // Getting the ID of the current job.
    print("Information about the current job:");
    jobId = CmonJob::currentJobId();
    print("       jobId: ", jobId);

    // Getting the job instance
    job = CmonJob::getJobInstance(jobId);
    found = job.isValid();
    if (!found)
    {
        error("Job ID was not found.");
        return false;
    }

    // Printing some info about the current job.
    print("   clusterId: ", job.clusterId());
    print("      status: ", job.status());
    print("       title: ", job.title());
    print("canBeDeleted: ", job.canBeDeleted());
    print("     created: ", job.created());
    print("  statusText: ", job.statusText());
    print("        tags: ", job.tags());

    if (job.status() != "RUNNING")
    {
        error("The job should be in RUNNING status.");
        return false;
    }

    return true;
}
