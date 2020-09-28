// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef FrameIO_h
#define FrameIO_h

#include "Geometry.h"

class RBuffer;
class WBuffer;

class FrameIO {
 public:
// input
    static int recognize(RBuffer& b); // ret -1=err, 0=no, 1=partial, 2=yes
    static int read(istream& is, Frame& f, int& obn, float& zoom,
                    int& bin);  // ret is_success
    static int read(RBuffer& b, Frame& f, int& obn, float& zoom,
                    int& bin);  // ret is_success
// output
    static const char* string(const Frame& f, int obn, float zoom);
    static int write(ostream& os, const Frame& f, int obn, float zoom,
                     int bin);  // ret is_success
    static int write(WBuffer& b, const Frame& f, int obn, float zoom,
                     int bin);  // ret is_success
// special frames
    static int isNaF(const Frame& f);
    static void MakeNaF(Frame& f);
 private:
    static void decode(istream& is, Frame& f, int& obn, float& zoom);
};

#endif
