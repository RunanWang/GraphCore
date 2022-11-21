//
// Created by 王润安 on 2022/10/9.
//

#include "Core.h"
#include <iostream>
#include <fstream>

#define _GLIBCXX_DEBUG

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
        if (iter->isFather(&cv)) {
            this->coreNumVec.erase(iter);
            iter -= 1;
        }
        iter += 1;
    }
    iter = this->coreNumVec.begin();
    while (iter != this->coreNumVec.end()) {
        if (cv.isFather(iter.base())) {
            return;
        }
        iter += 1;
    }
    this->coreNumVec.push_back(cv);
}

string CoreNum::toString() {
    string s;
    for (auto tempCV: this->coreNumVec) {
        s += tempCV.cvToString() + "\t";
    }
    return s;
}

void SimpleCoreInfo::initSCI(int maxNeighborNum, int threadNum, int nowCoreness) {
    // 给所有的数组做初始化
    _threadNum = threadNum;
    _nowCoreness = nowCoreness;
    _updateActivate = false;
    _msgNum = new int[threadNum];
    for (int i = 0; i < threadNum; i++) {
        _msgNum[i] = 0;
    }
    MsgBuffer = new SimpleCoreMessage *[threadNum];
    for (int i = 0; i < threadNum; i++) {
        MsgBuffer[i] = new SimpleCoreMessage[maxNeighborNum + 1];
    }
    NeighborNowCoreness = new int[nowCoreness + 1];
    for (int i = 0; i < nowCoreness + 1; i++) {
        NeighborNowCoreness[i] = 0;
    }
    bufferActivate = new bool[threadNum];
}

void SimpleCoreInfo::addMsg(int threadIx, SimpleCoreMessage msg) {
    // 其他的点发来的目前预估的新的coreness
    // 将之存入buffer里
    MsgBuffer[threadIx][_msgNum[threadIx]] = msg;
    _msgNum[threadIx]++;
    // 本点需要处理（需要merge一次处理buffer）
    bufferActivate[threadIx] = true;
}

void SimpleCoreInfo::initNeighborCoreness() {
    // 把buffer里的内容合并到NeighborNowCoreness数组里
    for (int threadIx = 0; threadIx < _threadNum; threadIx++) {
        for (int i = 0; i < _msgNum[threadIx]; i++) {
            SimpleCoreMessage msg = MsgBuffer[threadIx][i];
            if (msg.newCoreNum < _nowCoreness) {
                NeighborNowCoreness[msg.newCoreNum]++;
            } else {
                NeighborNowCoreness[_nowCoreness]++;
            }
        }
    }
    // buffer清空
    for (int threadIx = 0; threadIx < _threadNum; threadIx++) {
        _msgNum[threadIx] = 0;
        bufferActivate[threadIx] = false;
    }
    // 需要一次update
    _updateActivate = true;
}

bool SimpleCoreInfo::updateCoreness() {
    if (!_updateActivate) {
        return false;
    }
    int support = 0;
    _oldCoreness = _nowCoreness;
    // check now
    support += NeighborNowCoreness[_nowCoreness];
    while (support < _nowCoreness) {
        _nowCoreness--;
        support += NeighborNowCoreness[_nowCoreness];
    }
    NeighborNowCoreness[_nowCoreness] = support;
    _updateActivate = false;
    if (_oldCoreness != _nowCoreness) {
        return true;
    } else {
        return false;
    }
}

void SimpleCoreInfo::mergeMsgBufferIntoNeighborCoreness() {
    bool corenessChange = false;
    for (int threadIx = 0; threadIx < _threadNum; threadIx++) {
        for (int i = 0; i < _msgNum[threadIx]; i++) {
            SimpleCoreMessage msg = MsgBuffer[threadIx][i];
            if (msg.newCoreNum >= _nowCoreness and msg.oldCoreNum >= _nowCoreness) {
                continue;
            } else if (msg.newCoreNum < _nowCoreness and msg.oldCoreNum >= _nowCoreness) {
                corenessChange = true;
                NeighborNowCoreness[_nowCoreness]--;
                NeighborNowCoreness[msg.newCoreNum]++;
            } else if (msg.newCoreNum < _nowCoreness and msg.oldCoreNum < _nowCoreness){
                corenessChange = true;
                NeighborNowCoreness[msg.oldCoreNum]--;
                NeighborNowCoreness[msg.newCoreNum]++;
            }
        }
    }
    for (int threadIx = 0; threadIx < _threadNum; threadIx++) {
        bufferActivate[threadIx] = false;
        _msgNum[threadIx] = 0;
    }
    _updateActivate = corenessChange;
}

int SimpleCoreInfo::getNowCoreness() const{
    return _nowCoreness;
}

int SimpleCoreInfo::getOldCoreness() const {
    return _oldCoreness;
}

void SimpleCoreInfo::freeSCI() {
    delete[] _msgNum;
    delete[] bufferActivate;
    delete[] NeighborNowCoreness;
    for (int i = 0; i < _threadNum; i++) {
        delete[] MsgBuffer[i];
    }
    delete[] MsgBuffer;
}


