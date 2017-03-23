/*
 * 
 */
function recursive(int n)
{
    return recursive(n) + 1;
}

function main()
{
    int a = recursive(0);

    exit(a);
}
