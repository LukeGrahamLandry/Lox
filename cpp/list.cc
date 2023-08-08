#ifndef clox_list_h
#define clox_list_h

#include "memory.h"

template <typename T>
    class ArrayList {
        public:
            ArrayList();
            ArrayList(uint32_t size);
            ArrayList(T* source, uint length);
            ArrayList(const ArrayList& other);
            ~ArrayList();
            void push(T value);
            T get(uint index);
            void set(uint index, T value);
            bool isEmpty();
            T pop();
            void popMany(int count);
            T* peek(uint indexFromFront);
            T* peekLast();
            void grow(uint delta);
            void growExact(uint delta);
            void appendMemory(T* source, uint length);
            void append(ArrayList<T>* source);
            ArrayList<T> copy();
            T remove(uint index);
            void shrink();

            T* data;
            uint32_t count;
            uint32_t capacity;

        int size() {
            return count;
        }

        void clear() {
            count = 0;
        }
    };

// Useful if what you really want is a nice interface over a fixed size array.
template<typename T>
ArrayList<T>::ArrayList(uint32_t size) {
    data = nullptr;
    count = 0;
    capacity = 0;
    growExact(size);
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

    T result = get(index);
    count--;
    for (uint i=index;i<count;i++){
        set(i, get(i + 1));
    }

    return result;
}

template<typename T>
ArrayList<T>::ArrayList(T* source, uint32_t length) {
    capacity = length;
    data = (T*) reallocate(data, 0, sizeof(T) * capacity);
    memcpy(data, source, sizeof(T) * length);
    count = length;
}

template<typename T>
inline T* ArrayList<T>::peek(uint32_t indexFromFront) {
    #ifdef VM_SAFE_MODE
    if (indexFromFront > count) {
        cerr << "ArrayList of count " << count << " can't peek index " << indexFromFront  << endl;
    }
    #endif

    return data + indexFromFront;
}

template<typename T>
inline T* ArrayList<T>::peekLast() {
    return peek(count - 1);
}

template<typename T>
inline bool ArrayList<T>::isEmpty() {
    return count == 0 || data == nullptr;
}

// Ensure there's space for at least <delta> new entries, resize if not.
// Still resizes by at least <capacity> to keep the amortized constant time push().
template<typename T>
void ArrayList<T>::grow(uint32_t delta) {
    if (capacity > count + delta) return;
    int oldCapacity = capacity;
    capacity += capacity > delta ? (isEmpty() ? 8 : capacity) : delta;
    data = (T*) reallocate(data, sizeof(T) * oldCapacity, sizeof(T) * capacity);
}

// Removes wasted space.
// Only call this when you think there will be no more adds, or you lose the amortized constant time push().
// I think it doesn't even need to copy everything,
// because realloc should just be able to shrink the memory block from the end instead of making a new one.
template<typename T>
void ArrayList<T>::shrink() {
    data = (T*) reallocate(data, sizeof(T) * capacity, sizeof(T) * count);
    capacity = count;
}

// Ensure there's space for at least <delta> new entries, resize if not but don't create additional capacity beyond what's requested.
// If you know ahead of time exactly how big the list will be,
// we can avoid the memory shuffling of allocating double with grow() and then calling shrink() at the end.
// Normal grow has much better time performance for unknown sizes.
// Using growExact() once on an empty list makes it just a nicer interface to a normal fixed size array.
template<typename T>
void ArrayList<T>::growExact(uint delta) {
    if (capacity > count + delta) return;
    int oldCapacity = capacity;
    capacity = count + delta;
    data = (T*) reallocate(data, sizeof(T) * oldCapacity, sizeof(T) * capacity);
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
ArrayList<T>::ArrayList(){
    data = nullptr;
    count = 0;
    capacity = 0;
}

template <typename T>
ArrayList<T>::~ArrayList(){
    reallocate(data, sizeof(T) * capacity, 0);
}

template <typename T>
void ArrayList<T>::push(T value){
    grow(1);
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

template <typename T>
T ArrayList<T>::get(uint32_t index){
    #ifdef VM_SAFE_MODE
    if (index >= count)  {
        cerr << "ArrayList of count " << count << " can't get index " << index << endl;
    }
    #endif

    return data[index];
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
ArrayList<T> ArrayList<T>::copy() {
    ArrayList<T> other;
    other.append(this);
    return other;
}

template<typename T>
void ArrayList<T>::appendMemory(T* source, uint32_t length) {
    grow(length);
    memcpy(&data[count * sizeof(T)], source, sizeof(T) * length);
    count += length;
}

template<typename T>
void ArrayList<T>::append(ArrayList<T> *source) {
    appendMemory(source->data, source->count);
}

#endif
