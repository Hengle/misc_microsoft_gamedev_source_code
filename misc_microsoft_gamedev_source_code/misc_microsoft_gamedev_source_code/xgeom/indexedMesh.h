//------------------------------------------------------------------------------
// indexed_mesh.h
// Rich Geldreich 
// Copyright (C) 2002 Blue Shift, Inc.
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "containers\unifier.h"
#include "math/plane.h"

#include <set>
#include <vector>
#include <stack>

//------------------------------------------------------------------------------
typedef int Vert_Index;
const Vert_Index NULL_VERT_INDEX = 0;
const Vert_Index INVALID_VERT_INDEX = -1;
//------------------------------------------------------------------------------
typedef int Edge_Index;
const Edge_Index NULL_EDGE_INDEX = 0;
const Edge_Index INVALID_EDGE_INDEX = -1;
//------------------------------------------------------------------------------
typedef int Leg_Index;
const Leg_Index NULL_LEG_INDEX = 0;
const Leg_Index INVALID_LEG_INDEX = -1;
//------------------------------------------------------------------------------
typedef int Tri_Index;
const Tri_Index NULL_TRI_INDEX = 0; 
const Tri_Index INVALID_TRI_INDEX = -1; 
//------------------------------------------------------------------------------
typedef BDynamicArray<Vert_Index> Vert_Index_Vec;
typedef BDynamicArray<Edge_Index> Edge_Index_Vec;
typedef BDynamicArray<Tri_Index> Tri_Index_Vec;
//------------------------------------------------------------------------------
struct Indexed_Edge : public Utils::RelativeOperators<Indexed_Edge>
{
   enum { MAX_VERTS = 2 };
   Vert_Index Avert[MAX_VERTS];

   Indexed_Edge(bool init = true)
   {
      if (init)
         Avert[0] = Avert[1] = INVALID_VERT_INDEX;
   }

   Indexed_Edge(int a, int b)
   {
      Avert[0] = a;
      Avert[1] = b;
   }

   const Vert_Index& operator[] (int i) const
   {
      return Avert[debugRangeCheck(i, (int)MAX_VERTS)];
   }

   Vert_Index& operator[] (int i) 
   {
      return Avert[debugRangeCheck(i, (int)MAX_VERTS)];
   }

   Indexed_Edge permuted(int i) const
   {
      debugRangeCheck(i, (int)MAX_VERTS);

      return Indexed_Edge(
         Avert[i],
         Avert[i ^ 1]
         );
   }

   int find(Vert_Index i) const
   {
      if (Avert[0] == i) return 0;
      if (Avert[1] == i) return 1;
      return INVALID_VERT_INDEX;
   }

   // a PERMUTED comparison!!
   static bool PermuteEqual(const Indexed_Edge& a, const Indexed_Edge& b)
   {
      return (a.Avert[0] == b.Avert[1]) && (a.Avert[1] == b.Avert[0]);
   }

   friend bool operator == (const Indexed_Edge& a, const Indexed_Edge& b) const
   {
      return (a.Avert[0] == b.Avert[0]) && (a.Avert[1] == b.Avert[1]);
   }

   friend bool operator < (const Indexed_Edge& a, const Indexed_Edge& b)
   {
      for (int i = 0; i < MAX_VERTS; i++)
      {
         if (a.Avert[i] < b.Avert[i])
            return true;
         else if (a.Avert[i] > b.Avert[i])
            return false;
      }

      return false;
   }

   size_t hash(void) const
   {
      // HACK-- assumes simple type!
      return hashFast(this, sizeof(Indexed_Edge));
   }
   
   operator size_t() const { return hash(); }
};
//------------------------------------------------------------------------------
struct Indexed_Tri : public Utils::RelativeOperators<Indexed_Tri>
{
   enum { MAX_VERTS = 3 };
   Vert_Index Avert[MAX_VERTS];
   
   Indexed_Tri(bool init = true)
   {
      if (init)
         Avert[0] = Avert[1] = Avert[2] = INVALID_VERT_INDEX;
   }

