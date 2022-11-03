//
// Created by 王润安 on 2022/10/8.
//

#include "MultiLayerGraph.h"
#include <fstream>

using namespace std;

void MultiLayerGraph::loadGraphFromFile(const string &filename) {
    cout << "Loading MLG..." << endl;
    int layer, tail, head = 0;
    ifstream fin;
    fin.open(filename);
    fin >> layerNum >> nodeNum >> edgeNum;
    cout << "MLG has " << layerNum << " layers and " << nodeNum << " nodes and " << edgeNum << " edges." << endl;
    GraphList = new Graph[layerNum];
    for (int j = 0; j < layerNum; j++) {
        GraphList[j].setNode(nodeNum);
    }
    for (int j = 0; j < edgeNum; j++) {
        fin >> layer >> tail >> head;
        GraphList[layer - 1].addEdge(tail, head);
    }
    fin.close();
}

void MultiLayerGraph::showGraphProperties() {
    cout << "===MLG Properties===" << endl;
    for (int j = 0; j < layerNum; j++) {
        cout << "In Layer " << j << ": ";
        GraphList[j].showGraphProperties();
    }
}

Graph *MultiLayerGraph::getGraphList() const {
    return GraphList;
}

int **MultiLayerGraph::getDegreeList() {
    int **DegreeList = new int *[layerNum];
    for (int j = 0; j < layerNum; j++) {
        DegreeList[j] = GraphList[j].getNodeDegreeList();
    }
    return DegreeList;
}

int MultiLayerGraph::getNodeNum() const {
    return nodeNum;
}

int MultiLayerGraph::getLayerNum() const {
    return layerNum;
}
