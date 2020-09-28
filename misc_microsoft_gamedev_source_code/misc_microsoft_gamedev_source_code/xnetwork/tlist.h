//==============================================================================
// tlist.h
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

#ifndef _BTLIST_H_
#define _BTLIST_H_

//#include "tl.h"
//#include <list>

struct _BTListNode
{
   _BTListNode * mNext;
   _BTListNode * mPrevious;
};

template <class BTListValue>
struct _BTListValueNode : public _BTListNode
{
   BTListValue mValue;
};

template <class BTListValue, class BTListNode, class BTListValueNode, class BTList>
class _BTListIterator
{
   friend BTList;

   typedef _BTListIterator <const BTListValue, const BTListNode, const BTListValueNode, BTList> const_iterator;

public:
   _BTListIterator (void);

   BTListValue & operator * (void) const;
   BTListValue & operator->() const
			{return (&**this); }   
 
   _BTListIterator operator ++ (int);
   _BTListIterator operator -- (int);

   _BTListIterator & operator ++ (void);
   _BTListIterator & operator -- (void);
   
   bool operator == (const _BTListIterator &) const;
   bool operator != (const _BTListIterator &) const;

   operator const_iterator (void) const;

private:
   _BTListIterator (BTListNode *);

private:
   BTListNode * mNode;
};

template <class BTListValue>
class BTList
{
public:
    typedef _BTListIterator <BTListValue, _BTListNode, _BTListValueNode <BTListValue>, BTList> iterator;
    typedef typename iterator :: const_iterator const_iterator;
	    
public:
    BTList (void);
    ~BTList (void);

    iterator begin (void);
    iterator end (void);
    const_iterator begin (void) const;
    const_iterator end (void) const;
    
    BTListValue & front (void);
    BTListValue & back (void);
    const BTListValue & front (void) const;
    const BTListValue & back (void) const;

    void push_front (const BTListValue &);
    void push_back (const BTListValue &);

    void pop_front (void);
    void pop_back (void);

    bool empty (void) const;
    size_t size (void) const;
    
    void insert (iterator, const BTListValue & = BTListValue ());

    iterator erase (iterator);
    void clear (void);

private:
    _BTListNode mHead;  
};

template <class BTListValue, class BTListNode, class BTListValueNode, class BTList>
_BTListIterator <BTListValue, BTListNode, BTListValueNode, BTList> :: _BTListIterator (void) 
 : mNode (0)
{}

template <class BTListValue, class BTListNode, class BTListValueNode, class BTList>
_BTListIterator <BTListValue, BTListNode, BTListValueNode, BTList> 
:: operator _BTListIterator <const BTListValue, const BTListNode, const BTListValueNode, BTList> (void) const
{ return static_cast <const_iterator> (*this); }

template <class BTListValue, class BTListNode, class BTListValueNode, class BTList>
BTListValue & 
_BTListIterator <BTListValue, BTListNode, BTListValueNode, BTList> :: operator * (void) const 
{ return static_cast <BTListValueNode *> (mNode)->mValue; }

template <class BTListValue, class BTListNode, class BTListValueNode, class BTList>
_BTListIterator <BTListValue, BTListNode, BTListValueNode, BTList>
_BTListIterator <BTListValue, BTListNode, BTListValueNode, BTList> :: operator ++ (int)
{
      _BTListIterator tmp = *this;
      ++*this;
      return tmp;
}

template <class BTListValue, class BTListNode, class BTListValueNode, class BTList>
_BTListIterator <BTListValue, BTListNode, BTListValueNode, BTList>
_BTListIterator <BTListValue, BTListNode, BTListValueNode, BTList> :: operator -- (int)
{
      _BTListIterator tmp = *this;
      --*this;
      return tmp;
}

template <class BTListValue, class BTListNode, class BTListValueNode, class BTList>
_BTListIterator <BTListValue, BTListNode, BTListValueNode, BTList> &
_BTListIterator <BTListValue, BTListNode, BTListValueNode, BTList> :: operator ++ (void) 
{ mNode = mNode->mNext; return *this; } 

