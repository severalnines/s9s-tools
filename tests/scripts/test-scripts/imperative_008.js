/*
 * Testing the maximum recursion level limit.
 */
function myfunction()
{
    return myfunction();
}

function main()
{
    myfunction();
    return true;
}

