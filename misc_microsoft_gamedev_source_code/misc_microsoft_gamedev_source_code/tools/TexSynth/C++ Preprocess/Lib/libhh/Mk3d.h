// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Mk3d_h
#define Mk3d_h

#include "A3dStream.h"
#include "Stack.h"

#define MKNEW(x) mk.push(); { x } mk.pop()

class Mk3d {
 public:
    Mk3d(WA3dStream& pos);
    ~Mk3d();
    WA3dStream& oa3d();
    void push();
    void pop();
    void translate(float x, float y, float z);
    void translate(const Vector& v);
    enum { Xaxis=0, Yaxis=1, Zaxis=2 };
    void rotate(int axis, float angle);
    void scale(float x, float y, float z);
    void scale(float v);
    void apply(const Frame& t);
    Point transform(const Point& point);
    Vector transform(const Vector& normal);
    void pushcolor();
    void popcolor();
    void diffuse(float r, float g, float b);
    void diffuse(const A3dColor& col);
    void specular(float r, float g, float b);
    void specular(const A3dColor& col);
    void phong(float p);
    void color(const A3dVertexColor& color);
    void scalecolor(float sr, float sg, float sb);
    void point(float x, float y, float z);
    void point(const Point& p);
    void normal(float x, float y, float z);
    void normal(const Vector& normal);
    void beginForcePolyline(int force);
    void endForcePolyline();
    void beginForce2Sided(int force);
    void endForce2Sided();
    void beginForceFlip(int force);
    void endForceFlip();
    void endpolygon();
    void end2polygon();         // two sided polygon
    void endpolyline();
    void endpoint();
 private:
    WA3dStream& _os;
    int _ntrans;
    int _maxntrans;
    int _tottrans;
    Stack<Frame*> _Sframe;
    Stack<Frame*> _Sframei;
    Frame _ctm;
    Frame _ctmi;
    int _ncolor;
    Stack<A3dVertexColor*> _Scolor;
    A3dVertexColor _cc;
    int _maxvertices;
    int _totvertices;
    A3dElem _a3de;
    int _totpolygons, _totpolylines, _totpoints;
    int _forcepolyline, _force2sided, _forceflip;
    Stack<int> _Sforcepolyline;
    Stack<int> _Sforce2sided;
    Stack<int> _Sforceflip;
    //
    void outputpoly();
    void flippoly();
    DISABLE_COPY(Mk3d);
};

#endif
