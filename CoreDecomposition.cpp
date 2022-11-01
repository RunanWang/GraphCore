//
// Created by 王润安 on 2022/10/9.
//
#include <queue>
#include <set>
#include "CoreDecomposition.h"
#include "Utils/Timer.h"
#include "Utils/out.h"
#include "DataStructure/Core.h"
#include "DataStructure/CoreVector.h"

using namespace std;


void naiveMLGCoreDecomposition(MultiLayerGraph mlg) {
    Timer AlgoTimer{};
    AlgoTimer.startTimer();
    int layerNum = mlg.getLayerNum();
    int nodeNum = mlg.getNodeNum();

    auto degreeList = mlg.getDegreeList();
    CoreVector kCoreVector = *new CoreVector(layerNum);
    auto nodeSet = new set<int>{};
    for (int j = 0; j < nodeNum; j++) {
        nodeSet->insert(j);
    }

    // core[0]
    int NumberOfCores = 0;
    Core coreSet = *new Core(mlg.getLayerNum());
    coreSet.addCore(kCoreVector, *nodeSet);

    // initialize
    auto vectorsQueue = queue<CoreVector>{};
    auto computedVector = set<CoreVector, CVCompartor>{};
    auto inactiveNodes = queue<int>{};
    int ** nodeToLayerDegree;
    for (int j = 0; j < layerNum; j++) {
        auto vector = kCoreVector.build_descendant_vector(j);
        vectorsQueue.push(vector);
        computedVector.insert(vector);
    }

    while (!vectorsQueue.empty()) {
        auto vector = vectorsQueue.front();
        vectorsQueue.pop();
        nodeToLayerDegree = new int *[nodeNum];

        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            nodeToLayerDegree[tempNode] = new int[layerNum];
            for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
                nodeToLayerDegree[tempNode][tempLayer] = degreeList[tempLayer][tempNode];
            }
        }
        nodeSet = new set<int>{};
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            bool inCore = true;
            for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
                if (nodeToLayerDegree[tempNode][tempLayer] < vector.vec[tempLayer]) {
                    inCore = false;
                }
            }
            if (inCore) {
                nodeSet->insert(tempNode);
            } else {
                inactiveNodes.push(tempNode);
                while (!inactiveNodes.empty()) {
                    auto toPeelVertex = inactiveNodes.front();
                    inactiveNodes.pop();
                    for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
                        auto neighborVector = mlg.getGraphList()[tempLayer].getNeighbor(toPeelVertex);
                        for (auto neighborVertex: neighborVector) {
                            nodeToLayerDegree[neighborVertex][tempLayer] -= 1;
                            if (nodeToLayerDegree[neighborVertex][tempLayer] < vector.vec[tempLayer] and
                                nodeSet->count(neighborVertex)) {
                                nodeSet->erase(neighborVertex);
                                inactiveNodes.push(neighborVertex);
                            }
                        }
                    }
                }
            }
        }
        NumberOfCores += 1;
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            delete nodeToLayerDegree[tempNode];
        }
        delete nodeToLayerDegree;

        if (!nodeSet->empty()) {
            coreSet.addCore(vector, *nodeSet);
            for (int j = 0; j < layerNum; j++) {
                auto tempVector = vector.build_descendant_vector(j);
                if (!computedVector.count(tempVector)) {
                    vectorsQueue.push(tempVector);
                    computedVector.insert(tempVector);
                }
            }
        }
        delete nodeSet;

    }

    AlgoTimer.endTimer();
    std::cout << "Naive Time: " << AlgoTimer.getTimerSecond() << " s." << std::endl;
    std::cout << "core number: " << NumberOfCores << endl;
    coreSet.printCore();
}

