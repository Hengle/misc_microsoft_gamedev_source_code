/**********************************************************************

Filename    :   GRange.h
Content     :   Range template types and functions
Created     :   April 17, 2007
Authors     :   Artyom Bolgar
Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GRANGE_H
#define INC_GRANGE_H

#include "GTypes.h"
#include "GDebug.h"
#include "GMemory.h"
#include "GMath.h"

class GRange
{
public:
    SPInt    Index;
    UPInt    Length;

    enum BoundsType { Min, Max };

    GRange():Index(0), Length(0) {}
    explicit GRange(UPInt count)        { SetRange(count); }
    GRange(SPInt index, UPInt length)   { SetRange(index, length); }
    GRange(const GRange &r)             { SetRange(r); }
    GRange(BoundsType bt)               { SetRange(bt); }

    // *** Initialization

    void    SetRange(UPInt count)   { Index = 0; Length = count; }
    // first and last inclusive
    void    SetRange(SPInt index, UPInt length)     
    { 
        Index = index;
        Length = length;
    }
    void    SetRange(const GRange &r)           { SetRange(r.Index, r.Length); }
    void    SetRange(BoundsType bt)
    {
        switch (bt)
        {
        case Min:   SetRange(0, 0); break;
        case Max:   SetRange(0, GFC_MAX_UPINT); break;
        }
    }

    // moves index and decrease the length, thus NextIndex remains same. 
    // Positive delta shows direct direction, negative - reverse.
    SPInt MoveIndex(SPInt delta) 
    {
        delta = (delta > SPInt(Length)) ? SPInt(Length) : delta;
        Index += delta; 
        Length = (UPInt)(Length - delta);
        return Index; 
    }
    // moves whole range keeping the original length
    SPInt MoveRange(SPInt delta) 
    { 
        Index += delta; 
        return Index; 
    }

    // Shrink range's length
    UPInt ShrinkRange(UPInt lengthDelta) 
    {
        if (lengthDelta > Length) 
            Length = 0;
        else
            Length -= lengthDelta;
        return Length;
    }
    UPInt ExpandRange(UPInt lengthDelta)
    {
        Length += lengthDelta;
        return Length;
    }

    // returns the next available index after the range
    SPInt NextIndex() const { return Index + Length; }
    // returns the last included index in the range
    SPInt LastIndex() const { return Index + Length - 1; }
    SPInt FirstIndex() const { return Index; }
    bool IsEmpty() const { return Length == 0; }

    // Reset the range to first = 0 and count = 0       
    void    Clear()                             { SetRange(Min); }

    // *** Intersection

    // Tests whether a value is inside the range
    bool    Contains(SPInt index) const         { return (index >= Index && index <= LastIndex()); }    
    // Tests whether the range is completely inside this one
    bool    Contains(const GRange &r) const     { return (r.Index >= Index && r.LastIndex() <= LastIndex()); }

    // Tests whether the range intersects this one
    bool    Intersects(const GRange &r) const   { return (r.LastIndex() >= FirstIndex()) && (LastIndex() >= r.FirstIndex()); }

    int     CompareTo(SPInt index) const
    {
        if (Contains(index)) 
            return 0;
        if (index < Index) 
            return int(Index - index);
        else
            return int(NextIndex() - index);
    }
};

template <class T>
class GRangePrimData : public GRange
{
    T       Data;
public:
    GRangePrimData():GRange() {}
    // reference dowsn't work for primitive types' constants
    //??GRangeData(SPInt index, UPInt length, const T& data):GRange(index, length),Data(data) {}
    GRangePrimData(SPInt index, UPInt length, const T data):GRange(index, length),Data(data) {}

    T GetData() const               { return Data; }
    void SetData(T data)            { Data = data; }
    bool IsDataEqual(T data) const  { return Data == data; }
};

template <class T>
class GRangeData : public GRange
{
    T       Data;
public:
    GRangeData():GRange() {}
    GRangeData(SPInt index, UPInt length, const T& data):GRange(index, length),Data(data) {}

    T& GetData()                           { return Data; }
    const T& GetData() const               { return Data; }
    void SetData(const T& data)            { Data = data; }
    bool IsDataEqual(const T& data) const  { return Data == data; }
};

template <class T, class Array = GTL::garray<GRangeData<T> > >
class GRangeDataArray
{
    UPInt FindRangeIndex(SPInt index) const;
    UPInt FindNearestRangeIndex(SPInt index) const;
public:
    typedef GRangeData<T>                           GTypedRangeData;
    //typedef GTL::gindexed_array<GTypedRangeData>    GRangeArrayType;
    typedef Array                                   GRangeArrayType;
    //typedef typename GRangeArrayType::HeapType      HeapType;

    GRangeArrayType Ranges;

    // just returns range at the specified position in Ranges array
    GTypedRangeData& operator[](UPInt position)             { return Ranges[position]; }
    const GTypedRangeData& operator[](UPInt position) const { return Ranges[position]; }
    UPInt Count() const { return Ranges.size(); }

    void SetRange(SPInt index, UPInt length, const T& data)
    {
        SetRange(GTypedRangeData(index, length, data));
    }
    void SetRange(const GTypedRangeData& range);
    // just clears the range w/o collapsing the array
    void ClearRange(SPInt index, UPInt length);

    GTypedRangeData& Append(UPInt length, const T& data);
    void InsertRange(SPInt startPos, UPInt length, const T& pdata);
    void ExpandRange(SPInt startPos, UPInt length);
    // removes range and collapse the range array, thus no new hole is produced
    void RemoveRange(SPInt startPos, UPInt length);
    void RemoveAll() { Ranges.resize(0); }

    class Iterator
    {
        friend class GRangeDataArray<T>;
        GRangeDataArray<T>* pArray;
        SPInt               Index;

        explicit Iterator(GRangeDataArray<T>& arr) : pArray(&arr), Index(0) {}
        Iterator(GRangeDataArray<T>& arr, bool) : pArray(&arr), Index(pArray->Count() - 1) {}

        // inserts a range at the current position
        void InsertBefore(const GTypedRangeData& range);
        void InsertAfter(const GTypedRangeData& range);
        // remove one range at the current position
        void Remove();

    public:
        Iterator(const Iterator& it) : pArray(it.pArray), Index(it.Index) {}
        Iterator():pArray(0), Index(-1) {}

        Iterator& operator++() 
        { 
            if (Index < (SPInt)pArray->Count())
                ++Index; 
            return *this; 
        }    
        Iterator operator++(int) 
        { 
            Iterator it(*this); 
            if (Index < (SPInt)pArray->Count())
                ++Index; 
            return it; 
        }   
        Iterator& operator--() 
        { 
            if (Index >= 0)
                --Index; 
            return *this; 
        }    
        Iterator operator--(int) 
        { 
            Iterator it(*this); 
            if (Index >= 0)
                --Index; 
            return it; 
        }   
        GTypedRangeData& operator*()  { return (*pArray)[Index]; }
        GTypedRangeData* operator->() { return &(*pArray)[Index]; }
        GTypedRangeData* GetPtr()     { return (Index < (SPInt)pArray->Count()) ? &(*pArray)[Index] : NULL; }
        bool operator==(const Iterator& it) const
        {
            return (pArray == it.pArray && Index == it.Index);
        }
        Iterator operator+(SPInt delta) const
        {
            Iterator it(*this);
            if (Index + delta < 0)
                it.Index = 0;
            else if (UPInt(Index + delta) >= pArray->Count())
                it.Index = pArray->Count() - 1;
            else
                it.Index += delta;
            
            return it;
        }
        bool IsFinished() const { return Index < 0 || UPInt(Index) >= pArray->Count(); }
    };
    Iterator Begin() { return Iterator(*this); }
    Iterator Last()  { return Iterator(*this, true); }
    Iterator GetIteratorAt(SPInt index);
    Iterator GetIteratorByNearestIndex(SPInt index);

    class ConstIterator
    {
        friend class GRangeDataArray<T>;
        const GRangeDataArray<T>*   pArray;
        SPInt                       Index;

        explicit ConstIterator(const GRangeDataArray<T>& arr) : pArray(&arr), Index(0) {}
        ConstIterator(const GRangeDataArray<T>& arr, bool) : pArray(&arr), Index(pArray->Count() - 1) {}

    public:
        ConstIterator(const ConstIterator& it) : pArray(it.pArray), Index(it.Index) {}
        ConstIterator():pArray(0), Index(-1) {}

        ConstIterator& operator++() 
        { 
            if (Index < (SPInt)pArray->Count())
                ++Index; 
            return *this; 
        }    
        ConstIterator operator++(int) 
        { 
            ConstIterator it(*this); 
            if (Index < (SPInt)pArray->Count())
                ++Index; 
            return it; 
        }   
        ConstIterator& operator--() 
        { 
            if (Index >= 0)
                --Index; 
            return *this; 
        }    
        ConstIterator operator--(int) 
        { 
            ConstIterator it(*this); 
            if (Index >= 0)
                --Index; 
            return it; 
        }   
        const GTypedRangeData& operator*() const  { return (*pArray)[Index]; }
        const GTypedRangeData* operator->() const { return &(*pArray)[Index]; }
        const GTypedRangeData* GetPtr() const     { return (Index < (SPInt)pArray->Count()) ? &(*pArray)[Index] : NULL; }
        bool operator==(const ConstIterator& it) const
        {
            return (pArray == it.pArray && Index == it.Index);
        }
        ConstIterator operator+(SPInt delta) const
        {
            ConstIterator it(*this);
            if (Index + delta < 0)
                it.Index = 0;
            else if (UPInt(Index + delta) >= pArray->Count())
                it.Index = pArray->Count() - 1;
            else
                it.Index += delta;
            
            return it;
        }
        bool IsFinished() const { return Index < 0 || UPInt(Index) >= pArray->Count(); }
    };
    ConstIterator Begin() const { return ConstIterator(*this); }
    ConstIterator Last() const  { return ConstIterator(*this, true); }
    ConstIterator GetIteratorAt(SPInt index) const;
    ConstIterator GetIteratorByNearestIndex(SPInt index) const;

    class ConstPositionIterator
    {
        friend class GRangeDataArray<T>;
        ConstIterator    Iter;
        const GTypedRangeData* pCurrentRange; 
        SPInt             CurrentPos;
        SPInt             EndPos;
    public:
        ConstPositionIterator(const GRangeDataArray<T>& rangeArray) : Iter(rangeArray.Begin()) 
        {
            if (!Iter.IsFinished())
            {
                CurrentPos = Iter->FirstIndex();
                pCurrentRange = Iter.GetPtr();
                EndPos = rangeArray[rangeArray.Count() - 1].LastIndex();
            }
            else
            {
                CurrentPos = 0;
                pCurrentRange = NULL;
                EndPos = -1;
            }
        }
        ConstPositionIterator(const GRangeDataArray<T>& rangeArray, SPInt startIndex, UPInt length) :
            Iter(rangeArray.GetIteratorByNearestIndex(startIndex)), CurrentPos(startIndex),
            EndPos(CurrentPos + length - 1)
        {
            if (!Iter.IsFinished())
            {
                pCurrentRange = Iter.GetPtr();
            }
            else
            {
                pCurrentRange = NULL;
            }
        }

        bool IsFinished() const { return CurrentPos > EndPos; }

        const GTypedRangeData* operator*() const  { return GetPtr(); }
        const GTypedRangeData* operator->() const 
        { 
            return GetPtr(); 
        }
        const GTypedRangeData* GetPtr() const
        { 
            if (pCurrentRange)
            {
                if (pCurrentRange->Contains(CurrentPos))
                    return pCurrentRange;
            }
            return NULL; 
        }

        ConstPositionIterator& operator++() 
        { 
            if (CurrentPos <= EndPos)
            {
                SPInt oldPos = CurrentPos++;
                if (pCurrentRange)
                {
                    if (pCurrentRange->Contains(oldPos) && !pCurrentRange->Contains(CurrentPos))
                    {
                        ++Iter;
                        if (!Iter.IsFinished())
                            pCurrentRange = Iter.GetPtr();
                        else
                            pCurrentRange = NULL;
                    }
                }
            }
            return *this; 
        }    
        ConstPositionIterator operator++(int) 
        { 
            ConstIterator it(*this); 
            operator++();
            return it; 
        }   
    };

    /*void    set_heap(HeapType& heap)
    {
        Ranges.set_heap(heap);
    }
    void    set_heap(HeapType* pheap)
    {
        Ranges.set_heap(pheap);
    } */

