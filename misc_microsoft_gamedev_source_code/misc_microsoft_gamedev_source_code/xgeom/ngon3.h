//------------------------------------------------------------------------------
// ngon3.h
// Rich Geldreich 
// Copyright (C) 2002 Blue Shift, Inc.
//------------------------------------------------------------------------------
#pragma once

#include "rangeCheck.h"
#include "utils\utils.h"
#include "math\math.h"
#include "math\plane.h"
#include "math\generalVector.h"
#include "math\hyperRayLine.h"
#include "math\frustum.h"
#include "containers\staticArray.h"

#include "intersection.h"

inline float area_tri_x2(const BVec3& a, const BVec3& b, const BVec3& c)
{
  return ((c - a) % (a - b)).len();
}

// Generic 3D polygon class. 
// Not designed with speed in mind.
// Custom containers, vertices may be used, in theory anyway.
template <typename Container>
class NGon
{
public:
  typedef Container ContainerType;
  typedef typename Container::value_type VertexType;

protected:
  ContainerType m_verts;

public:
  NGon(int elements_to_reserve = 4) 
  { 
    m_verts.reserve(elements_to_reserve); 
  }

  NGon(const ContainerType& cont) : m_verts(cont)
  { 
  }

  NGon(const VertexType* Pverts, int n) : m_verts(Pverts, Pverts + n)
  {
  }


  NGon(const VertexType& a, const VertexType& b, const VertexType& c)
  {
    m_verts.pushBack(a);
    m_verts.pushBack(b);
    m_verts.pushBack(c);
  }

  NGon(const VertexType& a, const VertexType& b, const VertexType& c, const VertexType& d)
  {
    m_verts.pushBack(a);
    m_verts.pushBack(b);
    m_verts.pushBack(c);
    m_verts.pushBack(d);
  }

  NGon& operator= (const NGon& a)
  {
    m_verts = a.m_verts;
    return *this;
  }
  
  int size() const
  {
    return m_verts.size();
  }

  int num_verts() const
  {
    return size();
  }

  bool empty() const
  {
    return num_verts() == 0;
  }

  void clear() 
  {
    m_verts.erase(m_verts.begin(), m_verts.end());
  }

  void pushBack(const VertexType& v)
  {
    m_verts.pushBack(v);
  }

        VertexType& vertex(int i)       { return m_verts[debugRangeCheck(i, num_verts())];  }
  const VertexType& vertex(int i) const { return m_verts[debugRangeCheck(i, num_verts())];  }

        VertexType& operator[] (int i)       { return vertex(i); }
  const VertexType& operator[] (int i) const { return vertex(i); }

  bool operator== (const NGon& other) const
  {
    if (size() != other.size())
      return false;
    for (int i = 0; i < size(); i++)
      if (vertex(i) != other.vertex(i))
        return false;
    return true;
  }

  bool operator!= (const NGon& other) const { return !(*this == other); }

  // 0 <= i < num_verts()
  void erase(int i)
  {
    debugRangeCheck(i, num_verts());
    m_verts.erase(&m_verts[i]);
  }

  // 0 <= i <= num_verts()
  void insert_before(int i, const BVec3& a)
  {
    debugRangeCheck(i, num_verts() + 1);
    if (i == num_verts())
      pushBack(a);
    else
      m_verts.insert(&m_verts[i], a);
  }

  int num_tris() const
  {
    return Math::Max(0, num_verts() - 2);
  }

  bool is_ngon()  const { return num_verts() >= 3; }
  bool is_tri()  const { return num_verts() == 3; }
  bool is_quad()  const { return num_verts() == 4; }

  NGon tri(int i) const
  {
    debugRangeCheck(i, num_tris());
    return NGon(m_verts[0], m_verts[1 + i], m_verts[2 + i]);  
  }

  void tri_verts(VertexType& a, VertexType& b, VertexType& c, int i) const
  {
    debugRangeCheck(i, num_tris());
    a = m_verts[0];
    b = m_verts[1 + i];
    c = m_verts[2 + i];
  }

