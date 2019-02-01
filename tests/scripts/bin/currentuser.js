
function main(jobArg)
{
    var user   = CmonUser::currentUser();
    var map    = user.toMap();
    var retval = true;

    print("          user_id: ", map["user_id"].toInt());
    print("        user_name: ", map["user_name"]);
    print("    owner_user_id: ", map["owner_user_id"].toInt());
    print("  owner_user_name: ", map["owner_user_name"]);
    print("   owner_group_id: ", map["owner_group_id"].toInt());
    print(" owner_group_name: ", map["owner_group_name"]);

    //print("map: ", map);

    if (map["user_id"].toInt() <= 0)
    {
        error("User ID is not ok.");
        retval = false;
    }
    
    if (map["owner_user_id"].toInt() <= 0)
    {
        error("Owner user ID is not ok.");
        retval = false;
    }
    
    if (map["owner_group_id"].toInt() <= 0)
    {
        error("Owner group ID is not ok.");
        retval = false;
    }
    
    if (map["user_name"] = "")
    {
        error("User name is not ok.");
        retval = false;
    }
    
    if (map["owner_user_name"] = "")
    {
        error("Owner user name is not ok.");
        retval = false;
    }
    
    if (map["owner_group_name"] = "")
    {
        error("Owner group name is not ok.");
        retval = false;
    }

    return retval;
}

