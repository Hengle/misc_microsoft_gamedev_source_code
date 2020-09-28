//-------------------------------------------------------------------------------------------------
// strip_tunneler.h
// Rich Geldreich 
// Copyright (C) 2002 Blue Shift, Inc.
//-------------------------------------------------------------------------------------------------
#pragma once
//-------------------------------------------------------------------------------------------------
#include "triStrips.h"
//-------------------------------------------------------------------------------------------------
#define STRIP_TUNNELER_VERIFY_DATABASE
//-------------------------------------------------------------------------------------------------
class Strip_Tunneler
{
public:
  typedef int Strip_Index;
  enum { INVALID_STRIP_INDEX = -1 };

private:
  struct Tri_Node;

  struct Strip_Edge
  {
    bool m_solid;
    Strip_Edge() : m_solid(false) { }
  };

  struct Tri_Node
  {
    Strip_Index m_strip_index;
    int m_strip_ofs;

    Tri_Index m_prev_tri_index;
    Tri_Index m_next_tri_index;

    Edge_Index m_tunnel_parent_edge;
    Edge_Index m_tunnel_next_edge;
    bool m_solid;
    int m_distance;
    char m_strip_dir;
    int m_tunnel_id;
    
    Tri_Node() : 
      m_strip_index(INVALID_STRIP_INDEX), 
      m_strip_ofs(-1), 
      m_prev_tri_index(INVALID_TRI_INDEX),
      m_next_tri_index(INVALID_TRI_INDEX),
      m_tunnel_parent_edge(INVALID_EDGE_INDEX),
      m_tunnel_next_edge(INVALID_EDGE_INDEX),
      m_distance(0),
      m_strip_dir(0),
      m_tunnel_id(0)
    { 
    }

    bool at_strip_start(void) const 
    {
      return (INVALID_TRI_INDEX == m_prev_tri_index);
    }
    
    bool at_strip_end(void) const 
    {
      return (INVALID_TRI_INDEX == m_next_tri_index);
    }
  };

  struct Strip
  {
    Tri_Index m_first_tri_index;
    Tri_Index m_last_tri_index;
    Strip(Tri_Index first_tri_index = INVALID_TRI_INDEX, Tri_Index last_tri_index = INVALID_TRI_INDEX) : 
      m_first_tri_index(first_tri_index),
      m_last_tri_index(last_tri_index)
    { 
    }
  };

  BDynamicArray<Strip_Edge> m_strip_edges;
  BDynamicArray<Tri_Node> m_tri_nodes;
  BDynamicArray<Strip> m_strips;
        
  const Indexed_Mesh& m_src_mesh;

  Indexed_Mesh m_mesh;

  const Tri_Strips& m_source_strips;

  Tri_Strips m_output_strips;
  
  int m_num_strips;
  int m_num_valid_tris;

  BDynamicArray<int> m_bad_strip_offsets;

  // true if strip can't be used
  bool init_strip(
    const BDynamicArray<Tri_Index>& strip_tris, 
    std::set<Tri_Index>& seen_tris_set,
    BDynamicArray<int>& edge_tri_count)
  {
    int i;

    BASSERT(!strip_tris.empty());
    
    for (i = 0; i < strip_tris.size() - 1; i++)
    {
      Tri_Index ti_a = strip_tris[i];
      Tri_Index ti_b = strip_tris[i + 1];

      // Overly paranoid? A tristrip shouldn't ever jump between connected tri groups.
      if (m_src_mesh.tri_group(strip_tris[0]) != m_src_mesh.tri_group(ti_b))
        return true;

      if (!seen_tris_set.insert(ti_a).second)
        return true;

      if (i == strip_tris.size() - 1)
      {
        if (!seen_tris_set.insert(ti_b).second)
          return true;
      }

      std::pair<Edge_Index, Leg_Index> ea = m_src_mesh.shared_edge(ti_a, ti_b);
      std::pair<Edge_Index, Leg_Index> eb = m_src_mesh.shared_edge(ti_b, ti_a);
      
      BASSERT(ea.first != INVALID_EDGE_INDEX);
      BASSERT(eb.first != INVALID_EDGE_INDEX);
      BASSERT(ea.first == ea.first);

      Edge_Index shared_edge = debugRangeCheck(ea.first, m_src_mesh.num_edges());
            
      if ((m_src_mesh.edge(shared_edge).num_tris(0) > 1) ||
          (m_src_mesh.edge(shared_edge).num_tris(1) > 1))
        return true;
    }

    // Check to see if this strip would introduce multi-edges.
    for (i = 0; i < strip_tris.size(); i++)
    {
      const Tri_Index tri_index = strip_tris[i];
      for (Leg_Index leg_index = 0; leg_index < 3; leg_index++)
      {
        Edge_Index edge_index = m_src_mesh.tri_edges(tri_index)[leg_index];
        if (INVALID_EDGE_INDEX != edge_index)
        {
          debugRangeCheck(edge_index, m_src_mesh.num_edges());
          edge_tri_count[edge_index]++;
          if (edge_tri_count[edge_index] > 2)
            return true;

          // Nov.6: Ensure that each side of the edge only has 1 tri.
          if ((m_src_mesh.edge(edge_index).num_tris(0) > 1) ||
              (m_src_mesh.edge(edge_index).num_tris(1) > 1))
            return true;
        }
      }
    }
    
    // Strip looks OK, save it.

    Tri_Index prev_tri_index = INVALID_TRI_INDEX;

    Strip strip;
    for (i = 0; i < strip_tris.size(); i++)
    {
      const Tri_Index src_tri_index = strip_tris[i];
      debugRangeCheck(src_tri_index, m_src_mesh.num_tris());
      
      const Tri_Index tri_index = m_mesh.add_tri(m_src_mesh.tri(src_tri_index));
      
      if (!i)
        strip.m_first_tri_index = tri_index;
      if (i == strip_tris.size() - 1)
        strip.m_last_tri_index = tri_index;

      m_tri_nodes[tri_index].m_strip_index = m_strips.size();
      m_tri_nodes[tri_index].m_strip_ofs = i;
      
      if (INVALID_TRI_INDEX != prev_tri_index)
        m_tri_nodes[prev_tri_index].m_next_tri_index = tri_index;

      m_tri_nodes[tri_index].m_prev_tri_index = prev_tri_index;
      
      prev_tri_index = tri_index;
    }

    m_strips.push_back(strip);
    m_num_strips++;

    m_num_valid_tris += strip_tris.size();
    
    return false;
  }

