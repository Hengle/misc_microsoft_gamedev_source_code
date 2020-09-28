// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Hash_h
#define Hash_h

#if 0
{
    Map<Edge,Vertex> mev;
    ForMapKeyValue(mev,Edge,e,Vertex,v) { do(e,v); } EndFor;
    ForMapKey(mev,Edge,Vertex,e) { do(e); } EndFor;
    ForMapValue(mev,Edge,Vertex,v) { do(v); } EndFor;
}
#endif

#include "Pool.h"

class BMap {
 public:
    BMap();
    ~BMap();
    void clear();
    void enter(Univ k, Univ v); // k must be new
    int contains(Univ k) const;
    Univ retrieve(Univ k, int& present) const;
    Univ retrieve(Univ k) const; // ret 0 if absent (die if v==0)
    Univ get(Univ k) const;      // die if absent
    Univ remove(Univ k);         // ret prevv or 0(ambiguous)
    Univ replace(Univ k, Univ v); // ret prevv or 0(ambiguous)
    int num() const;
    int empty() const;
    void OK() const;
 public:                        // should be protected
    struct Node {
        POOL_ALLOCATION(Node);
        Univ k;
        Univ v;
        Node* n;
    };
 public:                        // not recommented:
    void specialenter(Univ k, Univ v); // k need not be new
    Univ* specialretrieveaddr(Univ k) const; // ret 0 if absent
    Univ* specialretrieveenteraddr(Univ k);
    int specialadd(Univ k, Univ v); // ret is_new
    void specialslowenter(Univ k, Univ v);
    Node** fastiter_initialize1() const;
    Node** fastiter_initialize2() const;
    int special_memsize() const;
    static Node EmptyNode;
    POOL_ALLOCATION(BMap);
 protected:
    // {_b,_size,_num} could be made Array<Node*> but for efficiency
    Node** _b;                  // is zero if empty set!
    //
    int hashk(Univ k) const;
    void removeaux(int buckn, Node* n, Node* last);
 private:
friend class BMapIter;
    int _size;                  // 0 if !_b
    int _num;
    int _fbuckn;                 // <= first non-zero index, 0 if !_b
    //
    int perhaps_increase_size(); // ret: was_increased
    int perhaps_decrease_size(); // ret: was_decreased
    void quickenter(Univ k, Univ v);
    Node* find(Univ k) const;
    void resize(int newsize);
    void iter_initialize(int& bn, Node*& n) const;
    DISABLE_COPY(BMap);
};

INITIALIZE_POOL(BMap);
INITIALIZE_POOL_NESTED(BMap::Node,BMapNode);

class Random;

class BMapIter {
 public:
    BMapIter(const BMap& hh);
    BMapIter(const BMap& hh, Random& r);
    ~BMapIter();
    operator void*() const;
    void next();
    Univ key() const;
    Univ value() const;
 private:
    const BMap& h;
    int bn;
    BMap::Node* n;
    void advance();
    void advance_unrolled();
    void advancewrap();
    // shallow copy is safe
};

//----------------------------------------------------------------------------

// #define MAP_USE_REMAINDER

#if defined(MAP_USE_REMAINDER)
inline int BMap::hashk(Univ k) const { return Conv<unsigned>::d(k)%_size; }
#else
inline int BMap::hashk(Univ k) const {
    // 259 obtained from ~/other/perl/find_good_hash
    return ((Conv<unsigned>::d(k)*259)>>8)&(_size-1);
    // Test difference in speed with this:
    // return Conv<unsigned>::d(k)%_size;
    // Significant difference on SGI (see tMap.cxx) -> drop MAP_USE_REMAINDER
}
#endif

inline int BMap::perhaps_increase_size()
{
#if defined(MAP_USE_REMAINDER)
    if (_num>_size*3) {
        resize(!_size ? 5 : (_size-1)*5+3);
        return 1;
    }
#else
    if (_num>_size*2) {
        resize(!_size ? 4 : _size*4);
        return 1;
    }
#endif
    return 0;
}

inline int BMap::perhaps_decrease_size()
{
    if (!_num) {
        clear();
        return 1;
    }
#if defined(MAP_USE_REMAINDER)
    if (_num<=_size/2) {
        if (!_num) {
            clear();
            return 1;
        }
        if (_size>5) {
            resize((_size-3)/5+1);
            return 1;
        }
    }
#else
    if (_num<_size/2) {
        if (!_num) {
            clear();
            return 1;
        }
        if (_size>4) {
            resize(_size/4);
            return 1;
        }
    }
#endif
    return 0;
}

// no _num++, no resize, no _fbuckn
inline void BMap::quickenter(Univ k, Univ v)
{
    register int buckn=hashk(k);
    register Node* n=new Node;
    n->k=k; n->v=v; n->n=_b[buckn]; _b[buckn]=n;
}

inline void BMap::specialenter(Univ k, Univ v)
{
    _num++;
    perhaps_increase_size();
    quickenter(k,v);
    _fbuckn=0;
}

inline void BMap::enter(Univ k, Univ v)
{
    ASSERTX(!contains(k));
    specialenter(k,v);
}

inline BMap::Node* BMap::find(Univ k) const
{
    if (!_b) return 0;
    for (Node* n=_b[hashk(k)];n;n=n->n)
        if (n->k==k) return n;
    return 0;
}

