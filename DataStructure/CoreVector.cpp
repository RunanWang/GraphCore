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

bool CoreVector::operator==(const CoreVector &rhs) const {
    if (length != rhs.length) {
        return false;
    }
    for (int j = 0; j < length; j++) {
        if (vec[j] != rhs.vec[j]) {
            return false;
        }
    }
    return true;
}

bool CoreVector::operator<(const CoreVector &rhs) const {
    if (length < rhs.length) {
        return true;
    }
    for (int j = 0; j < length; j++) {
        if (vec[j] < rhs.vec[j]) {
            return true;
        }
    }
    return false;
}


