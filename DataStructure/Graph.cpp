//
// Created by 王润安 on 2022/10/4.
//

#include "Graph.h"
#include <fstream>

using namespace std;

void Graph::loadGraphFromSnapFile(string filename) {
    cout << "Loading graph..." << endl;
    int tail, head = 0;
    ifstream fin;
    fin.open(filename);
    fin >> nodeNum >> edgeNum;
    nodeDegreeList = new int[nodeNum];
    neighborList = new nodeNeighbor[nodeNum];
    maxDeg = 0;
    for (int j = 0; j < nodeNum; j++) {
        nodeDegreeList[j] = 0;
    }
    for (int j = 0; j < edgeNum; j++) {
        fin >> tail >> head;
        neighborList[tail].emplace_back(head);
        neighborList[head].emplace_back(tail);
        nodeDegreeList[head]++;
        nodeDegreeList[tail]++;
        if (nodeDegreeList[tail] > maxDeg) maxDeg = nodeDegreeList[tail];
        if (nodeDegreeList[head] > maxDeg) maxDeg = nodeDegreeList[head];
    }
    fin.close();
}

void Graph::setNode(int _nodeNum) {
    nodeNum = _nodeNum;
    nodeDegreeList = new int[nodeNum];
    neighborList = new nodeNeighbor[nodeNum + 1];
}

void Graph::showGraphProperties() {
    setMaxDegree();
    cout << edgeNum << " edges, maxDeg: " << maxDeg << endl;
}

void Graph::addEdge(int fromEdge, int toEdge) {
    neighborList[fromEdge].push_back(toEdge);
    neighborList[toEdge].push_back(fromEdge);
    nodeDegreeList[fromEdge]++;
    nodeDegreeList[toEdge]++;
    edgeNum += 1;
}

void Graph::setMaxDegree() {
    for (int j = 0; j < nodeNum; j++) {
        maxDeg = (maxDeg > nodeDegreeList[j] ? maxDeg : nodeDegreeList[j]);
    }
}

int *Graph::getNodeDegreeList() const {
    return nodeDegreeList;
}

int Graph::getNodeNum() const {
    return nodeNum;
}

int Graph::getEdgeNum() const {
    return edgeNum;
}

int Graph::getMaxDeg() const {
    return maxDeg;
}

nodeNeighbor Graph::getNeighbor(int vertex) {
    auto neighborVec = neighborList[vertex];
    return neighborVec;
}