  // Returns indices of tris in tristrip starting at the indicated tri.
  void get_strip_tri_indices(BDynamicArray<Tri_Index>& tri_indices, Tri_Index tri_index)
  {
    tri_indices.clear();
    do
    {
      tri_indices.push_back(debugRangeCheck(tri_index, m_mesh.num_tris()));
      tri_index = m_tri_nodes[tri_index].m_next_tri_index;
    } while (INVALID_TRI_INDEX != tri_index);
  }
  
  // Initialize the solid edges.
  void create_solid_edges(void)
  {
    // One more check for multi-edges 
    for (int i = 0; i < m_mesh.num_edges(); i++)
    {
      BVERIFY(m_mesh.edge(i).num_tris(0) <= 1);
      BVERIFY(m_mesh.edge(i).num_tris(1) <= 1);
      
      if ((m_mesh.edge(i).num_tris(0) == 1) && (m_mesh.edge(i).num_tris(1) == 1))
      {
        Tri_Index a = m_mesh.edge(i).tri_index(0);
        Tri_Index b = m_mesh.edge(i).tri_index(1);
        
        std::pair<Edge_Index, Leg_Index> ea = m_mesh.shared_edge(a, b);
        std::pair<Edge_Index, Leg_Index> eb = m_mesh.shared_edge(b, a);
        BVERIFY(ea.first == eb.first);
        BVERIFY(ea.first == i);
      }
    }

    m_strip_edges.resize(m_mesh.num_edges());

    int num_tris = 0;

    for (int strip_index = 0; strip_index < m_num_strips; strip_index++)
    {
      BDynamicArray<Tri_Index> strip_tris;
      get_strip_tri_indices(strip_tris, m_strips[strip_index].m_first_tri_index);
      
      num_tris += strip_tris.size();

      for (int i = 0; i < strip_tris.size() - 1; i++)
      {
        Tri_Index ti_a = strip_tris[i];
        Tri_Index ti_b = strip_tris[i + 1];

        std::pair<Edge_Index, Leg_Index> ea = m_mesh.shared_edge(ti_a, ti_b);
        std::pair<Edge_Index, Leg_Index> eb = m_mesh.shared_edge(ti_b, ti_a);
        
        BASSERT(ea.first != INVALID_EDGE_INDEX);
        BASSERT(eb.first != INVALID_EDGE_INDEX);
        BASSERT(ea.first == ea.first);

        Edge_Index shared_edge = debugRangeCheck(ea.first, m_mesh.num_edges());
              
        BASSERT(m_mesh.edge(shared_edge).num_tris(0) <= 1);
        BASSERT(m_mesh.edge(shared_edge).num_tris(1) <= 1);
        BASSERT(-1 != m_mesh.edge(shared_edge).find_tri_side(ti_a));
        BASSERT(-1 != m_mesh.edge(shared_edge).find_tri_side(ti_b));

        BASSERT(!m_strip_edges[shared_edge].m_solid);
        m_strip_edges[shared_edge].m_solid = true;
      }
    }
    
    BVERIFY(num_tris == m_num_valid_tris);
  }

  // Scan input tristrip indices/flags, segment into separate tristrips.
  // Records offsets of rejected tristrips.
  void init_strips(void)
  {
    if (!m_src_mesh.has_connectivity())
      m_src_mesh.create_connectivity();

    if (!m_src_mesh.has_tri_groups())
      m_src_mesh.create_tri_groups();

    m_tri_nodes.resize(m_src_mesh.num_tris());
        
    bool winding = false;

    BDynamicArray<Tri_Index> strip_tris;
    
    std::set<Tri_Index> seen_tris_set;
    BDynamicArray<int> edge_tri_count(m_src_mesh.num_edges());
    
    int strip_start_ofs = -1;
    bool bad_strip = false;

    for (int i = 0; i < m_source_strips.size() - 2; i++)
    {
      const uchar flags0 = m_source_strips.flags(i + 0);
      const uchar flags1 = m_source_strips.flags(i + 1);
      const uchar flags2 = m_source_strips.flags(i + 2);

      if ((flags1 & Tri_Strips::TRISTRIP_FLAGS_START) || (flags2 & Tri_Strips::TRISTRIP_FLAGS_START))
        continue;
      
      if (flags0 & Tri_Strips::TRISTRIP_FLAGS_START)
      {
        if (!strip_tris.empty())
        {
          if (!bad_strip)
            bad_strip = init_strip(strip_tris, seen_tris_set, edge_tri_count);
          if (bad_strip)
            m_bad_strip_offsets.push_back(strip_start_ofs);
        }

        winding = (flags0 & Tri_Strips::TRISTRIP_FLAGS_FLIP) != 0;
              
        strip_tris.clear();

        strip_start_ofs = i;
        bad_strip = false;
      }

      Indexed_Tri tri(m_source_strips[i + 0], m_source_strips[i + 1], m_source_strips[i + 2]);
      
      // Might be a swap.
      if ((!tri.degenerate()) && (!bad_strip))
      {
        if (winding)
          tri.flip();
                  
        // check for flipped to handle generalized tristrips
        Tri_Index tri_index = m_src_mesh.find(tri, true);
        Tri_Index flipped_tri_index = m_src_mesh.find(tri.flipped(), true);
          
        // Two sided check: This rejects BOTH triangles! (Should only reject one side.)
        if ((INVALID_TRI_INDEX != tri_index) && (INVALID_TRI_INDEX != flipped_tri_index))
          bad_strip = true;
        else if (INVALID_TRI_INDEX != tri_index)
          strip_tris.push_back(tri_index);
        else if (INVALID_TRI_INDEX != flipped_tri_index)
          strip_tris.push_back(flipped_tri_index);
        else 
          bad_strip = true;
      }
        
      winding = !winding;
    }

    if (!strip_tris.empty())
    {
      if (!bad_strip)
        bad_strip = init_strip(strip_tris, seen_tris_set, edge_tri_count);
      if (bad_strip)
        m_bad_strip_offsets.push_back(strip_start_ofs);
    }

    m_tri_nodes.resize(m_mesh.num_tris());

    m_mesh.create_connectivity();
    m_mesh.create_tri_groups(); // shouldn't be necessary (don't use trigroups any more)

    create_solid_edges();
            
    trace("Strip_Tunneler::init_strips: Manifold tris: %i Strips: %i Ignored strips: %i", m_num_valid_tris, m_num_strips, m_bad_strip_offsets.size());
  }
  
