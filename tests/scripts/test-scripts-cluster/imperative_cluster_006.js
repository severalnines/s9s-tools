/*
 * A test created to check the hosts. 
 */
function main()
{
    var hosts = cluster::hosts();
    var retval = true;
    var command = "cp no_such_file_exists neither_this";
    var result;

    for (idx = 0; idx < hosts.size(); ++idx)
    {
        print("");

        result = hosts[idx].system(command);

        print("hostName: ", hosts[idx].hostName());
        print(" command: ", command);
        print("  result: ", result);

        // 
        // Here is a thing, we don't have the error messages/stderr.
        //
        if (result["success"])
        {
            error("Retcode is:", result["success"]);
            retval = false;
        }
    }

    return retval;
}
