#include <iostream>
#include "DataStructure/MultiLayerGraph.h"
#include "Utils/Timer.h"
#include "CoreDecomposition.h"
#include <cstring>

int main(int argc, char *argv[]) {
    if (argc <= 0) {
        std::cout << "";
    }
    Timer calTimer{};
    Timer loadTimer{};
    auto graphType = argv[0];
//    if (strncmp("mlg", graphType, 3) != 0) {
    if (true) {
        // Load GraphCore
        loadTimer.startTimer();
        MultiLayerGraph g{};
        g.loadGraphFromFile("../dataset/sample.txt");
        loadTimer.endTimer();
        std::cout << "Load Time: " << loadTimer.getTimerSecond() << "s." << std::endl;
        g.showGraphProperties();

        // Calculate Core on MLG
        calTimer.startTimer();
        naiveMLGCoreDecomposition(g);
        calTimer.endTimer();
        std::cout << "Total Cal Time: " << calTimer.getTimerSecond() << "s." << std::endl;
    } else if (strncmp("sg", graphType, 2) != 0) {
        // Load GraphCore
        loadTimer.startTimer();
        Graph g{};
        g.loadGraphFromSnapFile("/Users/ryan/Desktop/workspace/GraphCore/dataset/facebook.txt");
        loadTimer.endTimer();
        std::cout << "Load Time: " << loadTimer.getTimerSecond() << "s." << std::endl;
        g.showGraphProperties();

        // Calculate Core on GraphCore
        calTimer.startTimer();
        peelingCoreDecomposition(g, true);
        calTimer.endTimer();
        std::cout << "Total Cal Time: " << calTimer.getTimerSecond() << "s." << std::endl;
    } else {
        std::cout << "GraphCore supported is simple graph (sg) or multi-layer graph (mlg). Re-input it.\n";
    }
    return 0;
}