  // Scan for all connected tris, make new strip.
  void fix_strip(Tri_Index tri_index, int& next_tunnel_id, const int strip_index)
  {
    const int tunnel_id = next_tunnel_id;
    next_tunnel_id++;

    Tri_Index first_tri_index = INVALID_TRI_INDEX;
    Tri_Index last_tri_index = INVALID_TRI_INDEX;

    std::stack<Tri_Index> tri_stack;
    tri_stack.push(tri_index);

    std::set<Tri_Index> tris;
    tris.insert(tri_index);

    int num_tris = 0;

    // Search for tristrip start/end (1-D floodfill).
    do
    {
      const Tri_Index cur_tri_index = tri_stack.top();
      tri_stack.pop();

      debugRangeCheck(cur_tri_index, m_mesh.num_tris());

      BASSERT(m_tri_nodes[cur_tri_index].m_tunnel_id != tunnel_id);
      
      m_tri_nodes[cur_tri_index].m_tunnel_id = tunnel_id;
      m_tri_nodes[cur_tri_index].m_strip_index = strip_index;//m_strips.size();
      m_tri_nodes[cur_tri_index].m_strip_ofs = -1;
      num_tris++;

      int num_solid = 0;
      for (Leg_Index leg_index = 0; leg_index < 3; leg_index++)
      {
        const Edge_Index edge_index = m_mesh.tri_edges(cur_tri_index)[leg_index];
        if (INVALID_EDGE_INDEX == edge_index)
          continue;
        
        debugRangeCheck(edge_index, m_mesh.num_edges());

        if (m_strip_edges[edge_index].m_solid)
        {
          num_solid++;
          
          Tri_Index opp_tri_index = m_mesh.opposite_tri(cur_tri_index, leg_index);
          if (m_tri_nodes[opp_tri_index].m_tunnel_id != tunnel_id)
          {
            BASSERT(tris.find(opp_tri_index) == tris.end()); //loop! CRAP.
            tris.insert(opp_tri_index);

            tri_stack.push(opp_tri_index);
          }
        }
      }

      BASSERT(num_solid <= 2);

      if (num_solid == 0)
      {
        BASSERT(num_tris == 1);
        last_tri_index = first_tri_index = cur_tri_index;       
      }
      else if (num_solid == 1)
      {
        if (INVALID_TRI_INDEX == first_tri_index)
          first_tri_index = cur_tri_index;
        else if (INVALID_TRI_INDEX == last_tri_index)
          last_tri_index = cur_tri_index;
        else
        {
          BVERIFY(false);
        }
      }
    } while (!tri_stack.empty());

    BVERIFY(INVALID_TRI_INDEX != first_tri_index);
    BVERIFY(INVALID_TRI_INDEX != last_tri_index);

    // Found tristrip start/end, now fix up the tri node data.
    
    Tri_Index cur_tri_index = first_tri_index;
    Tri_Index prev_tri_index = INVALID_TRI_INDEX;
    int strip_ofs = 0;
    
    for ( ; ; )
    {
      BASSERT(tris.find(cur_tri_index) != tris.end());

      debugRangeCheck(cur_tri_index, m_mesh.num_tris());

      BASSERT(m_tri_nodes[cur_tri_index].m_strip_index == strip_index);//m_strips.size());
      BASSERT(m_tri_nodes[cur_tri_index].m_strip_ofs == -1);
      BASSERT(m_tri_nodes[cur_tri_index].m_tunnel_id == tunnel_id);

      if (INVALID_TRI_INDEX != prev_tri_index)
        m_tri_nodes[prev_tri_index].m_next_tri_index = cur_tri_index;

      m_tri_nodes[cur_tri_index].m_strip_ofs = strip_ofs;
      m_tri_nodes[cur_tri_index].m_prev_tri_index = prev_tri_index;
      m_tri_nodes[cur_tri_index].m_next_tri_index = INVALID_TRI_INDEX;
      strip_ofs++;

      if (cur_tri_index == last_tri_index)
        break;

      prev_tri_index = cur_tri_index;
      
      int leg_index;
      for (leg_index = 0; leg_index < 3; leg_index++)
      {
        const Edge_Index edge_index = m_mesh.tri_edges(cur_tri_index)[leg_index];
        if (INVALID_EDGE_INDEX == edge_index)
          continue;
        
        debugRangeCheck(edge_index, m_mesh.num_edges());

        if (!m_strip_edges[edge_index].m_solid)
          continue;
        
        Tri_Index opp_tri_index = m_mesh.opposite_tri(cur_tri_index, leg_index);
                
        if (INVALID_TRI_INDEX == opp_tri_index)
          continue;

        debugRangeCheck(opp_tri_index, m_mesh.num_tris());

        if (
          (m_tri_nodes[opp_tri_index].m_strip_ofs == -1) && 
          (m_tri_nodes[opp_tri_index].m_strip_index == strip_index) //m_strips.size())
          )
        {
          BASSERT(opp_tri_index != first_tri_index);
          BASSERT(m_tri_nodes[cur_tri_index].m_tunnel_id == tunnel_id);
          cur_tri_index = opp_tri_index;
          break;
        }
      }
      
//      BASSERT(leg_index < 3);
      BVERIFY(leg_index < 3);
    }
    
    BVERIFY(strip_ofs == num_tris);

    m_strips[strip_index] = Strip(first_tri_index, last_tri_index);
    m_num_strips++;
  }

  // Check to see if the strip contains a loop.
  bool verify_strip(Tri_Index tri_index, std::set<Tri_Index>& visited_tri_set)
  {
    Tri_Index first_tri_index = INVALID_TRI_INDEX;
    Tri_Index last_tri_index = INVALID_TRI_INDEX;

    std::stack<Tri_Index> tri_stack;
    tri_stack.push(tri_index);

    std::set<Tri_Index> tris;

    int num_tris = 0;
    bool found_loop = false;

    do
    {
      const Tri_Index cur_tri_index = tri_stack.top();
      tri_stack.pop();

      debugRangeCheck(cur_tri_index, m_mesh.num_tris());
  
      tris.insert(cur_tri_index);

      if (visited_tri_set.find(cur_tri_index) != visited_tri_set.end())
        found_loop = true;

      visited_tri_set.insert(cur_tri_index);
      
      num_tris++;

      int num_solid = 0;
      for (Leg_Index leg_index = 0; leg_index < 3; leg_index++)
      {
        const Edge_Index edge_index = m_mesh.tri_edges(cur_tri_index)[leg_index];
        if (INVALID_EDGE_INDEX == edge_index)
          continue;
        
        debugRangeCheck(edge_index, m_mesh.num_edges());

        if (m_strip_edges[edge_index].m_solid)
        {
          num_solid++;
          
          Tri_Index opp_tri_index = m_mesh.opposite_tri(cur_tri_index, leg_index);
          if (tris.find(opp_tri_index) == tris.end())
          {
            tri_stack.push(opp_tri_index);
          }
        }
      }

      BASSERT(num_solid <= 2);

      if (num_solid == 0)
      {
        BASSERT(num_tris == 1);
        last_tri_index = first_tri_index = cur_tri_index;       
      }
      else if (num_solid == 1)
      {
        if (INVALID_TRI_INDEX == first_tri_index)
          first_tri_index = cur_tri_index;
        else if (INVALID_TRI_INDEX == last_tri_index)
          last_tri_index = cur_tri_index;
        else
        {
          BVERIFY(false);
        }
      }
    } while (!tri_stack.empty());

    if ((INVALID_TRI_INDEX != first_tri_index) && (INVALID_TRI_INDEX != last_tri_index))
    {
      BASSERT(!found_loop);
      return false;
    }
    
    BASSERT(found_loop);
    
    trace("Strip_Tunneler:verify_strip: Loop found!");
    return true;
  }

