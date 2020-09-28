// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Materials_h
#define Materials_h

#include "Array.h"
#include "HString.h"

// An array of material strings, indexed by an
//  integer material identifier (matid).
// Some strings may be null.
// Strings are formatted like GMesh strings.
//  usually contain 'matid=%d'.
//  may contain 'simatid=%d'    (Softimage material identifier)
//  may contain 'mat="%s"'      (Material name)
//  may contain 'rgb=(%g %g %g)' (Diffuse color of material)
//  may contain 'groups="%s"'   (the model to which this face belongs)
class Materials {
 public:
    Materials();
    ~Materials();
    // Default operator=() and copy_constructor are safe.
    void read(istream& is);     // must be empty
    void write(ostream& os) const;
    void set(int matid, const char* matstring);
    const char* get(int matid) const;
    int num() const;
    enum { MAX_MATID=65535 };
    inline int ok(int i) const          { return _matstrings.ok(i); }
 private:
    Array<HString> _matstrings;
};

//------------------------------------------------------------------------

inline int Materials::num() const
{
    return _matstrings.num();
}

#endif