#ifdef GFC_BUILD_DEBUG
    void CheckIntegrity();
#else
    void CheckIntegrity() {}
#endif
protected:

};

// inserts a range at the current position
template <class T, class Array>
void GRangeDataArray<T, Array>::Iterator::InsertBefore(const GTypedRangeData& range)
{
    pArray->Ranges.insert(Index, range);
}

template <class T, class Array>
void GRangeDataArray<T, Array>::Iterator::InsertAfter(const GTypedRangeData& range)
{
    pArray->Ranges.insert(Index+1, range);
}

// remove one range at the current position
template <class T, class Array>
void GRangeDataArray<T, Array>::Iterator::Remove()
{
    if (Index >= 0 && UPInt(Index) < pArray->Count())
        pArray->Ranges.remove(Index);
}

template <class T, class Array>
UPInt GRangeDataArray<T, Array>::FindRangeIndex(SPInt index) const
{
    // do a binary search
    const UPInt count = Count();

    UPInt lower = 0;
    UPInt upper = count - 1;

    while (lower < upper && upper != (UPInt)-1) {
        UPInt middle = (lower + upper)/2;
        int cmpRes = Ranges[middle].CompareTo(index);
        if (cmpRes == 0) { // equal
            return middle;
        }
        if (cmpRes < 0) // *mpArray[middle] < *elem
            lower = middle+1;
        else
            upper = middle-1;
    }

    if (lower == upper && Ranges[lower].CompareTo(index) == 0)
        return lower;
    return GFC_MAX_UPINT;
}


