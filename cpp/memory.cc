#include "memory.h"
#include <cstdlib>
#include <cstdio>
#include "debug.h"
#include "vm.h"
#include "compiler/compiler.h"

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    return reallocate(pointer, oldSize, newSize, true);
}

void* reallocate(void* pointer, size_t oldSize, size_t newSize, bool allowGC){
    if (newSize == 0){
        free(pointer);
        return nullptr;
    }

    if (allowGC && newSize > oldSize) {
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

void traceReferences();
void sweep();

void collectGarbage() {
#ifdef DEBUG_LOG_GC
    cout << "-- gc begin\n";
#endif

    VM* vm = (VM*) evilVmGlobal;

    markRoots();
    traceReferences();
    vm->strings.removeUnmarkedKeys();
    sweep();

#ifdef DEBUG_LOG_GC
    cout << "-- gc end\n";
#endif
}

void markRoots() {
    if (evilVmGlobal == nullptr) return;

    VM* vm = (VM*) evilVmGlobal;
    for (Value* slot=vm->stack;slot<vm->stackTop;slot++) {
        markValue(*slot);
    }

    for (int i=0;i<vm->frameCount;i++) {
        markObject((Obj*) vm->frames[i].closure);
    }

    ObjUpvalue* val = vm->openUpvalues;
    while (val != nullptr) {
        markObject((Obj*) val);
        val = val->next;
    }

    if (evilCompilerGlobal != nullptr) ((Compiler*) evilCompilerGlobal)->markRoots();
}

void traceReferences() {
    VM* vm = (VM*) evilVmGlobal;
    while (!vm->grayStack->isEmpty()) {
        Obj* object = vm->grayStack->pop();
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

void sweep() {
    VM* vm = (VM*) evilVmGlobal;
    Obj** prevDotNext= &vm->objects;
    Obj* object = vm->objects;
    while (object != nullptr) {
#ifdef DEBUG_LOG_GC
        printf("%p sweep\n", (void*)object);
#endif
        if (object->isMarked) {
            object = object->next;
        } else {
            *prevDotNext = object->next;
            freeObject(object);
            object = *prevDotNext;
        }

        prevDotNext = &object->next;
    }

    // TODO: this sucks
    object = vm->objects;
    while (object != nullptr) {
        object->isMarked = false;
        object = object->next;
    }
}

void markValue(Value value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

void markObject(Obj* object) {
    if (object == nullptr) return;
#ifdef DEBUG_LOG_GC
    printf("%p mark (%d) ", (void*)object, object->isMarked);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif
    if (object->isMarked) return;
    object->isMarked = true;

    VM* vm = (VM*) evilVmGlobal;
    vm->grayStack->push(object);
}