   Indexed_Tri(const Indexed_Tri& tri)
   {
      *this = tri;
   }

   Indexed_Tri(int a, int b, int c) 
   { 
      Avert[0] = a;
      Avert[1] = b;
      Avert[2] = c;
   }

   Indexed_Tri flipped(void) const
   {
      return Indexed_Tri(Avert[2], Avert[1], Avert[0]);
   }

   Indexed_Tri& flip(void) 
   {
      Vert_Index temp = Avert[0];
      
      Avert[0] = Avert[2];
      Avert[2] = temp;

      return *this;
   }

   Indexed_Tri permuted(int i) const
   {
      debugRangeCheck(i, (int)MAX_VERTS);

      return Indexed_Tri(
         Avert[Math::PosWrap(0 + i, (int)MAX_VERTS)],
         Avert[Math::PosWrap(1 + i, (int)MAX_VERTS)],
         Avert[Math::PosWrap(2 + i, (int)MAX_VERTS)]
         );
   }

   Indexed_Tri& permute_once(void)
   {
      const Vert_Index temp = Avert[0];
      
      Avert[0] = Avert[1];
      Avert[1] = Avert[2];
      Avert[2] = temp;
      
      return *this;
   }

   Indexed_Tri& permute_general(int i)
   {
      debugRangeCheck(i, (int)MAX_VERTS);
      
      const Vert_Index a = Avert[Math::PosWrap(0 + i, (int)MAX_VERTS)];
      const Vert_Index b = Avert[Math::PosWrap(1 + i, (int)MAX_VERTS)];
      const Vert_Index c = Avert[Math::PosWrap(2 + i, (int)MAX_VERTS)];

      Avert[0] = a;
      Avert[1] = b;
      Avert[2] = c;
      
      return *this;
   }

   int find(Vert_Index i) const
   {
      if (Avert[0] == i) return 0;
      if (Avert[1] == i) return 1;
      if (Avert[2] == i) return 2;
      return INVALID_VERT_INDEX;
   }

   Vert_Index min_index() const
   {
      return Math::Min3(Avert[0], Avert[1], Avert[2]);
   }

   Vert_Index max_index() const
   {
      return Math::Max3(Avert[0], Avert[1], Avert[2]);
   }

   bool degenerate() const
   {
      return 
         (Avert[0] == Avert[1]) ||
         (Avert[0] == Avert[2]) ||
         (Avert[1] == Avert[2]);
   }

   const Vert_Index& operator[] (int i) const
   {
      debugRangeCheck(i, (int)MAX_VERTS);
      return Avert[i];
   }

   Vert_Index& operator[] (int i) 
   {
      debugRangeCheck(i, (int)MAX_VERTS);
      return Avert[i];
   }

   const Vert_Index& vert_index(int i) const {  return Avert[debugRangeCheck(i, (int)MAX_VERTS)];   }
         Vert_Index& vert_index(int i)       {  return Avert[debugRangeCheck(i, (int)MAX_VERTS)];   }

   const Vert_Index& vert_index_wrap(int i) const  { return Avert[Math::iPosMod(i, 3)]; }
         Vert_Index& vert_index_wrap(int i)        { return Avert[Math::iPosMod(i, 3)]; }

   friend bool operator == (const Indexed_Tri& a, const Indexed_Tri& b) const
   {
      return 
         (a.Avert[0] == b.Avert[0]) &&
         (a.Avert[1] == b.Avert[1]) &&
         (a.Avert[2] == b.Avert[2]);
   }

   friend bool operator < (const Indexed_Tri& a, const Indexed_Tri& b)
   {
      for (int i = 0; i < MAX_VERTS; i++)
      {
         if (a.Avert[i] < b.Avert[i])
            return true;
         else if (a.Avert[i] > b.Avert[i])
            return false;
      }

      return false;
   }
   
   Indexed_Edge edge(int i) const
   {
      debugRangeCheck(i, (int)MAX_VERTS);
      return Indexed_Edge(Avert[i], Avert[Math::NextWrap(i, (int)MAX_VERTS)]);
   }

