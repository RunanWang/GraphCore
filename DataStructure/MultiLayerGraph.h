//
// Created by 王润安 on 2022/10/8.
//
#include <iostream>
#include <set>
#include "Graph.h"

#ifndef GRAPH_MULTILAYERGRAPH_H
#define GRAPH_MULTILAYERGRAPH_H

class MultiLayerGraph {
public:
    void loadGraphFromFile(const std::string& filename);

    void showGraphProperties();

    int** getDegreeList();

    int getLayerNum() const;

    int getNodeNum() const;

    Graph *getGraphList() const;

private:
    int layerNum;
    int nodeNum;
    int edgeNum;
    Graph *GraphList;
};


#endif //GRAPH_MULTILAYERGRAPH_H
