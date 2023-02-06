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
    cout << value;
}
