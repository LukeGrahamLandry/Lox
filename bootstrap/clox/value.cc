#include "value.h"

ValueArray::ValueArray(){
    values = new ArrayList<Value>();
}

ValueArray::~ValueArray(){
    delete values;
}

void ValueArray::add(Value value){
    values->add(value);
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
