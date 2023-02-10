#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

// TODO: Could rewrite as a class but for now i want to make sure i understand how it works without.
//     : It's cooler to do types without types.

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define RAW_OBJ(objSubclassInstancePtr) ((Obj*) objSubclassInstancePtr)

#define IS_STRING(value)       isObjType(value, OBJ_STRING)

#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_STRING,
} ObjType;

typedef struct ObjString ObjString;

struct Obj {
    ObjType type;

    // An Obj is a singly linked list node so when the vm is done it can go through and find them all
    // to make sure it didn't lose any and cause a memory leak.
    // Since singly linked, the vm needs to make sure it always has the first in the chain.
    // That's easy enough that it's not worth the overhead of an extra pointer on every single Obj.
    struct Obj* next;  // TODO: what does the struct keyword do here?
};

// This expects <chars> to be null terminated but <length> does not include the terminator character.
struct ObjString {
    Obj obj;
    uint32_t length;
    char* chars;
    uint32_t hash;
};


typedef struct Value Value;
typedef class Set Set;


bool isObjType(Value value, ObjType type);
ObjString* copyString(Set* internedStrings, Obj** objectsHead, const char* chars, int length);
ObjString* takeString(Set* internedStrings, Obj** objectsHead, char* chars, int length);
ObjString* allocateString(char* chars, int length, uint32_t hash);
Obj* allocateObject(size_t size, ObjType type);
void printObject(Value value, ostream* output);
void printObject(Value value);
void printObjectOwnedAddresses(Value value);
uint32_t hashString(const char* chars, int length);
void printObjectsList(Obj* head);
void freeObject(Obj* object);

inline void freeStringChars(char* chars, int length){
    FREE_ARRAY(char, chars, length + 1);
}

inline void linkObjects(Obj** head, Obj* additional) {
    Obj* end = additional;
    while (end->next != nullptr){
        end = end->next;
    }
    end->next = *head;
    *head = additional;
}

#endif