int *peelingCoreDecomposition(Graph g, bool printResult) {
    int nodeNum = g.getNodeNum();
    int maxDegree = g.getMaxDeg();

    int *coreNumList = new int[nodeNum];                // 最终的结果，即每个点对应点core-num
    int *vertexByDegOrder = new int[nodeNum];           // 简称Order，按照度数排序的vertex数组
    int *vertexToOrderIndex = new int[nodeNum];         // 简称Index，Order[Index[vertex]] = vertex
    int *degreeToFirstIndex = new int[maxDegree + 1];   // 简称First，First[i]表示第一个度数为i的点在Order数组中的index
    auto tempNodeDegreeList = g.getNodeDegreeList();   // 图中每个点的度数
    int *nodeDegreeList = new int[nodeNum];
    for (int j = 0; j < nodeNum; j++) {
        nodeDegreeList[j] = tempNodeDegreeList[j];
    }
    // 上述三个数组会随着peeling过程逐渐发生变化

    // 初始化，首先生成vertexByDegOrder，是对所有vertex按照度数进行对一次排序。
    // 需要O(m)的时间和O(d)的空间。
    // 我们首先统计每个degree值对应着多少个vertex
    for (int j = 0; j < maxDegree + 1; j++) {
        degreeToFirstIndex[j] = 0;
    }
    for (int j = 0; j < nodeNum; j++) {
        degreeToFirstIndex[nodeDegreeList[j]] += 1;
    }
    // 然后计算出每个度数对应对第一个vertex的index
    for (int j = maxDegree; j > 0; j--) {
        degreeToFirstIndex[j] = degreeToFirstIndex[j - 1];
    }
    degreeToFirstIndex[0] = 0;
    for (int j = 1; j < maxDegree + 1; j++) {
        degreeToFirstIndex[j] += degreeToFirstIndex[j - 1];
    }
    // 最后根据index，扫描一边全部的点，把点放到index指示的为止上
    for (int j = 0; j < nodeNum; j++) {
        int degree = nodeDegreeList[j];
        int index = degreeToFirstIndex[degree];
        vertexByDegOrder[index] = j;
        vertexToOrderIndex[j] = index;
        degreeToFirstIndex[degree] += 1;
    }
    // 还原degreeCount
    for (int j = maxDegree; j > 0; j--) {
        degreeToFirstIndex[j] = degreeToFirstIndex[j - 1];
    }
    degreeToFirstIndex[0] = 0;
    //printList(degreeToFirstIndex, maxDegree + 1);
    //printList(vertexByDegOrder, nodeNum);

    // 开始peeling
    for (int j = 0; j < nodeNum; j++) {
        // 选出目前degree最小的点toPeelVertex
        auto toPeelVertex = vertexByDegOrder[j];
        // toPeelVertex点度数是toPeelVertex的core-num
        coreNumList[toPeelVertex] = nodeDegreeList[toPeelVertex];
        // 寻找toPeelVertex的全部邻居
        auto neighborVector = g.getNeighbor(toPeelVertex);
        auto toPeelVertexDegree = nodeDegreeList[toPeelVertex];
        for (auto neighborVertex: neighborVector) {
            // 对于比toPeelVertex度数大的邻居进行更新
            // 需要更新Order、First、Index三个数组和DegreeList
            if (nodeDegreeList[neighborVertex] > toPeelVertexDegree) {
                // 首先更新Order：邻居点的Degree要-1，那么我们可以把邻居点和Degree的首个点交换位置，再把对应点First+1
                int neighborVDegree = nodeDegreeList[neighborVertex];
                int toSwapU = vertexToOrderIndex[neighborVertex];       // 找到邻居点在Order中的index-toSwapU
                int toSwapV = degreeToFirstIndex[neighborVDegree];      // 找到邻居对应度数在Order中的index-toSwapV
                // 交换toSwapU和toSwapV，这时邻居点已经被移到degree-1的最后
                auto temp = vertexByDegOrder[toSwapV];
                vertexByDegOrder[toSwapV] = vertexByDegOrder[toSwapU];
                vertexByDegOrder[toSwapU] = temp;
                // 更新Index：把交换位置的两个点的index也互相交换
                vertexToOrderIndex[temp] = toSwapU;
                vertexToOrderIndex[neighborVertex] = toSwapV;
                // 更新First：这时邻居点已经被移到First[neighborVDegree]的位置上
                // First[neighborVDegree] + 1，使邻居点划入neighborVDegree - 1中
                degreeToFirstIndex[neighborVDegree] += 1;
                // 更新DegreeList：这个邻居的度数-1
                nodeDegreeList[neighborVertex] -= 1;
            }
        }
    }
    if (printResult) {
        cout << "Core Number as Follow: " << endl;
        printList(coreNumList, nodeNum);
    }
    return coreNumList;
}

