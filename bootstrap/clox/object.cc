#include "object.h"

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "common.h"

#define ALLOCATE_OBJ(prev, type, objectType) \
        (type*)allocateObject(prev, sizeof(type), objectType)

bool isObjType(Value value, ObjType type){
    return value.type == VAL_OBJ && AS_OBJ(value)->type == type;
}

// The Obj wants to own its memory, so it can garbage collect if it wants.
// So if you're looking at someone else's string, like the src from the scanner, you need to copy it.
// TODO: flag for being a constant string where the object doesnt own the memory, just point into the src string.
ObjString* copyString(Obj** previous, const char* chars, int length){
    char* ptr = ALLOCATE(char, length + 1);
    memcpy(ptr, chars, length);
    ptr[length] = '\0';
    return allocateString(previous, ptr, length);
}

// Used when we already own that memory.
ObjString* takeString(Obj** previous, char* chars, int length) {
    return allocateString(previous, chars, length);
}

ObjString* allocateString(Obj** previous, char* chars, int length) {
    ObjString* string = ALLOCATE_OBJ(previous, ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

Obj* allocateObject(Obj** previous, size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;

    object->next = *previous;
    *previous = object;

    return object;
}

void printObject(Value value){
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            cout << AS_CSTRING(value);
            break;
    }
}
