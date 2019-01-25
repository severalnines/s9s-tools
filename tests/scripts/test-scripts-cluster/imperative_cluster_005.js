/*
 * This is how a CmonParamSpec map looks like.
 * {
 *     "class_name": "CmonParamSpec",
 *     "default_value": "",
 *     "description": "The version for the cluster software.",
 *     "is_counter": false,
 *     "is_public": true,
 *     "is_writable": false,
 *     "owner_type_name": "CmonClusterInfo",
 *     "property_name": "version",
 *     "type_name": "String"
 * }
 */
function checkMap(theMap)
{
    var className;
    var keys;
    var retval = true;
    var paramSpec;
    
    keys        = theMap.keys();
    className   = theMap["class_name"];
    
    for (idx = 0; idx < keys.size(); ++idx)
    {
        name      = keys[idx];
        paramSpec = CmonMetaType::getParamSpec(className, name);
        ownerType = paramSpec["owner_type_name"];
        description = paramSpec["description"];
        
        print(name.leftAlign(24), " ", 
                ownerType.leftAlign(18), " ",
                description);

        if (ownerType.empty())
        {
            error("The ", className, " class has no property ", name, ".");
            retval = false;
            //print(paramSpec);
        }

        if (!paramSpec["is_public"])
        {
            error("The ", name, " property of class ", className, 
                    " should not be public.");
            retval = false;
        }
    }

    return retval;
}

function main()
{
    var clusterInfo;
    var jobId;
    var jobInstance;
    var theMap;
    var retval = true;

    // Checking the CmonClusterInfo properties.
    clusterInfo = CmonClusterInfo::getClusterInfo();
    theMap      = clusterInfo.toMap();

    if (!checkMap(theMap))
        retval = false;
    
    // Checking the CmonJobInfo properties.
    jobId       = CmonJob::currentJobId();
    jobInstance = CmonJob::getJobInstance(jobId);
    theMap      = jobInstance.toMap();

    if (!checkMap(theMap))
        retval = false;

    return retval;
}

