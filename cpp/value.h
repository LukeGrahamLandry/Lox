#ifndef clox_value_h
#define clox_value_h

#include "common.h"
#include "list.cc"


typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
    VAL_NATIVE_POINTER
} ValueType;

typedef struct Obj Obj;
typedef struct Value Value;

union ValueData {
    bool boolean;
    double number;
    int integer;
    Obj* obj;
    void* pointer;
};

struct Value {
    ValueType type;
    ValueData as;
};

#include "object.h"


// c type -> Value
#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})

#define NIL_VAL()         ((Value){VAL_NIL, {.number = 0}})

#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})

// Value -> c type
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define AS_OBJ(value)     ((value).as.obj)
#define AS_NATIVE(value) ((ObjNative*)AS_OBJ(value))

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)

void printValue(Value value);
void printValue(Value value, ostream* output);
void printValueArray(Value* startPtr, Value* endPtr);
bool valuesEqual(Value right, Value left);  // might need to move this back to the vm when equality needs runtime info

#endif