  void flip_tunnel(Tri_Index start_tri_index, Tri_Index end_tri_index, int& next_tunnel_id)
  {
    int i;
    const int strip_tunnel_id = next_tunnel_id - 1;

    BDynamicArray<Edge_Index> tunnel_edges;
    BDynamicArray<Tri_Index> tunnel_tris;
        
    std::set<int> source_strip_indices;

    Tri_Index cur_tri_index = end_tri_index;
    
    bool solid = false;
    for ( ; ; )
    {
      debugRangeCheck(cur_tri_index, m_mesh.num_tris());
      
      source_strip_indices.insert(m_tri_nodes[cur_tri_index].m_strip_index);
      
      BASSERT(m_tri_nodes[cur_tri_index].m_tunnel_id == strip_tunnel_id);

      Edge_Index tunnel_parent_edge = m_tri_nodes[cur_tri_index].m_tunnel_parent_edge;

      tunnel_tris.push_back(cur_tri_index);
      tunnel_edges.push_back(tunnel_parent_edge);
            
      if (INVALID_TRI_INDEX == tunnel_parent_edge)
      {
        BASSERT(cur_tri_index == start_tri_index);
        BASSERT(cur_tri_index != end_tri_index);
        break;
      }

      BASSERT(m_strip_edges[tunnel_parent_edge].m_solid == solid);

      cur_tri_index = m_mesh.edge(tunnel_parent_edge).other_tri(cur_tri_index);

      solid = !solid;
    }

    BASSERT(tunnel_tris.size() >= 2);
            
    solid = false;
    for (i = 0; i < tunnel_edges.size() - 1; i++)
    {
      const Edge_Index tunnel_edge_index = tunnel_edges[i];
      debugRangeCheck(tunnel_edge_index, m_mesh.num_edges());

      BASSERT(solid == m_strip_edges[tunnel_edge_index].m_solid);
      
      m_strip_edges[tunnel_edge_index].m_solid = !m_strip_edges[tunnel_edge_index].m_solid;

      solid = !solid;
    }
    BASSERT(solid);

    std::set<int>::const_iterator strip_indices_it = source_strip_indices.begin(); //it != source_strip_indices.end(); ++it)
    
    for (i = 0; i < tunnel_tris.size(); i++)
    {
      const Tri_Index tri_index = tunnel_tris[i];
      debugRangeCheck(cur_tri_index, m_mesh.num_tris());

      if (m_tri_nodes[tri_index].m_tunnel_id != strip_tunnel_id)
        continue;

      BASSERT(strip_indices_it != source_strip_indices.end());
      const int strip_index = debugRangeCheck<int>(*strip_indices_it, m_strips.size());
      ++strip_indices_it;
      m_strips[strip_index].m_first_tri_index = INVALID_TRI_INDEX;
      m_strips[strip_index].m_last_tri_index = INVALID_TRI_INDEX;
      m_num_strips--;

      fix_strip(tri_index, next_tunnel_id, strip_index);
    }

    BASSERT(strip_indices_it != source_strip_indices.end());
    const int strip_index = debugRangeCheck<int>(*strip_indices_it, m_strips.size());
    ++strip_indices_it;
    m_strips[strip_index].m_first_tri_index = INVALID_TRI_INDEX;
    m_strips[strip_index].m_last_tri_index = INVALID_TRI_INDEX;
    
    // Total number of tristrips should have been decreased by one after the tunnel flip.
    m_num_strips--;
  }

  // Invert edges, then see if any loops are created.
  // This _shouldn't_ be necessary if I'm interpreting the paper correctly, but 
  // in some rare cases loops are still created. WHY?
  bool verify_tunnel_full(Tri_Index start_tri_index, Tri_Index end_tri_index, const int strip_tunnel_id)
  {
    int i;

    BDynamicArray<Edge_Index> tunnel_edges;
    BDynamicArray<Tri_Index> tunnel_tris;
        
    std::set<int> source_strip_indices;

    Tri_Index cur_tri_index = end_tri_index;
    
    // Find tunnel tris/edges.
    bool solid = false;
    for ( ; ; )
    {
      debugRangeCheck(cur_tri_index, m_mesh.num_tris());
      
      source_strip_indices.insert(m_tri_nodes[cur_tri_index].m_strip_index);
      
      BASSERT(m_tri_nodes[cur_tri_index].m_tunnel_id == strip_tunnel_id);

      Edge_Index tunnel_parent_edge = m_tri_nodes[cur_tri_index].m_tunnel_parent_edge;

      tunnel_tris.push_back(cur_tri_index);
      tunnel_edges.push_back(tunnel_parent_edge);
            
      if (INVALID_TRI_INDEX == tunnel_parent_edge)
      {
        BASSERT(cur_tri_index == start_tri_index);
        BASSERT(cur_tri_index != end_tri_index);
        break;
      }

      BASSERT(m_strip_edges[tunnel_parent_edge].m_solid == solid);

      cur_tri_index = m_mesh.edge(tunnel_parent_edge).other_tri(cur_tri_index);

      solid = !solid;
    }

    BASSERT(tunnel_tris.size() >= 2);
        
    // Invert the tunnel.
    solid = false;
    for (i = 0; i < tunnel_edges.size() - 1; i++)
    {
      const Edge_Index tunnel_edge_index = tunnel_edges[i];
      debugRangeCheck(tunnel_edge_index, m_mesh.num_edges());

      BASSERT(solid == m_strip_edges[tunnel_edge_index].m_solid);
      
      m_strip_edges[tunnel_edge_index].m_solid = !m_strip_edges[tunnel_edge_index].m_solid;

      solid = !solid;
    }
    BASSERT(solid);
    
    // Check for loops.
    std::set<Tri_Index> visited_tri_set;
    for (i = 0; i < tunnel_tris.size(); i++)
    {
      Tri_Index tri_index = tunnel_tris[i];
      debugRangeCheck(cur_tri_index, m_mesh.num_tris());

      BASSERT(m_tri_nodes[tri_index].m_tunnel_id == strip_tunnel_id);
      
      if (visited_tri_set.find(tri_index) != visited_tri_set.end())
        continue;
                    
      if (verify_strip(tri_index, visited_tri_set))
        break;
    }

    const bool bad_tunnel = i < tunnel_tris.size();

    // Invert the tunnel back.
    solid = false;
    for (i = 0; i < tunnel_edges.size() - 1; i++)
    {
      const Edge_Index tunnel_edge_index = tunnel_edges[i];
      debugRangeCheck(tunnel_edge_index, m_mesh.num_edges());

      BASSERT(solid == !m_strip_edges[tunnel_edge_index].m_solid);
      
      m_strip_edges[tunnel_edge_index].m_solid = !m_strip_edges[tunnel_edge_index].m_solid;

      solid = !solid;
    }
    BASSERT(solid);
    
    return bad_tunnel;
  }

