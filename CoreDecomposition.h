//
// Created by 王润安 on 2022/10/9.
//

#ifndef GRAPH_COREDECOMPOSITION_H
#define GRAPH_COREDECOMPOSITION_H

#include "DataStructure/MultiLayerGraph.h"
#include "DataStructure/Graph.h"

void naiveMLGCoreDecomposition(MultiLayerGraph mlg);

void bfsMLGCoreDecomposition(MultiLayerGraph mlg);

int *peelingCoreDecomposition(Graph g, bool printResult);

#endif //GRAPH_COREDECOMPOSITION_H
