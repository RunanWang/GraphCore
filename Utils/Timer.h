//
// Created by 王润安 on 2022/10/9.
//

#ifndef GRAPH_TIMER_H
#define GRAPH_TIMER_H


#include<ctime>

class Timer {
public:
    void startTimer();

    void endTimer();

    double getTimerSecond();

private:
    clock_t startTime, endTime, totalTime;
};


#endif //GRAPH_TIMER_H
