#ifndef clox_value_h
#define clox_value_h

#include "common.h"
#include "list.cc"

typedef double Value;

class ValueArray {
    public:
        ValueArray();
        ~ValueArray();
        void add(Value value);
        Value get(int index);
        int size();
    private:
        ArrayList<Value>* values;
};

void printValue(Value value);

#endif