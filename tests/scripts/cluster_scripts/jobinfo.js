for (id = 1; id <= 20; ++id)
{
    job = CmonJob::getJobInstance(id);
    found = job.isValid();
    if (!found)
        continue;

    print(job.id(), " ", job.clusterId(), " ", 
            job.status(), " ", job.title());
}

