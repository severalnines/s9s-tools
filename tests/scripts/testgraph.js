//
// This is a test program that prints a graph on the sql statistics.
//
#include "cmon/graph.h"

function main()
{
    var hosts     = cluster::hosts();
    var host      = hosts[0];
    var endTime   = CmonDateTime::currentDateTime();
    var startTime = endTime - 10 * 60;
    var stats     = host.sqlStats(startTime, endTime);
    var array     = stats.toArray("created,interval,COM_SELECT,COM_INSERT");

    //
    // Calculating some values from the statistics
    //
    for (idx = 0; idx < array.columns(); idx++)
    {
        array[5, idx] = 1000 * array[2, idx] / array[1, idx];
        array[6, idx] = 1000 * array[3, idx] / array[1, idx];
    }
    
    x=array[8,8];
    print(x);
    var graph     = new CmonGraph;
    graph.setXDataIsTime(false);
    graph.setTitle("SQL Statistics " + host.toString());
    graph.setSize(800, 600);

    // This graph contains two plots, we set the various properties for them
    // here here. The plot index will be 1 and 2.
    graph.setPlotLegend(1, "Select (1/s)");
    graph.setPlotColumn(1, 0, 5);
    graph.setPlotStyle(1, Impulses);
    graph.setPlotLegend(2, "Insert (1/s)");
    graph.setPlotColumn(2, 0, 6);
    graph.setPlotStyle(2, Impulses);
    graph.setData(array);
    exit(graph);
}
