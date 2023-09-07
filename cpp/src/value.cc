#include "value.h"
#include "object.h"

void printValue(Value value, ostream* output){
    switch (value.type) {
        case VAL_BOOL:
            *output << (AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NUMBER:
            char buf[1024];
            snprintf(buf, 1024, "%.10g", AS_NUMBER(value));
            *output << buf;
            break;
        case VAL_NIL:
            *output << "nil";
            break;
        case VAL_OBJ:
            printObject(value, output);
            break;
        case VAL_NATIVE_POINTER:
            *output << "<native-ptr>";
            break;
    }
}

void printValue(Value value){
    printValue(value, &cout);
}

void debugPrintValueArray(Value *startPtr, Value *endPtr) {
    cerr << "          ";
    for (Value* slot = startPtr; slot < endPtr; slot++){
        cerr << "[";
        printValue(*slot, &cerr);
        cerr << "]";
    }
    cerr << endl;
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
