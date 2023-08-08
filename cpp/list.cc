#ifndef clox_list_h
#define clox_list_h

#include "common.h"

class Memory;

template <typename T>
    class ArrayList {
        public:
            ArrayList();
            explicit ArrayList(bool canGc);
            explicit ArrayList(uint32_t size);
            ArrayList(T* source, uint length, Memory& gc);
            ArrayList(const ArrayList& other);
            ~ArrayList();
            void release(Memory& gc);
            void push(T value, Memory& gc);
            void set(uint index, T value);
            bool isEmpty();
            T pop();
            void popMany(int count);
            T& peek(uint indexFromFront);
            T& peekLast();
            void grow(uint delta, Memory& gc);
            void growExact(uint delta, Memory& gc);
            void appendMemory(T* source, uint length, Memory& gc);
            void append(const ArrayList<T>& source, Memory& gc);
            ArrayList<T> copy(Memory& gc);
            T remove(uint index);
            void shrink(Memory& gc);
            T& operator[](size_t index);

            T* data;
            uint32_t count;
            uint32_t capacity;
            bool canGC;

        int size() {
            return count;
        }

        void clear() {
            count = 0;
        }
    };

#include "object.h"

// Useful if what you really want is a nice interface over a fixed size array.
template<typename T>
ArrayList<T>::ArrayList(uint32_t size) {
    data = nullptr;
    count = 0;
    capacity = 0;
    growExact(size);
}

template<typename T>
T& ArrayList<T>::operator[](size_t index) {
#ifdef VM_SAFE_MODE
    if (index >= count)  {
        cerr << "ArrayList of count " << count << " can't get index " << index << endl;
    }
#endif
    return data[index];
}

template<typename T>
void ArrayList<T>::popMany(int countIn) {
    #ifdef VM_SAFE_MODE
    if (count < countIn) {
        cerr << "ArrayList of count " << count << " can't pop " << countIn << endl;
    }
    #endif
    count -= countIn;
}

template<typename T>
T ArrayList<T>::remove(uint index) {
    #ifdef VM_SAFE_MODE
    if (index >= count) {
        cerr << "ArrayList of count " << count << " can't remove index " << index << endl;
    }
    #endif

    T result = (*this)[index];
    count--;
    for (uint i=index;i<count;i++){
        set(i, (*this)[i + 1]);
    }

    return result;
}

template<typename T>
ArrayList<T>::ArrayList(T* source, uint32_t length, Memory& gc) {
    capacity = length;
    data = (T*) gc.reallocate(data, 0, sizeof(T) * capacity, canGC);
    memcpy(data, source, sizeof(T) * length);
    count = length;
}

template<typename T>
inline T& ArrayList<T>::peek(uint32_t indexFromFront) {
    #ifdef VM_SAFE_MODE
    if (indexFromFront > count) {
        cerr << "ArrayList of count " << count << " can't peek index " << indexFromFront  << endl;
    }
    #endif

    return (*this)[indexFromFront];
}

template<typename T>
inline T& ArrayList<T>::peekLast() {
    return peek(count - 1);
}

template<typename T>
inline bool ArrayList<T>::isEmpty() {
    return count == 0 || data == nullptr;
}

// Ensure there's space for at least <delta> new entries, resize if not.
// Still resizes by at least <capacity> to keep the amortized constant time push().
template<typename T>
void ArrayList<T>::grow(uint32_t delta, Memory& gc) {
    if (capacity >= count + delta) return;
    int oldCapacity = capacity;
    capacity += capacity > delta ? (isEmpty() ? 8 : capacity) : delta;
    data = (T*) gc.reallocate(data, sizeof(T) * oldCapacity, sizeof(T) * capacity, canGC);
}

// Removes wasted space.
// Only call this when you think there will be no more adds, or you lose the amortized constant time push().
// I think it doesn't even need to copy everything,
// because realloc should just be able to shrink the memory block from the end instead of making a new one.
template<typename T>
void ArrayList<T>::shrink(Memory& gc) {
    data = (T*) gc.reallocate(data, sizeof(T) * capacity, sizeof(T) * count, canGC);
    capacity = count;
}

// Ensure there's space for at least <delta> new entries, resize if not but don't create additional capacity beyond what's requested.
// If you know ahead of time exactly how big the list will be,
// we can avoid the memory shuffling of allocating double with grow() and then calling shrink() at the end.
// Normal grow has much better time performance for unknown sizes.
// Using growExact() once on an empty list makes it just a nicer interface to a normal fixed size array.
template<typename T>
void ArrayList<T>::growExact(uint delta, Memory& gc) {
    if (capacity > count + delta) return;
    int oldCapacity = capacity;
    capacity = count + delta;
    data = (T*) gc.reallocate(data, sizeof(T) * oldCapacity, sizeof(T) * capacity, canGC);
}

template<typename T>
void ArrayList<T>::set(uint32_t index, T value) {
    #ifdef VM_SAFE_MODE
    if (index >= count) {
        cerr << "ArrayList of count " << count << " can't set index " << index << endl;
    }
    #endif

    data[index] = value;
}

template <typename T>
ArrayList<T>::ArrayList() : ArrayList(false){}

template <typename T>
ArrayList<T>::ArrayList(bool canGC){
    data = nullptr;
    count = 0;
    capacity = 0;
    this->canGC = canGC;
}

template <typename T>
ArrayList<T>::~ArrayList(){
    if (capacity > 0) {
        cout << "Leaked list!\n";
    }
    data = 0;  // crash if access
}

template <typename T>
void ArrayList<T>::release(Memory& gc){
    gc.reallocate(data, sizeof(T) * capacity, 0, canGC);
    data = 0;
    capacity = 0;
    count = 0;
}

template <typename T>
void ArrayList<T>::push(T value, Memory& gc){
    grow(1, gc);
    data[count] = value;
    count++;
}

template <typename T>
T ArrayList<T>::pop(){
    #ifdef VM_SAFE_MODE
    if (isEmpty()) {
        cerr << "Empty ArrayList can't pop" << endl;
    }
    #endif

    count--;
    return data[count];
}

// Allowing a shallow copy doesn't make sense.
// Modifying one would change the data in the array for both but not the metadata for the other one,
// since count and capacity aren't references (and what happens on delete if someone else might still need it).
// So if the code wants that then really they should just be passing around a pointer to the same array list.
// I think the deep copy should be explicit because I'm afraid of accidentally using the copy constructor by being bad at c++.
template<typename T>
ArrayList<T>::ArrayList(const ArrayList &other) {
    cerr << "Do not implicitly call ArrayList copy constructor." << endl;
}

// Performs a deep copy so the new one owns its memory.
template<typename T>
ArrayList<T> ArrayList<T>::copy(Memory& gc) {
    ArrayList<T> other;
    other.append(this, gc);
    return other;
}

template<typename T>
void ArrayList<T>::appendMemory(T* source, uint32_t length, Memory& gc) {
    grow(length, gc);
    memcpy(&data[count * sizeof(T)], source, sizeof(T) * length);
    count += length;
}

template<typename T>
void ArrayList<T>::append(const ArrayList<T>& source, Memory& gc) {
    appendMemory(source.data, source.count, gc);
}

#endif