inline int BMap::contains(Univ k) const
{
    return find(k)?1:0;
}

inline Univ BMap::retrieve(Univ k, int& present) const
{
    Node* n=find(k); present=n?1:0; return n?n->v:0;
}

inline Univ BMap::retrieve(Univ k) const
{
    Node* n=find(k);
    return n?n->v:0;
}

inline Univ BMap::get(Univ k) const
{
    // return assertv(find(k))->v;
    Node* n=find(k); ASSERTX(n); return n->v;
}

inline Univ BMap::replace(Univ k, Univ v)
{
    Node* n=find(k);
    Univ ov;
    return n?(ov=n->v,n->v=v,ov):0;
}

inline int BMap::num() const { return _num; }
inline int BMap::empty() const { return !_num; }

inline Univ* BMap::specialretrieveaddr(Univ k) const
{
    Node* n=find(k); return n?&n->v:0;
}

inline Univ* BMap::specialretrieveenteraddr(Univ k)
{
    if (!_b) { specialslowenter(k,0); return &find(k)->v; }
    int buckn=hashk(k);
    Node* n=_b[buckn];
    for (;n;n=n->n)
        if (n->k==k) return &n->v;
    n=new Node; n->k=k; n->v=0; n->n=_b[buckn]; _b[buckn]=n;
    _fbuckn=0;
    _num++;
    if (perhaps_increase_size()) return &find(k)->v;
    return &n->v;
}

inline int BMap::specialadd(Univ k, Univ v)
{
    if (!_b) { specialslowenter(k,v); return 1; }
    int buckn=hashk(k);
    Node* n=_b[buckn];
    for (;n;n=n->n)
        if (n->k==k) return 0;
    n=new Node; n->k=k; n->v=v; n->n=_b[buckn]; _b[buckn]=n;
    _fbuckn=0;
    _num++;
    perhaps_increase_size();
    return 1;
}

inline BMap::Node** BMap::fastiter_initialize1() const
{
    return &_b[_fbuckn-1];
}

inline BMap::Node** BMap::fastiter_initialize2() const
{
    return &_b[_size];
}


inline BMapIter::~BMapIter() { }
inline BMapIter::operator void*() const { return (void*)n; }
inline Univ BMapIter::key() const { return n->k; }
inline Univ BMapIter::value() const { return n->v; }

// unroll first iteration of advance() loop
inline void BMapIter::advance_unrolled()
{
    if (++bn<h._size && !(n=h._b[bn],n)) advance();
}

inline void BMapIter::next()
{
    if (!(n=n->n,n)) advance_unrolled();
}

//----------------------------------------------------------------------------

template<class K, class V> class MapIter;

template<class K, class V>
class Map : public BMap {
 public:
    inline Map() { }
    inline ~Map() { }
    inline void enter(K k, V v)
    { BMap::enter(Conv<K>::e(k),Conv<V>::e(v)); }
    inline int contains(K k) const
    { return BMap::contains(Conv<K>::e(k)); }
    inline V retrieve(K k, int& present) const
    { return Conv<V>::d(BMap::retrieve(Conv<K>::e(k),present)); }
    inline V retrieve(K k) const
    { return Conv<V>::d(BMap::retrieve(Conv<K>::e(k))); }
    inline V get(K k) const
    { return Conv<V>::d(BMap::get(Conv<K>::e(k))); }
    inline V remove(K k)
    { return Conv<V>::d(BMap::remove(Conv<K>::e(k))); }
    inline V replace(K k, V v)
    { return Conv<V>::d(BMap::replace(Conv<K>::e(k),Conv<V>::e(v))); }
//      typedef MapIter<K,V> Iter;
};

template<class K, class V>
class MapIter : public BMapIter {
 public:
    inline MapIter(const Map<K,V>& hh) : BMapIter(hh) { }
    inline MapIter(const Map<K,V>& hh, Random& r) : BMapIter(hh,r) { }
    inline ~MapIter() { }
    inline K key() const { return Conv<K>::d(BMapIter::key()); }
    inline V value() const { return Conv<V>::d(BMapIter::value()); }
};

#define ForMapKeyValue(S,T1,V1,T2,V2) \
{ register BMap::Node** zz_b0=(S).fastiter_initialize1(); \
  register BMap::Node** zz_bl=(S).fastiter_initialize2(); \
  register BMap::Node* zz_n=&BMap::EmptyNode; \
  for (;;) \
  { zz_n=zz_n->n; \
    if (!zz_n) { \
                 for (++zz_b0;zz_b0<zz_bl;zz_b0++) \
                     if (zz_n=*zz_b0,zz_n) break; \
                         if (!zz_n) break; \
                         } \
                         T1 V1=Conv< T1 >::d(zz_n->k); \
                         T2 V2=Conv< T2 >::d(zz_n->v);
#define DummyEndFor }}

#define ForMapKey(S,T1,T2,V) \
ForMapKeyValue(S,T1,V,T2,Vdummy) \
dummy_use(Vdummy);

#define ForMapValue(S,T1,T2,V) \
ForMapKeyValue(S,T1,Vdummy,T2,V) \
dummy_use(Vdummy);

#endif
