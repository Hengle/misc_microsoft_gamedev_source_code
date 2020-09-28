// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Postscript_h
#define Postscript_h

class Frame;

class Postscript {
 public:
    Postscript(ostream& os, int nxpix, int nypix);
    ~Postscript();
    // x,y in range 0..1 in postscript (math) coordinate system
    // note this is different from hps.c which had x reversed
    void line(float x1p, float y1p, float x2p, float y2p);
    void point(float xp, float yp);
    void flushwrite(const char* s); // include linefeeds in s
    void linewrite(const char* s);  // include linefeeds in s
 private:
    ostream& _os;
    int _nxpix;
    int _nypix;
    enum STATE { UNDEF, POINT, LINE } _state; // state for line width
    int _opx, _opy;                           // old pen position if LINE
    int _bbx0, _bbx1, _bby0, _bby1;           // bounding box
    Frame* ctm;
    //
    void convert(float x, float y, int& px, int& py);
    void setstate(STATE st);
    DISABLE_COPY(Postscript);
};

#endif
