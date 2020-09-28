//-------------------------------------------------------------------------------------------------
// tri_strips.h
// Rich Geldreich 
// Copyright (C) 2002 Blue Shift, Inc.
//-------------------------------------------------------------------------------------------------
#pragma once
//-------------------------------------------------------------------------------------------------
#include "indexedMesh.h"
//-------------------------------------------------------------------------------------------------
struct Tri_Strips
{
  enum 
  {
    TRISTRIP_FLAGS_DEFAULT  = 0x00, 
    TRISTRIP_FLAGS_START    = 0x80, 
    TRISTRIP_FLAGS_FLIP     = 0x01 
  };

  Vert_Index_Vec m_vert_indices;
  BDynamicArray<uchar> m_flags;

  Tri_Strips() 
  {
  }

  Tri_Strips(const Vert_Index_Vec& vert_indices, const BDynamicArray<uchar>& flags)
  {
    m_vert_indices = vert_indices;
    m_flags = flags;
  }

  Tri_Strips& operator= (const Tri_Strips& other)
  {
    m_vert_indices = other.m_vert_indices;
    m_flags = other.m_flags;    
    return *this;
  }

  Tri_Strips(const Tri_Strips& other)
  {
    m_vert_indices = other.m_vert_indices;
    m_flags = other.m_flags;
  }

  bool empty(void) const 
  {
    BASSERT(m_flags.empty() == m_vert_indices.empty());
    return m_vert_indices.empty();
  }

  int size(void) const
  {
    BASSERT(m_flags.size() == m_vert_indices.size());
    return m_vert_indices.size(); 
  }

  const Vert_Index& operator[](const int ofs) const { return m_vert_indices[debugRangeCheck(ofs, size())]; }
        Vert_Index& operator[](const int ofs)       { return m_vert_indices[debugRangeCheck(ofs, size())]; }

  const Vert_Index& vert_index(const int ofs) const { return m_vert_indices[debugRangeCheck(ofs, size())]; }
        Vert_Index& vert_index(const int ofs)       { return m_vert_indices[debugRangeCheck(ofs, size())]; }

  const uchar& flags(const int ofs) const { return m_flags[debugRangeCheck(ofs, size())]; }
        uchar& flags(const int ofs)       { return m_flags[debugRangeCheck(ofs, size())]; }
  
  bool start_of_strip(const int ofs) const
  {
    return (flags(ofs) & TRISTRIP_FLAGS_START) != 0;
  }

  bool flip_winding(const int ofs) const
  {
    return (flags(ofs) & TRISTRIP_FLAGS_FLIP) != 0;
  }

  void push_back(const Vert_Index vert_index, const uchar flags = TRISTRIP_FLAGS_DEFAULT)
  {
    BASSERT(m_flags.size() == m_vert_indices.size());
    m_vert_indices.push_back(vert_index);
    m_flags.push_back(flags);
  } 

  void push_back(const Indexed_Tri& tri, bool flip_winding = false)
  {
    push_back(tri[0], (uchar)(TRISTRIP_FLAGS_DEFAULT | TRISTRIP_FLAGS_START | (flip_winding ? TRISTRIP_FLAGS_FLIP : 0)));
    push_back(tri[1]);
    push_back(tri[2]);
  }

  void clear(void)
  {
    //m_vert_indices.erase(m_vert_indices.begin(), m_vert_indices.end());
    //m_flags.erase(m_flags.begin(), m_flags.end());
    m_vert_indices.resize(0);
    m_flags.resize(0);
  }

  void append(const Tri_Strips& other, const int start_ofs, const int len)
  {
    if (!len)
      return;

    debugRangeCheckIncl(len, other.size());
    debugRangeCheck(start_ofs, other.size());
    debugRangeCheck(start_ofs + len - 1, other.size());

    //m_vert_indices.insert(m_vert_indices.end(), other.m_vert_indices.begin() + start_ofs, other.m_vert_indices.begin() + start_ofs + len);
    //m_flags.insert(m_flags.end(), other.m_flags.begin() + start_ofs, other.m_flags.begin() + start_ofs + len);
    
    m_vert_indices.pushBack(other.m_vert_indices.begin() + start_ofs, len);
    m_flags.pushBack(other.m_flags.begin() + start_ofs, len);
  }

  int strip_size(int start_ofs) const
  {
    debugRangeCheck(start_ofs, size());
    BASSERT(start_of_strip(start_ofs));

    int len = 0;

    do
    {
      len++;
      start_ofs++;
    } while ((start_ofs < size()) && (!start_of_strip(start_ofs)));

    return len;
  }

  int num_strips(void) const
  {
    int num = 0;
    for (int i = 0; i < size(); i++)
      if (start_of_strip(i))
        num++;
    return num;
  }

  int get_tris(BDynamicArray<Indexed_Tri>& tris, int cur_ofs, const bool cull_degens = true) const
  {
    BASSERT(m_flags.size() == m_vert_indices.size());

    //tris.erase(tris.begin(), tris.end());
    tris.resize(0);

    if (cur_ofs >= m_vert_indices.size())
      return cur_ofs;

    if ((m_vert_indices.size() - cur_ofs) < 3)
      return cur_ofs;
        
    bool winding = false;

    for ( ; ; )
    {
      if (cur_ofs >= (m_vert_indices.size() - 2))
      {
        cur_ofs = m_vert_indices.size();
        break;
      }

      const uchar flags0 = m_flags[cur_ofs+0];
      const uchar flags1 = m_flags[cur_ofs+1];
      const uchar flags2 = m_flags[cur_ofs+2];

      if (tris.empty())
      {
        BASSERT(flags0 & TRISTRIP_FLAGS_START);
        winding = (flags0 & TRISTRIP_FLAGS_FLIP) != 0;
        
        BASSERT((flags2 & TRISTRIP_FLAGS_START) == 0);
      }
      else if (flags2 & TRISTRIP_FLAGS_START)
      {
        cur_ofs += 2;
        break;
      }
      
      BASSERT((flags1 & TRISTRIP_FLAGS_START) == 0);
    
      Indexed_Tri tri(m_vert_indices[cur_ofs + 0], m_vert_indices[cur_ofs + 1], m_vert_indices[cur_ofs + 2]);
      if (winding)
        tri.flip();

      if ((!cull_degens) || (!tri.degenerate()))
        tris.push_back(tri);
      
      cur_ofs++;
      
      winding = !winding;
    }

    return cur_ofs;
  }

