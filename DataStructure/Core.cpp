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
        out << kCoreVector.cvToString();
        out << " " << nodesInCore.size() << "\t";
        out << "Nodes: ";
        for (int iterNode: nodesInCore)
            out << iterNode << " ";
        out << endl;
        iter++;
    }
    out << "===Core end===" << endl;
}

void Core::printCoreNum(int nodeNum) {
    auto nodeToCoreNumVec = new map<int, CoreNum>{};
    map<CoreVector, set<int>>::iterator iter;
    iter = kCoreMap.begin();
    while (iter != kCoreMap.end()) {
        auto kCoreVector = iter->first;
        auto nodesInCore = iter->second;
        for (auto tempNode: nodesInCore) {
            if (nodeToCoreNumVec->find(tempNode) != nodeToCoreNumVec->end()) {
                nodeToCoreNumVec->find(tempNode)->second.addCoreVector(kCoreVector);
            } else {
                auto vc = new CoreNum{};
                vc->addCoreVector(kCoreVector);
                nodeToCoreNumVec->insert(pair<int, CoreNum>(tempNode, *vc));
            }
        }
        iter++;
    }
    ofstream out("../core-num.txt");
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        out << tempNode << "\t\t";
        out << nodeToCoreNumVec->find(tempNode)->second.toString();
        out << endl;
    }
}


void CoreNum::addCoreVector(CoreVector cv) {
    vector<CoreVector>::iterator iter;
    iter = this->coreNumVec.begin();
    while (iter != this->coreNumVec.end()) {
        if (iter->isFather(&cv)){
            this->coreNumVec.erase(iter);
            iter -= 1;
        }
        iter += 1;
    }
    iter = this->coreNumVec.begin();
    while (iter != this->coreNumVec.end()) {
        if (cv.isFather(iter.base())){
            return;
        }
        iter += 1;
    }
    this->coreNumVec.push_back(cv);
}

string CoreNum::toString() {
    string s;
    for (auto tempCV: this->coreNumVec){
        s += tempCV.cvToString() + "\t";
    }
    return s;
}
