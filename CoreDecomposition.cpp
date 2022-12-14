//
// Created by 王润安 on 2022/10/9.
//
#include <queue>
#include <set>
#include <map>
#include <algorithm>
#include <omp.h>
#include "CoreDecomposition.h"
#include "Utils/Timer.h"
#include "Utils/out.h"
#include "DataStructure/Core.h"
#include "DataStructure/CoreVector.h"
#include "DataStructure/Message.h"

using namespace std;

set<int> *kCore(MultiLayerGraph mlg, const set<int> &intersect, int layerNum, int NodeNum, CoreVector outQueueVector,
                int *hybrid) {
    auto nodeSet = new set<int>{};
    int intersectNodeNum = int(intersect.size());
    int *inactiveNodesList = new int[intersectNodeNum + 1];
    int inactiveNodeNum = 0;
    int *intersectList = new int[intersectNodeNum];
    bool *active = new bool[NodeNum];
    bool *inNodeSet = new bool[NodeNum];
    int **nodeToLayerDegree;
    nodeToLayerDegree = new int *[NodeNum];
    for (int tempNode = 0; tempNode < NodeNum; tempNode++) {
        active[tempNode] = false;
        inNodeSet[tempNode] = false;
        nodeToLayerDegree[tempNode] = new int[layerNum];
    }
    int j = 0;
    for (auto tempNode: intersect) {
        active[tempNode] = true;
        intersectList[j] = tempNode;
        j += 1;
    }

    for (j = 0; j < intersectNodeNum; j++) {
        auto tempNode = intersectList[j];
        bool inCore = true;
        for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
            int tempDegree = 0;
            auto neighborVector = mlg.getGraphList()[tempLayer].getNeighbor(tempNode);
            for (auto neighborVertex: neighborVector) {
                if (active[neighborVertex]) {
                    tempDegree += 1;
                }
            }
            nodeToLayerDegree[tempNode][tempLayer] = tempDegree;
            if (tempDegree < outQueueVector.vec[tempLayer]) {
                inCore = false;
            }
        }

        if (inCore) {
            inNodeSet[tempNode] = true;
        } else {
            active[tempNode] = false;
            inactiveNodeNum += 1;
            inactiveNodesList[inactiveNodeNum] = tempNode;

            while (inactiveNodeNum > 0) {
                auto toPeelVertex = inactiveNodesList[inactiveNodeNum];
                inactiveNodeNum -= 1;
                for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
                    auto neighborVector = mlg.getGraphList()[tempLayer].getNeighbor(toPeelVertex);
                    for (auto neighborVertex: neighborVector) {
                        if (!inNodeSet[neighborVertex]) continue;
                        nodeToLayerDegree[neighborVertex][tempLayer] -= 1;
                        if (nodeToLayerDegree[neighborVertex][tempLayer] < outQueueVector.vec[tempLayer] and
                            inNodeSet[neighborVertex]) {
                            inactiveNodeNum += 1;
                            inactiveNodesList[inactiveNodeNum] = neighborVertex;
                            active[neighborVertex] = false;
                            inNodeSet[neighborVertex] = false;
                        }
                    }
                }
            }
        }
    }
    for (int tempNode = 0; tempNode < NodeNum; tempNode++) {
        if (inNodeSet[tempNode]) nodeSet->insert(tempNode);
    }
    for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
        hybrid[tempLayer] = INT32_MAX;
        for (auto tempNode: *nodeSet) {
            if (hybrid[tempLayer] > nodeToLayerDegree[tempNode][tempLayer]) {
                hybrid[tempLayer] = nodeToLayerDegree[tempNode][tempLayer];
            }
        }
    }
    for (int tempNode = 0; tempNode < NodeNum; tempNode++) {
        delete nodeToLayerDegree[tempNode];
    }
    delete nodeToLayerDegree;
    delete[] active;
    delete[] inNodeSet;
    delete[] inactiveNodesList;
    return nodeSet;
}

map<CoreVector, set<int>, CVCompartor>
coreDecomposition(MultiLayerGraph mlg, CoreVector cv, int baseLayer, set<int> &nodes) {
    // 这个函数是一个peeling思路的函数，会保证除baseLayer以外的全部core-number为CoreVector中的number，
    // 只有baseLayer对应的那个core-number在不断变化。输入的nodes是参与的所有点，可以用来剪去那些不会影响结果的点。
    // 输出的结果是一个map，里面存（CoreVector，对应点集）

    // 声明
    int layerNum = mlg.getLayerNum();
    int nodeNum = mlg.getNodeNum();
    int baseLayerMaxDegree = mlg.getGraphList()[baseLayer].getMaxDeg();
    int currentCoreNum = int(nodes.size());
    auto currentCore = set<int>{};                          // 当前剩余的点集，peeling的时候会不断减少
    bool *inCurrentCore = new bool[nodeNum];                // 由于set的操作时间太久，我们用一个bool数组来表示
    int **layerNodeDegree;                                  // 维护目前的peeling子图中，每个点的degree
    // 以下是peeling用的bucket
    int *degreeBucket = new int[currentCoreNum + 1];        // 类似单层图peeling的实现，我们用一个数组加index来实现桶
    int *vertexToOrderIndex = new int[nodeNum];             // 简称Index，Order[Index[vertex]] = vertex
    int *degreeToFirstIndex = new int[baseLayerMaxDegree];  // 简称First，First[i]表示第一个度数为i的点在Order数组中的index
    int *coreNumList = new int[nodeNum];                    // 暂存core-num，最后恢复core
    map<CoreVector, set<int>, CVCompartor> ans = map<CoreVector, set<int>, CVCompartor>{};

    // 初始时子图所有点都在core中，非子图中点不在core中
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        inCurrentCore[tempNode] = false;
    }
    for (int tempNode: nodes) {
        inCurrentCore[tempNode] = true;
    }
    for (int tempNode = 0; tempNode < baseLayerMaxDegree; tempNode++) {
        degreeToFirstIndex[tempNode] = 0;
    }
    // 算出当前子图的layer-degree，并统计得到First
    layerNodeDegree = new int *[layerNum];
    for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
        layerNodeDegree[tempLayer] = new int[nodeNum];
    }
    for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
        for (auto tempNode: nodes) {
            int tempDegree = 0;
            auto neighborList = mlg.getGraphList()[tempLayer].getNeighbor(tempNode);
            for (auto neighborVertex: neighborList) {
                if (inCurrentCore[neighborVertex]) {
                    tempDegree++;
                }
            }
            if (tempLayer == baseLayer) {
                degreeToFirstIndex[tempDegree] += 1;
            }
            layerNodeDegree[tempLayer][tempNode] = tempDegree;
        }
    }
    // 然后计算出每个度数对应对第一个vertex的index
    for (int j = baseLayerMaxDegree; j > 0; j--) {
        degreeToFirstIndex[j] = degreeToFirstIndex[j - 1];
    }
    degreeToFirstIndex[0] = 0;
    for (int j = 1; j < baseLayerMaxDegree + 1; j++) {
        degreeToFirstIndex[j] += degreeToFirstIndex[j - 1];
    }
    // 最后根据index，扫描一边全部的点，把点放到index指示的为止上
    for (auto tempNode: nodes) {
        int degree = layerNodeDegree[baseLayer][tempNode];
        int index = degreeToFirstIndex[degree];
        degreeBucket[index] = tempNode;
        vertexToOrderIndex[tempNode] = index;
        degreeToFirstIndex[degree] += 1;
    }
    // 还原degreeCount
    for (int j = baseLayerMaxDegree; j > 0; j--) {
        degreeToFirstIndex[j] = degreeToFirstIndex[j - 1];
    }
    degreeToFirstIndex[0] = 0;

    int nowCoreNum = 0;
    // 开始peeling
    for (int j = 0; j < currentCoreNum; j++) {
        // 选出目前degree最小的点toPeelVertex
        auto toPeelVertex = degreeBucket[j];

        // toPeelVertex点度数是toPeelVertex的core-num
        while (nowCoreNum != layerNodeDegree[baseLayer][toPeelVertex]) {
            CoreVector newCV = *new CoreVector{cv.vec, cv.length};
            newCV.vec[baseLayer] = nowCoreNum + 1;
            set<int> initSet = set<int>{};
            for (auto tempNode: nodes) {
                if (inCurrentCore[tempNode]) {
                    initSet.insert(tempNode);
                }
            }
            ans.insert(pair<CoreVector, set<int>>{newCV, initSet});
            nowCoreNum += 1;
        }
        inCurrentCore[toPeelVertex] = false;
        nowCoreNum = layerNodeDegree[baseLayer][toPeelVertex];
        coreNumList[toPeelVertex] = layerNodeDegree[baseLayer][toPeelVertex];
        // 寻找toPeelVertex的全部邻居
        for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
            if (tempLayer == baseLayer) {
                // 本层的边，直接peeling
                auto neighborVector = mlg.getGraphList()[tempLayer].getNeighbor(toPeelVertex);
                for (auto neighborVertex: neighborVector) {
                    // 对于比toPeelVertex度数大的邻居进行更新
                    // 需要更新Order、First、Index三个数组和DegreeList
                    if (layerNodeDegree[baseLayer][neighborVertex] > nowCoreNum) {
                        // 首先更新Order：邻居点的Degree要-1，那么我们可以把邻居点和Degree的首个点交换位置，再把对应点First+1
                        int neighborVDegree = layerNodeDegree[baseLayer][neighborVertex];
                        int toSwapU = vertexToOrderIndex[neighborVertex];   // 找到邻居点在Order中的index-toSwapU
                        int toSwapV = degreeToFirstIndex[neighborVDegree];  // 找到邻居对应度数在Order中的index-toSwapV
                        // 交换toSwapU和toSwapV，这时邻居点已经被移到degree-1的最后
                        auto temp = degreeBucket[toSwapV];
                        degreeBucket[toSwapV] = degreeBucket[toSwapU];
                        degreeBucket[toSwapU] = temp;
                        // 更新Index：把交换位置的两个点的index也互相交换
                        vertexToOrderIndex[temp] = toSwapU;
                        vertexToOrderIndex[neighborVertex] = toSwapV;
                        // 更新First：这时邻居点已经被移到First[neighborVDegree]的位置上
                        // First[neighborVDegree] + 1，使邻居点划入neighborVDegree - 1中
                        degreeToFirstIndex[neighborVDegree] += 1;
                        // 更新DegreeList：这个邻居的度数-1
                        layerNodeDegree[baseLayer][neighborVertex] -= 1;
                    }
                }
            } else {
                // 外层的边，检查是否满足cv，不满足也要peeling
                auto neighborVector = mlg.getGraphList()[tempLayer].getNeighbor(toPeelVertex);
                for (auto neighborVertex: neighborVector) {
                    if (!inCurrentCore[neighborVertex]) {
                        continue;
                    }
                    auto toPeelVertexDegree = layerNodeDegree[tempLayer][neighborVertex];
                    if (toPeelVertexDegree >= cv.vec[tempLayer]) {
                        layerNodeDegree[tempLayer][neighborVertex] -= 1;
                        // 如果peel掉这个点之后，neighborVertex不满足cv了，要把这个点挪到现在的nowCoreNum下，等待peel
                        if (layerNodeDegree[tempLayer][neighborVertex] < cv.vec[tempLayer]) {
                            // 首先更新Order：邻居点的Degree要-1，那么我们可以把邻居点和Degree的首个点交换位置，再把对应点First+1
                            int neighborVDegree = layerNodeDegree[baseLayer][neighborVertex];
                            while (neighborVDegree != nowCoreNum) {
                                int toSwapU = vertexToOrderIndex[neighborVertex];   // 找到邻居点在Order中的index-toSwapU
                                int toSwapV = degreeToFirstIndex[neighborVDegree];  // 找到邻居对应度数在Order中的index-toSwapV
                                // 交换toSwapU和toSwapV，这时邻居点已经被移到degree-1的最后
                                auto temp = degreeBucket[toSwapV];
                                degreeBucket[toSwapV] = degreeBucket[toSwapU];
                                degreeBucket[toSwapU] = temp;
                                // 更新Index：把交换位置的两个点的index也互相交换
                                vertexToOrderIndex[temp] = toSwapU;
                                vertexToOrderIndex[neighborVertex] = toSwapV;
                                // 更新First：这时邻居点已经被移到First[neighborVDegree]的位置上
                                // First[neighborVDegree] + 1，使邻居点划入neighborVDegree - 1中
                                degreeToFirstIndex[neighborVDegree] += 1;
                                // 更新DegreeList：这个邻居的度数-1
                                layerNodeDegree[baseLayer][neighborVertex] -= 1;
                                neighborVDegree = layerNodeDegree[baseLayer][neighborVertex];
                            }

                        }
                    }
                }
            }

        }
    }
    delete[] inCurrentCore;
    for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
        delete layerNodeDegree[tempLayer];
    }
    delete layerNodeDegree;
    delete[] degreeBucket;
    delete[] vertexToOrderIndex;
    delete[] degreeToFirstIndex;
    delete[] coreNumList;
    return ans;
}

