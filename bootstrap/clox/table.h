#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

typedef struct {
    ObjString* key;
    Value value;
} Entry;

class Table {
public:
    Table();
    ~Table();

    int count;
    int capacity;
    Entry* entries;
    int maxEntries;

    bool set(ObjString* key, Value value);
    void adjustCapacity();
    void safeAddAll(const Table& from);
    bool get(ObjString* key, Value* valueOut);
    bool remove(ObjString* key);
    bool contains(ObjString* key);

    Entry* findEntry(ObjString *key);
    bool setEntry(Entry* entry, ObjString *key, Value value);
    void removeAll();
    void printContents();
    bool safeFindEntry(const char* chars, int length, uint32_t hash, Entry** outEntry);

protected:
    static Entry* findEntry(Entry* firstInTable, int tableCapacity, ObjString *key);

    static inline bool isEmpty(Entry* entry){
        return entry->key == nullptr;
    }

    // The key is a pointer to the object, we don't own the object's memory, so just discarding our reference to it is fine.
    // The value is an actual instance of the struct since they're always passed by value. If it's not an Obj, discarding it doesn't matter.
    // Whoever gave it to the table should still have their copy that, if an object, points to the same memory so the caller still owns that
    // address, and we don't have to worry about freeing it.
    static inline void setEmpty(Entry* entry){
        entry->key = nullptr;
        entry->value = NIL_VAL();
    }

    bool safeSet(ObjString *key, Value value);

    void addAll(const Table &from);
};

class Set : public Table {
public:
    bool add(ObjString* key);
};

#endif