  // Newell's method
  // concave/convex
  BVec3 normal(bool normalize = true) const
  {
    BDEBUG_ASSERT(is_ngon());

    BVec3 n(0.0f);

    for (ContainerType::const_iterator it = m_verts.begin(); it != m_verts.end(); it++) 
    {
      ContainerType::const_iterator it1 = it;
      if (++it1 == m_verts.end()) 
        it1 = m_verts.begin();
      
      BVec3 x(*it - *it1);
      BVec3 y(*it + *it1);

      n += BVec3(x[1] * y[2], x[2] * y[0], x[0] * y[1]);
    }

    if (normalize)
      n.normalize();

    return n;
  }

  // must be triangle
  BVec3 normal_tri(bool normalize = true) const
  {
    BDEBUG_ASSERT(is_tri());
    BVec3 n((vertex(2) - vertex(0)) % (vertex(0) - vertex(1)));
    if (normalize)
      n.normalize();
    return n;
  }

  // Graphic Gems 2, page 170
  // concave/convex
  float area_slow() const
  {
    BDEBUG_ASSERT(is_ngon());

    BVec3 s(0.0f);
    
    for (ContainerType::const_iterator it = m_verts.begin(); it != m_verts.end(); it++) 
    {
      ContainerType::const_iterator n_it(it);
      if (++n_it == m_verts.end())
        n_it = m_verts.begin();

      s += (*it % *n_it);
    }

//    BDEBUG_ASSERT(Math::EqualTol(s * normal(), s.len(), Math::fLargeEpsilon));
    //return fabs(s * normal()) * .5f;  

    return s.len() * .5f;
  }

  // Graphic Gems 5, page 37
  // concave/convex
  float area() const
  {
    BDEBUG_ASSERT(is_ngon());

    const int k = num_verts();
    const int h = (k - 1) >> 1;
    const int l = (k & 1) ? (0) : (k - 1);

    BVec3 s;

    // (2 - 0) % (0 - 1)

    s = ((vertex(2*h)-vertex(0)) % (vertex(l) - vertex(2*h-1)));
        
    for (int i = 1; i < h; i++)
      s += ((vertex(2*i)-vertex(0)) % (vertex(2*i+1)-vertex(2*i-1)));

    float a = s.len() * .5f;

//    BDEBUG_ASSERT(Math::EqualTol(area_slow(), a, Math::fLargeEpsilon));

    return a;
  }

  // must be a triangle
  float area_tri() const
  {
    BDEBUG_ASSERT(is_tri());
    return ((vertex(2) - vertex(0)) % (vertex(0) - vertex(1))).len() * .5f;
  }

  // must be a triangle
  float area_tri_x2() const
  {
    BDEBUG_ASSERT(is_tri());
    return ((vertex(2) - vertex(0)) % (vertex(0) - vertex(1))).len();
  }

  // concave/convex
  Plane plane() const
  {
    BDEBUG_ASSERT(is_ngon());

    BVec3 n = normal();
    float min_d = FLT_MAX;
    for (ContainerType::const_iterator it = m_verts.begin(); it != m_verts.end(); it++) 
      min_d = Math::Min(min_d, *it * n);
    return Plane(n, min_d);
  }
  
  // Convex only. 
  // Clips to positive side of plane.
  NGon clip(const Plane& plane) const
  {
    NGon result;

    if (empty())
      return result;

    float prev_dist = plane.distanceToPoint(vertex(0));
      
    for (int prev = 0; prev < num_verts(); prev++)
    {
      int cur = Math::NextWrap(prev, num_verts());
      float cur_dist = plane.distanceToPoint(vertex(cur));

      if (prev_dist >= 0.0f)
        result.pushBack(vertex(prev));

      // check for crossing, avoiding cases that could cause duplicate output verts!!
      if (((prev_dist < 0.0f) && (cur_dist > 0.0f)) ||
          ((prev_dist > 0.0f) && (cur_dist < 0.0f)))
      {
        result.pushBack(VertexType::lerp(vertex(prev), vertex(cur), prev_dist / (prev_dist - cur_dist)));
      }
      
      prev_dist = cur_dist;
    }

    return result;
  }