template <class BTListValue, class BTListNode, class BTListValueNode, class BTList>
_BTListIterator <BTListValue, BTListNode, BTListValueNode, BTList> &
_BTListIterator <BTListValue, BTListNode, BTListValueNode, BTList> :: operator -- (void) 
{ mNode = mNode->mPrevious; return *this; }

template <class BTListValue, class BTListNode, class BTListValueNode, class BTList>
bool
_BTListIterator <BTListValue, BTListNode, BTListValueNode, BTList> :: operator == (const _BTListIterator & i) const 
{ return mNode == i.mNode; }

template <class BTListValue, class BTListNode, class BTListValueNode, class BTList>
bool
_BTListIterator <BTListValue, BTListNode, BTListValueNode, BTList> :: operator != (const _BTListIterator & i) const 
{ return mNode != i.mNode; }

template <class BTListValue, class BTListNode, class BTListValueNode, class BTList>
_BTListIterator <BTListValue, BTListNode, BTListValueNode, BTList> :: _BTListIterator (BTListNode * node) 
 : mNode (node)
{}

template <class BTListValue>
BTList <BTListValue> :: BTList (void)
{ mHead.mNext = mHead.mPrevious = & mHead; }

template <class BTListValue>
BTList <BTListValue> :: ~BTList (void)
{ clear (); }

template <class BTListValue> typename
BTList <BTListValue> :: iterator
BTList <BTListValue> :: begin (void) 
{ return mHead.mNext; }

template <class BTListValue> typename
BTList <BTListValue> :: iterator
BTList <BTListValue> :: end (void) 
{ return &mHead; }

template <class BTListValue> typename
BTList <BTListValue> :: const_iterator
BTList <BTListValue> :: begin (void) const
{ return mHead.mNext; }

template <class BTListValue> typename
BTList <BTListValue> :: const_iterator
BTList <BTListValue> :: end (void) const
{ return &mHead; }

template <class BTListValue>
BTListValue &
BTList <BTListValue> :: front (void)    
{ return *begin (); }

template <class BTListValue>
BTListValue &
BTList <BTListValue> :: back (void) 
{ return *--end (); }

template <class BTListValue>
const BTListValue &
BTList <BTListValue> :: front (void) const 
{ return *begin (); }

template <class BTListValue>
const BTListValue &
BTList <BTListValue> :: back (void) const 
{ return *--end (); }

template <class BTListValue>
void
BTList <BTListValue> :: push_front (const BTListValue & element) 
{ insert (begin (), element); }

template <class BTListValue>    
void
BTList <BTListValue> :: push_back (const BTListValue & element) 
{ insert (end (), element); }

template <class BTListValue>    
void
BTList <BTListValue> :: pop_front (void) 
{ erase (begin ()); }

template <class BTListValue>    
void
BTList <BTListValue> :: pop_back (void) 
{ erase (--end ()); }

template <class BTListValue>    
bool
BTList <BTListValue> :: empty (void) const 
{ return( &mHead == mHead.mNext); }

template <class BTListValue>    
size_t
BTList <BTListValue> :: size (void) const 
{
        size_t value = 0;
        for (const_iterator i = begin ();
             i != end ();
             ++i)
          ++value;
        return value;
}
    
template <class BTListValue>    
void
BTList <BTListValue> :: insert (iterator position, const BTListValue & element)
{
        _BTListValueNode <BTListValue> * node = new _BTListValueNode <BTListValue>;
        node->mValue = element;
        node->mNext = position.mNode;
        node->mPrevious = position.mNode->mPrevious;
        position.mNode->mPrevious->mNext = node;
        position.mNode->mPrevious = node;
}

template <class BTListValue> typename
BTList <BTListValue> :: iterator
BTList <BTListValue> :: erase (iterator position)
{
        _BTListNode * next = position.mNode->mNext;
        position.mNode->mPrevious->mNext = position.mNode->mNext;
        position.mNode->mNext->mPrevious = position.mNode->mPrevious;
        delete position.mNode;
        return next;
}

template <class BTListValue>    
void
BTList <BTListValue> :: clear (void)
{
        for (_BTListNode * node = mHead.mNext, *next = node->mNext;
             node != &mHead;
             node = next, next = node->mNext)
          delete static_cast <_BTListValueNode <BTListValue> *> (node);
        mHead.mNext = mHead.mPrevious = &mHead;
}

#endif
