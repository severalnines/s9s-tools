/*
 * 
 */
function recursive(n)
{
    return recursive(n + k) + 1;
}

function main()
{
    var a = recursive(0);

    exit(a);
}
