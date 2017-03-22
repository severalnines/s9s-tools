//
// This test script will produce a backtrace.
//

function third()
{
    abort();
}

function second (sarg1)
{
    return third(sarg1);
}

function first(arg1, arg2)
{
    return second(arg1 + arg2);
}

function main(argument1)
{
    return first(10, "some string");
}