   // true of a and b share an edge.
   static bool CommonEdge(const Indexed_Tri& a, const Indexed_Tri& b)
   {
      for (int i = 0; i < MAX_VERTS; i++)
      {
         Indexed_Edge trial_edge = a.edge(i);
          
         for (int j = 0; j < MAX_VERTS; j++)
            if (Indexed_Edge::PermuteEqual(b.edge(j), trial_edge))
               return true;
      }
      
      return false;
   }

   size_t hash(void) const
   {
      // HACK-- assumes simple type!
      return hashFast(this, sizeof(Indexed_Tri));
   }
   
   operator size_t() const { return hash(); }

   // Permutes result until the first vertex is the lowest.
   Indexed_Tri canonical(void) const
   {
      Indexed_Tri res(*this);
      const Vert_Index min_vert_index = res.min_index();

      while (res[0] != min_vert_index)
         res.permute_once();
      
      return res;
   }
};
//------------------------------------------------------------------------------
typedef BDynamicArray<Indexed_Tri> Indexed_Tri_Vec;
//------------------------------------------------------------------------------
class Indexed_Mesh
{
   // Vert_Adj = set of all tris that use this vert
   typedef std::set<Tri_Index>            Vert_Adj;      
   typedef BDynamicArray<Vert_Adj>          Vert_Adj_Vec;
   
protected:
   Vert_Adj_Vec m_vert_adj;
   Indexed_Tri_Vec m_tris;

   void update_adj(Vert_Index vert_index, Tri_Index tri_index)
   {
      if (num_verts() <= vert_index)
         m_vert_adj.resize(Math::Max(vert_index + 1, num_verts() * 2));

      debugRangeCheck(vert_index, num_verts());
      debugRangeCheck(tri_index, num_tris());

      m_vert_adj[vert_index].insert(tri_index);
   }

public:
   struct Connected_Edge : Indexed_Edge
   {
      // Container of tris sharing this edge on each side.
      Tri_Index_Vec tris[2];
      
      Connected_Edge() { }
      Connected_Edge(const Indexed_Edge& edge) : Indexed_Edge(edge) { }

      int num_tris(int side = 0) const { return tris[debugRangeCheck(side, 2)].size(); }
      Tri_Index tri_index(int side = 0, int i = 0) const { return tris[debugRangeCheck(side, 2)][debugRangeCheck(i, num_tris(side))]; }
   
      const Tri_Index_Vec& get_tris(int side) const { return tris[debugRangeCheck(side, 2)]; }

      // Useful for 2-manifold meshes.
      Tri_Index tri_a(void) const
      {
         if (tris[0].empty())
            return -1;
         return tris[0][0];
      }

      // Useful for 2-manifold meshes.
      Tri_Index tri_b(void) const
      {
         if (tris[1].empty())
            return -1;
         return tris[1][0];
      }

      Tri_Index other_tri(Tri_Index ti) const
      {
         int opp_side = find_tri_side(ti);
         BDEBUG_ASSERT(-1 != opp_side);
         opp_side = 1 - opp_side;
         
         if (!num_tris(opp_side))
            return INVALID_TRI_INDEX;
         
         return tri_index(opp_side, 0);
      }

      // Returns offset of tri on side.
      // -1 if tri not found
      int find_tri(int side, Tri_Index ti) const
      {
         for (int i = 0; i < num_tris(side); i++)
            if (ti == tri_index(side, i))
               return i;
         return -1;
      }

      // Returns side of tri.
      // -1 if tri not found
      int find_tri_side(Tri_Index ti) const
      {
         int ofs = find_tri(0, ti);
         if (-1 != ofs)
            return 0;
         
         ofs = find_tri(1, ti);
         if (-1 != ofs)
            return 1;

         return -1;
      }
   };
   
   // Side of edge flag.
   enum { EDGE_INVERT_FLAG = 0x80000000 };

   struct Tri_Edges
   {
      Edge_Index Aedges[3];
      
      Tri_Edges() { }
      Tri_Edges(Edge_Index edge_a, Edge_Index edge_b, Edge_Index edge_c)
      {
         Aedges[0] = edge_a;
         Aedges[1] = edge_b;
         Aedges[2] = edge_c;
      }