  // Ensures the tunnel follows the tunnel special cases in BOTH directions.
  bool verify_tunnel_basic(Tri_Index first_tri_index, Tri_Index last_tri_index)
  {
    BASSERT(m_tri_nodes[first_tri_index].m_strip_index != m_tri_nodes[last_tri_index].m_strip_index);
    
    // Pass 0 is actually _backwards_ relative to the BFS.
    for (int pass = 0; pass < 2; pass++)
    {
      std::set<Tri_Index> tunnel_tri_set;
      std::set<Edge_Index> tunnel_edge_set;

      Tri_Index prev_tri_index = INVALID_TRI_INDEX;
      Tri_Index cur_tri_index = last_tri_index;
      Edge_Index prev_tunnel_parent_edge = INVALID_EDGE_INDEX;
      bool solid = false;
      int next_dir = 0, prev_strip_ofs = 0;
      
      for ( ; ; )
      {
        debugRangeCheck(cur_tri_index, m_mesh.num_tris());

        BASSERT(tunnel_tri_set.find(cur_tri_index) == tunnel_tri_set.end());
        tunnel_tri_set.insert(cur_tri_index);
    
        Edge_Index tunnel_parent_edge;
        if (pass == 0)
        {
          tunnel_parent_edge = m_tri_nodes[cur_tri_index].m_tunnel_parent_edge;
          m_tri_nodes[cur_tri_index].m_tunnel_next_edge = prev_tunnel_parent_edge;
        }
        else
        {
          tunnel_parent_edge = m_tri_nodes[cur_tri_index].m_tunnel_next_edge;
        }

        if (cur_tri_index == first_tri_index)
        {
          BASSERT(INVALID_EDGE_INDEX == tunnel_parent_edge);
          break;
        }
              
        debugRangeCheck(tunnel_parent_edge, m_mesh.num_edges());

        BASSERT(tunnel_edge_set.find(tunnel_parent_edge) == tunnel_edge_set.end());
        tunnel_edge_set.insert(tunnel_parent_edge);

        BASSERT(m_strip_edges[tunnel_parent_edge].m_solid == solid);

        const Tri_Index next_tri_index = m_mesh.edge(tunnel_parent_edge).other_tri(cur_tri_index);
        
        if (pass == 0)
        {
          BASSERT(m_tri_nodes[next_tri_index].m_distance + 1 == m_tri_nodes[cur_tri_index].m_distance);
        }
        else
        {
          BASSERT(m_tri_nodes[next_tri_index].m_distance - 1 == m_tri_nodes[cur_tri_index].m_distance);
        }
        
        const bool joining_same = m_tri_nodes[cur_tri_index].m_strip_index == m_tri_nodes[next_tri_index].m_strip_index;
        
        if (next_dir != 0)
        {
          BASSERT(joining_same);
          BASSERT(INVALID_TRI_INDEX != prev_tri_index);
          
          const int cur_strip_ofs = m_tri_nodes[cur_tri_index].m_strip_ofs;
          const int next_strip_ofs = m_tri_nodes[next_tri_index].m_strip_ofs;

          if (next_dir < 0)
          {
            BASSERT(next_strip_ofs < cur_strip_ofs);
          }
          else
          {
            BASSERT(next_strip_ofs > cur_strip_ofs);
          }

          const int cur_dist = labs(m_tri_nodes[cur_tri_index].m_strip_ofs - prev_strip_ofs);
          const int next_dist = labs(m_tri_nodes[next_tri_index].m_strip_ofs - prev_strip_ofs);
          BASSERT(next_dist < cur_dist);
        }

        next_dir = 0;
        prev_strip_ofs = 0;

        if (joining_same)
        {
          if (!solid)
          {
            if ((cur_tri_index == last_tri_index) || (next_tri_index == first_tri_index))
            {
              BASSERT(!joining_same);
            }

            const int cur_strip_ofs = m_tri_nodes[cur_tri_index].m_strip_ofs;
            const int next_strip_ofs = m_tri_nodes[next_tri_index].m_strip_ofs;
            if (cur_strip_ofs < next_strip_ofs)
              next_dir = -1;
            else
              next_dir = 1;
            
            prev_strip_ofs = m_tri_nodes[cur_tri_index].m_strip_ofs;
          }
        }
        
        prev_tri_index = cur_tri_index;     
        prev_tunnel_parent_edge = tunnel_parent_edge;

        cur_tri_index = next_tri_index;
        
        solid = !solid;
      }

      Tri_Index temp = first_tri_index;
      first_tri_index = last_tri_index;
      last_tri_index = temp;
    }

    return false;
  }
    