template <class T, class Array>
typename GRangeDataArray<T, Array>::Iterator GRangeDataArray<T, Array>::GetIteratorAt(SPInt index)
{
    UPInt rangeIndex = FindRangeIndex(index);
    if (rangeIndex != GFC_MAX_UPINT)
        return Begin() + rangeIndex;
    return Iterator();
}

template <class T, class Array>
typename GRangeDataArray<T, Array>::ConstIterator GRangeDataArray<T, Array>::GetIteratorAt(SPInt index) const
{
    UPInt rangeIndex = FindRangeIndex(index);
    if (rangeIndex != GFC_MAX_UPINT)
        return Begin() + rangeIndex;
    return ConstIterator();
}

template <class T, class Array>
UPInt GRangeDataArray<T, Array>::FindNearestRangeIndex(SPInt index) const
{
    // do a binary search
    const UPInt count = Count();
    if (count == 0)
    {
        return 0;
    }

    UPInt lower = 0;
    UPInt upper = count - 1;
    UPInt lowest = 0;

    while (lower < upper && upper != (UPInt)-1) {
        UPInt middle = (lower + upper)/2;
        int cmpRes = Ranges[middle].CompareTo(index);
        if (cmpRes == 0) { // equal
            return middle;
        }
        if (cmpRes < 0) // *mpArray[middle] < *elem
        {
            lowest = lower;
            lower = middle+1;
        }
        else
            upper = middle-1;
    }

    if (lower == upper && Ranges[lower].CompareTo(index) == 0)
        return lower;
    while(lowest < upper && Ranges[lowest + 1].CompareTo(index) < 0)
        ++lowest;
    return lowest;
}