  NGon clip(const BFrustum& f) const
  {
    NGon r0(*this), r1;
    NGon* Pr0 = &r0, *Pr1 = &r1;
    
    for (int i = 0; i < BFrustum::cPlaneMax; i++)
    {
      Plane p;
      *Pr1 = Pr0->clip(f.plane(p, static_cast<BFrustum::ePlaneIndex>(i)));
      if (Pr1->empty())
        return *Pr1;
      
      NGon* Ptemp = Pr0;
      Pr0 = Pr1;
      Pr1 = Ptemp;
    }

    return *Pr0;
  }

  // Convex only. 
  // Clips to negative side of plane.
  NGon neg_clip(const Plane& plane) const
  {
    NGon result;

    if (empty())
      return result;

    float prev_dist = plane.distanceToPoint(vertex(0));
      
    for (int prev = 0; prev < num_verts(); prev++)
    {
      int cur = Math::NextWrap(prev, num_verts());
      float cur_dist = plane.distanceToPoint(vertex(cur));

      if (prev_dist < 0.0f)
        result.pushBack(vertex(prev));

      // check for crossing, avoiding cases that could cause duplicate output verts!!
      if (((prev_dist <  0.0f) && (cur_dist >= 0.0f)) ||
          ((prev_dist >= 0.0f) && (cur_dist <  0.0f)))
      {
        result.pushBack(VertexType::lerp(vertex(prev), vertex(cur), prev_dist / (prev_dist - cur_dist)));
      }
      
      prev_dist = cur_dist;
    }

    return result;
  }

  // Convex only.
  // Clips to positive and negative sides of planes.
  void posneg_clip(NGon& pos, NGon& neg, const Plane& plane) const
  {
    pos.clear();
    neg.clear();
    
    if (empty())
      return;

    float prev_dist = plane.distanceToPoint(vertex(0));
      
    for (int prev = 0; prev < num_verts(); prev++)
    {
      int cur = Math::NextWrap(prev, num_verts());
      float cur_dist = plane.distanceToPoint(vertex(cur));

      if (prev_dist >= 0.0f)
        pos.pushBack(vertex(prev));
      else
        neg.pushBack(vertex(prev));

      // check for crossing, avoiding cases that could cause duplicate output verts/polys!!
      if (((prev_dist < 0.0f) && (cur_dist > 0.0f)) ||
          ((prev_dist > 0.0f) && (cur_dist < 0.0f)))
      {
        pos.pushBack(VertexType::lerp(vertex(prev), vertex(cur), prev_dist / (prev_dist - cur_dist)));
      }

      if (((prev_dist <  0.0f) && (cur_dist >= 0.0f)) ||
          ((prev_dist >= 0.0f) && (cur_dist <  0.0f)))
      {
        neg.pushBack(VertexType::lerp(vertex(prev), vertex(cur), prev_dist / (prev_dist - cur_dist)));
      }
      
      prev_dist = cur_dist;
    }

    return result;
  }

  // output: lo_bound <= P <= hi_bound
  NGon clip_inclusive(const AABB& bounds) const
  {
    NGon result;

    if (empty())
      return result;

    for (int i = 0; i < 6; i++)
    {
      Plane p;
      if (!i)
        result = clip(bounds.plane(p, i));
      else
        result = result.clip(bounds.plane(p, i));

      if (result.empty())
        break;
    }

    return result;
  }