  // BFS starting at first_tri_index.
  // Returns INVALID_TRI_INDEX if no tunnel found, otherwise the tri index of the tunnel's end.
  Tri_Index find_tunnel(const Tri_Index first_tri_index, int& next_tunnel_id)
  {
    const int tunnel_id = next_tunnel_id;
    next_tunnel_id++;

    BASSERT((m_tri_nodes[first_tri_index].at_strip_start()) || (m_tri_nodes[first_tri_index].at_strip_end()));

    std::deque<Tri_Index> tris_to_visit;
    
    tris_to_visit.push_back(first_tri_index);

    m_tri_nodes[first_tri_index].m_tunnel_parent_edge = INVALID_EDGE_INDEX;
    m_tri_nodes[first_tri_index].m_solid = false;
    m_tri_nodes[first_tri_index].m_distance = 0;
    m_tri_nodes[first_tri_index].m_strip_dir = 0;
    m_tri_nodes[first_tri_index].m_tunnel_id = tunnel_id;
            
    do
    {
      const Tri_Index cur_tri_index = tris_to_visit.front();
      debugRangeCheck(cur_tri_index, m_mesh.num_tris());
      BASSERT(m_tri_nodes[cur_tri_index].m_tunnel_id == tunnel_id);
      
      tris_to_visit.pop_front();

      for (int leg_index = 0; leg_index < 3; leg_index++)
      {
        const Tri_Index dst_tri_index = m_mesh.opposite_tri(cur_tri_index, leg_index);
        if (INVALID_TRI_INDEX == dst_tri_index)
          continue;
        
        BASSERT(dst_tri_index != cur_tri_index);
        
        debugRangeCheck(dst_tri_index, m_mesh.num_tris());

        if (INVALID_STRIP_INDEX == m_tri_nodes[dst_tri_index].m_strip_index)
          continue;

        if (m_tri_nodes[dst_tri_index].m_tunnel_id == tunnel_id) 
        {
          // Is this assert OK?
          BASSERT(m_tri_nodes[dst_tri_index].m_distance <= (m_tri_nodes[cur_tri_index].m_distance + 1));
          continue;
        }
                
        std::pair<Edge_Index, Leg_Index> src_to_dst_edge = m_mesh.shared_edge(cur_tri_index, dst_tri_index);
        std::pair<Edge_Index, Leg_Index> dst_to_src_edge = m_mesh.shared_edge(dst_tri_index, cur_tri_index);
        BASSERT(src_to_dst_edge.first == dst_to_src_edge.first);
        BASSERT(INVALID_EDGE_INDEX != src_to_dst_edge.first);

        Edge_Index shared_edge_index = src_to_dst_edge.first;
        debugRangeCheck(shared_edge_index, m_mesh.num_edges());
        
        const bool solid = m_tri_nodes[cur_tri_index].m_solid;
        if (m_strip_edges[shared_edge_index].m_solid != solid)
          continue;

        const bool joining_same = m_tri_nodes[cur_tri_index].m_strip_index == m_tri_nodes[dst_tri_index].m_strip_index;
        BASSERT((!solid) || (joining_same));

        if (joining_same)
        {
          BASSERT(m_tri_nodes[cur_tri_index].m_strip_ofs >= 0);
          BASSERT(m_tri_nodes[dst_tri_index].m_strip_ofs >= 0);
          BASSERT(m_tri_nodes[cur_tri_index].m_strip_ofs != m_tri_nodes[dst_tri_index].m_strip_ofs);
        }

        if ((joining_same) && (cur_tri_index == first_tri_index))
          continue;

        // Special case enforcement.
        if (0 != m_tri_nodes[cur_tri_index].m_strip_dir)
        {
          const int next_strip_ofs = m_tri_nodes[dst_tri_index].m_strip_ofs;
          const int cur_strip_ofs = m_tri_nodes[cur_tri_index].m_strip_ofs;

          if (solid)
          {
            // prev was nonstrip edge
            BASSERT(joining_same);
                        
            if (m_tri_nodes[cur_tri_index].m_strip_dir < 0)
            {
              if (next_strip_ofs > cur_strip_ofs)
                continue;
            }
            else
            {
              if (next_strip_ofs < cur_strip_ofs)
                continue;
            }
          }
          else if (joining_same)
          {
            // prev was strip edge
            if (m_tri_nodes[cur_tri_index].m_strip_dir < 0)
            {
              if (next_strip_ofs > cur_strip_ofs)
                continue;
            }
            else
            {
              if (next_strip_ofs < cur_strip_ofs)
                continue;
            }
          }
        }
                        
        if ((!solid) && (!joining_same))
        {
          if (m_tri_nodes[dst_tri_index].at_strip_start() || m_tri_nodes[dst_tri_index].at_strip_end())
          {
            if (m_tri_nodes[first_tri_index].m_strip_index != m_tri_nodes[dst_tri_index].m_strip_index)
            {
              m_tri_nodes[dst_tri_index].m_tunnel_parent_edge = shared_edge_index;
              m_tri_nodes[dst_tri_index].m_solid = !solid;
              m_tri_nodes[dst_tri_index].m_distance = m_tri_nodes[cur_tri_index].m_distance + 1;
              m_tri_nodes[dst_tri_index].m_strip_dir = 0;
              m_tri_nodes[dst_tri_index].m_tunnel_id = tunnel_id;

              BASSERT(!verify_tunnel_basic(first_tri_index, dst_tri_index));

              if (!verify_tunnel_full(first_tri_index, dst_tri_index, tunnel_id))
              {
                return dst_tri_index;
              }
            }
          }
        }

        // Should be externally tunable.
        const int MAX_SEARCH_DIST = 75;
        if (m_tri_nodes[cur_tri_index].m_distance < MAX_SEARCH_DIST)
        {
          m_tri_nodes[dst_tri_index].m_tunnel_parent_edge = shared_edge_index;
          m_tri_nodes[dst_tri_index].m_solid = !solid;
          m_tri_nodes[dst_tri_index].m_distance = m_tri_nodes[cur_tri_index].m_distance + 1;
          m_tri_nodes[dst_tri_index].m_strip_dir = 0;
          m_tri_nodes[dst_tri_index].m_tunnel_id = tunnel_id;
          
          if (joining_same)
          {
            const int cur_strip_ofs = m_tri_nodes[cur_tri_index].m_strip_ofs;
            const int next_strip_ofs = m_tri_nodes[dst_tri_index].m_strip_ofs;
            if (cur_strip_ofs < next_strip_ofs)
              m_tri_nodes[dst_tri_index].m_strip_dir = -1;
            else
              m_tri_nodes[dst_tri_index].m_strip_dir = 1;
          }
          
          tris_to_visit.push_back(dst_tri_index);               
        }
      }
    } while (!tris_to_visit.empty());

    return INVALID_TRI_INDEX;
  }

