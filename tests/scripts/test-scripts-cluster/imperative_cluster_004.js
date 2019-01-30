/*
 * A executing shell commands on hosts.
 */
function main()
{
    var hosts = cluster::hosts();
    var retval = true;
    var command = "echo $((41 + 1))";
    var result;
    var value;

    for (idx = 0; idx < hosts.size(); ++idx)
    {
        print("");

        result = hosts[idx].system(command);

        print("hostName: ", hosts[idx].hostName());
        print(" command: ", command);
        print("  result: ", result);

        if (result["errorMessage"] != "Success.")
        {
            error("Error message:", result["errorMessage"]);
            retval = false;
        }

        value = result["result"].toString().escape();
        if (value != "42\r\n")
        {
            error("Result is:", value);
            retval = false;
        }
    }

    return retval;
}

