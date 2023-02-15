#include "value.h"

void printValue(Value value, ostream* output){
    switch (value.type) {
        case VAL_BOOL:
            *output << (AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NUMBER:
            printf("%.10g", AS_NUMBER(value));
            break;
        case VAL_NIL:
            *output << "nil";
            break;
        case VAL_OBJ:
            printObject(value, output);
            break;
    }
}

void printValue(Value value){
    printValue(value, &cout);
}

void printValueArray(Value *startPtr, Value *endPtr) {
    cout << "          ";
    for (Value* slot = startPtr; slot < endPtr; slot++){
        cout << "[";
        printValue(*slot);
        cout << "]";
    }
    cout << endl;
}

bool valuesEqual(Value right, Value left) {
    if (right.type != left.type) return false;

    // Book: cant just memcmp() the structs cause there's padding so garbage bites
    switch (right.type) {
        case VAL_BOOL:
            return AS_BOOL(right) == AS_BOOL(left);
        case VAL_NUMBER:
            return AS_NUMBER(right) == AS_NUMBER(left);
        case VAL_NIL:
            return true;
        case VAL_OBJ: {
            // Since all strings are interned, it's safe to just compare memory addresses. 
            return AS_STRING(right) == AS_STRING(left);
        }
        default:
            return false;
    }
}