template <class T, class Array>
typename GRangeDataArray<T, Array>::Iterator GRangeDataArray<T, Array>::GetIteratorByNearestIndex(SPInt index)
{
    return Begin() + FindNearestRangeIndex(index);
}

template <class T, class Array>
typename GRangeDataArray<T, Array>::ConstIterator GRangeDataArray<T, Array>::GetIteratorByNearestIndex(SPInt index) const
{
    return Begin() + FindNearestRangeIndex(index);
}

template <class T, class Array>
void GRangeDataArray<T, Array>::SetRange(const GTypedRangeData& range)
{
    if (Count() > 0)
    {
        Iterator it = GetIteratorByNearestIndex(range.Index);
        Iterator insertionPoint;

        // split the first range
        if (it->Contains(range))
        {
            // inserting range is inside the existing range?
            if (it->Index == range.Index)
            {
                // beginnings of ranges are same?
                // |=======||====| - array
                // |+++|           - range
                // |+++|===||====| - result
                it->MoveIndex(range.Length);
                
                if (it->IsEmpty())
                {
                    *it = range;
                }
                else
                    it.InsertBefore(range);
                insertionPoint = it;
                ++it;
            }
            else if (it->NextIndex() > range.NextIndex())
            {
                // |==========||====| - array
                //    |+++|           - range
                // |==|+++|===||====| - result
                // inserting range is completely inside the current range
                // split the current range by 3 parts and replace the middle one by "range"
                GTypedRangeData r = *it;
                SPInt endIndex = it->NextIndex();
                it->ShrinkRange(endIndex - range.Index);
                r.MoveIndex(it->Length + range.Length);
                it.InsertAfter(range);
                ++it;
                insertionPoint = it;
                it.InsertAfter(r);
                ++it;
            }
            else
            {
                // |==========||====| - array
                //        |+++|       - range
                // |======|+++||====| - result
                // inserting range intersects the current range
                it->ShrinkRange(range.Length);
                insertionPoint = ++it;
                it.InsertBefore(range);
                ++it;
            }
        }
        else if (it->Contains(range.Index))
        {
            // inserting range intersects the current range
            // |==========||====| - array
            //        |+++++++|   - range
            // |======|   ||====| - result at this stage
            it->ShrinkRange(it->NextIndex() - range.Index);
            insertionPoint = ++it;
            it.InsertBefore(range);
            ++it;
        }
        else
        {
            // beginning of inserting range is on empty space
            // |==========|          |====| - array
            //                   |+++++++|   - range
            // or
            //                       |====| - array
            //                   |+++++++|   - range
            // or
            //                       |====| - array
            //            |+++++++|         - range
            if (it->CompareTo(range.Index) > 0)
            {
                it.InsertBefore(range);
                insertionPoint = it;
            }
            else
            {
                it.InsertAfter(range);
                insertionPoint = ++it;
            }
            ++it;
        }
        // remove all ranges in between
        while(!it.IsFinished() && range.Contains(*it))
        {
            it.Remove();
        }
        // shrink the last range
        if (!it.IsFinished() && it->Contains(range.NextIndex() - 1))
        {
            // |==========||======| - array
            //        |+++++++|     - range
            // |==========|   |===| - result
            it->MoveIndex(range.NextIndex() - it->Index);
        }
        // check if is it possible to unite ranges
        Iterator firstRangeIt = insertionPoint;
        --firstRangeIt;
        // check the previous range
        if (!firstRangeIt.IsFinished())
        {
            if (firstRangeIt->IsEmpty())
                firstRangeIt.Remove();
            else  if (firstRangeIt->NextIndex() == range.Index && firstRangeIt->IsDataEqual(insertionPoint->GetData()))
            {
                // expand first range by the range.Length
                firstRangeIt->ExpandRange(range.Length);
                insertionPoint.Remove();
                insertionPoint = firstRangeIt;
            }
        }
        // check the next range
        Iterator nextIt = insertionPoint;
        ++nextIt;
        if (!nextIt.IsFinished())
        {
            if (nextIt->IsEmpty())
                nextIt.Remove();
            else if (insertionPoint->NextIndex() == nextIt->Index && insertionPoint->IsDataEqual(nextIt->GetData()))
            {
                // expand range by the length of nextIt
                insertionPoint->ExpandRange(nextIt->Length);
                nextIt.Remove();
            }
        }
    }
    else
    {
        Begin().InsertBefore(range);
    }
    CheckIntegrity();
}