map<CoreVector, set<int>, CVCompartor>
coreDecompositionDS(MultiLayerGraph mlg, CoreVector cv, int baseLayer, set<int> &nodes) {
    // 这个函数是一个peeling思路的函数，会保证除baseLayer以外的全部core-number为CoreVector中的number，
    // 只有baseLayer对应的那个core-number在不断变化。输入的nodes是参与的所有点，可以用来剪去那些不会影响结果的点。
    // 输出的结果是一个map，里面存（CoreVector，对应点集）
    // 上面的函数是使用carefully designed的数据结构的。某些情况下存在bug。这个函数使用STL的数据结构实现。
    // 声明
    int layerNum = mlg.getLayerNum();
    int nodeNum = mlg.getNodeNum();
    int currentCoreSize = int(nodes.size());
    int baseLayerMaxDegree = mlg.getGraphList()[baseLayer].getMaxDeg() + 1;
    auto currentCore = set<int>{};                          // 当前剩余的点集，peeling的时候会不断减少
    bool *inCurrentCore = new bool[nodeNum];                // 由于set的操作时间太久，我们用一个bool数组来表示
    int **layerNodeDegree;                                  // 维护目前的peeling子图中，每个点的degree
    map<int, vector<int>> coreNumToNodesBucket = *new map<int, vector<int>>{};
    map<CoreVector, set<int>, CVCompartor> ans = map<CoreVector, set<int>, CVCompartor>{};

    for (int j = 0; j < baseLayerMaxDegree; j++) {
        auto v = *new vector<int>{};
        coreNumToNodesBucket.insert(pair<int, vector<int>>{j, v});
    }

    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        inCurrentCore[tempNode] = false;
    }
    for (int tempNode: nodes) {
        inCurrentCore[tempNode] = true;
    }
    // 算出当前子图的layer-degree
    layerNodeDegree = new int *[layerNum];
    for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
        layerNodeDegree[tempLayer] = new int[nodeNum];
    }
    for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
        for (auto tempNode: nodes) {
            int tempDegree = 0;
            auto neighborList = mlg.getGraphList()[tempLayer].getNeighbor(tempNode);
            for (auto neighborVertex: neighborList) {
                if (inCurrentCore[neighborVertex]) {
                    tempDegree++;
                }
            }
            layerNodeDegree[tempLayer][tempNode] = tempDegree;
            // coreNumToNodesBucket[node_degrees[layer]].add(node)
            if (tempLayer == baseLayer) {
                coreNumToNodesBucket.find(tempDegree)->second.push_back(tempNode);
            }
        }
    }

    map<int, vector<int>>::iterator bucketIter;
    bucketIter = coreNumToNodesBucket.begin();
    while (bucketIter != coreNumToNodesBucket.end()) {
        auto index = bucketIter->first;
        while (!bucketIter->second.empty()) {
            int tempNode = bucketIter->second.front();
            auto k = bucketIter->second.begin();
            bucketIter->second.erase(k);
            inCurrentCore[tempNode] = false;
            currentCoreSize -= 1;

            for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
                auto neighborVector = mlg.getGraphList()[tempLayer].getNeighbor(tempNode);
                for (auto neighborVertex: neighborVector) {
                    if (inCurrentCore[neighborVertex]) {
                        int neighborVertexDegree = layerNodeDegree[tempLayer][neighborVertex];

                        if (tempLayer == baseLayer and neighborVertexDegree > index) {
                            k = std::find(coreNumToNodesBucket.find(neighborVertexDegree)->second.begin(),
                                          coreNumToNodesBucket.find(neighborVertexDegree)->second.end(),
                                          neighborVertex);
                            coreNumToNodesBucket.find(neighborVertexDegree)->second.erase(k);
                            coreNumToNodesBucket.find(neighborVertexDegree - 1)->second.push_back(neighborVertex);
                            layerNodeDegree[tempLayer][neighborVertex] -= 1;
                        } else if (tempLayer != baseLayer and neighborVertexDegree >= cv.vec[tempLayer]) {
                            layerNodeDegree[tempLayer][neighborVertex] -= 1;
                            if (layerNodeDegree[tempLayer][neighborVertex] < cv.vec[tempLayer]) {
                                int toFindDegree = layerNodeDegree[baseLayer][neighborVertex];
                                k = std::find(coreNumToNodesBucket.find(toFindDegree)->second.begin(),
                                              coreNumToNodesBucket.find(toFindDegree)->second.end(),
                                              neighborVertex);
                                coreNumToNodesBucket.find(toFindDegree)->second.erase(k);
                                coreNumToNodesBucket.find(index)->second.push_back(neighborVertex);
                                layerNodeDegree[baseLayer][neighborVertex] = index;
                            }
                        }
                    }
                }
            }
        }
        if (currentCoreSize > 0) {
            CoreVector newCV = *new CoreVector{cv.vec, cv.length};
            newCV.vec[baseLayer] = index + 1;
            set<int> initSet = set<int>{};
            for (auto tempNode: nodes) {
                if (inCurrentCore[tempNode]) {
                    initSet.insert(tempNode);
                }
            }
            ans.insert(pair<CoreVector, set<int>>{newCV, initSet});
        }
        bucketIter++;
    }
    delete[] inCurrentCore;
    for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
        delete layerNodeDegree[tempLayer];
    }
    delete layerNodeDegree;
    return ans;
}

