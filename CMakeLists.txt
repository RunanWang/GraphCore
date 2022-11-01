cmake_minimum_required(VERSION 3.22)
project(GraphCore)

set(CMAKE_CXX_STANDARD 11)

add_executable(GraphCore main.cpp DataStructure/Graph.cpp DataStructure/Graph.h DataStructure/MultiLayerGraph.cpp DataStructure/MultiLayerGraph.h CoreDecomposition.cpp CoreDecomposition.h Utils/Timer.cpp Utils/Timer.h DataStructure/Core.cpp DataStructure/Core.h Utils/out.h DataStructure/CoreVector.h DataStructure/CoreVector.cpp)
