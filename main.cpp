#include <iostream>
#include "DataStructure/MultiLayerGraph.h"
#include "Utils/Timer.h"
#include "CoreDecomposition.h"

void checkSimpleGraphCorenessSame(int *lhs, int *rhs, int nodeNum) {
    bool isSame = true;
    for (int j = 0; j < nodeNum; j++) {
        if (lhs[j] != rhs[j]) {
            std::cout << "In coreness of " << j << " : method left " << lhs[j] << " vs method right " << rhs[j]
                      << std::endl;
            isSame = false;
        }
    }
    if (!isSame) {
        std::cout << "[Warning] Two core results not same." << std::endl;
    } else {
        std::cout << "Result check pass." << std::endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc <= 0) {
        std::cout << "";
    }
    Timer calTimer{};
    Timer loadTimer{};
    auto graphType = argv[0];
//    if (strncmp("mlg", graphType, 3) != 0) {
//    if (true) {
//        // Load GraphCore
//        loadTimer.startTimer();
//        MultiLayerGraph g{};
//        g.loadGraphFromFile("../dataset/homo.txt");
//        loadTimer.endTimer();
//        std::cout << "Load Time: " << loadTimer.getTimerSecond() << "s." << std::endl;
//        g.showGraphProperties();
//
//        // Calculate Core on MLG
//        calTimer.startTimer();
//        eachLayerCoreDecomposition(g);
//        bfsMLGCoreDecomposition(g);
//        calTimer.endTimer();
//        std::cout << "Total Cal Time: " << calTimer.getTimerSecond() << "s." << std::endl;
//    }
    if (true) {
        // Load GraphCore
        loadTimer.startTimer();
        Graph g{};
        g.loadGraphFromSnapFile("../dataset/facebook.txt");
        loadTimer.endTimer();
        std::cout << "Load Time: " << loadTimer.getTimerSecond() << "s." << std::endl;
        g.showGraphProperties();

        std::cout << "\n===Peeling===" << std::endl;
        // Calculate Core on GraphCore
        calTimer.startTimer();
        auto a = peelingCoreDecomposition(g, false);
        calTimer.endTimer();
        std::cout << "Total Cal Time of peeling: " << calTimer.getTimerSecond() << "s." << std::endl;

        std::cout << "\n===Vertex Centric===" << std::endl;
        calTimer.resetTimer();
        calTimer.startTimer();
        auto b = vertexCentricCoreDecomposition(g, false);
        calTimer.endTimer();
        std::cout << "Total Cal Time of vertex centric: " << calTimer.getTimerSecond() << "s." << std::endl;
        checkSimpleGraphCorenessSame(a, b, g.getNodeNum());

        std::cout << "\n===Vertex Centric Opt===" << std::endl;
        calTimer.resetTimer();
        calTimer.startTimer();
        auto c = optVertexCentricCoreDecomposition(g, false);
        calTimer.endTimer();
        std::cout << "Total Cal Time of opt vertex centric: " << calTimer.getTimerSecond() << "s." << std::endl;
        checkSimpleGraphCorenessSame(a, c, g.getNodeNum());
    } else {
        std::cout << "GraphCore supported is simple graph (sg) or multi-layer graph (mlg). Re-input it.\n";
    }
    return 0;
}
