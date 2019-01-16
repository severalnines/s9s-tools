for (id = 1; id < 2; ++id)
{
    job = CmonJob::getJobInstance(id);
    print("      id: ", id);
    //print("job: ", job);
    print("  status: ", job.status());
}