map<CoreVector, set<int>, CVCompartor>
pureCoreDecomposition(MultiLayerGraph mlg, CoreVector cv, int baseLayer) {
    int layerNum = mlg.getLayerNum();
    int nodeNum = mlg.getNodeNum();
    int currentCoreSize = nodeNum;
    int baseLayerMaxDegree = mlg.getGraphList()[baseLayer].getMaxDeg() + 1;
    bool *inCurrentCore = new bool[nodeNum];                // 由于set的操作时间太久，我们用一个bool数组来表示
    int *baseLayerNodeDegree = new int[nodeNum];            // 维护目前的peeling子图中，每个点的degree
    map<int, vector<int>> coreNumToNodesBucket = *new map<int, vector<int>>{};
    map<CoreVector, set<int>, CVCompartor> ans = map<CoreVector, set<int>, CVCompartor>{};

    for (int j = 0; j < baseLayerMaxDegree; j++) {
        auto v = *new vector<int>{};
        coreNumToNodesBucket.insert(pair<int, vector<int>>{j, v});
    }

    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        inCurrentCore[tempNode] = true;
    }
    // 算出当前子图的layer-degree
    auto tempBaseLayerNodeDegree = mlg.getGraphList()[baseLayer].getNodeDegreeList();
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        int tempDegree = tempBaseLayerNodeDegree[tempNode];
        baseLayerNodeDegree[tempNode] = tempDegree;
        coreNumToNodesBucket.find(tempDegree)->second.push_back(tempNode);
    }

    map<int, vector<int>>::iterator bucketIter;
    bucketIter = coreNumToNodesBucket.begin();
    while (bucketIter != coreNumToNodesBucket.end()) {
        auto index = bucketIter->first;
        while (!bucketIter->second.empty()) {
            int tempNode = bucketIter->second.front();
            auto k = bucketIter->second.begin();
            bucketIter->second.erase(k);
            inCurrentCore[tempNode] = false;
            currentCoreSize -= 1;

            auto neighborVector = mlg.getGraphList()[baseLayer].getNeighbor(tempNode);
            for (auto neighborVertex: neighborVector) {
                if (inCurrentCore[neighborVertex] and baseLayerNodeDegree[neighborVertex] > index) {
                    int neighborVertexDegree = baseLayerNodeDegree[neighborVertex];
                    k = std::find(coreNumToNodesBucket.find(neighborVertexDegree)->second.begin(),
                                  coreNumToNodesBucket.find(neighborVertexDegree)->second.end(),
                                  neighborVertex);
                    coreNumToNodesBucket.find(neighborVertexDegree)->second.erase(k);
                    coreNumToNodesBucket.find(neighborVertexDegree - 1)->second.push_back(neighborVertex);
                    baseLayerNodeDegree[neighborVertex] -= 1;
                }
            }
        }

        if (currentCoreSize > 0) {
            CoreVector newCV = *new CoreVector{cv.vec, cv.length};
            newCV.vec[baseLayer] = index + 1;
            set<int> initSet = set<int>{};
            for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
                if (inCurrentCore[tempNode]) {
                    initSet.insert(tempNode);
                }
            }
            ans.insert(pair<CoreVector, set<int>>{newCV, initSet});
        }
        bucketIter++;
    }
    delete[] inCurrentCore;
    delete[] baseLayerNodeDegree;
    return ans;
}

void bottomUpVisit(CoreVector startVector, CoreVector endVector, int layerNum, Core &core) {
    auto CVQueue = new queue<CoreVector>{};
    CVQueue->push(startVector);
    auto processedCV = new set<CoreVector, CVCompartor>{};
    processedCV->insert(startVector);
    auto newSet = core.getCore(endVector);

    while (!CVQueue->empty()) {
        auto tempCV = CVQueue->front();
        CVQueue->pop();
        if (!core.hasCore(tempCV)) {
            core.addCore(tempCV, newSet);
        }

        for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
            if (tempCV.vec[tempLayer] > (endVector.vec[tempLayer] + 1)) {
                auto ancestorVector = tempCV.build_ancestor_vector(tempLayer);
                if (!core.hasCore(ancestorVector) and processedCV->count(ancestorVector) == 0) {
                    CVQueue->push(ancestorVector);
                    processedCV->insert(ancestorVector);
                }
            }
        }
    }
}

void decrementDescendantCount(CoreVector cv, map<CoreVector, int, CVCompartor> *descendant_count) {
    int count = descendant_count->find(cv)->second;
    descendant_count->erase(cv);
    if (count - 1 != 0) {
        descendant_count->insert(pair<CoreVector, int>(cv, count - 1));
    }
}

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
    int **nodeToLayerDegree;
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

void bfsMLGCoreDecomposition(MultiLayerGraph mlg) {
    Timer AlgoTimer{};
    AlgoTimer.startTimer();
    int layerNum = mlg.getLayerNum();
    int nodeNum = mlg.getNodeNum();
    int numberOfComputedCores = 0;
    int *hybrid = new int[layerNum];

    CoreVector startVector = *new CoreVector(layerNum);
    auto nodeSet = new set<int>{};
    for (int j = 0; j < nodeNum; j++) {
        nodeSet->insert(j);
    }

    // core[0]
    int NumberOfCores = 0;
    Core coreSet = *new Core(mlg.getLayerNum());
    coreSet.addCore(startVector, *nodeSet);

    // initialize
    auto vectorsQueue = queue<CoreVector>{};
    auto ancestors = map<CoreVector, vector<CoreVector>, CVCompartor>{};
    for (int j = 0; j < layerNum; j++) {
        auto tempVector = startVector.build_descendant_vector(j);
        vectorsQueue.push(tempVector);
        auto fatherVectors = vector<CoreVector>{startVector};
        ancestors.insert(pair<CoreVector, vector<CoreVector>>(tempVector, fatherVectors));
    }
    auto descendant_count = map<CoreVector, int, CVCompartor>{};
    descendant_count.insert(pair<CoreVector, int>(startVector, layerNum));

    while (!vectorsQueue.empty()) {
        auto outQueueVector = vectorsQueue.front();
        vectorsQueue.pop();
        int numberOfNonZeroIndexes = outQueueVector.get_non_zero_index();
        int numberOfAncestors = int(ancestors.find(outQueueVector)->second.size());
        if (numberOfAncestors == numberOfNonZeroIndexes) {
            auto ancestorsVector = ancestors.find(outQueueVector)->second;
            auto ancestor0 = ancestorsVector.front();
            auto intersect = coreSet.getCore(ancestor0);
            decrementDescendantCount(ancestor0, &descendant_count);
            for (int j = 1; j < ancestorsVector.size(); j++) {
                auto tempCV = ancestorsVector[j];
                auto s = coreSet.getCore(tempCV);
                set_intersection(intersect.begin(), intersect.end(), s.begin(), s.end(),
                                 inserter(intersect, intersect.begin()));
                decrementDescendantCount(tempCV, &descendant_count);
            }
            if (!intersect.empty()) {
                // compute kcore on intersect
                nodeSet = kCore(mlg, intersect, layerNum, nodeNum, outQueueVector, hybrid);
                numberOfComputedCores += 1;
            } else {
                ancestors.erase(outQueueVector);
                continue;
            }
            if (!nodeSet->empty()) {
                coreSet.addCore(outQueueVector, *nodeSet);
                for (int j = 0; j < layerNum; j++) {
                    auto tempVector = outQueueVector.build_descendant_vector(j);
                    if (ancestors.count(tempVector) > 0) {
                        ancestors.find(tempVector)->second.push_back(outQueueVector);
                    } else {
                        vectorsQueue.push(tempVector);
                        auto fatherVectors = vector<CoreVector>{outQueueVector};
                        ancestors.insert(pair<CoreVector, vector<CoreVector>>(tempVector, fatherVectors));
                    }
                    int count = descendant_count.find(outQueueVector)->second;
                    descendant_count.erase(outQueueVector);
                    descendant_count.insert(pair<CoreVector, int>(outQueueVector, count + 1));
                }
            }

        } else {
            for (auto ancestorCV: ancestors.find(outQueueVector)->second) {
                decrementDescendantCount(ancestorCV, &descendant_count);
            }
        }
        ancestors.erase(outQueueVector);
    }
    AlgoTimer.endTimer();
    std::cout << "BFS Time: " << AlgoTimer.getTimerSecond() << " s." << std::endl;
    std::cout << "Computed core number: " << numberOfComputedCores << endl;
    std::cout << "Core number: " << coreSet.getSize() << endl;
    coreSet.printCore();
    coreSet.printCoreNum(nodeNum);
}

