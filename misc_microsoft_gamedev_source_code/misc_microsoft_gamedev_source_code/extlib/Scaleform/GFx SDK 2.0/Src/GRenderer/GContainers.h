/**********************************************************************

Filename    :   GContainers.h
Content     :   
Created     :   2005-2006
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   Specialized simple containers and functions

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GCONTAINERS_H
#define INC_GCONTAINERS_H

#include <string.h>


// ***** Declared Classes
template<class T> struct GPodAllocator;
template<class T> class GPodVector;
//template<class T, unsigned S=6> class GPodBVector;



#ifndef GCONTAINERS_STANDALONE
//----------------------------------------------------------------------------
#include "GTLTypes.h"
#include "GMemory.h"

template<class T> struct GPodAllocator
{
    // The policy of POD containers is that they 
    // don't need to be constructed or destructed. 
    // Even if the data type has nontrivial constructors 
    // the default constructor must be trivial and should not
    // initialize anything. Similarly, it's prohibited to 
    // have any nontrivial destructor. It all means that there's
    // no need to call construct_array and destruct_array.
    static T* allocate_array(unsigned size) 
    { 
        T* v = (T*)GALLOC(sizeof(T) * size);
        //if(v)
        //{
        // GTL::gconstruct_array<T>(v, size);
        //}
        return v;
    }
    static void free_array(T* v, unsigned size) 
    { 
        if(v)
        {
            GUNUSED(size);
        //    GTL::gdestruct_array<T>(v, size);
            GFREE(v);
        }
    }
};
//----------------------------------------------------------------------------
#else
//----------------------------------------------------------------------------
template<class T> struct GPodAllocator
{
    static T*   allocate_array(unsigned size)   { return new T [size]; }
    static void free_array(T* v, unsigned)      { delete [] v; }
};
#endif

// A simple class template to store Plain Old Data, similar to std::deque
// It doesn't reallocate memory but instead, uses blocks of data of size 
// of (1 << S), that is, power of two. The data is NOT contiguous in memory, 
// so the only valid access methods are operator [], at(), valueAt(), last()
// 
// There reallocs occur only when the pool of pointers to blocks needs 
// to be extended (it happens very rarely). You can control the value 
// of increment to reallocate the pointer buffer. See the second constructor.
// By default, the increment value equals (1 << S), i.e., the block size.
// 
//-------------------
// The code of this class template was taken from the Anti-Grain Geometry
// Project and modified for the use by Scaleform. 
// Permission to use without restrictions is hereby granted to 
// Scaleform Corp. by the author of Anti-Grain Geometry Project.
//------------------------------------------------------------------------
template<class T, unsigned S=6> class GPodBVector
{
public:
    enum BlockScale_e
    {   
        BlockShift = S,
        BlockSize  = 1 << BlockShift,
        BlockMask  = BlockSize - 1
    };

    typedef T ValueType;

    ~GPodBVector()
    {
        if(NumBlocks)
        {
            T** blk = Blocks + NumBlocks - 1;
            while(NumBlocks--)
            {
                GPodAllocator<T>::free_array(*blk, BlockSize);
                --blk;
            }
            GPodAllocator<T*>::free_array(Blocks, MaxBlocks);
        }
    }

    GPodBVector() : 
        Size(0),
        NumBlocks(0),
        MaxBlocks(0),
        Blocks(0),
        BlockPtrInc(BlockSize)
    {}

    GPodBVector(unsigned blockPtrInc) :
        Size(0),
        NumBlocks(0),
        MaxBlocks(0),
        Blocks(0),
        BlockPtrInc(blockPtrInc)
    {}

    void removeAll()  { Size = 0; }
    void clear()      { Size = 0; } // Compatibility with STL

    void add(const T& val)
    {
        *dataPtr() = val;
        ++Size;
    }

    void push_back(const T& val)      // Compatibility with STL
    { 
        add(val); 
    }

    void removeLast()
    {
        if(Size) --Size;
    }

    void cutAt(unsigned size)
    {
        if(size < Size) Size = size;
    }

    void insertAt(unsigned pos, const T& val)
    {
        if(pos >= Size) 
        {
            add(val);
        }
        else
        {
            dataPtr();
            ++Size;
            unsigned i;
            for(i = Size-1; i > pos; --i)
            {
                at(i) = at(i - 1);
            }
            at(pos) = val;
        }
    }

    void removeAt(unsigned pos)
    {
        if(Size)
        {
            for(++pos; pos < Size; pos++)
            {
                at(pos-1) = at(pos);
            }
            --Size;
        }
    }

    unsigned size() const 
    { 
        return Size; 
    }

    const T& operator [] (unsigned i) const
    {
        return Blocks[i >> BlockShift][i & BlockMask];
    }

    T& operator [] (unsigned i)
    {
        return Blocks[i >> BlockShift][i & BlockMask];
    }

    const T& at(unsigned i) const
    { 
        return Blocks[i >> BlockShift][i & BlockMask];
    }

    T& at(unsigned i) 
    { 
        return Blocks[i >> BlockShift][i & BlockMask];
    }

    T valueAt(unsigned i) const
    { 
        return Blocks[i >> BlockShift][i & BlockMask];
    }

    const T& last() const
    {
        return (*this)[Size - 1];
    }

    T& last()
    {
        return (*this)[Size - 1];
    }

private:
    // Copying is prohibited
    GPodBVector(const GPodBVector<T, S>& v);
    const GPodBVector<T, S>& operator = (const GPodBVector<T, S>& v);

    void allocateBlock(unsigned nb)
    {
        if(nb >= MaxBlocks) 
        {
            T** newBlocks = 
                GPodAllocator<T*>::allocate_array(MaxBlocks + BlockPtrInc);

            if(Blocks)
            {
                memcpy(newBlocks, 
                       Blocks, 
                       NumBlocks * sizeof(T*));

                GPodAllocator<T*>::free_array(Blocks, MaxBlocks);
            }
            Blocks = newBlocks;
            MaxBlocks += BlockPtrInc;
        }
        Blocks[nb] = GPodAllocator<T>::allocate_array(BlockSize);
        NumBlocks++;
    }

    T* dataPtr()
    {
        unsigned nb = Size >> BlockShift;
        if(nb >= NumBlocks)
        {
            allocateBlock(nb);
        }
        return Blocks[nb] + (Size & BlockMask);
    }

    unsigned Size;
    unsigned NumBlocks;
    unsigned MaxBlocks;
    T**      Blocks;
    unsigned BlockPtrInc;
};






// A simple class template to store Plain Old Data, a vector
// of a fixed size. The data is continuous in memory
//-------------------
// The code of this class template was taken from the Anti-Grain Geometry
// Project and modified for the use by Scaleform. 
// Permission to use without restrictions is hereby granted to 
// Scaleform Corp. by the author of Anti-Grain Geometry Project.
//------------------------------------------------------------------------
template<class T> class GPodVector
{
public:
    typedef T ValueType;

    ~GPodVector() { GPodAllocator<T>::free_array(Array, Capacity); }
    GPodVector() : Size(0), Capacity(0), Array(0) {}
    GPodVector(unsigned cap, unsigned extraTail=0) :
        Size(0), 
        Capacity(cap + extraTail), 
        Array(GPodAllocator<T>::allocate_array(Capacity)) 
    {}

    GPodVector(const GPodVector<T>& v) : 
        Size(v.Size), 
        Capacity(v.Capacity),
        Array(Capacity ? GPodAllocator<T>::allocate_array(Capacity) : 0)
    {
        if(Size) memcpy(Array, v.Array, sizeof(ValueType) * Size);
    }

    void removeAll() 
    { 
        Size = 0; 
    }

    void clear()           // Compatibility with STL
    { 
        Size = 0; 
    }

    void cutAt(unsigned num) 
    { 
        if(num < Size) Size = num; 
    }

    // Set new capacity. All data is lost, size is set to zero.
    void capacity(unsigned cap, unsigned extraTail=0)
    {
        Size = 0;
        if(cap > Capacity)
        {
            GPodAllocator<T>::free_array(Array, Capacity);
            Capacity = cap + extraTail;
            Array = Capacity ? GPodAllocator<T>::allocate_array(Capacity) : 0;
        }
    }

    unsigned capacity() const 
    { 
        return Capacity; 
    }

    // Allocate n elements. All data is lost, 
    // but elements can be accessed in range 0...size-1. 
    void allocate(unsigned size, unsigned extraTail=0)
    {
        capacity(size, extraTail);
        Size = size;
    }

    void zero()
    {
        memset(Array, 0, sizeof(T) * Size);
    }

    void add(const T& v) 
    { 
        Array[Size++] = v; 
    }

    void push_back(const T& v)       // Compatibility with STL
    { 
        Array[Size++] = v; 
    }

    void insertAt(unsigned pos, const T& val)
    {
        if(pos >= Size) 
        {
            Array[Size] = val;
        }
        else
        {
            memmove(Array + pos + 1, Array + pos, (Size - pos) * sizeof(T));
            Array[pos] = val;
        }
        ++Size;
    }

    unsigned size() const 
    { 
        return Size; 
    }

    const GPodVector<T>& operator = (const GPodVector<T>& v)
    {
        if(&v != this)
        {
            capacity(v.Capacity);
            Size = v.Size;
            if(Size) memcpy(Array, v.Array, sizeof(ValueType) * Size);
        }
        return *this;
    }

    const T& operator [] (unsigned i) const { return Array[i]; }
          T& operator [] (unsigned i)       { return Array[i]; }
    const T& at(unsigned i) const           { return Array[i]; }
          T& at(unsigned i)                 { return Array[i]; }
    T  value_at(unsigned i) const           { return Array[i]; }

    const T* data() const { return Array; }
          T* data()       { return Array; }

private:
    unsigned Size;
    unsigned Capacity;
    T*       Array;
};


// A simple adaptor that provides the size() method and overloads 
// operator []. Used to QuickSort plain arrays.
//------------------------------------------------------------------------
template<class T> class GArrayAdaptor
{
public:
    typedef T ValueType;
    GArrayAdaptor() : Array(0), Size(0) {}
    GArrayAdaptor(T* ptr, unsigned size) : Array(ptr), Size(size) {}
    unsigned size() const { return Size; }
    const T& operator [] (unsigned i) const { return Array[i]; }
          T& operator [] (unsigned i)       { return Array[i]; }
private:
    T*       Array;
    unsigned Size;
};


//------------------------------------------------------------------------
template<class T> class GConstArrayAdaptor
{
public:
    typedef T ValueType;
    GConstArrayAdaptor() : Array(0), Size(0) {}
    GConstArrayAdaptor(const T* ptr, unsigned size) : Array(ptr), Size(size) {}
    unsigned size() const { return Size; }
    const T& operator [] (unsigned i) const { return Array[i]; }
private:
    const T* Array;
    unsigned Size;
};


//------------------------------------------------------------------------
template<class T, unsigned BlockSize=127> class GPodStructAllocator
{
public:
    typedef T ValueType;

private:
    struct BlockType
    {
        ValueType  Data[BlockSize];
        BlockType* pNext;
    };

public:
    ~GPodStructAllocator()
    {
        FreeAll();
    }

    GPodStructAllocator() :
        FirstBlock(0),
        LastBlock(0),
        NumElementsInBlock(BlockSize),
        FirstEmptySlot(0)
    {}

    void FreeAll()
    {
        BlockType* block = FirstBlock;
        while (block)
        {
            BlockType* next = block->pNext;
            GPodAllocator<BlockType>::free_array(block, 1);
            block = next;
        }
        FirstBlock         = 0;
        LastBlock          = 0;
        NumElementsInBlock = BlockSize;
        FirstEmptySlot     = 0;
    }

    ValueType* Allocate()
    {
        if (FirstEmptySlot)
        {
            ValueType* ret = FirstEmptySlot;
            FirstEmptySlot = FirstEmptySlot->pNext;
            return ret;
        }
        if (NumElementsInBlock < BlockSize)
        {   
            return &LastBlock->Data[NumElementsInBlock++];
        }

        BlockType* next = GPodAllocator<BlockType>::allocate_array(1);
        next->pNext = 0;
        if (LastBlock)
            LastBlock->pNext = next;
        else
            FirstBlock = next;
        LastBlock = next;
        NumElementsInBlock = 1;
        return &LastBlock->Data[0];
    }

    ValueType* Clone(const ValueType& v)
    {
        ValueType* ret = Allocate();
        *ret = v;
        return ret;
    }

    void Free(ValueType* element)
    {
        element->pNext = FirstEmptySlot;
        FirstEmptySlot = element;
    }

private:
    // Copying is prohibited
    GPodStructAllocator(const GPodStructAllocator<T, BlockSize>& v);
    const GPodStructAllocator<T, BlockSize>& operator = (const GPodStructAllocator<T, BlockSize>& v);

    BlockType* FirstBlock;
    BlockType* LastBlock;
    unsigned   NumElementsInBlock;
    ValueType* FirstEmptySlot;
};


//------------------------------------------------------------------------
template<class T> struct GPodDListNode
{
    T* pPrev;
    T* pNext;
};

//------------------------------------------------------------------------
template<class T> class GPodDList
{
public:
    typedef T ValueType;

    GPodDList()
    {
        Root.pNext = Root.pPrev = (ValueType*)&Root;
    }

    void RemoveAll()
    {
        Root.pNext = Root.pPrev = (ValueType*)&Root;
    }

    const ValueType* GetFirst() const { return Root.pNext; }
    const ValueType* GetLast () const { return Root.pPrev; }
          ValueType* GetFirst()       { return Root.pNext; }
          ValueType* GetLast ()       { return Root.pPrev; }

    bool IsFirst(const ValueType* p) const { return p == Root.pNext; }
    bool IsLast (const ValueType* p) const { return p == Root.pPrev; }
    bool IsNull (const ValueType* p) const { return p == (const ValueType*)&Root; }

    inline static const ValueType* GetPrev(const ValueType* p) { return p->pPrev; }
    inline static const ValueType* GetNext(const ValueType* p) { return p->pNext; }
    inline static       ValueType* GetPrev(      ValueType* p) { return p->pPrev; }
    inline static       ValueType* GetNext(      ValueType* p) { return p->pNext; }

    void PushFront(ValueType* p)
    {
        p->pNext          =  Root.pNext;
        p->pPrev          = (ValueType*)&Root;
        Root.pNext->pPrev =  p;
        Root.pNext        =  p;
    }

    void PushBack(ValueType* p)
    {
        p->pPrev          =  Root.pPrev;
        p->pNext          = (ValueType*)&Root;
        Root.pPrev->pNext =  p;
        Root.pPrev        =  p;
    }

    void Remove(ValueType* p)
    {
        p->pPrev->pNext = p->pNext;
        p->pNext->pPrev = p->pPrev;
    }

    void BringToFront(ValueType* p)
    {
        Remove(p);
        PushFront(p);
    }

    void SendToBack(ValueType* p)
    {
        Remove(p);
        PushBack(p);
    }

private:
    // Copying is prohibited
    GPodDList(const GPodDList<T>& v);
    const GPodDList<T>& operator = (const GPodDList<T>& v);

    GPodDListNode<ValueType> Root;
};


//------------------------------------------------------------------------
template<class T, class Accessor> class GPodDList2
{
public:
    typedef T ValueType;

    inline static void SetPrev(ValueType* self, ValueType* what)  { Accessor::SetPrev(self, what);  }
    inline static void SetNext(ValueType* self, ValueType* what)  { Accessor::SetNext(self, what);  }
    inline static const ValueType* GetPrev(const ValueType* self) { return Accessor::GetPrev(self); }
    inline static const ValueType* GetNext(const ValueType* self) { return Accessor::GetNext(self); }
    inline static       ValueType* GetPrev(ValueType* self)       { return Accessor::GetPrev(self); }
    inline static       ValueType* GetNext(ValueType* self)       { return Accessor::GetNext(self); }

    GPodDList2()
    {
        SetPrev(&Root, &Root);
        SetNext(&Root, &Root);
    }

    void RemoveAll()
    {
        SetPrev(&Root, &Root);
        SetNext(&Root, &Root);
    }

    const ValueType* GetFirst() const { return GetNext(&Root); }
    const ValueType* GetLast () const { return GetPrev(&Root); }
          ValueType* GetFirst()       { return GetNext(&Root); }
          ValueType* GetLast ()       { return GetPrev(&Root); }

    bool IsFirst(const ValueType* p) const { return p == GetNext(&Root); }
    bool IsLast (const ValueType* p) const { return p == GetPrev(&Root); }
    bool IsNull (const ValueType* p) const { return p == &Root; }

    void PushFront(ValueType* p)
    {
        SetNext(p, GetNext(&Root));
        SetPrev(p, &Root);
        SetPrev(GetNext(&Root), p);
        SetNext(&Root, p);
    }

    void PushBack(ValueType* p)
    {
        SetPrev(p, GetPrev(&Root));
        SetNext(p, &Root);
        SetNext(GetPrev(&Root), p);
        SetPrev(&Root, p);
    }

    void InsertBefore(ValueType* existing, ValueType* newOne)
    {
        ValueType* prev = GetPrev(existing);
        SetNext(newOne,   existing);
        SetPrev(newOne,   prev);
        SetNext(prev,     newOne);
        SetPrev(existing, newOne);
    }

    void InsertAfter(ValueType* existing, ValueType* newOne)
    {
        ValueType* next = GetNext(existing);
        SetPrev(newOne,   existing);
        SetNext(newOne,   next);
        SetPrev(next,     newOne);
        SetNext(existing, newOne);
    }

    void Remove(ValueType* p)
    {
        SetNext(GetPrev(p), GetNext(p));
        SetPrev(GetNext(p), GetPrev(p));
    }

    void BringToFront(ValueType* p)
    {
        Remove(p);
        PushFront(p);
    }

    void SendToBack(ValueType* p)
    {
        Remove(p);
        PushBack(p);
    }

private:
    // Copying is prohibited
    GPodDList2(const GPodDList2<T,Accessor>& v);
    const GPodDList2<T,Accessor>& operator = (const GPodDList2<T,Accessor>& v);

    ValueType Root;
};



//------------------------------------------------------------------------
template<class VectorType> class GVectorSliceAdaptor
{
public:
    typedef typename VectorType::ValueType ValueType;

    GVectorSliceAdaptor(VectorType& vector, unsigned start, unsigned size) :
        Vector(vector), Start(start), Size(size)
    {}

    unsigned size() const { return Size; }
    const ValueType& operator [] (unsigned i) const { return Vector[Start + i]; }
          ValueType& operator [] (unsigned i)       { return Vector[Start + i]; }
    const ValueType& at(unsigned i) const           { return Vector[Start + i]; }
          ValueType& at(unsigned i)                 { return Vector[Start + i]; }
    ValueType   valueAt(unsigned i) const           { return Vector[Start + i]; }

private:
    VectorType& Vector;
    unsigned    Start;
    unsigned    Size;
};



namespace GAlg
{

//------------------------------------------------------------------------
template<class CDst, class CSrc> 
void CopyContainer(CDst& dst, const CSrc& src)
{
    unsigned i;
    dst.removeAll();
    for(i = 0; i < src.size(); i++) 
        dst.add(src[i]);
}

//-------------------
// The code of SwapElements, QuickSort, RemoveDuplicates, 
// and BinarySearchPos was taken from the Anti-Grain Geometry
// Project and modified for the use by Scaleform. 
// Permission to use without restrictions is hereby granted to 
// Scaleform Corp. by the author of Anti-Grain Geometry Project.
//------------------------------------------------------------------------
template<class T> inline void SwapElements(T& a, T& b)
{
    T temp = a;
    a = b;
    b = temp;
}

//------------------------------------------------------------------------
template<class Array, class Less> 
void QuickSort(Array& arr, Less less, int limit=0)
{
    enum 
    {
        Threshold = 9
    };

    if(limit == 0) limit = (int)arr.size();
    if(limit <  2) return;

    int  stack[80];
    int* top = stack; 
    int  base = 0;


    for(;;)
    {
        int len = limit - base;

        int i;
        int j;
        int pivot;

        if(len > Threshold)
        {
            // we use base + len/2 as the pivot
            pivot = base + len / 2;
            SwapElements(arr[base], arr[pivot]);

            i = base + 1;
            j = limit - 1;

            // now ensure that *i <= *base <= *j 
            if(less(arr[j],    arr[i])) SwapElements(arr[j],    arr[i]);
            if(less(arr[base], arr[i])) SwapElements(arr[base], arr[i]);
            if(less(arr[j], arr[base])) SwapElements(arr[j], arr[base]);

            for(;;)
            {
                do i++; while( less(arr[i], arr[base]) );
                do j--; while( less(arr[base], arr[j]) );

                if( i > j )
                {
                    break;
                }

                SwapElements(arr[i], arr[j]);
            }

            SwapElements(arr[base], arr[j]);

            // now, push the largest sub-array
            if(j - base > limit - i)
            {
                top[0] = base;
                top[1] = j;
                base   = i;
            }
            else
            {
                top[0] = i;
                top[1] = limit;
                limit  = j;
            }
            top += 2;
        }
        else
        {
            // the sub-array is small, perform insertion sort
            j = base;
            i = j + 1;

            for(; i < limit; j = i, i++)
            {
                for(; less(arr[j + 1], arr[j]); j--)
                {
                    SwapElements(arr[j + 1], arr[j]);
                    if(j == base)
                    {
                        break;
                    }
                }
            }
            if(top > stack)
            {
                top  -= 2;
                base  = top[0];
                limit = top[1];
            }
            else
            {
                break;
            }
        }
    }
}

//------------------------------------------------------------------------
template<class Array, class Less> 
void InsertionSort(Array& arr, Less less)
{
    int j = 0;
    int i = j + 1;
    int size = arr.size();

    for(; i < size; j = i, i++)
    {
        for(; less(arr[j + 1], arr[j]); j--)
        {
            SwapElements(arr[j + 1], arr[j]);
            if(j <= 0)
            {
                break;
            }
        }
    }
}



// Remove duplicates from a sorted array. It doesn't cut the 
// tail of the array, it just returns the number of remaining elements.
//-----------------------------------------------------------------------
template<class Array, class Equal>
unsigned RemoveDuplicates(Array& arr, Equal equal)
{
    if(arr.size() < 2) return arr.size();

    unsigned i, j;
    for(i = 1, j = 1; i < arr.size(); i++)
    {
        typename Array::ValueType& e = arr[i];
        if(!equal(e, arr[i - 1]))
        {
            arr[j++] = e;
        }
    }
    return j;
}

//-----------------------------------------------------------------------
//template<class Array, class Value, class Less>
//unsigned BinarySearchPos(const Array& arr, const Value& val, Less less)
//{
//    if(arr.size() == 0) return 0;
//
//    unsigned beg = 0;
//    unsigned end = arr.size() - 1;
//
//    if(less(val, arr[0])) return 0;
//    if(less(arr[end], val)) return end + 1;
//
//    while(end - beg > 1)
//    {
//        unsigned mid = (end + beg) >> 1;
//        if(less(val, arr[mid])) end = mid; 
//        else                    beg = mid;
//    }
//    return end;
//}


//-----------------------------------------------------------------------
template<class Array, class Value, class Less>
unsigned LowerBound(const Array& arr, const Value& val, Less less)
{
    int len   = arr.size();
    int first = 0;
    int half;
    int middle;
    
    while(len > 0) 
    {
        half = len >> 1;
        middle = first + half;
        if(less(arr[middle], val)) 
        {
            first = middle + 1;
            len   = len - half - 1;
        }
        else
        {
            len = half;
        }
    }
    return first;
}


//-----------------------------------------------------------------------
template<class Array, class Value, class Less>
unsigned UpperBound(const Array& arr, const Value& val, Less less)
{
    int len   = arr.size();
    int first = 0;
    int half;
    int middle;
    
    while(len > 0) 
    {
        half = len >> 1;
        middle = first + half;
        if(less(val, arr[middle]))
        {
            len = half;
        }
        else 
        {
            first = middle + 1;
            len   = len - half - 1;
        }
    }
    return first;
}


//-----------------------------------------------------------------------
template<class Array> void ReverseElements(Array& arr, int from, int to)
{
    int u = arr.size() - 1;
    if(from < 0) from = 0;
    if(from > u) from = u;
    if(to   < 0) to   = 0;
    if(to   > u) to   = u;
    while(from < to)
    {
        SwapElements(arr[from], arr[to]);
        ++from;
        --to;
    }
}


//-----------------------------------------------------------------------
template<class Array> void ReverseContainer(Array& arr)
{
    int from = 0;
    int to   = arr.size() - 1;
    while(from < to)
    {
        SwapElements(arr[from], arr[to]);
        ++from;
        --to;
    }
}



} // GAlg

#endif
