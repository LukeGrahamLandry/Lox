#ifndef clox_list_h
#define clox_list_h

#include "memory.h"

template <typename T>
    class ArrayList {
        public:
            ArrayList();
            ~ArrayList();
            void add(T value);
            T get(int index);

            T* data;
            int count;
            int capacity;
    };

template <typename T>
    ArrayList<T>::ArrayList(){
        this->data = NULL;
        this->count = 0;
        this->capacity = 0;
    }

template <typename T>
    ArrayList<T>::~ArrayList(){
        reallocate(data, sizeof(T) * capacity, 0);
    }

template <typename T>
    void ArrayList<T>::add(T value){
        if (capacity < count + 1){
            int oldCapacity = capacity;
            capacity = capacity * 2;
            if (capacity < 8){
                capacity = 8;
            }

            data = (T*) reallocate(data, sizeof(T) * oldCapacity, sizeof(T) * capacity);
        }
        data[count] = value;
        count++;
    }

template <typename T>
    T ArrayList<T>::get(int index){
        return data[index];
    }

#endif
