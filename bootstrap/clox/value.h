#ifndef clox_value_h
#define clox_value_h

#include "common.h"
#include "list.h"

typedef double Value;

class ValueArray {
    public:
        ValueArray();
        ~ValueArray();
        void write(Value byte);
        Value get(int index);

        ArrayList<Value>* values;
};

void printValue(Value value);

#endif