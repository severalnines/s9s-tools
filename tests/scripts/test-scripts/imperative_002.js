function main()
{
    var start;
    var now;

    start = CmonDateTime::currentDateTime();
    for (idx1 = 1; idx1 <= 100; ++idx1)
    {
        for (idx2 = 0; idx2 < 100000; ++idx2)
            a = sin(idx1) + sin(idx2);

        now = CmonDateTime::currentDateTime();
        print(  "elapsed: ", now - start, "s ",
                " idx1: ", idx1,
                " percent: ", idx1);
    }
}