void dfsMLGCoreDecomposition(MultiLayerGraph mlg) {
    Timer AlgoTimer{};
    AlgoTimer.startTimer();
    int layerNum = mlg.getLayerNum();
    int nodeNum = mlg.getNodeNum();
    int numberOfComputedCores = 0;

    CoreVector startVector = *new CoreVector(layerNum);
    auto nodeSet = new set<int>{};
    for (int j = 0; j < nodeNum; j++) {
        nodeSet->insert(j);
    }

    // core[0]
    int NumberOfCores = 0;
    Core coreSet = *new Core(mlg.getLayerNum());
    coreSet.addCore(startVector, *nodeSet);

    // initialize
    auto layerSet = set<int>{};
    auto baseLayerSet = set<int>{};
    for (int j = 0; j < layerNum; j++) {
        layerSet.insert(j);
        baseLayerSet.insert(j);
    }
    set<int> initSet = set<int>{};
    for (int i = 0; i < nodeNum; ++i) {
        initSet.insert(i);
    }
    map<CoreVector, set<int>, CVCompartor> baseCores = map<CoreVector, set<int>, CVCompartor>{};
    baseCores.insert(pair<CoreVector, set<int>>{startVector, initSet});

    auto computedCoreVectors = set<CoreVector, CVCompartor>{startVector};

    while (!baseLayerSet.empty()) {
        baseLayerSet.erase(baseLayerSet.begin());
        auto tempBaseCores = map<CoreVector, set<int>, CVCompartor>{};
        map<CoreVector, set<int>>::iterator iter;
        iter = baseCores.begin();
        while (iter != baseCores.end()) {
            for (auto baseLayer: baseLayerSet) {
                if (iter->first.vec[baseLayer] == 0) {
                    auto newCores = coreDecompositionDS(mlg, iter->first, baseLayer, iter->second);
                    map<CoreVector, set<int>>::iterator iter2;
                    iter2 = newCores.begin();
                    while (iter2 != newCores.end()) {
                        coreSet.addCore(iter2->first, iter2->second);
                        tempBaseCores.insert(pair<CoreVector, set<int>>{iter2->first, iter2->second});
                        iter2++;
                    }
                    numberOfComputedCores += int(newCores.size());
                }
            }
            auto addLayerSet = set<int>{};
            for (auto baseLayer: layerSet) {
                if (baseLayerSet.count(baseLayer) == 0) {
                    addLayerSet.insert(baseLayer);
                }
            }
            for (auto baseLayer: addLayerSet) {
                if (iter->first.vec[baseLayer] == 0) {
                    auto newCores = coreDecompositionDS(mlg, iter->first, baseLayer, iter->second);
                    map<CoreVector, set<int>>::iterator iter2;
                    iter2 = newCores.begin();
                    while (iter2 != newCores.end()) {
                        coreSet.addCore(iter2->first, iter2->second);
                        tempBaseCores.insert(pair<CoreVector, set<int>>{iter2->first, iter2->second});
                        iter2++;
                    }
                    numberOfComputedCores += int(newCores.size());
                }
            }
            iter++;
        }
        baseCores = tempBaseCores;
    }

    AlgoTimer.endTimer();
    std::cout << "DFS Time: " << AlgoTimer.getTimerSecond() << " s." << std::endl;
    std::cout << "Computed core number: " << numberOfComputedCores << endl;
    std::cout << "Core number: " << coreSet.getSize() << endl;
    coreSet.printCore();
}

void hybridMLGCoreDecomposition(MultiLayerGraph mlg) {
    Timer AlgoTimer{};
    AlgoTimer.startTimer();
    int layerNum = mlg.getLayerNum();
    int nodeNum = mlg.getNodeNum();
    int numberOfComputedCores = 0;
    int *hybrid = new int[layerNum];

    CoreVector startVector = *new CoreVector(layerNum);
    auto nodeSet = new set<int>{};
    for (int j = 0; j < nodeNum; j++) {
        nodeSet->insert(j);
    }

    // core[0]
    int NumberOfCores = 0;
    Core coreSet = *new Core(mlg.getLayerNum());
    coreSet.addCore(startVector, *nodeSet);

    auto vectorsQueue = queue<CoreVector>{};
    auto ancestors = map<CoreVector, vector<CoreVector>, CVCompartor>{};
    for (int j = 0; j < layerNum; j++) {
        auto tempVector = startVector.build_descendant_vector(j);
        vectorsQueue.push(tempVector);
        auto fatherVectors = vector<CoreVector>{startVector};
        ancestors.insert(pair<CoreVector, vector<CoreVector>>(tempVector, fatherVectors));
    }

    auto descendant_count = map<CoreVector, int, CVCompartor>{};

    for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
        auto newCores = pureCoreDecomposition(mlg, startVector, tempLayer);
        map<CoreVector, set<int>>::iterator iter2;
        iter2 = newCores.begin();
        while (iter2 != newCores.end()) {
            coreSet.addCore(iter2->first, iter2->second);
            iter2++;
        }
        numberOfComputedCores += int(newCores.size());
    }

    while (!vectorsQueue.empty()) {
        auto outQueueVector = vectorsQueue.front();
        vectorsQueue.pop();
        int numberOfNonZeroIndexes = outQueueVector.get_non_zero_index();
        int numberOfAncestors = int(ancestors.find(outQueueVector)->second.size());
        if (numberOfAncestors == numberOfNonZeroIndexes and numberOfNonZeroIndexes > 1 and
            not coreSet.hasCore(outQueueVector)) {
            auto ancestorsVector = ancestors.find(outQueueVector)->second;
            auto ancestor0 = ancestorsVector.front();
            auto intersect = coreSet.getCore(ancestor0);
            decrementDescendantCount(ancestor0, &descendant_count);
            for (int j = 1; j < ancestorsVector.size(); j++) {
                auto tempCV = ancestorsVector[j];
                auto s = coreSet.getCore(tempCV);
                set_intersection(intersect.begin(), intersect.end(), s.begin(), s.end(),
                                 inserter(intersect, intersect.begin()));
                decrementDescendantCount(tempCV, &descendant_count);
            }

            if (!intersect.empty()) {
                nodeSet = kCore(mlg, intersect, layerNum, nodeNum, outQueueVector, hybrid);
                numberOfComputedCores += 1;
                if (!nodeSet->empty()) {
                    coreSet.addCore(outQueueVector, *nodeSet);
                    for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
                        if (hybrid[tempLayer] != outQueueVector.vec[tempLayer]) {
                            auto *tempcv = new CoreVector(hybrid, outQueueVector.length);
                            bottomUpVisit(*tempcv, outQueueVector, layerNum, coreSet);
                        }
                    }
                }
            }

        } else {
            for (auto ancestorCV: ancestors.find(outQueueVector)->second) {
                decrementDescendantCount(ancestorCV, &descendant_count);
            }
        }

        if (coreSet.hasCore(outQueueVector)) {
            for (int tempLayer = 0; tempLayer < layerNum; tempLayer++) {
                auto descendantVector = outQueueVector.build_descendant_vector(tempLayer);
                auto iter = ancestors.find(descendantVector);
                if (iter != ancestors.end()) {
                    iter->second.push_back(outQueueVector);
                } else {
                    vectorsQueue.push(descendantVector);
                    auto fatherVectors = vector<CoreVector>{outQueueVector};
                    ancestors.insert(pair<CoreVector, vector<CoreVector>>(descendantVector, fatherVectors));
                }
                descendant_count.find(outQueueVector)->second++;
            }
        }

        ancestors.erase(outQueueVector);
    }

    AlgoTimer.endTimer();
    std::cout << "Hybrid Time: " << AlgoTimer.getTimerSecond() << " s." << std::endl;
    std::cout << "Computed core number: " << numberOfComputedCores << endl;
    std::cout << "Core number: " << coreSet.getSize() << endl;
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

int *naivePeelingCoreDecomposition(Graph g, bool printResult) {
    int nodeNum = g.getNodeNum();
    int maxDegree = g.getMaxDeg();
    auto degreeList = g.getNodeDegreeList();
    int *coreNumList = new int[nodeNum];

    map<int, vector<int>> coreNumToNodeBucket;

    for (int tempDegree = 0; tempDegree < maxDegree; tempDegree++) {
        coreNumToNodeBucket[tempDegree] = vector<int>{};
    }

    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        coreNumToNodeBucket[degreeList[tempNode]].push_back(tempNode);
    }

    for (int tempDegree = 0; tempDegree <= maxDegree; tempDegree++) {
        while (!coreNumToNodeBucket[tempDegree].empty()) {
            auto tempNode = coreNumToNodeBucket[tempDegree].front();
            coreNumToNodeBucket[tempDegree].erase(coreNumToNodeBucket[tempDegree].begin());
            coreNumList[tempNode] = tempDegree;
            for (auto neighborVertex: g.getNeighbor(tempNode)) {
                int tempVDegree = degreeList[neighborVertex];
                if (tempVDegree > tempDegree) {
                    coreNumToNodeBucket[tempVDegree].erase(
                            std::find(coreNumToNodeBucket[tempVDegree].begin(), coreNumToNodeBucket[tempVDegree].end(),
                                      neighborVertex));
                    coreNumToNodeBucket[tempVDegree - 1].push_back(neighborVertex);
                    degreeList[neighborVertex]--;
                }
            }
        }
    }

    if (printResult) {
        cout << "Core Number as Follow: " << endl;
        printList(coreNumList, nodeNum);
    }
    return coreNumList;
}

