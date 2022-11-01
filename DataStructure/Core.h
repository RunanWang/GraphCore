//
// Created by 王润安 on 2022/10/9.
//

#ifndef GRAPH_CORE_H
#define GRAPH_CORE_H

#include <map>
#include <set>
#include "CoreVector.h"

using namespace std;


class Core {
public:
    Core(int _layerNum);

    void addCore(CoreVector coreVector, const set<int>& nodesInCore);

    void printCore();

    int getSize();

    set<int> getCore(CoreVector cv);

private:
    int layerNum;
    map<CoreVector, set<int>, CVCompartor> kCoreMap;
};


#endif //GRAPH_CORE_H