  int get_tris(Tri_Strips& tris, int cur_ofs, const bool cull_degens = true) const
  {
    BASSERT(m_flags.size() == m_vert_indices.size());

    tris.clear();

    if (cur_ofs >= m_vert_indices.size())
      return cur_ofs;

    if ((m_vert_indices.size() - cur_ofs) < 3)
      return cur_ofs;
        
    bool winding = false;

    for ( ; ; )
    {
      if (cur_ofs >= (m_vert_indices.size() - 2))
      {
        cur_ofs = m_vert_indices.size();
        break;
      }

      const uchar flags0 = m_flags[cur_ofs+0];
      const uchar flags1 = m_flags[cur_ofs+1];
      const uchar flags2 = m_flags[cur_ofs+2];

      if (tris.empty())
      {
        BASSERT(flags0 & TRISTRIP_FLAGS_START);
        winding = (flags0 & TRISTRIP_FLAGS_FLIP) != 0;
        
        BASSERT((flags2 & TRISTRIP_FLAGS_START) == 0);
      }
      else if (flags2 & TRISTRIP_FLAGS_START)
      {
        cur_ofs += 2;
        break;
      }
      
      BASSERT((flags1 & TRISTRIP_FLAGS_START) == 0);
    
      Indexed_Tri tri(m_vert_indices[cur_ofs + 0], m_vert_indices[cur_ofs + 1], m_vert_indices[cur_ofs + 2]);
      if (winding)
        tri.flip();

      if ((!cull_degens) || (!tri.degenerate()))
        tris.push_back(tri);
      
      cur_ofs++;
      
      winding = !winding;
    }

    return cur_ofs;
  }
    
  // Returns num_tris, num_missing.
  std::pair<int, int> validate(const Indexed_Mesh& mesh) const
  {
    int num_strips = 0;
    int num_tris = 0;
    int num_degens = 0;
    int num_missing = 0;

    int strip_start_ofs = -1;
    
    bool winding = false;

    for (int i = 0; i < size() - 2; i++)
    {
      const uchar flags0 = flags(i + 0);
      const uchar flags1 = flags(i + 1);
      const uchar flags2 = flags(i + 2);

      if ((flags1 & Tri_Strips::TRISTRIP_FLAGS_START) || (flags2 & Tri_Strips::TRISTRIP_FLAGS_START))
        continue;

      if (!i)
      {
        BASSERT(flags0 & Tri_Strips::TRISTRIP_FLAGS_START);
      }
      
      if (flags0 & Tri_Strips::TRISTRIP_FLAGS_START)
      {
        num_strips++;

        winding = (flags0 & Tri_Strips::TRISTRIP_FLAGS_FLIP) != 0;
              
        strip_start_ofs = i;
      }

      Indexed_Tri tri(vert_index(i + 0), vert_index(i + 1), vert_index(i + 2));
      if (winding)
        tri.flip();
      
      // Might be a swap.
      if (tri.degenerate())
      {
        num_degens++;
      }
      else
      {
        const Tri_Index tri_index = mesh.find(tri, true);
        
        if (INVALID_TRI_INDEX != tri_index)
          num_tris++;
        else  
          num_missing++;
      }

      winding = !winding;
    }

    trace("Tri_Strips::validate: Len: %i, Strips: %i, Found Tris: %i, Missing Tris: %i, Degen. Tris: %i\n",
      size(),
      num_strips,
      num_tris,
      num_missing,
      num_degens);

    return std::make_pair(num_tris, num_missing);
  }
  
  void convertToSingleSequential(Tri_Strips& dst) const
  {
    dst.clear();
    
    for (uint i = 0; i < size(); i++)
    {
      const bool start_flag = start_of_strip(i);
      const bool flip_flag = flip_winding(i);
      const uint index = vert_index(i);
      
      const uint cur_len = dst.size();
      
      if (start_flag)
      {
         if (!cur_len)
         {
            if (flip_flag)
               dst.push_back(index);
         }
         else
         {
            const uint prev_index = dst.vert_index(dst.size() - 1);
            
            dst.push_back(prev_index);
            
            // There's an optimization that I think could be done here that seems only 
            // rarely useful in practice (when prev_vert_index == vert_index).
            if (!flip_flag)
            {
               if (cur_len & 1)
                  dst.push_back(prev_index);
            }
            else
            {
               if ((cur_len & 1) == 0)
                  dst.push_back(prev_index);
            } 

            dst.push_back(index);
         }
      }
      
      if (i < (size() - 2))
      {
         if (  (!start_of_strip(i + 1)) &&
               (!start_of_strip(i + 2))
            )
         {
            BVERIFY(((dst.size() & 1) != 0) == flip_flag);
         }
      }
            
      dst.push_back(index);
    }
    
    if (dst.size())
      dst.flags(0) |= TRISTRIP_FLAGS_START;
  }
  
};
