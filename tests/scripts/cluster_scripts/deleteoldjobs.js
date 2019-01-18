function main()
{
    var nDeletedJobs = 0;
    var limit        = 1;
    var now = CmonDateTime::currentDateTime();

    for (id = 1; id <= 20; ++id)
    {
        job = CmonJob::getJobInstance(id);
        found = job.isValid();
        if (!found)
            continue;

        if (job.status() != "FINISHED")
            continue;

        if ((now - job.ended()) / (60 * 60) < 12)
            continue;

        print("");
        print("    title: ", job.title());
        print("       id: ", id);
        print("   status: ", job.status());
        print("    ended: ", (now - job.ended()) / (60 * 60), "h");

        print("Deleting...");
        if (!CmonJob::deleteJobInstance(id))
        {
            print("Failed to delete job.");
            break;
        }

        ++nDeletedJobs;
        if (nDeletedJobs >= limit)
        {
            print("Reached limit, deleted ", nDeletedJobs, " job(s)");
            break;
        }
    }
}
