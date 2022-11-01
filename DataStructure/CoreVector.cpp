//
// Created by 王润安 on 2022/10/31.
//

#include "CoreVector.h"


CoreVector::CoreVector(int _length) {
    length = _length;
    vec = new int[length];
    for (int j = 0; j < length; j++) {
        vec[j] = 0;
    }
}

CoreVector::CoreVector(const int *_vec, int _length) {
    length = _length;
    vec = new int[length];
    for (int j = 0; j < length; j++) {
        vec[j] = _vec[j];
    }
}

CoreVector CoreVector::build_descendant_vector(int index) const {
    CoreVector descendant = *new CoreVector(vec, length);
    descendant.vec[index] += 1;
    return descendant;
}

int CoreVector::get_non_zero_index() const {
    int count = 0;
    for (int j = 0; j < length; j++) {
        if (vec[j] > 0) {
            count += 1;
        }
    }
    return count;
}


