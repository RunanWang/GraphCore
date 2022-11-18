# GraphCore

A testbed for all kinds of graph algorithms.

## Data Format

All data are in dataset.

The first line of file contains the number of nodes and number of edges.

Lines followed are the edges linking two nodes.

## Algorithms Available

### Simple Graph Core Decomposition

An O(m) Algorithm for Cores Decomposition of Networks, arXiv 2003

- peeling (STL)
- peeling (Well-designed data structure)

Distributed k-Core Decomposition, TPDS 2013

- vertex-centric (STL)
- vertex-centric (Well-designed data structure)

### Multi-layer Graph Core Decomposition

Core Decomposition in Multilayer Networks: Theory, Algorithms, and Applications, TKDD 2020

- naive
- bfs
- dfs
- hybrid