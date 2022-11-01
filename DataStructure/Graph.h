//
// Created by 王润安 on 2022/10/4.
//
#include <iostream>
#include <vector>

#ifndef GRAPH_GRAPH_H
#define GRAPH_GRAPH_H

typedef std::vector<int> nodeNeighbor;

class Graph {
public:
    void loadGraphFromSnapFile(std::string filename);

    void showGraphProperties();

    void addEdge(int fromEdge, int toEdge);

    void setNode(int _nodeNum);

    void setMaxDegree();

    nodeNeighbor getNeighbor(int vertex);

    int *getNodeDegreeList() const;

    int getNodeNum() const;

    int getEdgeNum() const;

    int getMaxDeg() const;

    int *nodeDegreeList;

private:
    nodeNeighbor *neighborList;
    int nodeNum;
    int edgeNum;
    int maxDeg;

};


#endif //GRAPH_GRAPH_H
