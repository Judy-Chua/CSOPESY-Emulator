#pragma once

//state 0 = ready, 1 = running, 2 = waiting, 3 = finished
struct ProcessInfo {
    int count;
    int total;
    int pid;
    int state;
    int coreId;
};