  // Checks entire mesh database for: 
  // Strip loops, tris missing from a strip, improper m_num_strips, etc.
  void verify_database(int& next_tunnel_id)
  {
    const int tunnel_id = next_tunnel_id;
    next_tunnel_id++;

    BDynamicArray<bool> seen_edge(m_mesh.num_edges());

    int num_strips = 0;
    int num_tris = 0;
    int i;
    for (i = 0; i < m_strips.size(); i++)
    {
      if (INVALID_TRI_INDEX == m_strips[i].m_first_tri_index)
        continue;
      num_strips++;

      debugRangeCheck(m_strips[i].m_first_tri_index, m_mesh.num_tris());
      debugRangeCheck(m_strips[i].m_last_tri_index, m_mesh.num_tris());

      Tri_Index cur_tri_index = m_strips[i].m_first_tri_index;
      Tri_Index prev_tri_index = INVALID_TRI_INDEX;
      int strip_ofs = 0;

      do
      {
        debugRangeCheck(cur_tri_index, m_mesh.num_tris());
        BVERIFY(m_tri_nodes[cur_tri_index].m_strip_index == i);
        BVERIFY(m_tri_nodes[cur_tri_index].m_strip_ofs == strip_ofs);
        BVERIFY(m_tri_nodes[cur_tri_index].m_tunnel_id != tunnel_id);
        BVERIFY(m_tri_nodes[cur_tri_index].m_prev_tri_index == prev_tri_index);

        num_tris++;
        m_tri_nodes[cur_tri_index].m_tunnel_id = tunnel_id;

        Edge_Index parent_edge_index = INVALID_EDGE_INDEX;

        if (INVALID_TRI_INDEX != prev_tri_index)
        {
          std::pair<Edge_Index, Leg_Index> shared_edge_f = m_mesh.shared_edge(prev_tri_index, cur_tri_index); 
          std::pair<Edge_Index, Leg_Index> shared_edge_b = m_mesh.shared_edge(cur_tri_index, prev_tri_index); 
          BVERIFY(shared_edge_f.first == shared_edge_b.first);
          debugRangeCheck(shared_edge_f.first, m_mesh.num_edges());
          debugRangeCheck(shared_edge_b.first, m_mesh.num_edges());

          parent_edge_index = shared_edge_f.first;

          BVERIFY(m_strip_edges[parent_edge_index].m_solid);
          
          BVERIFY(!seen_edge[parent_edge_index]);
          seen_edge[parent_edge_index] = true;
        }

        Tri_Index next_tri_index = INVALID_TRI_INDEX;

        int num_solid = 0;
        bool found_parent_edge = false;
        for (Leg_Index leg_index = 0; leg_index < 3; leg_index++)
        {
          Edge_Index edge_index = m_mesh.tri_edges(cur_tri_index)[leg_index];
          int edge_side = m_mesh.tri_edges(cur_tri_index).edge_side(leg_index);
          if (INVALID_EDGE_INDEX == edge_index)
            continue;
          
          debugRangeCheck(edge_index, m_mesh.num_edges());

          BVERIFY(m_mesh.edge(edge_index).num_tris(edge_side) == 1);
          BVERIFY(m_mesh.edge(edge_index).tri_index(edge_side, 0) == cur_tri_index);

          int other_edge_side = 1 - edge_side;
                              
          if (edge_index == parent_edge_index)
          {
            BVERIFY(m_mesh.edge(edge_index).num_tris(other_edge_side) == 1);
            BVERIFY(m_mesh.edge(edge_index).tri_index(other_edge_side, 0) == prev_tri_index);

            BVERIFY(!found_parent_edge);
            found_parent_edge = true;
            BVERIFY(m_strip_edges[edge_index].m_solid);
          }

          if (m_strip_edges[edge_index].m_solid)
          {
            num_solid++;
            
            BVERIFY(num_solid <= 2);

            BVERIFY(m_mesh.edge(edge_index).num_tris(other_edge_side) == 1);
            BVERIFY(m_mesh.edge(edge_index).tri_index(other_edge_side, 0) != cur_tri_index);

            Tri_Index opp_tri_index = m_mesh.edge(edge_index).tri_index(other_edge_side, 0);
            debugRangeCheck(opp_tri_index, m_mesh.num_tris());
            
            if (INVALID_TRI_INDEX != next_tri_index)
            {
              BVERIFY(INVALID_TRI_INDEX != prev_tri_index);
              BVERIFY(opp_tri_index == prev_tri_index);
              BVERIFY(opp_tri_index != next_tri_index);
            }
            else if (opp_tri_index == prev_tri_index)
            {
              BVERIFY(m_tri_nodes[cur_tri_index].m_prev_tri_index == opp_tri_index);
            }
            else
            {
              BVERIFY(m_tri_nodes[cur_tri_index].m_next_tri_index == opp_tri_index);
              next_tri_index = opp_tri_index;
            }
          }
        }

        if (m_strips[i].m_first_tri_index == m_strips[i].m_last_tri_index)
        {
          BVERIFY(num_solid == 0);
        }
        else if (cur_tri_index == m_strips[i].m_first_tri_index)
        {
          BVERIFY(num_solid == 1);
        }
        else if (cur_tri_index == m_strips[i].m_last_tri_index)
        {
          BVERIFY(num_solid == 1);
        }
        else
        {
          BVERIFY(num_solid == 2);
        }

        BVERIFY(found_parent_edge || (parent_edge_index == INVALID_EDGE_INDEX));
        BVERIFY(m_tri_nodes[cur_tri_index].m_next_tri_index == next_tri_index);

        prev_tri_index = cur_tri_index;
        cur_tri_index = m_tri_nodes[cur_tri_index].m_next_tri_index;
        
        strip_ofs++;
      } while (INVALID_TRI_INDEX != cur_tri_index);
    }

    BVERIFY(m_num_strips == num_strips);
    BVERIFY(m_num_valid_tris == num_tris);

    for (i = 0; i < m_mesh.num_edges(); i++)
    {
      if (!seen_edge[i])
      {
        BVERIFY(!m_strip_edges[i].m_solid);
      }
      else
      {
        BVERIFY(m_strip_edges[i].m_solid);
      }
    }

    trace("Strip_Tunneler::verify_database: OK Strips: %i", m_num_strips);
  }

  void reduce_strips(void)
  {
    int next_tunnel_id = 1;

    verify_database(next_tunnel_id);

    int num_failed_attempts = 0;

    uint cur_rand_seed = 0xDEADBEEF + m_num_strips; 

    while ((m_num_strips > 1) && (num_failed_attempts < m_num_strips * 2))
    {
      BASSERT(next_tunnel_id < 0x7FFFFF00);

      int strip_index;
      
      // Find a valid tunnel start strip.
      do 
      {
        strip_index = Math::LCGNextRand(cur_rand_seed) % m_strips.size();
      } while (INVALID_TRI_INDEX == m_strips[strip_index].m_first_tri_index);

      const int side = (Math::LCGNextRand(cur_rand_seed) & 0x80000000) != 0;

      Tri_Index first_tri_index = side ? m_strips[strip_index].m_last_tri_index : m_strips[strip_index].m_first_tri_index;
      debugRangeCheck(first_tri_index, m_mesh.num_tris());
      BASSERT(m_tri_nodes[first_tri_index].m_strip_index == strip_index);
      BASSERT(m_tri_nodes[first_tri_index].at_strip_start() || m_tri_nodes[first_tri_index].at_strip_end());

      Tri_Index last_tri_index = find_tunnel(first_tri_index, next_tunnel_id);

      if (INVALID_TRI_INDEX != last_tri_index)
      {
        num_failed_attempts = 0;

        const int prev_num_strips = m_num_strips;
              
        flip_tunnel(first_tri_index, last_tri_index, next_tunnel_id);

        BVERIFY(prev_num_strips == m_num_strips + 1);

#ifdef STRIP_TUNNELER_VERIFY_DATABASE
        verify_database(next_tunnel_id);
#endif
//return;
      }
      else
        num_failed_attempts++;
    }

    verify_database(next_tunnel_id);
  }