  // output: lo_bound <= P < hi_bound
  NGon clip(const AABB& bounds) const
  {
    NGon result;

    if (empty())
      return result;

    for (int i = 0; i < 6; i++)
    {
      Plane p;
      bounds.plane(p, i);
      
      if (!i)
        result = clip(p);
      else if (i & 1)
        result = result.neg_clip(p.flipped());
      else
        result = result.clip(p);
      
      if (result.empty())
        break;
    }

    return result;
  }

  NGon project(const Plane& plane) const
  {
    NGon result;
    for (ContainerType::const_iterator it = m_verts.begin(); it != m_verts.end(); it++)
      result.pushBack(plane.project(*it));
    return result;
  }

  NGon project(const ParametricPlane& pp) const
  {
    NGon result;
    for (ContainerType::const_iterator it = m_verts.begin(); it != m_verts.end(); it++)
      result.pushBack(pp.project(*it));
    return result;
  }

  template<class MatrixType>
  NGon transform(const MatrixType& m) const
  {
    NGon result;
    for (ContainerType::const_iterator it = m_verts.begin(); it != m_verts.end(); it++)
      result.pushBack(*it * m);
    return result;
  }

  AABB bounds() const
  {
    BDEBUG_ASSERT(!empty());

    AABB result;
    result.initExpand();
    for (ContainerType::const_iterator it = m_verts.begin(); it != m_verts.end(); it++)
      result.expand(*it);
    return result;
  }

  // Uses crossing test.
  // Normal does not need to be unitized.
  // Supports concave ngons.
  bool vs_point(const BVec3& i, const BVec3* Ppoly_norm = NULL) const
  {
    BVec3 temp;
    if (!Ppoly_norm)
      Ppoly_norm = &(temp = normal(false));

    int x_axis, y_axis;
    Ppoly_norm->projectionAxesFast(x_axis, y_axis);

    const float ix = i[x_axis];
    const float iy = i[y_axis];

    int num_crossings = 0;
    
    bool prev_sign = (m_verts.front())[y_axis] < iy;
    
    for (ContainerType::const_iterator it = m_verts.begin(); it != m_verts.end(); it++) 
    {
      ContainerType::const_iterator n_it(it);
      if (++n_it == m_verts.end())
        n_it = m_verts.begin();

      const float x0 = (*it)[x_axis];
      const float y0 = (*it)[y_axis];
      const float x1 = (*n_it)[x_axis];
      const float y1 = (*n_it)[y_axis];

      bool cur_sign = y1 < iy;

      if ((prev_sign != cur_sign) && ((x0 > ix) || (x1 > ix)))
      {
        if ((x0 > ix) && (x1 > ix))
          num_crossings++;
        else if (y1 != y0)
        {
          float t = x0 + (x1 - x0) * (iy - y0) / (y1 - y0);
          if (t > ix)
            num_crossings++;
        }
      }

      prev_sign = cur_sign;
    }

    return (num_crossings & 1) != 0;    
  }

  bool vs_ray(BVec3& i, const Ray3& ray) const
  {
    BDEBUG_ASSERT(is_ngon());

    Plane p(plane());

    float t;
    EResult res = Intersection::ray3Plane(t, p, ray, true); 
               
    if (res != SUCCESS)
      return false;
      
    i = ray.evaluate(t);

    return vs_point(i, &p);
  }

  // sum of point masses
  BVec3 barycenter() const
  {
    BDEBUG_ASSERT(!empty());

    BVec3 res(0.0f);
    
    for (int i = 0; i < num_verts(); i++)
      res += vertex(i);
    
    return res * (1.0f / num_verts());
  }

  // center of gravity
  BVec3 centroid() const
  {
    float area2_total = 0.0f;
    BVec3 res(0.0f);
    
    for (int i = 0; i < num_tris(); i++)
    {
      BVec3 a, b, c;
      tri_verts(a, b, c, i);

      float area2 = ((c - a) % (a - b)).len();
      area2_total += area2;

      res += (a + b + c) * area2;
    }

    if (area2_total == 0.0f)
      return BVec3(0.0f);
    
    return res * (1.0f / (3.0f * area2_total));
  }

