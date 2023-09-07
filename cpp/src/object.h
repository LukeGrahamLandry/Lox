#ifndef clox_object_h
#define clox_object_h

class Memory;
typedef struct Value Value;

#include "common.h"
#include <vector>

// TODO: Could rewrite as a class but for now i want to make sure i understand how it works without.
//     : It's cooler to do types without types.

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define RAW_OBJ(objSubclassInstancePtr) ((Obj*) objSubclassInstancePtr)

#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define IS_CLOSURE(value)      isObjType(value, OBJ_CLOSURE)
#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)

#define AS_FUNCTION(value)       ((ObjFunction *)AS_OBJ(value))
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      ((char*) ((ObjString*)AS_OBJ(value))->array.contents)
#define AS_CLOSURE(value)       ((ObjClosure*)AS_OBJ(value))
#define AS_CLASS(value)       ((ObjClass*)AS_OBJ(value))
#define AS_INSTANCE(value)       ((ObjInstance*)AS_OBJ(value))


#define ALLOCATE(type, length) (type*) reallocate(nullptr, 0, sizeof(type) * length)
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)
#define FREE_ARRAY(type, pointer, count) reallocate(pointer, sizeof(type) * count, 0)


typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_FREED
} ObjType;

typedef struct ObjString ObjString;
typedef struct Table Table;
typedef struct Set Set;
typedef struct ObjInstance ObjInstance;

struct Obj {
    ObjType type;

    // An Obj is a singly linked list node so when the vm is done it can go through and find them all
    // to make sure it didn't lose any and cause a memory leak.
    // Since singly linked, the vm needs to make sure it always has the first in the chain.
    // That's easy enough that it's not worth the overhead of an extra pointer on every single Obj.
    struct Obj* next;  // TODO: what does the struct keyword do here?
    bool isMarked;
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

#include "value.h"

typedef struct ObjUpvalue {
    Obj obj;
    Value* location;
    ObjUpvalue* next;
    Value closed;
} ObjUpvalue;

typedef struct ObjClass {
    Obj obj;
    ObjString* name;
} ObjClass;

bool isObjType(Value value, ObjType type);
void printObject(Value value, ostream* output);
void printObject(Value value);
void printObjectOwnedAddresses(Value value);
uint32_t hashString(const char* chars, uint32_t length);
void printObjectsList(Obj* head);

static void linkObjects(Obj** head, Obj* additional) {
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

typedef struct ObjClosure ObjClosure;

typedef struct {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

class Table;
class Set;

class Memory {
public:
    // interned strings. prevents allocating separate memory for duplicated identical strings.
    Set* strings;
    Table* natives;
    // a linked list of all Values with heap allocated memory, so we can free them when we terminate.
    Obj* objects;
    ObjUpvalue* openUpvalues;
    vector<Obj*> grayStack;
    Value stack[STACK_MAX];  // working memory. my equivalent of registers
    Value* stackTop;  // where the next value will be inserted


    CallFrame frames[FRAMES_MAX];  // it annoys me to have a separate bonus stack instead of storing return addresses in the normal value stack
    int frameCount;

    bool enable;

    ObjString* copyString(const char* chars, int length);
    ObjString* takeString(char* chars, uint32_t length);
    ObjString* allocateString(char* chars, uint32_t length, uint32_t hash);
    Obj* allocateObject(size_t size, ObjType type);
    void freeObject(Obj* object);
    ObjFunction* newFunction();
    ObjClosure* newClosure(ObjFunction* function);
    ObjUpvalue* newUpvalue(Value* function);
    ObjClass* newClass(ObjString* name);
    ObjInstance* newInstance(ObjClass* klass);
    void markTable(Table& table);
    ObjNative* newNative(NativeFn function, uint8_t arity, ObjString* name);
    inline void freeStringChars(ObjString* string){
        FREE_ARRAY(char, string->array.contents, string->array.length);
    }

    void* reallocate(void* pointer, size_t oldSize, size_t newSize);
    void collectGarbage();
    void markRoots();
    void traceReferences();
    void sweep();
    void markValue(Value value);
    void markObject(Obj* object);

    void push(Value value){
        // TODO: bounds check
        *stackTop = value;
        stackTop++;
    }

    Value pop(){
        // TODO: bounds check
        stackTop--;
        return *stackTop;
    }
};


#include "list.cc"
#include "table.h"

typedef struct ObjClosure {
    Obj obj;
    ObjFunction* function;
    ArrayList<ObjUpvalue*> upvalues;
} ObjClosure;

struct ObjInstance {
    Obj obj;
    ObjClass* klass;
    // TODO: this doesnt need the level of indirection. ive just gotten myself in a hole by being bad at c++ when I started
    Table* fields;
};


#endif