  // This class could be moved into Tri_Strips.
  void generate_strip(Tri_Strips& strips, const BDynamicArray<Tri_Index>& tri_indices)
  {
    const int strip_start_ofs = strips.size();

    if (tri_indices.size() == 1)
    {
      const Tri_Index first_tri_index = tri_indices.front();
      const Indexed_Tri first_tri = m_mesh.tri(first_tri_index);
      strips.push_back(first_tri);
      return;
    }
    
    for (int l = 1; l < tri_indices.size(); l++)
    {
      const Tri_Index prev_tri_index = tri_indices[l - 1];
      const Indexed_Tri prev_tri = m_mesh.tri(prev_tri_index);

      const Tri_Index cur_tri_index = tri_indices[l];
      const Indexed_Tri cur_tri = m_mesh.tri(cur_tri_index);

#if STRIP_TUNNELER_DEBUG
      trace("%03i %04i %04i %04i : %04i %04i %04i", 
        l,
        prev_tri[0], prev_tri[1], prev_tri[2], 
        cur_tri[0], cur_tri[1], cur_tri[2]);
#endif

      std::pair<Edge_Index, Leg_Index> pc = m_mesh.shared_edge(prev_tri_index, cur_tri_index);
      std::pair<Edge_Index, Leg_Index> cp = m_mesh.shared_edge(cur_tri_index, prev_tri_index);
      BASSERT(INVALID_EDGE_INDEX != pc.first);
      BASSERT(pc.first == cp.first);

      const Edge_Index shared_edge_index = pc.first;
      const Leg_Index prev_leg_index = pc.second;
      const Leg_Index cur_leg_index = cp.second;

      if (l == 1)
      {
        const Vert_Index a = prev_tri.vert_index_wrap(prev_leg_index + 2);
        const Vert_Index b = prev_tri.vert_index_wrap(prev_leg_index + 0);
        const Vert_Index c = prev_tri.vert_index_wrap(prev_leg_index + 1);

        BASSERT(INVALID_VERT_INDEX == cur_tri.find(a));

        strips.push_back(a, Tri_Strips::TRISTRIP_FLAGS_START);
        strips.push_back(b);
        strips.push_back(c);
      }
    
      const Vert_Index a = strips[strips.size() - 2];
      const Vert_Index b = strips[strips.size() - 1];

      int i;
      for (i = 0; i < 3; i++)
        if (INVALID_VERT_INDEX == prev_tri.find(cur_tri[i]))
          break;
      BASSERT(i != 3);
        
      const Vert_Index c = cur_tri[i];
      BASSERT(c == cur_tri.vert_index_wrap(cur_leg_index + 2));
      
      Indexed_Tri trial_tri(a, b, c);
      if (((strips.size() - strip_start_ofs) & 1) == 1)
        trial_tri = Indexed_Tri(b, a, c);
        
      if (trial_tri.canonical() != cur_tri.canonical())         
      {
        const Vert_Index p0 = strips[strips.size() - 1];
        const Vert_Index p1 = strips[strips.size() - 2];
        const Vert_Index s  = strips[strips.size() - 3];
                    
        Indexed_Tri trial_tri2(s, p0, c);
        if (((strips.size() - strip_start_ofs) & 1) == 0)
          trial_tri2 = Indexed_Tri(p0, s, c);

        BASSERT(trial_tri2.canonical() == cur_tri.canonical());
        
        strips[strips.size() - 1] = s;

        strips.push_back(p0);
      }

      strips.push_back(c);

      {
        Vert_Index x = strips[strips.size() - 3];
        Vert_Index y = strips[strips.size() - 2];
        Vert_Index z = strips[strips.size() - 1];
    
        Indexed_Tri trial_tri(x, y, z);
        if (((strips.size() - strip_start_ofs) & 1) == 0)
          trial_tri = Indexed_Tri(y, x, z);

        BASSERT(trial_tri.canonical() == cur_tri.canonical());
      }
    }
  }
  
  void append_skipped_strips(void)
  {
    for (int i = 0; i < m_bad_strip_offsets.size(); i++)
    {
      const int bad_strip_ofs = m_bad_strip_offsets[i];
      BASSERT(m_source_strips.start_of_strip(bad_strip_ofs));

      const int bad_strip_len = m_source_strips.strip_size(bad_strip_ofs);

      m_output_strips.append(m_source_strips, bad_strip_ofs, bad_strip_len);
    }

    trace("Strip_Tunneler::append_skipped_strips: Appended %i strips to output", m_bad_strip_offsets.size());
  }

  void generate_output_strips(void)
  {
    m_output_strips.clear();
    
    for (int strip_index = 0; strip_index < m_strips.size(); strip_index++)
    {
      const Tri_Index first_tri_index = m_strips[strip_index].m_first_tri_index;
      const Tri_Index last_tri_index = m_strips[strip_index].m_last_tri_index;
      if (INVALID_TRI_INDEX == first_tri_index)
        continue;
  
      BDynamicArray<Tri_Index> tri_indices;
      get_strip_tri_indices(tri_indices, first_tri_index);
            
      generate_strip(m_output_strips, tri_indices);
    }

    std::pair<int, int> validate_result;

    validate_result = m_output_strips.validate(m_mesh);
    BVERIFY(m_num_valid_tris == validate_result.first);
    BVERIFY(!validate_result.second);

    validate_result = m_output_strips.validate(m_src_mesh);
    BVERIFY(m_num_valid_tris == validate_result.first);
    BVERIFY(!validate_result.second);
  }
      
public:
  Strip_Tunneler(
    const Indexed_Mesh& src_mesh, 
    const Tri_Strips& source_strips) :
      m_src_mesh(src_mesh),
      m_source_strips(source_strips),
      m_num_strips(0),
      m_num_valid_tris(0)
  {
    init_strips();

    reduce_strips();
  
    generate_output_strips();

    append_skipped_strips();
  }

  const Indexed_Mesh& indexed_mesh(void) const
  {
    return m_src_mesh;
  }

  const Tri_Strips& tristrips(void) const
  {
    return m_output_strips;
  }
};
