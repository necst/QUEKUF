#ifndef QEC_UNION_FIND_VECTOR_H
#define QEC_UNION_FIND_VECTOR_H

#include "Defines.h"

template<class T, unsigned int SIZE = SYN_LEN>
class Vector
{
public:
    T array[SIZE];
    uint8_t size = 0;

    Vector()
    {
//#pragma HLS ARRAY_PARTITION variable=array type=complete
        size = 0;
    }

    void reset()
    {
        size = 0;
    }

    void pushIn(T el)
    {
        array[size] = el;
        size++;
    }

    void erase(uint8_t pos)
    {
        ERASING:
        for(int i = 0; i < SIZE; i++)
        {
            if(i >= pos && i+1 < SIZE)
            {
                T tmp = array[i+1];
                array[i] = tmp;
            }
        }
        size--;
    }

    void eraseElement(T el)
    {
        int tmp;
        FINDING_TO_ERASE:
        for(int i = 0; i < SIZE; i++)
        {
#pragma HLS UNROLL
            if(i < size && array[i] == el)
            {
                tmp = i;
            }
        }
        erase(tmp);
    }

    T at(uint8_t pos)
    {
#pragma HLS INLINE
        return array[pos];
    }

    uint8_t getSize()
    {
#pragma HLS INLINE
        return size;
    }

    Vector<T>& operator=(const Vector<T>& vec)
    {
        EQUAL_OPERATOR:
        for(int i = 0; i < SIZE; i++)
        {
#pragma HLS UNROLL
            array[i] = vec.array[i];
        }
        size = vec.size;

        return *this;
    }

};

#endif //QEC_UNION_FIND_VECTOR_H