template <class T, class Array>
typename GRangeDataArray<T, Array>::GTypedRangeData& GRangeDataArray<T, Array>::Append(UPInt length, const T& data)
{
    // check data at the last element in the array: if same as our, then just expand existing range by length
    UPInt count = Count();
    GTypedRangeData* pLastRange = NULL;
    if (count > 0)
    {
        pLastRange = &Ranges[count - 1];
        if (count > 0 && pLastRange->IsDataEqual(data))
        {
            pLastRange->Length += length;
            return *pLastRange;
        }
    }
    // otherwise, create a new range
    Ranges.resize(count + 1);
    GTypedRangeData& newRange = Ranges[count];
    if (pLastRange)
    {
        newRange.Index = pLastRange->Index + pLastRange->Length;
    }
    else
    {
        // insert new range as first one
        newRange.Index = 0;
    }
    newRange.Length = length;
    newRange.SetData(data);
    CheckIntegrity();
    return newRange;
}

template <class T, class Array>
void GRangeDataArray<T, Array>::InsertRange(SPInt startPos, UPInt length, const T& data)
{
    ExpandRange(startPos, length);
    SetRange(startPos, length, data);
    CheckIntegrity();
}

template <class T, class Array>
void GRangeDataArray<T, Array>::ExpandRange(SPInt startPos, UPInt length)
{
    if (Count() > 0)
    {
        Iterator it = GetIteratorByNearestIndex(startPos);
        GTypedRangeData* pnearest = it.GetPtr();
        if (pnearest)
        {
            if (pnearest->Contains(startPos) || pnearest->NextIndex() == startPos)
                pnearest->Length += length;
        }
        // update indices for all following ranges
        for(++it; !it.IsFinished(); ++it)
        {
            it->MoveRange((SPInt)length);
        }
    }
    CheckIntegrity();
}

