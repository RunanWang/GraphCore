//
// Created by 王润安 on 2022/10/9.
//
#include <queue>
#include <set>
#include <map>
#include <algorithm>
#include "CoreDecomposition.h"
#include "Utils/Timer.h"
#include "Utils/out.h"
#include "DataStructure/Core.h"
#include "DataStructure/CoreVector.h"

using namespace std;

set<int> *kCore(MultiLayerGraph mlg, const set<int> &intersect, int layerNum, int NodeNum, CoreVector outQueueVector) {
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
                nodeSet = kCore(mlg, intersect, layerNum, nodeNum, outQueueVector);
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
    std::cout << "BFS Time: " << AlgoTimer.getTimerSecond() << " s." << std::endl;
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

