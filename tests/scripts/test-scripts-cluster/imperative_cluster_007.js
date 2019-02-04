/*
 * A executing shell commands on a host using the system() tag function.
 */
function main()
{
    var hosts = cluster::hosts();
    var retval = true;
    var command;
    var result;
    var value;

    command = 
        "for f in $(seq 1 3); do" +
        "  echo  $f;" +
        "  sleep 1;" +
        "done";

    for (idx = 0; idx < hosts.size(); ++idx)
    {
        print("");

        result = hosts[idx].system(command);

        print("hostName: ", hosts[idx].hostName());
        print(" command: ", command);
        print("  result: ", result);

        continue;
        if (result["errorMessage"] != "Success.")
        {
            error("Error message:", result["errorMessage"]);
            retval = false;
        }

        if (!result["success"])
        {
            error("Retcode is:", result["success"]);
            retval = false;
        }

        value = result["result"].toString().escape();
        if (value != "42")
        {
            error("Result is:", value);
            retval = false;
        }
    }

    return retval;
}
