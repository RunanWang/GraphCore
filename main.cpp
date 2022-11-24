#include <iostream>
#include "DataStructure/MultiLayerGraph.h"
#include "Utils/Timer.h"
#include "CoreDecomposition.h"
#include <string>

using namespace std;

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

void testSimpleGraphCoreDecomposition(string &path) {
    Timer calTimer{};
    Timer loadTimer{};
    loadTimer.startTimer();
    Graph g{};
    g.loadGraphFromSnapFile(path);
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

    std::cout << "\n===Vertex Centric Lazy===" << std::endl;
    calTimer.resetTimer();
    calTimer.startTimer();
    auto d = lazyVertexCentricCoreDecomposition(g, false);
    calTimer.endTimer();
    std::cout << "Total Cal Time of lazy vertex centric: " << calTimer.getTimerSecond() << "s." << std::endl;
    checkSimpleGraphCorenessSame(a, d, g.getNodeNum());
}

void testMLGCoreDecomposition(string &path) {
    Timer calTimer{};
    Timer loadTimer{};
    // Load GraphCore
    loadTimer.startTimer();
    MultiLayerGraph g{};
    g.loadGraphFromFile(path);
    loadTimer.endTimer();
    std::cout << "Load Time: " << loadTimer.getTimerSecond() << "s." << std::endl;
    g.showGraphProperties();

    // Calculate Core on MLG
    calTimer.startTimer();
    eachLayerCoreDecomposition(g);
    bfsMLGCoreDecomposition(g);
    calTimer.endTimer();
    std::cout << "Total Cal Time: " << calTimer.getTimerSecond() << "s." << std::endl;
}

void testSGmpCD(string &path) {
    Timer calTimer{};
    Timer loadTimer{};
    loadTimer.startTimer();
    Graph g{};
    g.loadGraphFromSnapFile(path);
    loadTimer.endTimer();
    std::cout << "Load Time: " << loadTimer.getTimerSecond() << "s." << std::endl;
    g.showGraphProperties();

    std::cout << "\n===Peeling===" << std::endl;
    // Calculate Core on GraphCore
    calTimer.startTimer();
    auto a = peelingCoreDecomposition(g, false);
    calTimer.endTimer();
    std::cout << "Total Cal Time of peeling: " << calTimer.getTimerSecond() << "s." << std::endl;

    std::cout << "\n===Vertex Centric Opt===" << std::endl;
    calTimer.resetTimer();
    calTimer.startTimer();
    auto c = optVertexCentricCoreDecomposition(g, false);
    calTimer.endTimer();
    std::cout << "Total Cal Time of opt vertex centric: " << calTimer.getTimerSecond() << "s." << std::endl;
    checkSimpleGraphCorenessSame(a, c, g.getNodeNum());

    std::cout << "\n===Vertex Centric MP thread=1===" << std::endl;
    calTimer.resetTimer();
    calTimer.startTimer();
    auto d = optMPVertexCentricCoreDecomposition(g, false,1);
    calTimer.endTimer();
    std::cout << "Total Cal Time of opt vertex centric: " << calTimer.getTimerSecond() << "s." << std::endl;
    checkSimpleGraphCorenessSame(a, d, g.getNodeNum());

    std::cout << "\n===Vertex Centric MP thread=2===" << std::endl;
    calTimer.resetTimer();
    calTimer.startTimer();
    auto e = optMPVertexCentricCoreDecomposition(g, false,2);
    calTimer.endTimer();
    std::cout << "Total Cal Time of opt vertex centric: " << calTimer.getTimerSecond() << "s." << std::endl;
    checkSimpleGraphCorenessSame(a, e, g.getNodeNum());

    std::cout << "\n===Vertex Centric MP thread=4===" << std::endl;
    calTimer.resetTimer();
    calTimer.startTimer();
    auto f = optMPVertexCentricCoreDecomposition(g, false,4);
    calTimer.endTimer();
    std::cout << "Total Cal Time of opt vertex centric: " << calTimer.getTimerSecond() << "s." << std::endl;
    checkSimpleGraphCorenessSame(a, f, g.getNodeNum());
}

int main(int argc, char *argv[]) {
    if (argc <= 0) {
        std::cout << "";
    }
    int toTestCase = 3;

    switch (toTestCase) {
        case 1: {
            cout << "Testing Simple Graph" << endl;
            string path = "../dataset/facebook.txt";
            testSimpleGraphCoreDecomposition(path);
            break;
        }
        case 2: {
            string path = "../dataset/homo.txt";
            testMLGCoreDecomposition(path);
            break;
        }
        case 3: {
            string path = "../dataset/facebook.txt";
            testSGmpCD(path);
        }
        default: {

        }
    }
    return 0;
}
