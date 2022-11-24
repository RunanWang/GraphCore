//
// Created by 王润安 on 2022/10/9.
//

#include "Timer.h"
#include <omp.h>

void Timer::startTimer() {
    startTime = omp_get_wtime();
    cpuStartTime = clock();
}

void Timer::endTimer() {
    endTime = omp_get_wtime();
    cpuEndTime = clock();
    totalTime += endTime - startTime;
    cpuTotalTime += cpuEndTime - cpuStartTime;
}

double Timer::getTimerSecond() const {
    return totalTime;
}

double Timer::getCpuTimeSecond() const {
    return (double) (cpuTotalTime) / CLOCKS_PER_SEC;
}

void Timer::resetTimer() {
    totalTime = 0;
    cpuTotalTime = 0;
}
