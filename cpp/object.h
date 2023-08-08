#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

// TODO: Could rewrite as a class but for now i want to make sure i understand how it works without.
//     : It's cooler to do types without types.

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define RAW_OBJ(objSubclassInstancePtr) ((Obj*) objSubclassInstancePtr)

#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define IS_BYTE_ARRAY(value)       (isObjType(value, OBJ_BYTE_ARRAY))
#define IS_VALUE_ARRAY(value)       (isObjType(value, OBJ_VALUE_ARRAY))
#define IS_ARRAY(value)       (IS_STRING(value) || IS_BYTE_ARRAY(value) IS_VALUE_ARRAY(value))
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define IS_CLOSURE(value)     isObjType(value, OBJ_CLOSURE)

#define AS_FUNCTION(value)       ((ObjFunction *)AS_OBJ(value))
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      ((char*) ((ObjString*)AS_OBJ(value))->array.contents)
#define AS_ARRAY(value)   ((ObjArray){VAL_OBJ, {.obj = (Obj*)AS_OBJ()})
#define AS_CLOSURE(value)       ((ObjClosure*)AS_OBJ(value))

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_CLOSURE,
    OBJ_UPVALUE,

    OBJ_VALUE_ARRAY,
    OBJ_BYTE_ARRAY
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
// TODO: Strings should not be a magic special case. Should expose ObjInternedArray to the language.
// If you have a typed array, each thing doesn't need a type tag. A Value array can have anything since those have a type tag.
struct ObjString {
    ObjArray array;
    uint32_t hash;
};

typedef struct Value Value;
typedef class Set Set;
typedef class Chunk Chunk;
typedef class VM VM;

typedef struct {
    Obj obj;
    // The compiler limits number of arguments to 1 byte.
    // Even once I allow higher stack indexes for getLocals, the limitation here doesn't feel unreasonable.
    // Nobody manually writes a function with 256 arguments and if you're generating one there's probably a better way.
    uint8_t arity;
    Chunk* chunk;
    ObjString* name;
    int upvalueCount;
} ObjFunction;

typedef Value (*NativeFn)(VM* vm, Value* args);

typedef struct {
    Obj obj;
    NativeFn function;
    uint8_t arity;
    ObjString* name;
} ObjNative;

typedef struct ObjUpvalue {
    Obj obj;
    Value* location;
    ObjUpvalue* next;
    Value closed;
} ObjUpvalue;

typedef struct ObjClosure {
    Obj obj;
    ObjFunction* function;
    ArrayList<ObjUpvalue*> upvalues;
} ObjClosure;


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
ObjFunction* newFunction();
ObjClosure* newClosure(ObjFunction* function);
ObjUpvalue* newUpvalue(Value* function);

ObjNative* newNative(NativeFn function, uint8_t arity, ObjString* name);

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