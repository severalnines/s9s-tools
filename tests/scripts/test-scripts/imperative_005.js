function main()
{
    for (id = CmonJob::firstJobId(); id <= CmonJob::lastJobId(); ++id)
    {
        job = CmonJob::getJobInstance(id);
        found = job.isValid();
        if (!found)
            continue;

        print(job.id(), " ", job.clusterId(), " ", 
                job.status(), " ", job.title());
    }
}
