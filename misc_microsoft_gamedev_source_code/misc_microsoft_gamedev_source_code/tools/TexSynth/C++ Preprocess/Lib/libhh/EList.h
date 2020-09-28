// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef EList_h
#define EList_h

#include "stddef.h"             // for offsetof()

#if 0
{
    ForEListOuter(list,SRVertex,_active,v) {
        assertx(v->_active.linked());
    } EndFor;
}
#endif

// Implements a doubly-linked list node to embed within other classes.
// Place this node at the beginning of the struct so that the _next field
//  has higher chance of being in same cache line with data.
class EListNode {
 public:
    EListNode();
    ~EListNode() { }
    int linked() const;
    EListNode* prev() const;
    EListNode* next() const;
    void unlink();
    void link_after(EListNode* n);
    void link_before(EListNode* n);
    void re_link_after(EListNode* n);
    void re_link_before(EListNode* n);
    void OK() const;
 private:
friend class EList;
    EListNode* _prev;
    EListNode* _next;           // placed second so close to rest of struct
    DISABLE_COPY(EListNode);
};

// The list object which heads a list pointing into EListNode's.
// Note that a single EListNode member can be assigned to one of
//  several mutually exclusive lists (but then the EListNode by itself
//  does not provide an efficient way to detect to which list
//  the node belongs).
class EList {
 public:
    EList();
    ~EList();
    const EListNode* delim() const;
    EListNode* delim();
    int empty() const;
    // Use n->link_before(this) to "enqueue".
    // Use n->link_after(this) to "insertfirst".
 private:
    EListNode _delim;
    DISABLE_COPY(EList);
};

//----------------------------------------------------------------------

inline EListNode::EListNode() : _next(0) { }
inline int EListNode::linked() const { return _next!=0; }
inline EListNode* EListNode::prev() const { return _prev; }
inline EListNode* EListNode::next() const { return _next; }

inline void EListNode::OK() const
{
    if (linked()) assertx(_next->_prev==this && _prev->_next==this);
}

inline void EListNode::unlink()
{
    ASSERTX(linked());
    // _prev->_next=_next;
    // _next->_prev=_prev;
    EListNode* cp=_prev;
    EListNode* cn=_next;
    cp->_next=cn;
    cn->_prev=cp;
    _next=0;
}

inline void EListNode::link_after(EListNode* n)
{
    ASSERTX(!linked());
    EListNode* nn=n->_next;
    _next=nn;
    _prev=n;
    n->_next=this;
    nn->_prev=this;
}

inline void EListNode::link_before(EListNode * n)
{
    ASSERTX(!linked());
    EListNode* np=n->_prev;
    _prev=np;
    _next=n;
    n->_prev=this;
    np->_next=this;
}

inline void EListNode::re_link_after(EListNode* n)
{
    ASSERTX(linked());
    EListNode* cp=_prev;
    EListNode* cn=_next;
    cp->_next=cn;
    cn->_prev=cp;
    EListNode* nn=n->_next;
    _next=nn;
    _prev=n;
    n->_next=this;
    nn->_prev=this;
}

inline void EListNode::re_link_before(EListNode* n)
{
    ASSERTX(linked());
    EListNode* cp=_prev;
    EListNode* cn=_next;
    cp->_next=cn;
    cn->_prev=cp;
    EListNode* np=n->_prev;
    _prev=np;
    _next=n;
    n->_prev=this;
    np->_next=this;
}

inline const EListNode* EList::delim() const { return &_delim; }
inline EListNode* EList::delim() { return &_delim; }
inline int EList::empty() const { return delim()->next()==delim(); }

#define EListOuter(T,F,p) ((T*)((char*)p-offsetof(T,F)))

#define ForEList(L,N) \
{ const EListNode* const zzdelim=(L).delim(); \
  for (const EListNode* N=zzdelim->next();N!=zzdelim;N=N->next()) {
#define DummyEndFor }}

#define ForEListOuter(L,T,F,N) \
ForEList(L,zz) T* N=EListOuter(T,F,zz);

#endif
