//============================================================================
//
//  Set.h
//  
//  Copyright 2002, Ensemble Studios
//
//============================================================================



#ifndef __SET_H__
#define __SET_H__


//----------------------------------------------------------------------------
//  Public Constants
//----------------------------------------------------------------------------
static const unsigned long BSET_GROW_EXPONENTIAL = 0xFFFFFFFF;
static const unsigned long BSET_ITEM_NOT_FOUND   = 0xFFFFFFFF;


//----------------------------------------------------------------------------
//  Class BSet
//----------------------------------------------------------------------------
template <class T> class BSet
{
public:
   //-- Construction/Destruction
   BSet(unsigned long initialSize = 0, unsigned long growSize = BSET_GROW_EXPONENTIAL);
   ~BSet();

   //-- Set Operations
   void    empty       ();
   unsigned long   add         (const T& item);
   void    add         (const BSet<T>& set);
   void    remove      (unsigned long index, bool preserveOrder = true);
   void    remove      (const T& item, bool preserveOrder = true);
   void    remove      (const BSet<T>& set, bool preserveOrder = true);
   unsigned long   find        (const T& item) const;
   bool    contains    (const T& item) const;
   bool    contains    (const BSet<T>& set) const;
   bool    isSubsetOf  (const BSet<T>& set) const;
   bool    isSupersetOf(const BSet<T>& set) const;
   bool    isEmpty     () const;
   unsigned long   getNumItems () const;
   const T& getItem     (unsigned long index) const;

   //-- Double Set Operations
   unsigned long   setToUnion              (const BSet<T>& set1, const BSet<T>& set2);
   unsigned long   setToIntersection       (const BSet<T>& set1, const BSet<T>& set2);
   unsigned long   setToDifference         (const BSet<T>& set1, const BSet<T>& set2);
   unsigned long   setToSymmetricDifference(const BSet<T>& set1, const BSet<T>& set2);

   //-- Set Control
   void    reset            (unsigned long initialSize = 0, unsigned long growSize = BSET_GROW_EXPONENTIAL);
   void    setGrowSize      (unsigned long growSize);
   void    setAllocatedSize (unsigned long allocatedSize);
   unsigned long   getGrowSize      () const;
   unsigned long   getAllocatedSize () const;
   bool    isGrowExponential() const;

   //-- Operators
   BSet<T>& operator =  (const BSet<T>& set);
   bool    operator == (const BSet<T>& set) const;

private:
   //-- Private Functions
   void  grow();

   //-- Private Data
   unsigned long mSize;
   unsigned long mGrowSize;
   unsigned long mAllocatedSize;
   T*     mpArray;
};


//----------------------------------------------------------------------------
//  Template Implementation
//----------------------------------------------------------------------------
#include "Set.inl"


#endif




