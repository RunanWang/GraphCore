//
// Created by 王润安 on 2022/10/9.
//

#include "Timer.h"

void Timer::startTimer() {
    startTime = clock();
}

void Timer::endTimer() {
    endTime = clock();
    totalTime += endTime - startTime;
}

double Timer::getTimerSecond() {
    return (double) (totalTime) / CLOCKS_PER_SEC;
}