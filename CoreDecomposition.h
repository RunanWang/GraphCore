//
// Created by 王润安 on 2022/10/9.
//

#ifndef GRAPH_COREDECOMPOSITION_H
#define GRAPH_COREDECOMPOSITION_H

#include "DataStructure/MultiLayerGraph.h"
#include "DataStructure/Graph.h"

void naiveMLGCoreDecomposition(MultiLayerGraph mlg);

void bfsMLGCoreDecomposition(MultiLayerGraph mlg);

void dfsMLGCoreDecomposition(MultiLayerGraph mlg);

void hybridMLGCoreDecomposition(MultiLayerGraph mlg);

int *peelingCoreDecomposition(Graph g, bool printResult);

int *naivePeelingCoreDecomposition(Graph g, bool printResult);

void eachLayerCoreDecomposition(MultiLayerGraph mlg);

int *vertexCentricCoreDecomposition(Graph g, bool printResult);

int *optVertexCentricCoreDecomposition(Graph g, bool printResult);

int *lazyVertexCentricCoreDecomposition(Graph g, bool printResult);

int *mpVertexCentricCoreDecomposition(Graph g, bool printResult);

int *mpSingleThreadVertexCentricCoreDecomposition(Graph g, bool printResult);

int *optMPVertexCentricCoreDecomposition(Graph g, bool printResult, int threadNum);

int *pullMPVertexCentricCoreDecomposition(Graph g, bool printResult, int threadNum);

int *naiveVertexCentricCoreDecomposition(Graph g, bool printResult);

#endif //GRAPH_COREDECOMPOSITION_H