      Edge_Index edge(Leg_Index li) const
      {
         return Aedges[debugRangeCheck(li, 3)] & ~EDGE_INVERT_FLAG;
      }

      Edge_Index operator [] (Leg_Index li) const 
      {
         return edge(li);
      }

      int edge_side(Leg_Index li) const
      {
         return (Aedges[debugRangeCheck(li, 3)] & EDGE_INVERT_FLAG) != 0;
      }

      // I don't like this. (See opposite_tri() below.)
      Tri_Index opposite_tri(const Indexed_Mesh& indexed_mesh, Leg_Index li) const
      {
         const Edge_Index edge_index = edge(li);
         const int side = edge_side(li);
         
         if (!indexed_mesh.edge(edge_index).num_tris(1 - side))
            return INVALID_TRI_INDEX;

         return indexed_mesh.edge(edge_index).tri_index(1 - side);
      }
   };

   typedef BDynamicArray<Connected_Edge> Connected_Edge_Cont;
   typedef BDynamicArray<Tri_Edges> Tri_Edges_Cont;

protected:     
   mutable Connected_Edge_Cont m_connected_edges;
   mutable Tri_Edges_Cont m_tri_edges;
   mutable bool m_has_connectivity;
   
   int m_lowest_index;
   int m_highest_index;

   mutable BDynamicArray<int> m_tri_groups;
   mutable int m_num_tri_groups;
   mutable bool m_has_tri_groups;
   
public:
   Indexed_Mesh(int num_verts = 0) : 
      m_vert_adj(num_verts),
      m_lowest_index(INT_MAX),
      m_highest_index(INT_MIN),
      m_num_tri_groups(0),
      m_has_connectivity(false),
      m_has_tri_groups(false)
   {
   }

   void clear(void)
   {
      m_tris.clear();
      m_vert_adj.clear();
      m_tri_edges.clear();
      m_connected_edges.clear();
      m_tri_groups.clear();

      m_lowest_index = INT_MAX;
      m_highest_index = INT_MIN;
      m_num_tri_groups = 0;
      m_has_connectivity = false;
      m_has_tri_groups = false;
   }

   // Adds one tri, updates adjacency, and lowest/highest index values.
   Tri_Index add_tri(const Indexed_Tri& tri)
   {
      Tri_Index tri_index = num_tris();
      m_tris.push_back(tri);

      for (int i = 0; i < 3; i++)
      {
         m_lowest_index = Math::Min(m_lowest_index, tri[i]);
         m_highest_index = Math::Max(m_highest_index, tri[i]);

         update_adj(tri[i], tri_index);
      }
      
      return tri_index;
   }

   // Adds array of tris.
   void add_tris(const Indexed_Tri* Ptris, int num_tris)
   {
      debugCheckNull(Ptris);
      for (int i = 0; i < num_tris; i++)
         add_tri(Ptris[i]);
   }
   
   // Finds triangle t.
   Tri_Index find(const Indexed_Tri& t, bool allow_permut = false) const
   {
      Vert_Index v = t[0];
      
      if (v >= num_verts())
         return INVALID_TRI_INDEX;

      Vert_Adj adj = vert_adj(v);

      for (Vert_Adj::const_iterator it = adj.begin(); it != adj.end(); it++)
      {  
         Tri_Index trial_ti = *it;
         const Indexed_Tri& trial_tri = tri(trial_ti);

         if (t == trial_tri) 
            return trial_ti;

         if (allow_permut)
         {
            if (t.permuted(1) == trial_tri)
               return trial_ti;
            if (t.permuted(2) == trial_tri)
               return trial_ti;
         }
      }

      return INVALID_TRI_INDEX;
   }

   typedef std::set<Tri_Index> Tri_Adj;

