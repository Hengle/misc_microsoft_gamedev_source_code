#if !defined(GRANNY_WINGED_EDGE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_winged_edge.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #3 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

struct winged_vertex;
struct winged_face;
struct winged_edge;

struct winged_vertex
{
    winged_edge *Edge;

    winged_vertex *Next;
    winged_vertex *Previous;

    //
    int32x SerializedIndex;
};

struct winged_face
{
    winged_edge *Edge;

    winged_face *Next;
    winged_face *Previous;

    //
    int32x SerializedIndex;
};

/*
               \     /
     Edge[1][0] \   / Edge[1][1]
                 \ /
                  * Vertex[1]
                  ^
                  |
        Face[0]   |   Face[1]
                  |
                  * Vertex[0]
                 / \
     Edge[0][1] /   \ Edge[0][0]
               /     \
*/
struct winged_edge
{
    winged_edge *Edge[2][2];
    winged_edge *Vertex[2];
    winged_edge *Face[2];

    winged_edge *Next;
    winged_edge *Previous;

    //
    int32x SerializedIndex;
};

struct winged_mesh
{
    fixed_allocator VertexAllocator;
    fixed_allocator EdgeAllocator;
    fixed_allocator FaceAllocator;
};

#include "header_postfix.h"
#define GRANNY_WINGED_EDGE_H
#endif