  int num_edges() const 
  {
    return num_verts();
  }

  Line3 edge(int i) const
  {
    debugRangeCheck(i, num_edges());
    return Line3(vertex(i), vertex(Math::NextWrap(i, num_edges())));
  }

  NGon flipped() const
  {
    NGon result;

    ContainerType::const_iterator it = m_verts.end();

    while (it != m_verts.begin())
    {
      it--;
      result.pushBack(*it);
    }

    return result;
  }

  NGon rotated() const
  {
    NGon result;
  
    if (num_verts())
    {
      for (int i = 1; i < num_verts(); i++)
        result.pushBack(vertex(i));
    
      result.pushBack(vertex(0));
    }
    
    return result;
  }

  // -1 if not found
  int find(const VertexType& v, float tol = Math::fSmallEpsilon) const
  {
    for (int i = 0; i < num_verts(); i++)
    {
      if (VertexType::EqualTol(v, vertex(i), tol))
        return i;
    }
    return -1;
  }

  // -1 if not found
  int find(const Line3& l, float tol = Math::fSmallEpsilon) const
  {
    for (int i = 0; i < num_edges(); i++)
    {
      if (Line3::equalTol(l, edge(i), tol))
        return i;
    }
    return -1;
  }

  int smallest_vertex() const
  {
    BDEBUG_ASSERT(num_verts());

    BVec3 l(vertex(0));
    int result = 0;
      
    for (int i = 1; i < num_verts(); i++) 
    {
      for (int j = 0; j < 3; j++)
      {
        if (vertex(i)[j] != l[j])
        {
          if (vertex(i)[j] < l[j])
            result = i;
          break;
        }
      }
    }

    return result;
  }

  int largest_vertex() const
  {
    BDEBUG_ASSERT(num_verts());

    BVec3 l(vertex(0));
    int result = 0;
      
    for (int i = 1; i < num_verts(); i++) 
    {
      for (int j = 0; j < 3; j++)
      {
        if (vertex(i)[j] != l[j])
        {
          if (vertex(i)[j] > l[j])
            result = i;
          break;
        }
      }
    }

    return result;
  }

#ifdef TODO
  // If polygon normal can't be determined, Invalid is returned.
  // If polygon is convex, ConvexCCW is returned (as the normal is always oriented
  // as if the poly is CCW).
  Polygon_Class classify() const
  {
    if (!num_verts())
      return Invalid;

    MSstate;
    float* Ptemp = MSnew float[num_verts() * 2];
    
    BVec3 norm(normal(false));
    if (norm.len2() == 0.0f)
      return Invalid;

    int x_axis, y_axis;
    norm.projection_axes(x_axis, y_axis); 

    for (int i = 0; i < num_verts(); i++)
    {
      Ptemp[i*2+0] = vertex(i)[x_axis];
      Ptemp[i*2+1] = vertex(i)[y_axis];
    }

    return classifyPolygon2(num_verts(), Ptemp);
  }
#endif
  