int *naiveVertexCentricCoreDecomposition(Graph g, bool printResult) {
    int nodeNum = g.getNodeNum();
    int maxDegree = g.getMaxDeg();

    int *coreNumList = g.getNodeDegreeList();           // 最终的结果，即每个点对应点core-num
    vector<bool> activeList;                            // 被激活的点

    vector<queue<SingleCoreMessage>> messageList;
    vector<map<int, set<int>>> vertexInfo;

    int BSPNum = 0;

    // Init
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        messageList.emplace_back();
        vertexInfo.emplace_back();
        activeList.push_back(true);
    }
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        auto tempNodeNeighbors = g.getNeighbor(tempNode);
        for (auto neighborV: tempNodeNeighbors) {
            auto tempMessage = new SingleCoreMessage{tempNode, 0, coreNumList[tempNode]};
            messageList[neighborV].push(*tempMessage);
        }
    }

    // 循环直至所有点都不再active
    bool active = true;
    while (active) {
        vector<bool> newActiveList;
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            newActiveList.push_back(false);
        }
        int activeNum = 0;
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            // 处理每一个被active的点（也就是收到信息的点）
            if (activeList[tempNode]) {
                activeNum++;
                // 把所有信息收集起来，更改目前的邻居的状态（也就是本点邻居目前的coreness）
                while (!messageList[tempNode].empty()) {
                    auto tempMessage = messageList[tempNode].front();
                    messageList[tempNode].pop();
                    if (tempMessage.oldCoreNum) {
                        auto VSet = &vertexInfo[tempNode][tempMessage.oldCoreNum];
                        VSet->erase(VSet->find(tempMessage.vertexID));
                    }
                    vertexInfo[tempNode][tempMessage.newCoreNum].insert(tempMessage.vertexID);
                }

                // 检查目前邻居的coreness还能不能支持现有的coreness，并计算出新的coreness
                int oldCoreness = coreNumList[tempNode];
                int nowCoreness = coreNumList[tempNode];
                int support = 0;
                for (int j = g.getMaxDeg(); j >= 0; j--) {
                    support += int(vertexInfo[tempNode][j].size());
                    if (support >= nowCoreness) {
                        break;
                    }
                    if (nowCoreness == j) {
                        nowCoreness--;
                    }
                }

                // 如果发生了变化，需要向邻居节点发送变化，并激活邻居节点
                if (oldCoreness != nowCoreness) {
                    coreNumList[tempNode] = nowCoreness;
                    auto tempNodeNeighbors = g.getNeighbor(tempNode);
                    for (auto neighborV: tempNodeNeighbors) {
                        auto tempMessage = *new SingleCoreMessage{tempNode, oldCoreness, nowCoreness};
                        messageList[neighborV].push(tempMessage);
                        newActiveList[neighborV] = true;
                    }
                }
            }
        }

        // barrier阶段，更换active-list，并检查结束条件
        active = false;
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            activeList[tempNode] = newActiveList[tempNode];
            active = active or newActiveList[tempNode];
        }
        if (active) {
            activeList = newActiveList;
        }
        cout << "In round-" << BSPNum << ", " << activeNum << " nodes activated." << endl;
        BSPNum++;
    }

    if (printResult) {
        cout << "Vertex Centric Core Number as Follow: " << endl;
        printList(coreNumList, nodeNum);
    }
    return coreNumList;
}

int *vertexCentricCoreDecomposition(Graph g, bool printResult) {
    int nodeNum = g.getNodeNum();
    int maxDegree = g.getMaxDeg();

    int *tempCoreNumList = g.getNodeDegreeList();           // 最终的结果，即每个点对应点core-num
    int *coreNumList = new int[nodeNum];
    bool *activeList = new bool[nodeNum];               // 被激活的点
    bool *tempActiveList = new bool[nodeNum];

    int **nodeToCoreInfoMat = new int *[nodeNum];

    int BSPNum = 0;
    Timer timer{};

    // Init
    timer.startTimer();
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        coreNumList[tempNode] = tempCoreNumList[tempNode];
        nodeToCoreInfoMat[tempNode] = new int[maxDegree];
        for (int j = 0; j <= maxDegree; j++) {
            nodeToCoreInfoMat[tempNode][j] = 0;
        }
        activeList[tempNode] = true;
    }
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        auto tempNodeNeighbors = g.getNeighbor(tempNode);
        for (auto neighborV: tempNodeNeighbors) {
            nodeToCoreInfoMat[neighborV][coreNumList[tempNode]] += 1;
        }
    }
    timer.endTimer();
    cout << "Load Time = " << timer.getTimerSecond() << "s." << endl;
    // 循环直至所有点都不再active
    timer.startTimer();
    bool active = true;
    while (active) {
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            tempActiveList[tempNode] = false;
        }
        int activeNum = 0;
        int msgNum = 0;
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            // 处理每一个被active的点（也就是收到信息的点）
            if (activeList[tempNode]) {
                activeNum++;

                // 检查目前邻居的coreness还能不能支持现有的coreness，并计算出新的coreness
                int oldCoreness = coreNumList[tempNode];
                int nowCoreness = coreNumList[tempNode];
                int support = 0;
                // check now
                for (int j = nowCoreness; j <= maxDegree; j++) {
                    support += nodeToCoreInfoMat[tempNode][j];
                }
                while (support < nowCoreness) {
                    nowCoreness--;
                    support += nodeToCoreInfoMat[tempNode][nowCoreness];
                }

                // 如果发生了变化，需要向邻居节点发送变化，并激活邻居节点
                if (oldCoreness != nowCoreness) {
                    coreNumList[tempNode] = nowCoreness;
                    auto tempNodeNeighbors = g.getNeighbor(tempNode);
                    for (auto neighborV: tempNodeNeighbors) {
                        nodeToCoreInfoMat[neighborV][oldCoreness]--;
                        nodeToCoreInfoMat[neighborV][nowCoreness]++;
                        tempActiveList[neighborV] = true;
                        msgNum++;
                    }
                }
            }
        }

        // barrier阶段，更换active-list，并检查结束条件
        active = false;
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            activeList[tempNode] = tempActiveList[tempNode];
            active = active or tempActiveList[tempNode];
        }
        cout << "In round-" << BSPNum << ", " << activeNum << " nodes activated with " << msgNum << " msg." << endl;
        BSPNum++;
    }
    timer.endTimer();
    cout << "Cal Time = " << timer.getTimerSecond() << "s." << endl;
    if (printResult) {
        cout << "Vertex Centric Core Number as Follow: " << endl;
        printList(coreNumList, nodeNum);
    }
    return coreNumList;
}

