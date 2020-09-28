// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Histogram_h
#define Histogram_h

#include "Stat.h"
#include "Array.h"

class Histogram {
 public:
    Histogram(const char* filename, int nbuckets);
    ~Histogram();
    void clear();
    void add(float val);
    void add_to_stat(Stat& stat);
    void set_nbuckets(int nbuckets);
    void set_filename(const char* filename);
 private:
    Array<float> _ar_val;
    const char* _filename;
    int _nbuckets;
    DISABLE_COPY(Histogram);
};

//----------------------------------------------------------------------

inline void Histogram::add(float val)
{
    _ar_val+=val;
}

#endif
