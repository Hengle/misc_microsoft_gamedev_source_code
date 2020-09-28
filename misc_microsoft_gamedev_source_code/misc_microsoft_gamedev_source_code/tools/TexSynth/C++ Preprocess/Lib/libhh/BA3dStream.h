// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef BA3dStream_h
#define BA3dStream_h

#include "A3dStream.h"
#include "Buffer.h"

class RBA3dStream : public RA3dStream { // Read from Buffer
 public:
    RBA3dStream(RBuffer& b);
    ~RBA3dStream();
    int recognize() const;      // ret -1=parse_err, 0=no, 1=partial, 2=yes
 protected:
    int readline(int& binary, int& type, float f[3], const char*& comment);
 private:
    RBuffer& _buf;
};

class WBA3dStream : public WA3dStream { // Write to Buffer
 public:
    WBA3dStream(WBuffer& b);
    ~WBA3dStream();
    void flush();
 protected:
    void output(int binary, int type, float f1, float f2, float f3);
    void outputcomment(const char* s);
    void blankline();
 private:
    WBuffer& _buf;
};

#endif