int *optVertexCentricCoreDecomposition(Graph g, bool printResult) {
    int nodeNum = g.getNodeNum();
    int maxDegree = g.getMaxDeg();

    int *tempCoreNumList = g.getNodeDegreeList();           // 最终的结果，即每个点对应点core-num
    int *coreNumList = new int[nodeNum];
    bool *activeList = new bool[nodeNum];               // 被激活的点
    bool *tempActiveList = new bool[nodeNum];

    int **nodeToCoreInfoMat = new int *[nodeNum];

    int BSPNum = 0;
    Timer timer{};

    // Init
    timer.startTimer();
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        coreNumList[tempNode] = tempCoreNumList[tempNode];
        int length = coreNumList[tempNode];
        nodeToCoreInfoMat[tempNode] = new int[length + 1];
        for (int j = 0; j <= length; j++) {
            nodeToCoreInfoMat[tempNode][j] = 0;
        }
        activeList[tempNode] = true;
    }
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        auto tempNodeNeighbors = g.getNeighbor(tempNode);
        for (auto neighborV: tempNodeNeighbors) {
            int j = coreNumList[tempNode] > coreNumList[neighborV] ? coreNumList[neighborV] : coreNumList[tempNode];
            nodeToCoreInfoMat[neighborV][j] += 1;
        }
    }
    timer.endTimer();
    if (printResult) cout << "Load Time = " << timer.getTimerSecond() << "s." << endl;
    // 循环直至所有点都不再active
    timer.startTimer();
    bool active = true;
    while (active) {
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            tempActiveList[tempNode] = false;
        }
        int activeNum = 0;
        int msgNum = 0;
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            // 处理每一个被active的点（也就是收到信息的点）
            if (activeList[tempNode]) {
                activeNum++;

                // 检查目前邻居的coreness还能不能支持现有的coreness，并计算出新的coreness
                int oldCoreness = coreNumList[tempNode];
                int nowCoreness = coreNumList[tempNode];
                int support = 0;
                // check now
                support += nodeToCoreInfoMat[tempNode][nowCoreness];
                while (support < nowCoreness) {
                    nowCoreness--;
                    support += nodeToCoreInfoMat[tempNode][nowCoreness];
                }
                nodeToCoreInfoMat[tempNode][nowCoreness] = support;

                // 如果发生了变化，需要向邻居节点发送变化，并激活邻居节点
                if (oldCoreness != nowCoreness) {
                    coreNumList[tempNode] = nowCoreness;
                    auto tempNodeNeighbors = g.getNeighbor(tempNode);
                    for (auto neighborV: tempNodeNeighbors) {
                        if (oldCoreness >= coreNumList[neighborV] and
                            nowCoreness >= coreNumList[neighborV]) { continue; }
                        int j = oldCoreness > coreNumList[neighborV] ? coreNumList[neighborV] : oldCoreness;
                        nodeToCoreInfoMat[neighborV][j]--;
                        j = nowCoreness > coreNumList[neighborV] ? coreNumList[neighborV] : nowCoreness;
                        nodeToCoreInfoMat[neighborV][j]++;
                        tempActiveList[neighborV] = true;
                        msgNum++;
                    }
                }
            }
        }
        // barrier阶段，更换active-list，并检查结束条件
        active = false;
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            activeList[tempNode] = tempActiveList[tempNode];
            active = active or tempActiveList[tempNode];
        }
        if (printResult)
            cout << "In round-" << BSPNum << ", " << activeNum << " nodes activated with " << msgNum << " msg." << endl;
        BSPNum++;
    }
    timer.endTimer();
    if (printResult) cout << "Cal Time = " << timer.getTimerSecond() << "s." << endl;
    if (printResult) {
        cout << "Vertex Centric Core Number as Follow: " << endl;
        printList(coreNumList, nodeNum);
    }
    delete[]activeList;
    delete[]tempActiveList;
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        delete[] nodeToCoreInfoMat[tempNode];
    }
    delete[] nodeToCoreInfoMat;
    return coreNumList;
}

int *lazyVertexCentricCoreDecomposition(Graph g, bool printResult) {
    int nodeNum = g.getNodeNum();
    int maxDegree = g.getMaxDeg();

    int *tempCoreNumList = g.getNodeDegreeList();           // 最终的结果，即每个点对应点core-num
    int *coreNumList = new int[nodeNum];
    bool *activeList = new bool[nodeNum];               // 被激活的点
    bool *tempActiveList = new bool[nodeNum];

    int **nodeToCoreInfoMat = new int *[nodeNum];

    int BSPNum = 0;
    Timer timer{};

    // Init
    timer.startTimer();
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        coreNumList[tempNode] = tempCoreNumList[tempNode];
        int length = coreNumList[tempNode];
        nodeToCoreInfoMat[tempNode] = new int[length + 1];
        for (int j = 0; j <= length; j++) {
            nodeToCoreInfoMat[tempNode][j] = 0;
        }
        activeList[tempNode] = true;
    }
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        auto tempNodeNeighbors = g.getNeighbor(tempNode);
        for (auto neighborV: tempNodeNeighbors) {
            int j = coreNumList[tempNode] > coreNumList[neighborV] ? coreNumList[neighborV] : coreNumList[tempNode];
            nodeToCoreInfoMat[neighborV][j] += 1;
        }
    }
    timer.endTimer();
    cout << "Load Time = " << timer.getTimerSecond() << "s." << endl;
    // 循环直至所有点都不再active
    timer.startTimer();
    bool active = true;
    while (active) {
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            tempActiveList[tempNode] = false;
        }
        int activeNum = 0;
        int msgNum = 0;
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            // 处理每一个被active的点（也就是收到信息的点）
            if (activeList[tempNode]) {
                activeNum++;

                // 检查目前邻居的coreness还能不能支持现有的coreness，并计算出新的coreness
                int oldCoreness = coreNumList[tempNode];
                int nowCoreness = coreNumList[tempNode];
                int support = 0;
                // check now
                support += nodeToCoreInfoMat[tempNode][nowCoreness];
                while (support < nowCoreness) {
                    nowCoreness--;
                    support += nodeToCoreInfoMat[tempNode][nowCoreness];
                }
                nodeToCoreInfoMat[tempNode][nowCoreness] = support;

                // 如果发生了变化，需要向邻居节点发送变化，并激活邻居节点
                if (oldCoreness != nowCoreness) {
                    coreNumList[tempNode] = nowCoreness;
                    auto tempNodeNeighbors = g.getNeighbor(tempNode);
                    for (auto neighborV: tempNodeNeighbors) {
                        if (oldCoreness >= coreNumList[neighborV] and
                            nowCoreness >= coreNumList[neighborV]) { continue; }
                        int j = oldCoreness > coreNumList[neighborV] ? coreNumList[neighborV] : oldCoreness;
                        nodeToCoreInfoMat[neighborV][j]--;
                        j = nowCoreness > coreNumList[neighborV] ? coreNumList[neighborV] : nowCoreness;
                        nodeToCoreInfoMat[neighborV][j]++;
                        msgNum++;
                    }
                }
            }
        }
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            int oldCoreness = coreNumList[tempNode];
            if (nodeToCoreInfoMat[tempNode][oldCoreness] < oldCoreness)
                tempActiveList[tempNode] = true;
        }
        // barrier阶段，更换active-list，并检查结束条件
        active = false;
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            activeList[tempNode] = tempActiveList[tempNode];
            active = active or tempActiveList[tempNode];
        }
        cout << "In round-" << BSPNum << ", " << activeNum << " nodes activated with " << msgNum << " msg." << endl;
        BSPNum++;
    }
    timer.endTimer();
    cout << "Cal Time = " << timer.getTimerSecond() << "s." << endl;
    if (printResult) {
        cout << "Vertex Centric Core Number as Follow: " << endl;
        printList(coreNumList, nodeNum);
    }
    delete[]activeList;
    delete[]tempActiveList;
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        delete[] nodeToCoreInfoMat[tempNode];
    }
    delete[] nodeToCoreInfoMat;
    return coreNumList;
}

int *mpSingleThreadVertexCentricCoreDecomposition(Graph g, bool printResult) {
    int threadNum = 1;
    int nodeNum = g.getNodeNum();
    int maxDegree = g.getMaxDeg();

    int *tempCoreNumList = g.getNodeDegreeList();           // 最终的结果，即每个点对应点core-num
    int *coreNumList = new int[nodeNum];

    int BSPNum = 0;
    int activeNum = nodeNum;
    int msgNum = 0;

    auto *infoMat = new SimpleCoreInfo[nodeNum];

    for (int i = 0; i < nodeNum; i++) {
        infoMat[i].initSCI(maxDegree, threadNum, tempCoreNumList[i]);
    }

    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        // 首先初始化
        auto tempNodeNeighbors = g.getNeighbor(tempNode);
        for (auto neighborV: tempNodeNeighbors) {
            SimpleCoreMessage msg = SimpleCoreMessage{0, tempCoreNumList[tempNode]};
            infoMat[neighborV].addMsg(0, msg);
        }
    }

    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        infoMat[tempNode].initNeighborCoreness();
    }

    bool needUpdate = true;
    while (needUpdate) {
        msgNum = 0;
        activeNum = 0;
        needUpdate = false;
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            bool hasUpdate = infoMat[tempNode].updateCoreness();
            needUpdate = needUpdate or hasUpdate;
            if (hasUpdate) {
                activeNum++;
                int newCoreness = infoMat[tempNode].getNowCoreness();
                int oldCoreness = infoMat[tempNode].getOldCoreness();
                auto tempNodeNeighbors = g.getNeighbor(tempNode);
                for (auto neighborV: tempNodeNeighbors) {
                    SimpleCoreMessage msg = SimpleCoreMessage{oldCoreness, newCoreness};
                    infoMat[neighborV].addMsg(0, msg);
                    msgNum++;
                }
            }
        }
        BSPNum++;
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            infoMat[tempNode].mergeMsgBufferIntoNeighborCoreness();
        }
        if (printResult) {
            cout << "In round-" << BSPNum << ", " << activeNum << " nodes activated with " << msgNum << " msg." << endl;
        }
    }


    for (int i = 0; i < nodeNum; i++) {
        coreNumList[i] = infoMat[i].getNowCoreness();
        infoMat[i].freeSCI();
    }
    delete[] infoMat;
    return coreNumList;
}

