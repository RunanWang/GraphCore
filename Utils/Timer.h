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

    double getTimerSecond() const;

    double getCpuTimeSecond() const;

    void resetTimer();

private:
    double startTime, endTime, totalTime;
    clock_t cpuStartTime, cpuEndTime, cpuTotalTime;

};


#endif //GRAPH_TIMER_H
