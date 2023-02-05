#include "value.h"

ValueArray::ValueArray(){
    values = new ArrayList<Value>();
}

ValueArray::~ValueArray(){
    delete values;
}

void ValueArray::write(Value value){
    values->add(value);
}

Value ValueArray::get(int index){
    return values->get(index);
}

void printValue(Value value){
    cout << value;
}