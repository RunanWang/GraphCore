//
// Created by 王润安 on 2022/10/31.
//

#ifndef GRAPH_COREVECTOR_H
#define GRAPH_COREVECTOR_H


class CoreVector {
public:
    int *vec;
    int length;

    CoreVector(const int *vec, int length);

    bool operator==(const CoreVector &rhs) const;

    bool operator<(const CoreVector &rhs) const;

    CoreVector(int length);

    CoreVector build_descendant_vector(int index) const;
};

struct CVCompartor {
    bool operator()(const CoreVector lhs, const CoreVector rhs) const {
        if (lhs.length < rhs.length) {
            return true;
        } else if (lhs.length < rhs.length) {
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