template <class T, class Array>
void GRangeDataArray<T, Array>::RemoveRange(SPInt startPos, UPInt length)
{
    CheckIntegrity();
    if (Count() > 0)
    {
        Iterator it = GetIteratorByNearestIndex(startPos);
        Iterator removalPoint;

        if (length == GFC_MAX_UPINT) // special case, remove everything till the end
            length = GFC_MAX_SPINT - startPos;

        GTypedRangeData range(startPos, length, T());
        GTypedRangeData& r = *it;
        // split the first range
        if (r.Contains(range))
        {
            // is inserting range inside the existing range?
            if (r.Index == range.Index)
            {
                // beginnings of ranges are the same?
                // |0123456||====| - array
                // |+++|           - range
                //    |3456||====| - result at this stage
                r.MoveIndex(range.Length);

                if (r.IsEmpty())
                    it.Remove();
                removalPoint = it;
            }
            else if (r.NextIndex() > range.NextIndex())
            {
                // |1234567890||====| - array
                //    |+++|           - range
                // |1237890|   |====| - result  at this stage
                // inserting range is completely inside the current range
                // split the current range by 3 parts and replace the middle one by "range"
                r.ShrinkRange(range.Length);
                if (r.IsEmpty())
                    it.Remove();
                else
                    ++it;
                removalPoint = it;
            }
            else
            {
                // |==========||====| - array
                //        |+++|       - range
                // |======|    |====| - result at this stage
                // inserting range intersects the current range
                r.ShrinkRange(range.Length);
                removalPoint = ++it;
                ++it;
            }
        }
        else if (r.Contains(range.Index))
        {
            // inserting range intersects the current range
            // |==========||====| - array
            //        |+++++++|   - range
            // |======|   ||====| - result at this stage
            r.ShrinkRange(r.NextIndex() - range.Index);
            if (r.IsEmpty())
                it.Remove();
            else
                ++it;
            removalPoint = it;
        }
        else
        {
            // beginning of inserting range is on empty space
            // |==========|          |====| - array
            //                   |+++++++|   - range
            // or
            //                       |====| - array
            //                   |+++++++|   - range
            // or
            //                       |====| - array
            //            |+++++++|         - range
            if (r.CompareTo(range.Index) > 0)
            {
                removalPoint = it;
            }
            else
            {
                removalPoint = ++it;
            }
        }
        // remove all ranges in between
        while(!it.IsFinished() && range.Contains(*it))
        {
            it.Remove();
        }
        // shrink the last range
        if (!it.IsFinished() && it->Contains(range.NextIndex() - 1))
        {
            // |==========||======| - array
            //        |+++++++|     - range
            // |==========|   |===| - result
            it->MoveIndex(range.NextIndex() - it->Index);
        }

        Iterator firstRangeIt = removalPoint;
        --firstRangeIt;
        // check, can we unite ranges
        if (!firstRangeIt.IsFinished() && !removalPoint.IsFinished() && 
            firstRangeIt->NextIndex() == removalPoint->Index - SPInt(length) && 
            firstRangeIt->IsDataEqual(removalPoint->GetData()))
        {
            // we can
            firstRangeIt->ExpandRange(removalPoint->Length);
            removalPoint.Remove();
        }

        // update indices for all following ranges
        for(; !removalPoint.IsFinished(); ++removalPoint)
        {
            removalPoint->MoveRange(-((SPInt)length));
        }
    }
    CheckIntegrity();
}

