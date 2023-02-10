#include "value.h"

ValueArray::ValueArray(){
    values = new ArrayList<Value>();
}

ValueArray::~ValueArray(){
    delete values;
}

void ValueArray::add(Value value){
    values->push(value);
}

Value ValueArray::get(int index){
    return values->get(index);
}

int ValueArray::size() {
    return values->count;
}

void printValue(Value value){
    switch (value.type) {
        case VAL_BOOL:
            cout << (AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NUMBER:
            cout << AS_NUMBER(value);
            break;
        case VAL_NIL:
            cout << "nil";
            break;
        case VAL_OBJ:
            printObject(value);
            break;
    }
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