   // Returns set of all tris that share any of the tri's vertices (excluding passed tri).
   void tri_adj(Tri_Adj& ret, Tri_Index ti) const
   {
      ret.clear();

      const Indexed_Tri& t = tri(ti);

      for (int i = 0; i < 3; i++)
      {
         Vert_Index vi = t[i];
         const Vert_Adj& adj = vert_adj(vi);

         for (Vert_Adj::const_iterator it = adj.begin(); it != adj.end(); it++)
         {
            Tri_Index trial_ti = *it;

            if (trial_ti == ti)
               continue;

            ret.insert(trial_ti);
         }
      }
   }

   // Creates triangle/edge connectivity tables.
   void create_connectivity(void) const
   {
      m_tri_edges.resize(num_tris());

      typedef Unifier<Indexed_Edge> Indexed_Edge_Unifier;
      Indexed_Edge_Unifier m_edge_unifier;//(num_tris() * 3);
      
      int i;
      for (i = 0; i < num_tris(); i++)
      {
         for (int j = 0; j < 3; j++)
         {
            Indexed_Edge edge(tri(i).edge(j));
            Indexed_Edge permute_edge(edge.permuted(1));
                        
            Indexed_Edge_Unifier::Index found_index = m_edge_unifier.find(permute_edge);

            if (found_index != Indexed_Edge_Unifier::cInvalidIndex)
               m_tri_edges[i].Aedges[j] = found_index | EDGE_INVERT_FLAG;
            else
               m_tri_edges[i].Aedges[j] = m_edge_unifier.insert(edge).first;
         }
      }

      m_connected_edges.resize(m_edge_unifier.size());
            
      for (i = 0; i < m_edge_unifier.size(); i++)
         m_connected_edges[i] = Connected_Edge(m_edge_unifier[i]);

      for (i = 0; i < num_tris(); i++)
      {
         for (int j = 0; j < 3; j++)
         {
            int Iedge = m_tri_edges[i].Aedges[j];
            int side = (Iedge & EDGE_INVERT_FLAG) != 0;
            Iedge &= ~EDGE_INVERT_FLAG;

            if (m_connected_edges[Iedge].tris[side].size())
            {
               // RG [3/9/05] - I think this was for gladiator!
               //ErrorAbort("Indexed_Mesh::create_connectivity: Tri %i Vert %i Edge: %i Side: %i: too many tris sharing edge",
               //   i, j, Iedge, side);
            }
            
            m_connected_edges[Iedge].tris[side].push_back(i);
         }
      }

      m_has_connectivity = true;
   }

   // Lowest/highest triangle indices.
   int lowest_index(void) const { return m_lowest_index; }
   int highest_index(void) const { return m_highest_index; }

   int num_verts() const   {  return m_vert_adj.size(); }
   const Vert_Adj& vert_adj(Vert_Index vi) const { return m_vert_adj[debugRangeCheck(vi, num_verts())]; }

   int num_tris() const { return m_tris.size(); }
   const Indexed_Tri_Vec& tri_cont(void) const { return m_tris; }

   const Indexed_Tri& tri(Tri_Index ti) const { return m_tris[debugRangeCheck(ti, num_tris())]; }
   
   // Edge, tri edge info only valid after create_connectivity() is called!
   
   // create_connectivity() must be called first!
   int num_edges(void) const {   BDEBUG_ASSERT(m_has_connectivity); return m_connected_edges.size(); }

   // Returns container of "indexed mesh edges".
   // Each element contains the edge vertices, and the two tris on either side.
   // create_connectivity() must be called first!
   const Connected_Edge_Cont& edge_cont(void) const { BDEBUG_ASSERT(m_has_connectivity); return m_connected_edges; }
   const Connected_Edge& edge(Edge_Index ei) const { BDEBUG_ASSERT(m_has_connectivity); return m_connected_edges[debugRangeCheck(ei, num_edges())]; }

   // Returns container of "indexed tri edges".
   // Each element contains each tri's 3 edges.
   // create_connectivity() must be called first!
   const Tri_Edges_Cont& tri_edges_cont(void) const { BDEBUG_ASSERT(m_has_connectivity); return m_tri_edges;   }
   const Tri_Edges& tri_edges(Tri_Index ti) const { BDEBUG_ASSERT(m_has_connectivity); return m_tri_edges[debugRangeCheck(ti, num_tris())]; }

