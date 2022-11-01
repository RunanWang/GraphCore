//
// Created by 王润安 on 2022/10/19.
//

#ifndef GRAPH_OUT_H
#define GRAPH_OUT_H

#include <iostream>

using namespace std;

void printList(int *list, int length) {
    cout << "[";
    for (int j = 0; j < length; j++) {
        cout << list[j] << ", ";
    }
    cout << "]" << endl;
}

#endif //GRAPH_OUT_H
