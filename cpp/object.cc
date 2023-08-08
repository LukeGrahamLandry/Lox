#include "value.h"
#include "vm.h"
#include "common.h"
#include "object.h"

bool isObjType(Value value, ObjType type){
    return value.type == VAL_OBJ && AS_OBJ(value)->type == type;
}

#define ALLOCATE_OBJ(type, objectType) \
        (type*)allocateObject(sizeof(type), objectType)

// The Obj wants to own its memory, so it can garbage collect if it wants.
// So if you're looking at someone else's string, like the src from the scanner, you need to copy it.
// <length> must not include the null terminator.
// TODO: flag for being a constant string where the object doesnt own the memory, just point into the src string.
ObjString* Memory::copyString(const char* chars, int length){
    #ifdef VM_SAFE_MODE
    if (length < 0) cerr << "Cannot allocate string of negative length " << length << endl;
    #endif

    uint32_t hash = hashString(chars, length);

    Entry* slot;
    ObjString* str;
    bool wasInterned = strings->safeFindEntry(chars, length, hash, &slot);
    if (wasInterned){
        str = slot->key;
    } else {
        char* ptr = ALLOCATE(char, length + 1);
        memcpy(ptr, chars, length);
        ptr[length] = '\0';

        str = allocateString(ptr, length, hash);
        strings->setEntry(slot, str, NIL_VAL());
    }

    return str;
}

// Used when we already own that memory.
// The caller is giving away the memory to the new ObjString returned, so we're allowed to free it if it's a duplicate.
// This requires <chars> to be a null terminated string with the terminator NOT included in the <length>.
ObjString* Memory::takeString(char* chars, uint32_t length) {
    uint32_t hash = hashString(chars, length);

    Entry* slot;
    ObjString* str;
    bool wasInterned = strings->safeFindEntry(chars, length, hashString(chars, length), &slot);
    if (wasInterned){
        str = slot->key;
        FREE_ARRAY(char, chars,  length + 1);  // + 1 for null terminator
    } else {
        VM* vm = (VM*) evilVmGlobal;
        str = allocateString(chars, length, hash);
        vm->push(OBJ_VAL(str));
        strings->setEntry(slot, str, NIL_VAL());
        vm->pop();
    }

    return str;
}

ObjString* Memory::allocateString(char* chars, uint32_t length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->array.contents = chars;
    string->array.length = length + 1;
    string->hash = hash;
    return string;
}

Obj* Memory::allocateObject(size_t size, ObjType type) {
    Obj* object = RAW_OBJ(reallocate(nullptr, 0, size));
    object->next = nullptr;
    linkObjects(&objects, object);
    object->type = type;
    object->isMarked = false;
#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif
    return object;
}

void Memory::freeObject(Obj* object){
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
#endif
    switch (object->type) {
        case OBJ_STRING: {
            // We own the char array.
            ObjString* string = (ObjString*)object;
            freeStringChars(string);
            FREE(ObjString, object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            function->chunk->release(*this);
            delete function->chunk;
            FREE(ObjFunction , object);
            break;
        }
        case OBJ_NATIVE: {
            FREE(ObjNative, object);
            break;
        }
        case OBJ_CLOSURE: {
            // Multiple closures can reference the same function.
            //  We don't own the upvalue objects.
            ((ObjClosure*)object)->upvalues.release(*this);
            FREE(ObjClosure, object);
            break;
        }
        case OBJ_UPVALUE: {
            FREE(ObjUpvalue, object);
            break;
        }
    }
}

uint32_t hashString(const char* key, uint32_t length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

void printObject(Value value, ostream* output){
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            *output << AS_CSTRING(value);
            break;
        case OBJ_FUNCTION: {
            ObjString* name = AS_FUNCTION(value)->name;
            *output << "<raw-fn " << (name == nullptr ? "script" : (char*) name->array.contents) << ">";
            break;
        }
        case OBJ_NATIVE: {
            ObjString* name = AS_NATIVE(value)->name;
            *output << "<native-fn " << (name == nullptr ? "?" : (char*) name->array.contents) << ">";
            break;
        }
        case OBJ_CLOSURE: {
            ObjString *name = AS_CLOSURE(value)->function->name;
            *output << "<fn " << (name == nullptr ? "script" : (char *) name->array.contents) << ">";
            break;
        }
        case OBJ_UPVALUE:
            *output << "<upvalue>";
            break;
        default:
            *output << "Untagged Obj " << AS_OBJ(value);
    }
}

void printObject(Value value){
    printObject(value, &cout);
}

void printObjectOwnedAddresses(Value value){
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            cout << (void*) AS_CSTRING(value);
            break;
        case OBJ_FUNCTION:
            cout << (void*) AS_FUNCTION(value)->chunk;
            break;
        case OBJ_NATIVE:
            cout << "TODO";
            break;
        case OBJ_CLOSURE:
            cout << "TODO";
            break;
    }
}

