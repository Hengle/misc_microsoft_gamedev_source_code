/**********************************************************************

Filename    :   GEdgeAA.cpp
Content     :
Created     :   2005-2006
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.
                Patent Pending. Contact Scaleform for more information.

Notes       :

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GEdgeAA.h"

#ifndef GFC_NO_FXPLAYER_EDGEAA


const unsigned GEdgeAA::VertexIdx[6] = {0, 1, 2, 0, 1, 2 };

//------------------------------------------------------------------------
void GEdgeAA::RemoveAll()
{
    Vertices.removeAll();
    Edges.removeAll();
    MeshTriangles.removeAll();
}

//------------------------------------------------------------------------
void GEdgeAA::AddVertex(const GPointType& v)
{
    VertexType tv;
    tv.x  = v.x;
    tv.y  = v.y;
    tv.id = -1;
    Vertices.add(tv);
}

//------------------------------------------------------------------------
void GEdgeAA::AddTriangle(unsigned v1, unsigned v2, unsigned v3, unsigned style)
{
    EdgeType e;
    MeshTriType tri;

    tri.iniVer[0] = v1;
    tri.iniVer[1] = v2;
    tri.iniVer[2] = v3;
    tri.newVer[0] = v1;
    tri.newVer[1] = v2;
    tri.newVer[2] = v3;

    tri.startEdge = Edges.size();
    tri.adjTri[0] =
    tri.adjTri[1] =
    tri.adjTri[2] = -1;
    tri.extVer[0] =
    tri.extVer[1] =
    tri.extVer[2] = -1;
    tri.edgeStat[0] =
    tri.edgeStat[1] =
    tri.edgeStat[2] = 0;
    tri.style = style;

    e.tri = MeshTriangles.size() << 2;
    e.v1  = v1;
    e.v2  = v2;
    Edges.add(e);
    e.tri++;
    e.v1  = v2;
    e.v2  = v3;
    Edges.add(e);
    e.tri++;
    e.v1  = v3;
    e.v2  = v1;
    Edges.add(e);

    MeshTriangles.add(tri);
}

//------------------------------------------------------------------------
void GEdgeAA::buildAdjacencyTable()
{
    EdgeIdx.allocate(Edges.size());
    unsigned i;
    for(i = 0; i < Edges.size(); i++)
    {
        EdgeIdx[i] = i;
    }
    EdgeIdxLess edgeLess(Vertices, Edges);
    GAlg::QuickSort(EdgeIdx, edgeLess);

    for(i = 0; i < MeshTriangles.size(); i++)
    {
        MeshTriType& tri = MeshTriangles[i];
        tri.adjTri[0] = findAdjacentTriangle(tri.startEdge);
        tri.adjTri[1] = findAdjacentTriangle(tri.startEdge + 1);
        tri.adjTri[2] = findAdjacentTriangle(tri.startEdge + 2);
    }
}


//------------------------------------------------------------------------
bool GEdgeAA::buildEdgesFan(unsigned triIdx)
{
    int  adjStart = triIdx;
    int  adj      = adjStart;
    int  adj1, adj2;
    bool cyclic   = false;
    int  lastEdge = 0x7FFFFFFF;
    unsigned lastIdx = 0;

    FanEdges.removeAll();
    for(;;)
    {
        FanEdges.add(adj1 = adj);
        FanEdges.add(adj2 = adj = triangleNextIdx(adj, 2));
        if(adj1 == lastEdge || adj2 == lastEdge)
        {
            // Protection from infinite loops in degenerate cases
            FanEdges.removeAll();
            return true;
        }

        adj = adjacentTriangle(adj);
        if(adj == adjStart)
        {
            cyclic = true;
            break;
        }
        if(adj < 0)
        {
            break;
        }
        lastEdge = FanEdges[lastIdx++];
    }

    GAlg::ReverseContainer(FanEdges);
    if(cyclic)
    {
        return true;
    }

    adj = adjStart;
    for(;;)
    {
        adj = adjacentTriangle(adj);
        if(adj < 0)
        {
            break;
        }
        FanEdges.add(adj1 = adj);
        FanEdges.add(adj2 = adj = triangleNextIdx(adj, 1));
        if(adj1 == lastEdge || adj2 == lastEdge)
        {
            // Protection from infinite loops in degenerate cases
            FanEdges.removeAll();
            return true;
        }
        lastEdge = FanEdges[lastIdx++];
    }
    return false;
}

//------------------------------------------------------------------------
void GEdgeAA::calcIntersectionPoint(GCoordType width, GCoordType lim,
                                    unsigned start, unsigned end,
                                    GCoordType* x, GCoordType* y) const
{
    VertexType v1 = triangleVertex(FanEdges[start], 0);
    VertexType v2 = triangleVertex(FanEdges[start], 1);
    VertexType v3 = triangleVertex(FanEdges[end],   0);
    VertexType v4 = triangleVertex(FanEdges[end],   1);
    GCoordType vx = v2.x;
    GCoordType vy = v2.y;
    GCoordType len1 = GMath2D::CalcDistance(v1, v2);
    GCoordType len2 = GMath2D::CalcDistance(v3, v4);
    GCoordType epsilon = (len1 + len2) * G_IntersectionEpsilonAA;
    GCoordType dx1 = width * (v2.y - v1.y) / len1;
    GCoordType dy1 = width * (v1.x - v2.x) / len1;
    GCoordType dx2 = width * (v4.y - v3.y) / len2;
    GCoordType dy2 = width * (v3.x - v4.x) / len2;

    if(GMath2D::CalcIntersection(v1.x + dx1, v1.y + dy1,
                                 v2.x + dx1, v2.y + dy1,
                                 v3.x + dx2, v3.y + dy2,
                                 v4.x + dx2, v4.y + dy2, x, y, epsilon))
    {
        GCoordType dist = GMath2D::CalcDistance(*x, *y, vx, vy);
        if (len1 < len2)
            len1 = len2;
        if (lim  > len1)
            lim  = len1;
        if(dist > lim)
        {
            GCoordType k = lim / dist;
            *x = vx + (*x - vx) * k;
            *y = vy + (*y - vy) * k;
        }
    }
    else
    {
        if (len1 > len2)
        {
            *x = v2.x + dx1;
            *y = v2.y + dy1;
        }
        else
        {
            *x = v3.x + dx2;
            *y = v3.y + dy2;
        }
    }
}


//------------------------------------------------------------------------
GCoordType GEdgeAA::trianglePerimeterSquare(const MeshTriType& tri) const
{
    const VertexType& v1 = Vertices[tri.newVer[0]];
    const VertexType& v2 = Vertices[tri.newVer[1]];
    const VertexType& v3 = Vertices[tri.newVer[2]];
    GCoordType dx1 = v1.x - v2.x;
    GCoordType dy1 = v1.y - v2.y;
    GCoordType dx2 = v2.x - v3.x;
    GCoordType dy2 = v2.y - v3.y;
    GCoordType dx3 = v3.x - v1.x;
    GCoordType dy3 = v3.y - v1.y;
    return dx1 * dx1 + dy1 * dy1 +
           dx2 * dx2 + dy2 * dy2 +
           dx3 * dx3 + dy3 * dy3;
}


//------------------------------------------------------------------------
void GEdgeAA::correctCrossIntersection(const VertexType& iniVer,
                                       const MeshTriType& tri,
                                       unsigned triVerIdx,
                                       VertexType* newVer,
                                       GCoordType width) const
{
    GUNUSED(width);

    const VertexType& v1 = Vertices[tri.newVer[VertexIdx[triVerIdx + 1]]];
    const VertexType& v2 = Vertices[tri.newVer[VertexIdx[triVerIdx + 2]]];

    if(GMath2D::CrossProduct(*newVer, v1, v2) < -0.1)
    {
        return;
    }

    // A simple variant, works fairly well
    GCoordType x, y;
    GCoordType epsilon = (fabsf(iniVer.x - newVer->x) + 
                          fabsf(iniVer.y - newVer->y) + 
                          fabsf(v1.x     - v2.x) + 
                          fabsf(v1.y     - v2.y)) * G_IntersectionEpsilonAA;
    if(GMath2D::CalcIntersection(iniVer, *newVer,
                                 v1, v2,
                                 &x, &y,
                                 epsilon))
    {
        x += (iniVer.x - x) * (GCoordType)0.25; // Correction to reduce possible overlaps
        y += (iniVer.y - y) * (GCoordType)0.25;
        newVer->x = x;
        newVer->y = y;
    }

    //// An attempt to improve it. Doesn't help much
    //GCoordType cp1 = GMath2D::CrossProduct(iniVer, *newVer, v1);
    //GCoordType cp2 = GMath2D::CrossProduct(iniVer, *newVer, v2);
    //GCoordType x, y;
    //
    //if(cp1 > 0 && cp2 < 0)
    //{
    //    // The new vector spits the triangle
    //    if(GMath2D::CalcIntersection(iniVer, *newVer,
    //                                 v1, v2,
    //                                 &x, &y,
    //                                 G_IntersectionEpsilonAA))
    //    {
    //        newVer->x = x;
    //        newVer->y = y;
    //    }
    //}
    //else
    //{
    //    // The vector is outside of the triangle
    //    GCoordType d;
    //    GCoordType k = 1;
    //
    //    if(cp1 <= 0)
    //    {
    //        d = GMath2D::CalcDistance(iniVer, v1);
    //        if(width < d)
    //        {
    //            k = width / d;
    //        }
    //        newVer->x = iniVer.x + (v1.x - iniVer.x) * k;
    //        newVer->y = iniVer.y + (v1.y - iniVer.y) * k;
    //    }
    //    else
    //    if(cp2 >= 0)
    //    {
    //        d = GMath2D::CalcDistance(iniVer, v2);
    //        if(width < d)
    //        {
    //            k = width / d;
    //        }
    //        newVer->x = iniVer.x + (v2.x - iniVer.x) * k;
    //        newVer->y = iniVer.y + (v2.y - iniVer.y) * k;
    //    }
    //}
}

//------------------------------------------------------------------------
int GEdgeAA::findAdjacentTriangle(unsigned ei) const
{
    EdgeType e;
    e.v1 = Edges[ei].v2;
    e.v2 = Edges[ei].v1;
    EdgeLess edgeLess(Vertices, Edges);
    unsigned pos = GAlg::LowerBound(EdgeIdx, e, edgeLess);
    if(pos < EdgeIdx.size())
    {
        const EdgeType& e2 = Edges[EdgeIdx[pos]];
        if(e2.v1 == e.v1 && e2.v2 == e.v2)
        {
            return e2.tri;
        }
    }
    return -1;
}


//------------------------------------------------------------------------
void GEdgeAA::ProcessEdges(GCoordType width, AA_Method aaMethod)
{
    unsigned i, j, k;

    Triangles.removeAll();
    buildAdjacencyTable();

    if(width >= 0)
    {
        return;
    }

    if(aaMethod == AA_OuterEdges)
    {
        width *= 2;
    }

    GCoordType lim = fabsf(width) * GetIntersectionMiterLimit();

    for(i = 0; i < MeshTriangles.size(); i++)
    {
        const MeshTriType& tri = MeshTriangles[i];
        for(j = 0; j < 3; j++)
        {
            if((tri.edgeStat[j] & EdgeModified) == 0)
            {
                bool cyclic = buildEdgesFan((i << 2) + j);

                if(FanEdges.size() == 0)
                {
                    continue;
                }

                if(cyclic &&
                   MeshTriangles[FanEdges[0] >> 2].style ==
                   MeshTriangles[FanEdges.last() >> 2].style)
                {
                    // Process only those cases that have different
                    // styles at start start and the end.
                    // It protects us from using improper starting edges,
                    // as well as from processing of cyclic fans with
                    // all the same styles.
                    continue;
                }

                GCoordType x, y;
                unsigned newVerIdx;
                unsigned adjVal;
                TmpStarVer.removeAll();
                const VertexType& center = Vertices[tri.iniVer[j]];

                if(!cyclic)
                {
                    // Non-circular fan means that we have external edges
                    calcIntersectionPoint(-width, lim, 0, FanEdges.size() - 1, &x, &y);

                    newVerIdx = Vertices.size();
                    Vertices.add(VertexType(x, y));
                    TmpStarVer.add(newVerIdx);

                    adjVal = FanEdges[0];
                    MeshTriangles[adjVal >> 2].extVer[VertexIdx[(adjVal & 3) + 1]] = newVerIdx;

                    adjVal = FanEdges.last();
                    MeshTriangles[adjVal >> 2].extVer[adjVal & 3] = newVerIdx;
                }

                if(aaMethod != AA_OuterEdges)
                {
                    if(aaMethod == AA_AllEdges)
                    {
                        unsigned start, end;
                        for(start = 0; start < FanEdges.size(); )
                        {
                            const MeshTriType& t1 = MeshTriangles[FanEdges[start] >> 2];
                            for(end = start + 1; end < FanEdges.size(); end++)
                            {
                                if(t1.style != MeshTriangles[FanEdges[end] >> 2].style)
                                {
                                    break;
                                }
                            }

                            calcIntersectionPoint(width, lim, start, end - 1, &x, &y);
                            newVerIdx = Vertices.size();
                            Vertices.add(VertexType(x, y));
                            TmpStarVer.add(newVerIdx);
                            VertexType& newVer = Vertices.last();
                            for(k = start; k < end; k += 2)
                            {
                                unsigned triVerIdx = triangleNextIdx(FanEdges[k], 1);
                                MeshTriType& t2 = MeshTriangles[triVerIdx >> 2];
                                correctCrossIntersection(Vertices[tri.iniVer[j]],
                                                         t2,
                                                         triVerIdx & 3,
                                                         &newVer,
                                                         -width);
                                t2.newVer[triVerIdx & 3]  = newVerIdx;
                                t2.edgeStat[triVerIdx & 3] |= EdgeModified;
                            }
                            start = end;
                        }
                    }
                    else
                    {
                        // Anti-Alias all triangles
                        //-------------------------
                        for(k = 0; k < FanEdges.size(); k += 2)
                        {
                            MeshTriType& t1   = MeshTriangles[FanEdges[k] >> 2];
                            unsigned triVerIdx = triangleNextIdx(FanEdges[k], 1) & 3;
                            if(t1.newVer[triVerIdx] == t1.iniVer[triVerIdx]) // Not yet modified
                            {
                                VertexType v1 = Vertices[t1.iniVer[0]];
                                VertexType v2 = Vertices[t1.iniVer[1]];
                                VertexType v3 = Vertices[t1.iniVer[2]];

                                // In this case it is not very important to calculate proper epsilon
                                if(GMath2D::ShrinkTriangle(&v1, &v2, &v3, width, G_IntersectionEpsilonAA))
                                {
                                    t1.newVer[0] = Vertices.size();
                                    Vertices.add(v1);
                                    t1.newVer[1] = Vertices.size();
                                    Vertices.add(v2);
                                    t1.newVer[2] = Vertices.size();
                                    Vertices.add(v3);
                                }
                                else
                                {
                                    t1.newVer[0] = t1.newVer[1] = t1.newVer[2] = Vertices.size();
                                    Vertices.add(v1);
                                }
                            }
                            t1.edgeStat[triVerIdx] |= EdgeModified;
                            TmpStarVer.add(t1.newVer[triVerIdx]);
                        }
                    }

                    while(TmpStarVer.size() > 2)
                    {
                        GCoordType maxDist = 0;
                        unsigned   maxIdx  = 0;
                        unsigned   s = TmpStarVer.size();
                        for(k = 0; k < s; k++)
                        {
                            GCoordType d = GMath2D::CalcDistance(Vertices[TmpStarVer[k]],
                                                                 center);
                            if(d > maxDist)
                            {
                                maxDist = d;
                                maxIdx = k;
                            }
                        }
                        unsigned i1 = (maxIdx + s - 1) % s;
                        unsigned i2 =  maxIdx;
                        unsigned i3 = (maxIdx + 1) % s;

                        TriangleType sr;
                        sr.v1 = TmpStarVer[i1];
                        sr.v2 = TmpStarVer[i2];
                        sr.v3 = TmpStarVer[i3];
                        Triangles.add(sr);
                        TmpStarVer.removeAt(maxIdx);
                    }
                }
            }
        }
    }

    for(i = 0; i < MeshTriangles.size(); i++)
    {
        MeshTriType& t1 = MeshTriangles[i];
        TriangleType tr;
        if(t1.newVer[0] != t1.newVer[1] &&
           t1.newVer[1] != t1.newVer[2] &&
           t1.newVer[2] != t1.newVer[0])
        {
            tr.v1 = t1.newVer[0];
            tr.v2 = t1.newVer[1];
            tr.v3 = t1.newVer[2];
            Triangles.add(tr);
        }

        for(j = 0; j < 3; j++)
        {
            Vertices[t1.newVer[j]].id = t1.style;
            if((t1.edgeStat[j] & TrapezoidEmitted) == 0)
            {
                int adj = t1.adjTri[j];
                int v1, v2, v3, v4;

                if(adj < 0)
                {
                    // External edge
                    v3 = t1.extVer[VertexIdx[j + 1]];
                    v4 = t1.extVer[j];
                    if(v3 >= 0 && v4 >= 0)
                    {
                        v1 = triangleNewVerIdx((i << 2) | j);
                        v2 = triangleNewVerIdx((i << 2) | j, 1);

                        tr.v1 = v1;
                        tr.v2 = v2;
                        tr.v3 = v4;
                        Triangles.add(tr);

                        tr.v1 = v4;
                        tr.v2 = v2;
                        tr.v3 = v3;
                        Triangles.add(tr);
                    }
                }
                else
                {
                    // Internal edge;
                    MeshTriType& t2 = MeshTriangles[adj >> 2];
                    if(t2.style != t1.style || aaMethod == AA_AllTriangles)
                    {
                        v1 = triangleNewVerIdx((i << 2) | j);
                        v2 = triangleNewVerIdx((i << 2) | j, 1);
                        v3 = triangleNewVerIdx(adj);
                        v4 = triangleNewVerIdx(adj, 1);

                        tr.v1 = v1;
                        tr.v2 = v2;
                        tr.v3 = v4;
                        Triangles.add(tr);

                        tr.v1 = v4;
                        tr.v2 = v2;
                        tr.v3 = v3;
                        Triangles.add(tr);
                        t2.edgeStat[adj & 3] |= TrapezoidEmitted;
                    }
                }
                t1.edgeStat[j] |= TrapezoidEmitted;
            }
        }
    }
}

//------------------------------------------------------------------------
void GEdgeAA::SortTrianglesByStyle()
{
    unsigned i;

    // Arrange styles in triangles
    for(i = 0; i < Triangles.size(); i++)
    {
        TriangleType& tri = Triangles[i];
        if(Vertices[tri.v2].id < Vertices[tri.v1].id) GAlg::SwapElements(tri.v2, tri.v1);
        if(Vertices[tri.v3].id < Vertices[tri.v2].id) GAlg::SwapElements(tri.v3, tri.v2);
        if(Vertices[tri.v2].id < Vertices[tri.v1].id) GAlg::SwapElements(tri.v2, tri.v1);
    }

    // Arrange triangles by styles in lexicographical order
    TriangleLess triangleLess(Vertices);
    GAlg::QuickSort(Triangles, triangleLess);
}

#endif // #ifndef GFC_NO_FXPLAYER_EDGEAA