int *mpVertexCentricCoreDecomposition(Graph g, bool printResult) {
    int threadNum = 4;
    omp_set_num_threads(threadNum);
    int nodeNum = g.getNodeNum();
    int maxDegree = g.getMaxDeg();

    int *tempCoreNumList = g.getNodeDegreeList();           // 最终的结果，即每个点对应点core-num
    int *coreNumList = new int[nodeNum];

    int begin;

    auto *infoMat = new SimpleCoreInfo[nodeNum];

    for (int i = 0; i < nodeNum; i++) {
        infoMat[i].initSCI(maxDegree, threadNum, tempCoreNumList[i]);
    }

#pragma omp parallel
    {
#pragma omp for
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            // 首先初始化
            int threadID = omp_get_thread_num();
            auto tempNodeNeighbors = g.getNeighbor(tempNode);
            for (auto neighborV: tempNodeNeighbors) {
                SimpleCoreMessage msg = SimpleCoreMessage{0, tempCoreNumList[tempNode]};
                infoMat[neighborV].addMsg(threadID, msg);
            }
        }
    }
//#pragma omp parallel for
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        infoMat[tempNode].initNeighborCoreness();
    }

    bool needUpdate = true;
    while (needUpdate) {
        needUpdate = false;
//#pragma omp parallel for
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            bool hasUpdate = infoMat[tempNode].updateCoreness();
            needUpdate = needUpdate or hasUpdate;
            if (hasUpdate) {
                int threadID = omp_get_thread_num();
                int newCoreness = infoMat[tempNode].getNowCoreness();
                int oldCoreness = infoMat[tempNode].getOldCoreness();
                auto tempNodeNeighbors = g.getNeighbor(tempNode);
                for (auto neighborV: tempNodeNeighbors) {
                    SimpleCoreMessage msg = SimpleCoreMessage{oldCoreness, newCoreness};
                    infoMat[neighborV].addMsg(threadID, msg);
                }
            }
        }
//#pragma omp parallel for
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            infoMat[tempNode].mergeMsgBufferIntoNeighborCoreness();
        }
    }

//#pragma omp parallel for
    for (int i = 0; i < nodeNum; i++) {
        coreNumList[i] = infoMat[i].getNowCoreness();
        infoMat[i].freeSCI();
    }
    delete[] infoMat;
    return coreNumList;
}

int *optMPVertexCentricCoreDecomposition(Graph g, bool printResult, int threadNum) {
    int nodeNum = g.getNodeNum();
    int maxDegree = g.getMaxDeg();
    int *tempCoreNumList = g.getNodeDegreeList();

    bool active = true;
    Timer timer{};
    int *coreNumList = new int[nodeNum];
    bool *activeList = new bool[nodeNum];
    int ***nodeToCoreInfoMat = new int **[threadNum];

    // Init
    timer.startTimer();
    omp_set_num_threads(threadNum);

    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        coreNumList[tempNode] = tempCoreNumList[tempNode];
    }

    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        activeList[tempNode] = true;
    }

#pragma omp parallel default(none) shared(nodeNum, coreNumList, g, active, activeList, nodeToCoreInfoMat)
    {
        int BSPNum = 0;
        int thread_id = omp_get_thread_num();
        int thread_num = omp_get_num_threads();
        int beginNode = thread_id * (nodeNum / thread_num);
        int endNode = (thread_id + 1) * (nodeNum / thread_num);
        if (thread_id == thread_num - 1) endNode = nodeNum;

        nodeToCoreInfoMat[thread_id] = new int *[nodeNum];
        bool *nextActiveList = new bool[nodeNum];
        int *threadCoreNumList = new int[nodeNum];
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            int nodeDegree = coreNumList[tempNode];
            int *tempCoreInfo = new int[nodeDegree + 1];
            for (int j = 0; j <= nodeDegree; j++) {
                tempCoreInfo[j] = 0;
            }
            nodeToCoreInfoMat[thread_id][tempNode] = tempCoreInfo;
        }
        for (int tempNode = beginNode; tempNode < endNode; tempNode++) {
            threadCoreNumList[tempNode] = coreNumList[tempNode];
            auto tempNodeNeighbors = g.getNeighbor(tempNode);
            int nodeDegree = coreNumList[tempNode];
            for (auto neighborV: tempNodeNeighbors) {
                int neighborDegree = coreNumList[neighborV];
                int j = nodeDegree > neighborDegree ? neighborDegree : nodeDegree;
                nodeToCoreInfoMat[thread_id][tempNode][j] += 1;
            }
        }
        while (active) {
            BSPNum += 1;
            for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
                nextActiveList[tempNode] = false;
            }
            for (int tempNode = beginNode; tempNode < endNode; tempNode++) {
                // 处理每一个被active的点（也就是收到信息的点）
                if (activeList[tempNode]) {
                    // 检查目前邻居的coreness还能不能支持现有的coreness，并计算出新的coreness
                    int oldCoreness = threadCoreNumList[tempNode];
                    int nowCoreness = threadCoreNumList[tempNode];
                    int support = 0;
                    // check now
                    support += nodeToCoreInfoMat[thread_id][tempNode][nowCoreness];
                    while (support < nowCoreness) {
                        nowCoreness--;
                        support += nodeToCoreInfoMat[thread_id][tempNode][nowCoreness];
                    }
                    nodeToCoreInfoMat[thread_id][tempNode][nowCoreness] = support;

                    // 如果发生了变化，需要在下一轮中激活邻居节点
                    if (oldCoreness != nowCoreness) {
                        // 首先本地的coreness发生变化
                        threadCoreNumList[tempNode] = nowCoreness;
                        // 然后遍历一遍邻居节点
                        auto tempNodeNeighbors = g.getNeighbor(tempNode);
                        tempNodeNeighbors = g.getNeighbor(tempNode);
                        for (auto neighborV: tempNodeNeighbors) {
                            int neighborCoreness = 0;
                            if (neighborV >= beginNode and neighborV < endNode) {
                                // 本地更新
                                neighborCoreness = threadCoreNumList[neighborV];
                            } else {
                                // 外地更新，先存本地，再统一sync
                                neighborCoreness = coreNumList[neighborV];
                            }
                            if (oldCoreness >= neighborCoreness and nowCoreness >= neighborCoreness) { continue; }
                            int j = oldCoreness > neighborCoreness ? neighborCoreness : oldCoreness;
                            nodeToCoreInfoMat[thread_id][neighborV][j]--;
                            j = nowCoreness > neighborCoreness ? neighborCoreness : nowCoreness;
                            nodeToCoreInfoMat[thread_id][neighborV][j]++;
                            nextActiveList[neighborV] = true;
                        }
                    }
                }
            }
#pragma omp barrier
            // barrier阶段，合并active-list，合并info-mat，合并coreness-list，并检查结束条件
#pragma omp single
            {
                active = false;
                for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
                    activeList[tempNode] = false;
                }
            }
#pragma omp barrier
#pragma omp critical
            {
//                cout << "Thread: " << thread_id << " In Round-" << BSPNum << endl;
                for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
                    activeList[tempNode] = activeList[tempNode] or nextActiveList[tempNode];
                    active = active or nextActiveList[tempNode];
                }
                for (int tempNode = beginNode; tempNode < endNode; tempNode++) {
                    for (int tempThread = 0; tempThread < thread_num; tempThread++) {
                        for (int nowCoreness = 0; nowCoreness <= coreNumList[tempNode]; nowCoreness++) {
                            if (tempThread != thread_id) {
                                int j = nowCoreness > threadCoreNumList[tempNode] ? threadCoreNumList[tempNode]
                                                                                  : nowCoreness;
                                nodeToCoreInfoMat[thread_id][tempNode][j] += nodeToCoreInfoMat[tempThread][tempNode][nowCoreness];
                                nodeToCoreInfoMat[tempThread][tempNode][nowCoreness] = 0;
                            }
                        }
                    }
                    coreNumList[tempNode] = threadCoreNumList[tempNode];
                }
            }
#pragma omp barrier
        }
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            delete[] nodeToCoreInfoMat[thread_id][tempNode];
        }
        delete[]nodeToCoreInfoMat[thread_id];
        delete[]nextActiveList;
        delete[]threadCoreNumList;
    }
    timer.endTimer();
    if (printResult) {
        cout << "Vertex Centric Core Number as Follow: " << endl;
        printList(coreNumList, nodeNum);
    }
    return coreNumList;
}

int *pullMPVertexCentricCoreDecomposition(Graph g, bool printResult, int threadNum) {
    int nodeNum = g.getNodeNum();
    int *tempCoreNumList = g.getNodeDegreeList();

    bool active = true;
    int *coreNumList = new int[nodeNum];
    bool *activeList = new bool[nodeNum];

    // Init
    omp_set_num_threads(threadNum);

    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        coreNumList[tempNode] = tempCoreNumList[tempNode];
    }

    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        activeList[tempNode] = true;
    }

