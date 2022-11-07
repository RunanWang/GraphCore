//
// Created by 王润安 on 2022/10/31.
//

#include <string>

#ifndef GRAPH_COREVECTOR_H
#define GRAPH_COREVECTOR_H


class CoreVector {
public:
    int *vec;
    int length;

    CoreVector(const int *vec, int length);

    CoreVector(int length);

    CoreVector build_descendant_vector(int index) const;

    CoreVector build_ancestor_vector(int index) const;

    int get_non_zero_index() const;

    std::string cvToString() const;

    bool isFather(CoreVector *father) const;
};

struct CVCompartor {
    bool operator()(const CoreVector lhs, const CoreVector rhs) const {
        if (lhs.length < rhs.length) {
            return true;
        } else if (lhs.length > rhs.length) {
            return false;
        }
        for (int j = 0; j < lhs.length; j++) {
            if (lhs.vec[j] < rhs.vec[j]) {
                return true;
            } else if (lhs.vec[j] > rhs.vec[j]) {
                return false;
            }
        }
        return false;
    }
};


#endif //GRAPH_COREVECTOR_H
