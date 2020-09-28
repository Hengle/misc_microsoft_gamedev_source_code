//============================================================================
//
//  Stack.h
//  
//  Copyright 2002, Ensemble Studios
//
//============================================================================




#ifndef __STACK_H__
#define __STACK_H__


//----------------------------------------------------------------------------
//  Public Constants
//----------------------------------------------------------------------------
static const unsigned long BSTACK_GROW_EXPONENTIAL = 0xFFFFFFFF;


//----------------------------------------------------------------------------
//  Class BStack
//----------------------------------------------------------------------------
template <class T> class BStack
{
public:
   //-- Construction/Destruction
   BStack(unsigned long initialSize = 0, unsigned long growSize = BSTACK_GROW_EXPONENTIAL);
   ~BStack();

   //-- Stack Operations
   void    empty      ();
   void    push       (const T& item);
   void    pop        ();
   void    pop        (T& item);
   bool    isEmpty    () const;
   unsigned long   getNumItems() const;
   const T& getItem    (unsigned long index) const;

   //-- Stack Control
   void    reset            (unsigned long initialSize = 0, unsigned long growSize = BSTACK_GROW_EXPONENTIAL);
   void    setGrowSize      (unsigned long growSize);
   void    setAllocatedSize (unsigned long allocatedSize);
   unsigned long   getGrowSize      () const;
   unsigned long   getAllocatedSize () const;
   bool    isGrowExponential() const;

   //-- Operators
   BStack<T>& operator =  (const BStack<T>& stack);
   bool      operator == (const BStack<T>& stack) const;

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
#include "Stack.inl"


#endif