#pragma omp parallel default(none) shared(active, activeList, coreNumList) firstprivate(g, nodeNum)
    {
        int BSPNum = 0;
        int thread_id = omp_get_thread_num();
        int thread_num = omp_get_num_threads();
        int beginNode = thread_id * (nodeNum / thread_num);
        int endNode = (thread_id + 1) * (nodeNum / thread_num);
        if (thread_id == thread_num - 1) endNode = nodeNum;

        int **nodeToCoreInfoMat = new int *[nodeNum];
        bool *nextActiveList = new bool[nodeNum];
        int *threadCoreNumList = new int[nodeNum];
        auto *threadNeighborList = new nodeNeighbor[nodeNum];
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            int nodeDegree = coreNumList[tempNode];
            int *tempCoreInfo = new int[nodeDegree + 1];
            for (int j = 0; j <= nodeDegree; j++) {
                tempCoreInfo[j] = 0;
            }
            nodeToCoreInfoMat[tempNode] = tempCoreInfo;
        }
        for (int tempNode = beginNode; tempNode < endNode; tempNode++) {
            threadCoreNumList[tempNode] = coreNumList[tempNode];
            auto tempNodeNeighbors = g.getNeighbor(tempNode);
            threadNeighborList[tempNode] = g.getNeighbor(tempNode);
            int nodeDegree = coreNumList[tempNode];
            for (auto neighborV: tempNodeNeighbors) {
                int neighborDegree = coreNumList[neighborV];
                int j = nodeDegree > neighborDegree ? neighborDegree : nodeDegree;
                nodeToCoreInfoMat[tempNode][j] += 1;
            }
        }
        while (active) {
            BSPNum += 1;
            for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
                nextActiveList[tempNode] = false;
                threadCoreNumList[tempNode] = coreNumList[tempNode];
            }
            for (int tempNode = beginNode; tempNode < endNode; tempNode++) {
                // 处理每一个被active的点（也就是收到信息的点）
                if (activeList[tempNode]) {
                    int nodeDegree = threadCoreNumList[tempNode];
                    for (int j = 0; j <= nodeDegree; j++) {
                        nodeToCoreInfoMat[tempNode][j] = 0;
                    }
                    auto tempNodeNeighbors = threadNeighborList[tempNode];
                    for (auto neighborV: tempNodeNeighbors) {
                        int neighborDegree = threadCoreNumList[neighborV];
                        int j = nodeDegree > neighborDegree ? neighborDegree : nodeDegree;
                        nodeToCoreInfoMat[tempNode][j] += 1;
                    }
                    // 检查目前邻居的coreness还能不能支持现有的coreness，并计算出新的coreness
                    int oldCoreness = threadCoreNumList[tempNode];
                    int nowCoreness = threadCoreNumList[tempNode];
                    int support = 0;
                    // check now
                    support += nodeToCoreInfoMat[tempNode][nowCoreness];
                    while (support < nowCoreness) {
                        nowCoreness--;
                        support += nodeToCoreInfoMat[tempNode][nowCoreness];
                    }
                    nodeToCoreInfoMat[tempNode][nowCoreness] = support;

                    // 如果发生了变化，需要在下一轮中激活邻居节点
                    if (oldCoreness != nowCoreness) {
                        // 首先本地的coreness发生变化
                        threadCoreNumList[tempNode] = nowCoreness;
                        // 然后遍历一遍邻居节点
                        tempNodeNeighbors = g.getNeighbor(tempNode);
                        for (auto neighborV: tempNodeNeighbors) {
                            int neighborCoreness = 0;
                            nextActiveList[neighborV] = true;
                        }
                    }
                }
            }
#pragma omp barrier
            // barrier阶段，合并active-list，合并info-mat，合并coreness-list，并检查结束条件
#pragma omp single
            {
                active = false;
                for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
                    activeList[tempNode] = false;
                }
            }
#pragma omp barrier
#pragma omp critical
            {
//                cout << "Thread: " << thread_id << " In Round-" << BSPNum << endl;
                for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
                    activeList[tempNode] = activeList[tempNode] or nextActiveList[tempNode];
                    active = active or nextActiveList[tempNode];
                }
                for (int tempNode = beginNode; tempNode < endNode; tempNode++) {
                    coreNumList[tempNode] = threadCoreNumList[tempNode];
                }
            }
#pragma omp barrier
        }
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            delete[] nodeToCoreInfoMat[tempNode];
        }
        delete[]nodeToCoreInfoMat;
        delete[]nextActiveList;
        delete[]threadCoreNumList;
        delete[]threadNeighborList;
    }
    if (printResult) {
        cout << "Vertex Centric Core Number as Follow: " << endl;
        printList(coreNumList, nodeNum);
    }
    return coreNumList;
}

int *optMultiVertexCentricCoreDecomposition(Graph g, bool printResult, int threadNum) {
    int nodeNum = g.getNodeNum();
    int maxDegree = g.getMaxDeg();

    int *tempCoreNumList = g.getNodeDegreeList();           // 最终的结果，即每个点对应点core-num
    int *coreNumList = new int[nodeNum];
    bool *activeList = new bool[nodeNum];               // 被激活的点
    bool **tempActiveList = new bool *[threadNum];

    omp_set_num_threads(threadNum);
    int BSPNum = 0;

    // Init
    omp_set_num_threads(threadNum);
    for (int j = 0; j < threadNum; j++) {
        tempActiveList[j] = new bool[nodeNum];
    }
#pragma omp parallel for default(none) shared(nodeNum, coreNumList, tempCoreNumList, activeList, maxDegree) schedule(dynamic, 256)
    for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
        coreNumList[tempNode] = tempCoreNumList[tempNode];
        activeList[tempNode] = true;
    }
    bool active = true;
    while (active) {
#pragma omp parallel for default(none) shared(tempActiveList, nodeNum, threadNum) schedule(static)
        for (int j = 0; j < threadNum; j++) {
            for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
                tempActiveList[omp_get_thread_num()][tempNode] = false;
            }
        }
#pragma omp parallel for default(none) shared(activeList, coreNumList, nodeNum, g, tempActiveList, maxDegree) schedule(dynamic, 256)
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            // 处理每一个被active的点（也就是收到信息的点）
            if (activeList[tempNode]) {
                // 检查目前邻居的coreness还能不能支持现有的coreness，并计算出新的coreness
                int oldCoreness, nowCoreness;
                oldCoreness = coreNumList[tempNode];
                nowCoreness = coreNumList[tempNode];
                int support = 0;
                int *smallerDegreeCount = new int[nowCoreness];
                auto tempNodeNeighbors = g.getNeighbor(tempNode);
                for (int j = 0; j < nowCoreness; j++) {
                    smallerDegreeCount[j] = 0;
                }
                for (auto neighborV: tempNodeNeighbors) {
                    if (coreNumList[neighborV] >= nowCoreness) {
                        support++;
                    } else {
                        smallerDegreeCount[coreNumList[neighborV]]++;
                    }
                }
                while (support < nowCoreness) {
                    nowCoreness--;
                    support += smallerDegreeCount[nowCoreness];
                }
                delete[] smallerDegreeCount;
                // 如果发生了变化，需要向邻居节点发送变化，并激活邻居节点
                if (oldCoreness != nowCoreness) {
                    coreNumList[tempNode] = nowCoreness;
                    tempNodeNeighbors = g.getNeighbor(tempNode);
                    for (auto neighborV: tempNodeNeighbors) {
                        int neighborDegree;
                        tempActiveList[omp_get_thread_num()][neighborV] = true;
                    }
                }
            }
        }
        // barrier阶段，更换active-list，并检查结束条件
#pragma omp barrier
        active = false;
        for (int tempNode = 0; tempNode < nodeNum; tempNode++) {
            activeList[tempNode] = false;
            for (int j = 0; j < threadNum; j++) {
                activeList[tempNode] = tempActiveList[j][tempNode] or activeList[tempNode];
            }
            active = active or activeList[tempNode];
        }
    }
    if (printResult) {
        cout << "Vertex Centric Core Number as Follow: " << endl;
        printList(coreNumList, nodeNum);
    }
    delete[]activeList;
    delete[]tempActiveList;
    return coreNumList;
}

void eachLayerCoreDecomposition(MultiLayerGraph mlg) {
    int **layerNodeMaxCoreNum = new int *[mlg.getLayerNum()];

    for (int tempLayer = 0; tempLayer < mlg.getLayerNum(); tempLayer++) {
        auto g = mlg.getGraphList()[tempLayer];
        layerNodeMaxCoreNum[tempLayer] = peelingCoreDecomposition(g, false);
    }

    for (int tempNode = 0; tempNode < mlg.getNodeNum(); tempNode++) {
        auto *tempCV = new CoreVector(mlg.getLayerNum());
        for (int tempLayer = 0; tempLayer < mlg.getLayerNum(); tempLayer++) {
            tempCV->vec[tempLayer] = layerNodeMaxCoreNum[tempLayer][tempNode];
        }
        cout << tempNode << " " << tempCV->cvToString() << endl;
    }
}