// TODO!
template <class T, class Array>
void GRangeDataArray<T, Array>::ClearRange(SPInt startPos, UPInt length)
{
    CheckIntegrity();
    if (Count() > 0)
    {
        Iterator it = GetIteratorByNearestIndex(startPos);
        Iterator removalPoint;

        if (length == GFC_MAX_UPINT) // special case, remove everything till the end
            length = GFC_MAX_SPINT - startPos;

        GTypedRangeData range(startPos, length, T());
        // split the first range
        if (it->Contains(range))
        {
            // is inserting range inside the existing range?
            if (it->Index == range.Index)
            {
                // beginnings of ranges are the same?
                // |=======||====| - array
                // |+++|           - range
                //     |===||====| - result
                it->MoveIndex(range.Length);

                removalPoint = it;
                if (it->IsEmpty())
                    it.Remove();
                else
                    ++it;
            }
            else if (it->NextIndex() > range.NextIndex())
            {
                // |==========||====| - array
                //    |+++|           - range
                // |==|   |===||====| - result
                // inserting range is completely inside the current range
                // split the current range by 3 parts and replace the middle one by "range"
                GTypedRangeData r = *it;
                SPInt endIndex = it->NextIndex();
                it->ShrinkRange(endIndex - range.Index);
                r.MoveIndex(it->Length + range.Length);
                ++it;
                removalPoint = it;
                it.InsertBefore(r);
                ++it;
            }
            else
            {
                // |==========||====| - array
                //        |+++|       - range
                // |======|    |====| - result
                // inserting range intersects the current range
                it->ShrinkRange(range.Length);
                removalPoint = ++it;
                ++it;
            }
        }
        else if (it->Contains(range.Index))
        {
            // inserting range intersects the current range
            // |==========||====| - array
            //        |+++++++|   - range
            // |======|   ||====| - result at this stage
            it->ShrinkRange(it->NextIndex() - range.Index);
            removalPoint = ++it;
            ++it;
        }
        else
        {
            // beginning of inserting range is on empty space
            // |==========|          |====| - array
            //                   |+++++++|   - range
            // or
            //                       |====| - array
            //                   |+++++++|   - range
            // or
            //                       |====| - array
            //            |+++++++|         - range
            if (it->CompareTo(range.Index) > 0)
            {
                removalPoint = it;
            }
            else
            {
                removalPoint = ++it;
            }
            ++it;
        }
        // remove all ranges in between
        while(!it.IsFinished() && range.Contains(*it))
        {
            it.Remove();
        }
        // shrink the last range
        if (!it.IsFinished() && it->Contains(range.NextIndex() - 1))
        {
            // |==========||======| - array
            //        |+++++++|     - range
            // |==========|   |===| - result
            it->MoveIndex(range.NextIndex() - it->Index);
        }
    }
    CheckIntegrity();
}

#ifdef GFC_BUILD_DEBUG
template <class T, class Array>
void GRangeDataArray<T, Array>::CheckIntegrity()
{
    SPInt curIndex = GFC_MIN_SPINT;
    GTypedRangeData* pprev = NULL;
    for (UPInt i = 0, n = Ranges.size(); i < n; ++i)
    {
        GTypedRangeData& r = Ranges[i];
        if (pprev)
            GASSERT(pprev->GetData() != r.GetData()); // detect not united ranges
        GASSERT(r.Length); // detect empty ranges
        GASSERT(curIndex <= r.FirstIndex());
        curIndex = r.NextIndex();
        pprev = &r;
    }
}
#endif

#endif // INC_GRANGE_H
