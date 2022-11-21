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

    void addCore(CoreVector coreVector, const set<int> &nodesInCore);

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

struct SimpleCoreMessage {
    int oldCoreNum;
    int newCoreNum;
};

class SimpleCoreInfo {
public:
    void freeSCI();

    void initSCI(int maxNeighborNum, int threadNum, int nowCoreness);

    void addMsg(int threadIx, SimpleCoreMessage msg);

    bool updateCoreness();

    void mergeMsgBufferIntoNeighborCoreness();

    void initNeighborCoreness();

    int getNowCoreness() const;

    int getOldCoreness() const;

private:
    int _threadNum;
    int _nowCoreness;
    int _oldCoreness;
    bool _updateActivate;
    int *_msgNum;
    int *NeighborNowCoreness;

    // 以下是一些在线程中会竞争的资源，本数据结构采用的方法是无锁的方法，使用thread—id存储多个数组，在使用时合并。
    SimpleCoreMessage **MsgBuffer;
    bool *bufferActivate;
};

#endif //GRAPH_CORE_H
