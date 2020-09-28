// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Encoding_h
#define Encoding_h

#include "Map.h"
#include "Geometry.h"

class BMoveToFront {
 public:
    BMoveToFront(int size=0);
    ~BMoveToFront();
    int enter(Univ e);
 private:
    int _size;
    Array<Univ> _list;
    DISABLE_ASSIGN_INT(BMoveToFront);
};

template<class T>
class MoveToFront : public BMoveToFront {
 public:
    inline MoveToFront() { }
    inline ~MoveToFront() { }
    inline int enter(T e) { return BMoveToFront::enter(Conv<T>::e(e)); }
};

class BEncoding {
 public:
    BEncoding();
    ~BEncoding();
    void add(Univ e, float prob);
    void print() const;
    float huffman_cost() const; // return sum of prob to encode binary tree
    float entropy() const;      // return entropy
    float norm_entropy() const; // normalized entropy (entropy()/tot_prob)
    float worst_entropy() const; // include probability table
    typedef const char* (*CB_ENTRY_NAME)(Univ e);
    void print_top_entries(const char* name, int ntop,
                           CB_ENTRY_NAME cb_entry_name) const;
 private:
    Map<Univ,float> _map;
    MoveToFront<Univ>* _movetofront;
    DISABLE_COPY(BEncoding);
};

template<class T>
class Encoding : public BEncoding {
 public:
    inline Encoding() { }
    inline ~Encoding() { }
    inline void add(T e, float prob) { BEncoding::add(Conv<T>::e(e),prob); }
    inline void print_top_entries(const char* name, int ntop,
                                  const char* (cb)(T e)) const
    { BEncoding::print_top_entries(name,ntop,(const char* (*)(Univ))cb); }
};

class DeltaEncoding {
 public:
    DeltaEncoding();
    ~DeltaEncoding();
    void enter_bits(int nbits);
    void enter_sign(int vsign);
    // total_bits is based on arithmetic coding of nbits and sign.
    void analyze(const char* s, int& ret_total_bits) const;
    static int val_bits(float v);
    static int val_sign(float v);
    void enter_coords(const float v[], int n);
    void enter_vector(const float v[], int n);
    void enter_coords(const Vector& v);
    void enter_vector(const Vector& v);
    float total_entropy() const;
 private:
    int _num;
    int _delta_bits;
    Encoding<int> _enc_nbits;
    int _prev_sign;
    Encoding<int> _enc_sign[2]; // based on previous sign
};

#endif
