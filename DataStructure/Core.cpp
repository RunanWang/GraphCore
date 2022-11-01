//
// Created by 王润安 on 2022/10/9.
//

#include "Core.h"
#include <iostream>

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

void Core::printCore() {
    map<CoreVector, set<int>>::iterator iter;
    iter = kCoreMap.begin();
    cout << "===Below is core===" << endl;
    cout << "output cores: " << kCoreMap.size() << endl;
    while (iter != kCoreMap.end()) {
        auto kCoreVector = iter->first;
        auto nodesInCore = iter->second;
        string str;
        cout << "(";
        for (int j = 0; j < layerNum; j++) {
            if (j != layerNum - 1) {
                cout << to_string(kCoreVector.vec[j]) << ", ";
            } else {
                cout << to_string(kCoreVector.vec[j]);
            }
        }
        cout << ") " << nodesInCore.size() << "\t";
//        cout << endl;
        cout << "Nodes: ";
        for (int iterNode: nodesInCore)
            cout << iterNode << " ";
        cout << endl;
        iter++;
    }
    cout << "===Core end===" << endl;
}