   // create_connectivity() must be called first!
   Tri_Index opposite_tri(Tri_Index ti, Leg_Index li) const
   {
      BDEBUG_ASSERT(m_has_connectivity);

      debugRangeCheck(ti, num_tris());
      debugRangeCheck(li, 3);

      const Edge_Index edge_index = tri_edges(ti).edge(li);
      const int edge_side = tri_edges(ti).edge_side(li);
      
      BDEBUG_ASSERT(edge(edge_index).num_tris(edge_side) > 0);
      BDEBUG_ASSERT(-1 != edge(edge_index).find_tri(edge_side, ti));

      if (edge(edge_index).num_tris(1 - edge_side) == 0)
         return INVALID_TRI_INDEX;
      
      return edge(edge_index).tri_index(1 - edge_side);
   }

   // create_connectivity() must be called first!
   std::pair<Edge_Index, Leg_Index> shared_edge(Tri_Index ti_a, Tri_Index ti_b) const
   {
      BDEBUG_ASSERT(m_has_connectivity);

      for (Leg_Index li = 0; li < 3; li++)
      {
         const Edge_Index edge_index = tri_edges(ti_a).edge(li);
         const int edge_side = tri_edges(ti_a).edge_side(li);
         
         BDEBUG_ASSERT(edge(edge_index).num_tris(edge_side) > 0);
         BDEBUG_ASSERT(-1 != edge(edge_index).find_tri(edge_side, ti_a));

         if (-1 != edge(edge_index).find_tri(1 - edge_side, ti_b))
            return std::make_pair(edge_index, li);
      }
   
      return std::make_pair(INVALID_EDGE_INDEX, INVALID_LEG_INDEX);
   }
   
   // create_connectivity() must be called first!
   void create_tri_groups(void) const
   {
      BDEBUG_ASSERT(m_has_connectivity);

      m_tri_groups.resize(num_tris());
      std::fill(m_tri_groups.begin(), m_tri_groups.end(), -1);
         
      m_num_tri_groups = 0;

      for (int i = 0; i < num_tris(); i++)
      {
         if (-1 != m_tri_groups[i]) 
            continue;

         std::stack<int> mark_stack;
         
         mark_stack.push(i);
         
         do
         {
            Tri_Index tri_index = mark_stack.top();
            mark_stack.pop();

            m_tri_groups[debugRangeCheck(tri_index, num_tris())] = m_num_tri_groups;
               
            for (int l = 0; l < 3; l++)
            {
               Tri_Index opposite_tri_index = opposite_tri(tri_index, l);
               if ((INVALID_TRI_INDEX != opposite_tri_index) && (-1 == m_tri_groups[debugRangeCheck(opposite_tri_index, num_tris())]))
                  mark_stack.push(opposite_tri_index);
            }
         } while (!mark_stack.empty());

         m_num_tri_groups++;
      }

      m_has_tri_groups = true;
   }

   int num_tri_groups(void) const { BDEBUG_ASSERT(m_has_tri_groups); return m_num_tri_groups; }
   int tri_group(Tri_Index ti) const { BDEBUG_ASSERT(m_has_tri_groups); return m_tri_groups[debugRangeCheck(ti, num_tris())]; }

   bool has_connectivity(void) const { return m_has_connectivity; }
   bool has_tri_groups(void) const { return m_has_tri_groups; }
};
//------------------------------------------------------------------------------
// Vertex type must expose operator==, hash(), and an operator Vec3() method.
template <class VERT>
class Connected_Mesh_Builder
{
public:
   typedef VERT Vert;

   typedef Indexed_Mesh::Connected_Edge Edge;
   
   struct Tri : Indexed_Tri
   {
      Indexed_Mesh::Tri_Edges edges;

      Plane plane;

      Tri() : Indexed_Tri() { }
      Tri(const Indexed_Tri& t, const Indexed_Mesh::Tri_Edges & e, const Plane& p) : 
         Indexed_Tri(t), edges(e), plane(p) { }
   };

