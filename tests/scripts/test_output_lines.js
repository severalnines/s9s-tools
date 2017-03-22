function main()
{
    print("Test for producing a few messages.");

    for (a = 0; a < 3; ++a)
        print("  a*a = ", a * a);

    warning("This is a warning message.");
    error("This is an error message.");

    exit(0);
}
