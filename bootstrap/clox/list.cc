#ifndef clox_list_h
#define clox_list_h

#include "memory.h"

template <typename T>
    class ArrayList {
        public:
            ArrayList();
            ArrayList(const ArrayList& other);
            ~ArrayList();
            void push(T value);
            T get(int index);
            void set(int index, T value);
            void resize();
            bool isEmpty();
            T pop();
            T* peek(int indexFromFront);
            T* peekLast();

            T* data;
            int count;
            int capacity;

    };

template<typename T>
    inline T* ArrayList<T>::peek(int indexFromFront) {
        if (isEmpty()) cerr << "Cannot peek empty ArrayList." << endl;
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
    void ArrayList<T>::resize() {
        if (capacity < count + 1){
            int oldCapacity = capacity;
            capacity = isEmpty() ? 8 : capacity * 2;
            data = (T*) reallocate(data, sizeof(T) * oldCapacity, sizeof(T) * capacity);
        }
    }

template<typename T>
    void ArrayList<T>::set(int index, T value) {
        if (index >= count || index < 0){
            cerr << "ArrayList of count " << count << " tried to set at index " << index << endl;
        }
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
        resize();
        data[count] = value;
        count++;
    }

template <typename T>
    T ArrayList<T>::pop(){
        count--;
        return data[count];
    }

template <typename T>
    T ArrayList<T>::get(int index){
        return data[index];
    }

template<typename T>
    ArrayList<T>::ArrayList(const ArrayList &other) {
        data = (T*) reallocate(nullptr, 0, sizeof(T) * other.count);
        memcpy(data, other.data, sizeof(T) * other.count);
        capacity = other.count;
    }
#endif
