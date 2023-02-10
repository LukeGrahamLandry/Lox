#ifndef clox_list_h
#define clox_list_h

#include "memory.h"

template <typename T>
    class ArrayList {
        public:
            ArrayList();
            ArrayList(T* source, uint length);
            ArrayList(const ArrayList& other);
            ~ArrayList();
            void push(T value);
            T get(uint index);
            void set(uint index, T value);
            bool isEmpty();
            T pop();
            T* peek(uint indexFromFront);
            T* peekLast();
            void grow(uint delta);
            void appendMemory(T* source, uint length);
            ArrayList<T> copy();

            T* data;
            uint32_t count;
            uint32_t capacity;
    };

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
        if (indexFromFront > count) cerr << "ArrayList of count " << count << " can't peek index " << indexFromFront  << endl;
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

template<typename T>
    void ArrayList<T>::grow(uint32_t delta) {
        if (capacity > count + delta) return;
        int oldCapacity = capacity;
        capacity += capacity > delta ? (isEmpty() ? 8 : capacity) : delta;
        data = (T*) reallocate(data, sizeof(T) * oldCapacity, sizeof(T) * capacity);
    }

template<typename T>
    void ArrayList<T>::set(uint32_t index, T value) {
        #ifdef VM_SAFE_MODE
        if (index >= count) cerr << "ArrayList of count " << count << " can't set index " << index << endl;
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
        if (isEmpty()) cerr << "Empty ArrayList can't pop" << endl;
        #endif

        count--;
        return data[count];
    }

template <typename T>
    T ArrayList<T>::get(uint32_t index){
        #ifdef VM_SAFE_MODE
        if (index >= count)  cerr << "ArrayList of count " << count << " can't get index " << index << endl;
        #endif

        return data[index];
    }

template<typename T>
    ArrayList<T>::ArrayList(const ArrayList &other) {
        cerr << "Do not implicitly call ArrayList copy constructor." << endl;
    }

template<typename T>
    ArrayList<T> ArrayList<T>::copy() {
        ArrayList<T> other;
        other.appendMemory(data, count);
        other.capacity = count;
        return other;
    }

template<typename T>
    void ArrayList<T>::appendMemory(T* source, uint32_t length) {
        grow(length);
        memcpy(&data[count * sizeof(T)], source, sizeof(T) * length);
        count += length;
    }
#endif
