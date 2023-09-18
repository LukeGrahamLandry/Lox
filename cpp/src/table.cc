#include "table.h"

#define TABLE_MAX_LOAD 0.75
// Note: staying a power of 2 is important
#define GROW_CAPACITY(old) (old == 0 ? 8 : old * 2)

Table::Table(Memory& gc) : gc(gc) {
    count = 0;
    capacity = 0;
    entries = nullptr;
    maxEntries = 0;
}

Table::~Table() {
    gc.FREE_ARRAY(Entry, entries, capacity);
    count = 0;
    capacity = 0;
    entries = nullptr;
    maxEntries = 0;
}

// returns true if the key was not already in the table. false if it was in the table and its value was replaced.
bool Table::set(ObjString *key, Value value) {
    Entry* entry = findEntry(key);
    return setEntry(entry, key, value);
}

// returns true if the key was not already in the table. false if it was in the table and its value was replaced.
// This does the resize check, so the entry pointer may point to garbage after calling.
bool Table::setEntry(Entry* entry, ObjString *key, Value value){
    bool isNewKey = isEmpty(entry);
    if (isNewKey && IS_NIL(entry->value)) {
        if (count + 1 > maxEntries) {
            adjustCapacity();
            entry = findEntry(key);
        }
        count++;
    }
    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool Table::get(ObjString *key, Value *valueOut) {
    if (count == 0) return false;
    Entry* entry = findEntry(key);
    if (isEmpty(entry)) return false;
    *valueOut = entry->value;
    return true;
}

// does not decrement count because the tombstone is still there and should be included in load.
bool Table::remove(ObjString *key) {
    if (count == 0) return false;
    Entry* entry = findEntry(key);
    if (isEmpty(entry)) return false;
    entry->key = nullptr;
    entry->value = BOOL_VAL(true);
    return true;
}

void Table::removeAll(){
    gc.FREE_ARRAY(Entry, entries, capacity);
    maxEntries = 0;
    count = 0;
    capacity = 0;
    entries = nullptr;
}

Entry* Table::findEntry(ObjString *key){
    if (capacity == 0) adjustCapacity();
    return findEntry(entries, capacity, key);
}

Entry* Table::findEntry(Entry* firstInTable, uint32_t tableCapacity, ObjString *key){
    uint32_t index = key->hash & (tableCapacity - 1);
    Entry* tombstone = nullptr;

    // we know this terminates because there will always be an empty slot since LOAD_FACTOR < 1
    for (;;){
        Entry* slot = firstInTable + index;
        if (isEmpty(slot)){
            if (IS_NIL(slot->value)){  // actually empty
                return tombstone == nullptr ? slot : tombstone;
            } else {  // tombstone
                if (tombstone == nullptr) tombstone = slot;
            }
        } else if (slot->key == key){  // found key. since strings are interned, this equality check works.
            return slot;
        }

        index = (index + 1) & (tableCapacity - 1);
    }
}

void Table::adjustCapacity() {
    Entry* oldEntries = entries;
    uint32_t oldCapacity = capacity;
    uint32_t newCapacity = GROW_CAPACITY(oldCapacity);

    Entry* newEntries = (Entry*) gc.reallocate(nullptr, 0, sizeof(Entry) * newCapacity);
    // allocate doesn't initialize to 0 so there will just be random garbage there,
    // but we need to be able to check for empty slots safely when inserting.
    for (int i = 0; i < newCapacity; i++) {
        setEmpty(newEntries + i);
    }

    // Move all entries into their slots in the new array.
    int newCount = 0;
    for (int i=0;i<oldCapacity;i++){
        Entry* original = oldEntries + i;
        if (isEmpty(original)) continue;

        newCount++;
        Entry* slot = findEntry(newEntries, newCapacity, original->key);
        slot->key = original->key;
        slot->value = original->value;
    }

    gc.FREE_ARRAY(Entry, oldEntries, oldCapacity);
    entries = newEntries;
    capacity = newCapacity;
    count = newCount;
    maxEntries = (int) (newCapacity * TABLE_MAX_LOAD);
}

// Uses slow equality on the keys to ensure deduplication.
// But values pointing to the old ones aren't reset so maybe this is a terrible system.
void Table::safeAddAll(const Table& from) {
    for (int i=0;i<from.capacity;i++){
        Entry fromEntry = from.entries[i];
        if (isEmpty(&fromEntry)) continue;
        safeSet(fromEntry.key, fromEntry.value);
    }
}

// This uses memory addresses for equality so the keys must come from a deduplicated set.
void Table::addAll(const Table& from) {
    for (int i=0;i<from.capacity;i++){
        Entry fromEntry = from.entries[i];
        if (isEmpty(&fromEntry)) continue;
        set(fromEntry.key, fromEntry.value);
    }
}

// Uses slow equality on the keys to ensure deduplication.
// return: was this a new key for the table? false means a previous value was replaced.
// if it returns false, that means that this key has the same equality as the old one
// but, it might have a different memory address so the caller has to deal with that.
bool Table::safeSet(ObjString *key, Value value) {
    Entry* toEntry;
    safeFindEntry((char*) key->array.contents, key->array.length - 1, key->hash, &toEntry);
    return setEntry(toEntry, key, value);
}

// returns true if it was added and false if it was already there.
bool Set::add(ObjString *key) {
    return set(key, NIL_VAL());
}

bool Table::contains(ObjString *key) {
    Value unused;
    return get(key, &unused);
}


// Does the slow full equality check instead of comparing memory addresses.
// This allows for deduplication where identical keys don't get multiple entries because their memory address is used as the definitive equality check at get.
// The keys used in a table should always be put through a hash set using this method.
// returns true if the string is already a key in the table.
// <outEntry> is set to either the entry containing the value or the best one to put it in if not found.
bool Table::safeFindEntry(const char* chars, uint32_t length, uint32_t hash, Entry** outEntry){
    if (capacity == 0) adjustCapacity();
    uint32_t index = hash & (capacity - 1);
    Entry* tombstone = nullptr;
    for (;;) {
        Entry* slot = entries + index;
        if (isEmpty(slot)) {
            if (IS_NIL(slot->value)){  // actually empty
                *outEntry = tombstone == nullptr ? slot : tombstone;
                return false;
            } else {  // tombstone
                if (tombstone == nullptr) tombstone = slot;
            }
        } else if (slot->key->array.length-1 == length &&
                    slot->key->hash == hash &&
                    memcmp(slot->key->array.contents, chars, length) == 0) {
            *outEntry = slot;
            return true;
        }

        index = (index + 1) & (capacity - 1);
    }
}

void Table::printContents() const {
    cout << "   HashTable count=" << count << " capacity=" << capacity << endl;
    for (uint32_t i=0;i<capacity;i++){
        Entry* entry = entries + i;
        if (!isEmpty(entry)){
            cout << "          ";
            printObjectOwnedAddresses(OBJ_VAL(entry->key));
            cout << " [";
            for (uint32_t j=0;j<entry->key->array.length-1;j++){
                cout << ((char*) entry->key->array.contents)[j];
            }
            cout << "] = ";
            if (IS_STRING(entry->value)){
                printObjectOwnedAddresses(entry->value);
                cout << " [";
                printObject(entry->value);
                cout << "]" << endl;
            } else {
                cout << "val: ";
                printValue(entry->value);
                cout << endl;
            }
        }
    }
}

void Table::removeUnmarkedKeys() {
    for (uint32_t i=0;i<capacity;i++){
        Entry* entry = entries + i;
        if (!isEmpty(entry) && !entry->key->array.obj.isMarked) {
#ifdef DEBUG_LOG_GC
            printf("%p table drop ", (void*)entry->key);
            printValue(OBJ_VAL(entry->key));
            cout << endl;
#endif
            remove(entry->key);  // TODO: dont need to re-find the entry
        }
    }
}