  // must be triangle
  // 12/13/01 references:
  // http://www.flipcode.com/cgi-bin/msg.cgi?showThread=21September2001-InterpolatingNormalsForRay-Tracing&forum=askmid&id=-1
  // http://research.microsoft.com/~hollasch/cgindex/math/barycentric.html
  // http://www.flipcode.com/geometry/issue09.shtml
  BVec2 to_barycentric(const BVec3& a, bool clamp = true) const
  {
    BDEBUG_ASSERT(is_tri());

// so many ways to do this!
// 3D area method
// 2D area method (proj. 3D tri to 2D)
// find 3D perp vectors of two edges
#if 0
    float s0, t0;
    {
      BVec3 n = normal(false);
      BVec3 s = n % (vertex(0) - vertex(2));
      BVec3 t = n % (vertex(0) - vertex(1));
      
      float s_scale = ((vertex(1) - vertex(0)) * s);
      float t_scale = ((vertex(2) - vertex(0)) * t);
      
      s0 = ((a - vertex(0)) * s) / s_scale;
      t0 = ((a - vertex(0)) * t) / t_scale;
    }
#endif

    int x_axis, y_axis;
    normal_tri(false).projection_axes_fast(x_axis, y_axis); 

    float v0x = vertex(0)[x_axis];
    float v0y = vertex(0)[y_axis];
    float v1x = vertex(1)[x_axis];
    float v1y = vertex(1)[y_axis];
    float v2x = vertex(2)[x_axis];
    float v2y = vertex(2)[y_axis];
    float p0x = a[x_axis];
    float p0y = a[y_axis];
    float d = 1.0f / (v1x * v2y - v1x * v0y - v0x * v2y - v1y * v2x + v1y * v0x + v0y * v2x);
    float s =  (v0x * p0y - v0x * v2y - p0y * v2x + p0x * v2y + v0y * v2x - v0y * p0x) * d;
    float t = -(v1x * v0y + v0x * p0y - v1y * v0x - v1x * p0y - v0y * p0x + v1y * p0x) * d;

    //BDEBUG_ASSERT(s >= -Math::fSmallEpsilon);
    //BDEBUG_ASSERT(t >= -Math::fSmallEpsilon);
    //BDEBUG_ASSERT(s + t <= (1.0f + Math::fSmallEpsilon));
    
    if (clamp)
    {
      // in case of precision/roundoff error
      s = Math::Clamp(s, 0.0f, 1.0f);
      t = Math::Clamp(t, 0.0f, 1.0f);
    }
    
    return BVec2(s, t);   
  }

  bool vs_point_tri(const BVec3& i, float tol = 0.0f) const
  {
    BVec2 b(to_barycentric(i, false));
    return ((b[0] >= -tol) && (b[1] >= -tol) && ((b[0] + b[1]) <= (1.0f + tol)));
  }

  // must be triangle
  // 1-x-y >= 0, x >= 0, y >= 0
  BVec3 from_barycentric(const BVec2& a) const
  {
    BDEBUG_ASSERT(is_tri());
    
    BDEBUG_ASSERT(a[0] >= 0.0f);
    BDEBUG_ASSERT(a[1] >= 0.0f);
    BDEBUG_ASSERT((1.0f - a[0] - a[1]) > -Math::fSmallEpsilon);

    return vertex(0) + (vertex(1) - vertex(0)) * a[0] + (vertex(2) - vertex(0)) * a[1];
  }

  // must be triangle
  // 0 <= s,t <= 1
  BVec3 random_point_tri(float s, float t) const
  {
    BDEBUG_ASSERT(is_tri());

#if 0
    // a barycentric combination, so this is easily optimized
    return vertex(0) * (1.0f - sqrt(t)) +
           vertex(1) * (1.0f - s) * sqrt(t) +
           vertex(2) * s * sqrt(t);
#endif
    
    float st = sqrt(t);
    return from_barycentric(BVec2(1.0f - st, s * st));
  }

  // must be convex
  BVec3 random_point() const
  {
    BDEBUG_ASSERT(is_ngon());

    float total_area = 0.0f;
    for (int i = 0; i < num_tris(); i++)
      total_area += tri(i).area_tri();

    BDEBUG_ASSERT(total_area > 0.0f);

    float s = Math::fRand(0.0f, 1.0f);
    float t = Math::fRand(0.0f, 1.0f);

    float sum_thresh = s * total_area;
    float sum = 0.0f;
    for (i = 0; i < num_tris(); i++)
    {
      float area = tri(i).area_tri();
      sum += area;
      if ((sum >= sum_thresh) || (i == num_tris() - 1))
      {
        //s = (area + sum_thresh - sum) / area;
        s = (sum_thresh - (sum - area)) / area;
        s = Math::Clamp(s, 0.0f, 1.0f);
        break;
      }
    }
    BDEBUG_ASSERT(i < num_tris());

    return tri(i).random_point_tri(s, t);
  }
  
