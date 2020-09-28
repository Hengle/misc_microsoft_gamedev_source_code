// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef BSac_h
#define BSac_h

// I split away this extra include file from Sac.h so as to unconfuse
// SGI's MIPS PRO C++ compiler, which was pulling many functions out of
// the old Sac.cxx and compiling it in all files!

typedef void (*SACFUNC)(void*);
const int SACDUMMY=4;

class BSac {
 public:
// per-object
    void* access(int k) { return _a+k; }
    BSac() { }
 protected:
    void callfuncs(int num, int akeys[], SACFUNC afuncs[]);
    char _a[SACDUMMY];
};

#endif
