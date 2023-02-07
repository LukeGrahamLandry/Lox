#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

// TODO: Could rewrite as a class but for now i want to make sure i understand how it works without.
//     : It's cooler to do types without type.

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define IS_STRING(value)       isObjType(value, OBJ_STRING)

#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

typedef struct ObjString ObjString;
typedef enum {
    OBJ_STRING,
} ObjType;

typedef struct ObjString ObjString;

struct Obj {
    ObjType type;
    struct Obj* next;  // TODO: what does the struct keyword do here?
};

struct ObjString {
    Obj obj;
    int length;
    char* chars;
};


typedef struct Value Value;

bool isObjType(Value value, ObjType type);
ObjString* copyString(Obj** previous, const char* chars, int length);
ObjString* takeString(Obj** previous, char* chars, int length);
ObjString* allocateString(Obj** previous, char* chars, int length);
Obj* allocateObject(Obj** previous, size_t size, ObjType type);
void printObject(Value value);

#endif