   BVec3 point_on_quad(float u, float v) const
   {
      BDEBUG_ASSERT(num_verts() == 4);
      
      const BVec3* pQuad = &vertex(0);
      
      return BVec3(
         pQuad[0][0] * (1.0f - u) * (1.0f - v) + pQuad[1][0] * (1.0f - u) * v + pQuad[2][0] * u * v + pQuad[3][0] * u * (1.0f - v),
         pQuad[0][1] * (1.0f - u) * (1.0f - v) + pQuad[1][1] * (1.0f - u) * v + pQuad[2][1] * u * v + pQuad[3][1] * u * (1.0f - v),
         pQuad[0][2] * (1.0f - u) * (1.0f - v) + pQuad[1][2] * (1.0f - u) * v + pQuad[2][2] * u * v + pQuad[3][2] * u * (1.0f - v) );
   }
  
};

typedef NGon< BStaticArray<BVec3, 6> > NGon3;

template <class T> class NGon_Vector : public BStaticArray<T>  // evil!
{
  typedef BStaticArray<T> Inherited;

public:

  typedef T NGon_Type;

  NGon_Vector() : Inherited()
  {
  }

  int num_ngons() const 
  { 
    return size(); 
  }
  
  const T& ngon(int i) const { return (*this)[debugRangeCheck(i, num_ngons())]; }
        T& ngon(int i)       { return (*this)[debugRangeCheck(i, num_ngons())]; }

  AABB bounds() const
  {
    BDEBUG_ASSERT(!empty());

    AABB result;
    result.initExpand();
    for (int i = 0; i < num_ngons(); i++)
      result.expand(ngon(i));
    return result;
  }

  NGon_Vector clip(const Plane& plane) const
  {
    NGon_Vector result;
    for (int i = 0; i < num_ngons(); i++)
    {
      T temp(ngon(i).clip(plane));
      if ((temp.num_verts() >= 3) && (temp.area() > 0.0f))
        result.pushBack(temp);
    }
    return result;
  }

  NGon_Vector neg_clip(const Plane& plane) const
  {
    NGon_Vector result;
    for (int i = 0; i < num_ngons(); i++)
    {
      T temp(ngon(i).neg_clip(plane));
      if ((temp.num_verts() >= 3) && (temp.area() > 0.0f))
        result.pushBack(temp);
    }
    return result;
  }

  void posneg_clip(NGon_Vector& pos, NGon_Vector& neg, const Plane& plane) const
  {
    pos.clear();
    neg.clear();
    for (int i = 0; i < num_ngons(); i++)
    {
      T pos_temp, neg_temp;
      ngon(i).posneg_clip(pos_temp, neg_temp, plane);

      if ((pos_temp.num_verts() >= 3) && (pos_temp.area() > 0.0f))
        pos.pushBack(pos_temp);

      if ((neg_temp.num_verts() >= 3) && (neg_temp.area() > 0.0f))
        neg.pushBack(neg_temp);
    }
  }

  NGon_Vector clip_inclusive(const AABB& bounds) const
  {
    NGon_Vector result;
    for (int i = 0; i < num_ngons(); i++)
    {
      T temp(ngon(i).clip_inclusive(bounds));
      if ((temp.num_verts() >= 3) && (temp.area() > 0.0f))
        result.pushBack(temp);
    }
    return result;
  }

  NGon_Vector clip(const AABB& bounds) const
  {
    NGon_Vector result;
    for (int i = 0; i < num_ngons(); i++)
    {
      T temp(ngon(i).clip(bounds));
      if ((temp.num_verts() >= 3) && (temp.area() > 0.0f))
        result.pushBack(temp);
    }
    return result;
  }
};

typedef NGon_Vector<NGon3> NGon3_Vector;