   typedef BDynamicArray<Vert> Vert_Cont;
   typedef BDynamicArray<Edge> Edge_Cont;
   typedef BDynamicArray<Tri> Tri_Cont;

protected:
   Vert_Cont m_verts;
   Tri_Cont m_tris;
   Indexed_Mesh m_indexed_mesh;
                     
public:
   Connected_Mesh_Builder(
      int num_tris, 
      const Vert* Pverts, 
      float min_tri_area = Math::fSmallEpsilon) 
   {
      BDEBUG_ASSERT(num_tris > 0);
      debugCheckNull(Pverts);

      typedef Unifier<Vert> Vert_Unifier;
      Vert_Unifier vert_unifier(num_tris * 3);
      
      for (int Itri = 0; Itri < num_tris; Itri++)
      {
         const Vert* Ptri_verts = &Pverts[Itri * 3];
         NGon3 tri(Ptri_verts[0], Ptri_verts[1], Ptri_verts[2]);
                     
         if (tri.area_tri() < min_tri_area)
            continue;

         Tri trial_tri;
         trial_tri.plane = tri.plane();
         
         for (int i = 0; i < 3; i++)
            trial_tri[i] = vert_unifier.insert(Ptri_verts[i]).first;

         if (trial_tri.degenerate()) 
            continue;

         m_indexed_mesh.add_tri(trial_tri);
         m_tris.push_back(trial_tri);
      }
      
      m_verts.resize(vert_unifier.size());
      for (int Ivert = 0; Ivert < vert_unifier.size(); Ivert++)
         m_verts[Ivert] = vert_unifier[Ivert];

      m_indexed_mesh.create_connectivity();
            
      const Indexed_Mesh::Tri_Edges_Cont& tri_edges = m_indexed_mesh.tri_edges_cont();
      BDEBUG_ASSERT(m_tris.size() == m_indexed_mesh.num_tris());
      BDEBUG_ASSERT(m_tris.size() == tri_edges.size());
                     
      for (Itri = 0; Itri < m_tris.size(); Itri++)
         m_tris[Itri].edges = tri_edges[Itri];
   }
   
   const Vert_Cont&    verts(void)             const { return m_verts; }
   const Edge_Cont&    edges(void)             const { return m_indexed_mesh.edge_cont(); }
   const Tri_Cont&     tris(void)              const { return m_tris; }
   const Indexed_Mesh& indexed_mesh(void) const { return m_indexed_mesh; }

   int num_verts() const { return verts().size(); }
   int num_edges() const { return edges().size(); }
   int num_tris() const { return tris().size();  }
};
//------------------------------------------------------------------------------
template <class VERT, class UNIFIER>
struct Merged_Mesh_Builder
{
   typedef VERT Vert;
   typedef UNIFIER Unifier;

   Merged_Mesh_Builder(int max_tris) : m_unifier(max_tris * 3), m_cur_tri_edge(3) 
   { 
      m_tris.reserve(max_tris); 
   }

   int insert(const Vert& vert)
   {
      if (m_cur_tri_edge == 3)
      {
         m_tris.push_back(Indexed_Tri());
         m_cur_tri_edge = 0;
      }

      Indexed_Tri& tri = m_tris.back();
      int index = m_unifier.insert(vert).first;
      tri[m_cur_tri_edge] = index;
      
      ++m_cur_tri_edge;

      return index;
   }

         Unifier& unifier()         { return m_unifier; }
   const Unifier& unifier() const   { return m_unifier; }

         Unifier& verts()        { return m_unifier; }
   const Unifier& verts() const  { return m_unifier; }
      
         Indexed_Tri_Vec& tris()       { return m_tris; }
   const Indexed_Tri_Vec& tris() const { return m_tris; }

   int num_verts() const { return verts().size(); }
   int num_tris() const { return tris().size();  }

   const Indexed_Tri& tri(int i) const { return m_tris[debugRangeCheck(i, num_tris())]; }

   const Vert& vert(int i) const { return m_unifier[i]; }
      
protected:
   int m_cur_tri_edge;
   Unifier m_unifier;
   Indexed_Tri_Vec m_tris;
};
