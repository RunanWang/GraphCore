//
// Created by 王润安 on 2022/10/9.
//

#include "Core.h"
#include <iostream>
#include <fstream>

Core::Core(int _layerNum) {
    layerNum = _layerNum;
}

void Core::addCore(CoreVector coreVector, const set<int> &nodesInCore) {
    kCoreMap.insert(pair<CoreVector, set<int>>(coreVector, nodesInCore));
}


int Core::getSize() {
    return kCoreMap.size();
}

set<int> Core::getCore(CoreVector cv) {
    return kCoreMap.find(cv)->second;
}

bool Core::hasCore(CoreVector cv) {
    return (kCoreMap.count(cv) != 0);
}

void Core::printCore() {
    ofstream out("../cores.txt");
    map<CoreVector, set<int>>::iterator iter;
    iter = kCoreMap.begin();
    out << "===Below is core===" << endl;
    out << "output cores: " << kCoreMap.size() << endl;
    while (iter != kCoreMap.end()) {
        auto kCoreVector = iter->first;
        auto nodesInCore = iter->second;
        string str;
        out << "(";
        for (int j = 0; j < layerNum; j++) {
            if (j != layerNum - 1) {
                out << to_string(kCoreVector.vec[j]) << ", ";
            } else {
                out << to_string(kCoreVector.vec[j]);
            }
        }
        out << ") " << nodesInCore.size() << "\t";
        out << "Nodes: ";
        for (int iterNode: nodesInCore)
            out << iterNode << " ";
        out << endl;
        iter++;
    }
    out << "===Core end===" << endl;
}






