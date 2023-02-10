#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

// TODO: Could rewrite as a class but for now i want to make sure i understand how it works without.
//     : It's cooler to do types without types.

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define RAW_OBJ(objSubclassInstancePtr) ((Obj*) objSubclassInstancePtr)

#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define IS_ARRAY(value)       (IS_STRING(value))

#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      ((char*) ((ObjString*)AS_OBJ(value))->array.contents)
#define AS_ARRAY(value)       \
        (IS_STRING(value) ? \
        (ObjArray){VAL_OBJ, {.obj = (Obj*)AS_OBJ()}                      \
        : nullptr)



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

struct ObjArray {
    Obj obj;
    uint32_t length;
    void* contents;
};

// This expects <chars> to be null terminated but <length> does not include the terminator character.
struct ObjString {
    ObjArray array;
    uint32_t hash;
};

typedef struct Value Value;
typedef class Set Set;


bool isObjType(Value value, ObjType type);
ObjString* copyString(Set* internedStrings, Obj** objectsHead, const char* chars, int length);
ObjString* takeString(Set* internedStrings, Obj** objectsHead, char* chars, uint32_t length);
ObjString* allocateString(char* chars, uint32_t length, uint32_t hash);
Obj* allocateObject(size_t size, ObjType type);
void printObject(Value value, ostream* output);
void printObject(Value value);
void printObjectOwnedAddresses(Value value);
uint32_t hashString(const char* chars, uint32_t length);
void printObjectsList(Obj* head);
void freeObject(Obj* object);

inline void freeStringChars(ObjString* string){
    FREE_ARRAY(char, string->array.contents, string->array.length);
}

inline void linkObjects(Obj** head, Obj* additional) {
    Obj* end = additional;
    while (end->next != nullptr){
        end = end->next;
    }
    end->next = *head;
    *head = additional;
}

inline char* asCString(ObjString* str){
    return (char*) str->array.contents;
}

#endif