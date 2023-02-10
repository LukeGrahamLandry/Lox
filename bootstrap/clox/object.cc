#include "object.h"

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "common.h"

#define ALLOCATE_OBJ(type, objectType) \
        (type*)allocateObject(sizeof(type), objectType)

bool isObjType(Value value, ObjType type){
    return value.type == VAL_OBJ && AS_OBJ(value)->type == type;
}

// The Obj wants to own its memory, so it can garbage collect if it wants.
// So if you're looking at someone else's string, like the src from the scanner, you need to copy it.
// <length> must not include the null terminator.
// TODO: flag for being a constant string where the object doesnt own the memory, just point into the src string.
ObjString* copyString(Set* internedStrings, Obj** objectsHead, const char* chars, int length){
    uint32_t hash = hashString(chars, length);

    Entry* slot;
    ObjString* str;
    bool wasInterned = internedStrings->safeFindEntry(chars, length, hash, &slot);
    if (wasInterned){
        str = slot->key;
    } else {
        char* ptr = ALLOCATE(char, length + 1);
        memcpy(ptr, chars, length);
        ptr[length] = '\0';

        str = allocateString(ptr, length, hash);
        internedStrings->setEntry(slot, str, NIL_VAL());
        linkObjects(objectsHead, RAW_OBJ(str));
    }

    return str;
}

// Used when we already own that memory.
// The caller is giving away the memory to the new ObjString returned, so we're allowed to free it if it's a duplicate.
// This requires <chars> to be a null terminated string with the terminator NOT included in the <length>.
ObjString* takeString(Set* internedStrings, Obj** objectsHead, char* chars, int length) {
    uint32_t hash = hashString(chars, length);

    Entry* slot;
    ObjString* str;
    bool wasInterned = internedStrings->safeFindEntry(chars, length, hashString(chars, length), &slot);
    if (wasInterned){
        str = slot->key;
        freeStringChars(chars, length);
    } else {
        str = allocateString(chars, length, hash);
        internedStrings->setEntry(slot, str, NIL_VAL());
        linkObjects(objectsHead, RAW_OBJ(str));
    }

    return str;
}

ObjString* allocateString(char* chars, int length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
//    cout << "String Allocated [";
//    printValue(OBJ_VAL(string));
//    cout << "]" << endl;
    return string;
}

// Note: this doesn't push it to the linked list! The caller is responsible for not loosing it.
Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = RAW_OBJ(reallocate(nullptr, 0, size));
    object->type = type;

    return object;
}

void freeObject(Obj* object){
    switch (object->type) {
        case OBJ_STRING: {
            // We own the char array.
            ObjString* string = (ObjString*)object;
            freeStringChars(string->chars, string->length);
            FREE(ObjString, object);
            break;
        }
    }
}

uint32_t hashString(const char* key, int length) {
    if (length < 0) cerr << "Cannot hash string of length " << length << endl;

    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

void printObject(Value value){
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            cout << AS_CSTRING(value);
            break;
        default:
            cout << "Untagged Obj " << AS_OBJ(value);
    }
}

void printObjectOwnedAddresses(Value value){
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            cout << (void*) AS_CSTRING(value);
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