void printObjectsList(Obj* head){
    Obj* object = head;
    while (object != nullptr) {
        Obj* next = object->next;
        cout << "          ";
        printObjectOwnedAddresses(OBJ_VAL(object));
        cout << " [";
        printObject(OBJ_VAL(object));
        cout << "]" << endl;
        object = next;
    }
}

ObjFunction* Memory::newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    function->chunk = new Chunk;
    function->upvalueCount = 0;
    return function;
}

ObjNative *Memory::newNative(NativeFn function, uint8_t arity, ObjString* name) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    native->arity = arity;
    native->name = name;
    return native;
}

ObjClosure* Memory::newClosure(ObjFunction* function) {
    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    // Not updating count here. Caller uses push on array list.
    closure->upvalues.growExact(function->upvalueCount, *this);
    for (int i=0;i<function->upvalueCount;i++) {
        closure->upvalues.data[i] = nullptr;
    }
    return closure;
}

ObjUpvalue* Memory::newUpvalue(Value* location) {
    ObjUpvalue* val = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    val->location = location;
    val->next = nullptr;
    val->closed = NIL_VAL();
    return val;
}

void* Memory::reallocate(void* pointer, size_t oldSize, size_t newSize) {
    return reallocate(pointer, oldSize, newSize, true);
}

void* Memory::reallocate(void* pointer, size_t oldSize, size_t newSize, bool allowGC){
    if (newSize == 0){
        free(pointer);
        return nullptr;
    }

    if (allowGC && enable && newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif
    }

    void* result = realloc(pointer, newSize);
    if (result == nullptr) {
        cerr << "Failed to reallocate from " << oldSize << " to " << newSize << endl;
        free(pointer);
        exit(1);
    }

    return result;
}

void Memory::collectGarbage() {
#ifdef DEBUG_LOG_GC
    cout << "-- gc begin\n";
#endif

    markRoots();
    traceReferences();
    strings->removeUnmarkedKeys();
    sweep();

#ifdef DEBUG_LOG_GC
    cout << "-- gc end\n";
#endif
}

void Memory::markRoots() {
    if (evilVmGlobal == nullptr) return;

    for (Value* slot=stack;slot<stackTop;slot++) {
        markValue(*slot);
    }

    ObjUpvalue* val = openUpvalues;
    while (val != nullptr) {
        markObject((Obj*) val);
        val = val->next;
    }

    for (uint32_t i=0;i<natives->capacity;i++){
        Entry* entry = natives->entries + i;
        if (!Table::isEmpty(entry)) {
            markObject((Obj*) entry->key);
            markValue(entry->value);
        }
    }

    if (evilCompilerGlobal != nullptr) ((Compiler*) evilCompilerGlobal)->markRoots();
}

void Memory::traceReferences() {
    while (!grayStack.empty()) {
        Obj* object = grayStack.back();
        grayStack.pop_back();
#ifdef DEBUG_LOG_GC
        printf("%p blacken ", (void*)object);
        printValue(OBJ_VAL(object));
        printf("\n");
#endif
        switch (object->type) {
            case OBJ_STRING:
            case OBJ_NATIVE:
                break;

            case OBJ_FUNCTION: {
                auto* function = (ObjFunction*) object;
                markObject((Obj*) function->name);
                for (int i=0;i<function->chunk->getConstantsSize();i++){
                    markValue(function->chunk->getConstant(i));
                }
                break;
            }
            case OBJ_CLOSURE: {
                auto* closure = (ObjClosure*) object;
                markObject((Obj*) closure->function);
                for (int i=0;i<closure->upvalues.count;i++){
                    markObject((Obj*) closure->upvalues[i]);
                }
                break;
            }
            case OBJ_UPVALUE: {
                auto* val = (ObjUpvalue*) object;
                markValue(val->closed);
                break;
            }
        }
    }
}

void Memory::sweep() {
    Obj** prevDotNext= &objects;
    Obj* object = objects;
    int markedCount = 0;
    while (object != nullptr) {
#ifdef DEBUG_LOG_GC
        printf("%p sweep\n", (void*)object);
#endif
        if (object->isMarked) {
            prevDotNext = &object->next;
            object = object->next;
            markedCount++;
        } else {
            *prevDotNext = object->next;
            freeObject(object);
            object = *prevDotNext;
        }
    }

    // TODO!: this sucks. instead just flip the expected flag each iteration
    //        the constructor will need to know the right initial flag
    object = objects;
    while (object != nullptr) {
        object->isMarked = false;
        object = object->next;
        markedCount--;
    }
    if (markedCount != 0) {
        cerr << "markedCount=" << markedCount << endl;
    }
}

void Memory::markValue(Value value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

void Memory::markObject(Obj* object) {
    if (object == nullptr) return;
    if (object->isMarked) return;
#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    object->isMarked = true;
    grayStack.push_back(object);
}
