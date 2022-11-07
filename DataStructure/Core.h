//
// Created by 王润安 on 2022/10/9.
//

#ifndef GRAPH_CORE_H
#define GRAPH_CORE_H

#include <vector>
#include <map>
#include <set>
#include "CoreVector.h"

using namespace std;


class Core {
public:
    Core(int _layerNum);

    void addCore(CoreVector coreVector, const set<int>& nodesInCore);

    void printCore();

    void printCoreNum(int nodeNum);

    int getSize();

    set<int> getCore(CoreVector cv);

    bool hasCore(CoreVector cv);

private:
    int layerNum;
    map<CoreVector, set<int>, CVCompartor> kCoreMap;
};

class CoreNum {
public:
    vector<CoreVector> coreNumVec;

    void addCoreVector(CoreVector cv);

    string toString();
};

#endif //GRAPH_CORE_H
