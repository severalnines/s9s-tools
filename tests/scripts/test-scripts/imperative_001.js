/*
 * A very simple test to check a few operators.
 */
function main()
{
    variable1 = 10 * (2 + 8);
    variable2 = 10 * 2 + 8;

    passed = variable1 == 100 && variable2 == 28;
    print("variable1:", variable1);
    print("variable2:", variable2);
    print("passed:", passed);

    exit(passed);
}
