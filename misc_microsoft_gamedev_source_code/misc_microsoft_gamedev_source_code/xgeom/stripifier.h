//-------------------------------------------------------------------------------------------------
// stripifier.h
// Rich Geldreich 
// Copyright (C) 2002 Blue Shift, Inc.
//-------------------------------------------------------------------------------------------------
#pragma once
//-------------------------------------------------------------------------------------------------
#include "indexedMesh.h"
#include "stripTunneler.h"
#include "xbstrip.h"
//#include "3rdparty/actc/tc.h"
//-------------------------------------------------------------------------------------------------
class Stripifier
{
private:
  const Indexed_Mesh& m_indexed_mesh;

  Tri_Strips m_tristrips;

  //Vert_Index_Vec m_tristrip;
  //UCharVec m_tristrip_flags;

#if 0
  static void ACTCCheckError(int a)
  {
    if (a < 0)
    {
      char buf[256];
      sprintf_s(buf, sizeof(buf), "Stripifier::create_strips: ACTC Failed! Status %i\n", a);
      BFATAL_FAIL(buf);
    }
  }
#endif  

  void create_strips_ms(void)
  {
    BDynamicArray<WORD> indices;

    for (int i = 0; i < m_indexed_mesh.num_tris(); i++)
    {
      if (!m_indexed_mesh.tri(i).degenerate())
      {
        indices.push_back(m_indexed_mesh.tri(i)[0]);
        indices.push_back(m_indexed_mesh.tri(i)[1]);
        indices.push_back(m_indexed_mesh.tri(i)[2]);
      }
    }

    if (indices.empty())
      return;

    DWORD strip_size = 0;
    WORD* Pstrip_indices = NULL;
    
    Stripify(indices.size() / 3,
             &indices[0],       // Ptr to triangle indices
             &strip_size,    // Number of output indices
             &Pstrip_indices,   // Output indices
             0);    // Flags controlling optimizer.
    
    BVERIFY(strip_size > 0);

    for (int i = 0; i < strip_size; i++)
      m_tristrips.push_back(Pstrip_indices[i], (i == 0) ? Tri_Strips::TRISTRIP_FLAGS_START : Tri_Strips::TRISTRIP_FLAGS_DEFAULT);

    delete [] Pstrip_indices;

    trace("Stripifier::create_strips_ms: Tris: %i, List indices: %i, Strips: %i, Strip indices: %i, Strip indices/strip: %f, Strip indices/tri: %f", 
      m_indexed_mesh.num_tris(), 
      m_indexed_mesh.num_tris() * 3, 
      1,
      m_tristrips.size(), 
      m_tristrips.size() / float(1),
      m_tristrips.size() / float(m_indexed_mesh.num_tris()));
  }

#if 0
  void create_strips_actc(void)
  {
    ACTC::ACTCData* Ptc = ACTC::actcNew();

    if (!Ptc)
      BFATAL_FAIL("Stripifier: actcNew() failed!\n");

    ACTCCheckError(ACTC::actcParami(Ptc, ACTC_OUT_MIN_FAN_VERTS, INT_MAX));
    //ACTCCheckError(ACTC::actcParami(Ptc, ACTC_OUT_MAX_PRIM_VERTS, 3));//INT_MAX));
    //ACTCCheckError(ACTC::actcParami(Ptc, ACTC_OUT_HONOR_WINDING, ACTC_FALSE));
    ACTCCheckError(ACTC::actcParami(Ptc, ACTC_OUT_HONOR_WINDING, ACTC_TRUE));
  
    ACTCCheckError(ACTC::actcParami(Ptc, ACTC_IN_MIN_VERT, m_indexed_mesh.lowest_index()));
    ACTCCheckError(ACTC::actcParami(Ptc, ACTC_IN_MAX_VERT, m_indexed_mesh.highest_index()));

    ACTCCheckError(ACTC::actcBeginInput(Ptc));
    
    const int lowest_index = m_indexed_mesh.lowest_index();
    const int highest_index = m_indexed_mesh.highest_index();

    for (int i = 0; i < m_indexed_mesh.num_tris(); i++)
    {
      // This is EVIL. It could cause problems down the line.
      // ACTC tries to do BAD things to the heap when passed degenerate tris.
      if (!m_indexed_mesh.tri(i).degenerate())
      {
        ACTCCheckError(ACTC::actcAddTriangle(Ptc, 
          debugRangeCheckIncl(m_indexed_mesh.tri(i)[0], lowest_index, highest_index), 
          debugRangeCheckIncl(m_indexed_mesh.tri(i)[1], lowest_index, highest_index), 
          debugRangeCheckIncl(m_indexed_mesh.tri(i)[2], lowest_index, highest_index)));
      }
    }

    ACTCCheckError(ACTC::actcEndInput(Ptc));

    ACTCCheckError(ACTC::actcBeginOutput(Ptc));
      
    int total_strips = 0;

    int prim;
    uint v1, v2, v3;
    while ((prim = ACTC::actcStartNextPrim(Ptc, &v1, &v2)) != ACTC_DATABASE_EMPTY) 
    {
      ACTCCheckError(prim);
    
      if (prim != ACTC_PRIM_STRIP) 
        BFATAL_FAIL("Stripifier: actcStartNextPrim did not return a strip!\n");
          
      m_tristrips.push_back(v1, Tri_Strips::TRISTRIP_FLAGS_START);
      m_tristrips.push_back(v2, Tri_Strips::TRISTRIP_FLAGS_DEFAULT);

      while (ACTC::actcGetNextVert(Ptc, &v3) != ACTC_PRIM_COMPLETE) 
      {
        ACTCCheckError(v3);
        m_tristrips.push_back(v3, Tri_Strips::TRISTRIP_FLAGS_DEFAULT);
      }

      total_strips++;
    }

    ACTCCheckError(ACTC::actcEndOutput(Ptc));

    ACTC::actcDelete(Ptc);

    trace("Stripifier::create_strips_actc: Tris: %i, List indices: %i, Strips: %i, Strip indices: %i, Strip indices/strip: %f, Strip indices/tri: %f", 
      m_indexed_mesh.num_tris(), 
      m_indexed_mesh.num_tris() * 3, 
      total_strips,
      m_tristrips.size(), 
      m_tristrips.size() / float(total_strips),
      m_tristrips.size() / float(m_indexed_mesh.num_tris()));
  }
#endif

  void create_winding_bits(void)
  {
    UCharVec tri_seen(m_indexed_mesh.num_tris());
    
    bool cur_flipped = false;

    for (int i = 0; i < m_tristrips.size() - 2; i++)
    {
      const uchar flags0 = m_tristrips.flags(i + 0);
      const uchar flags1 = m_tristrips.flags(i + 1);
      const uchar flags2 = m_tristrips.flags(i + 2);

      if ((flags1 & Tri_Strips::TRISTRIP_FLAGS_START) || (flags2 & Tri_Strips::TRISTRIP_FLAGS_START))
        continue;
      
      if (flags0 & Tri_Strips::TRISTRIP_FLAGS_START)
        cur_flipped = false;

      Indexed_Tri tri(m_tristrips[i + 0], m_tristrips[i + 1], m_tristrips[i + 2]);
      
      if (tri.degenerate())
      {
        if (cur_flipped)
          m_tristrips.flags(i) |= Tri_Strips::TRISTRIP_FLAGS_FLIP;

        cur_flipped = !cur_flipped;

        continue;
      }

      // Attempt to find the triangle, its permutations, or its inverse in the indexed mesh database.
      Tri_Index tri_index = m_indexed_mesh.find(tri, true);
      Tri_Index flipped_tri_index = m_indexed_mesh.find(tri.flipped(), true);
      
      if ((INVALID_TRI_INDEX == tri_index) &&  (INVALID_TRI_INDEX == flipped_tri_index))
        BFATAL_FAIL("Stripifier::create_winding_bits: Tristrip contains an invalid triangle! (THIS IS VERY BAD.)");

      bool flipped = false;

      if ((INVALID_TRI_INDEX != tri_index) && (INVALID_TRI_INDEX != flipped_tri_index))
      {
        debugRangeCheck(tri_index, m_indexed_mesh.num_tris());
        debugRangeCheck(flipped_tri_index, m_indexed_mesh.num_tris());

        if (cur_flipped)
        {
          if (tri_seen[flipped_tri_index])
          {
            trace("create_winding_bits: duplicate tri detected in tristrip");
          }

          tri_seen[flipped_tri_index] = 1;
          flipped = true;
        }
        else
        {
          if (tri_seen[tri_index])
          { 
            trace("create_winding_bits: duplicate tri detected in tristrip");
          }

          tri_seen[tri_index] = 1;
        }
      }
      else if (INVALID_TRI_INDEX != tri_index)
      {
        debugRangeCheck(tri_index, m_indexed_mesh.num_tris());

        if (tri_seen[tri_index])
        {
          trace("create_winding_bits: duplicate tri detected in tristrip");
        }

        tri_seen[tri_index] = 1;
      }
      else if (INVALID_TRI_INDEX != flipped_tri_index)
      {
        debugRangeCheck(flipped_tri_index, m_indexed_mesh.num_tris());

        if (tri_seen[flipped_tri_index])
        {
          trace("create_winding_bits: duplicate tri detected in tristrip");
        }

        tri_seen[flipped_tri_index] = 1;
        flipped = true;
      }
      else
      {
        BFATAL_FAIL("Unable to find strip tri in database. This is VERY BAD.");
      }
            
      BVERIFY(flipped == cur_flipped);

      if (flipped)
        m_tristrips.flags(i) |= Tri_Strips::TRISTRIP_FLAGS_FLIP;
      
      cur_flipped = !cur_flipped;
    }
  }

public:
  enum EStripifierMode
  {
    eTunnelFastStripifier,     
    eTunnelSlowStripifier,     
    eIndividualTrisAsStrips,
    eMSStripifier,             
//    eACTCStripifier,           
  };
  
  Stripifier(const Indexed_Mesh& mesh, EStripifierMode mode = eTunnelFastStripifier) : 
    m_indexed_mesh(mesh)
  {
    if (!m_indexed_mesh.num_tris())
      return;
      
     switch (mode)
     {
       case eTunnelFastStripifier:
       case eTunnelSlowStripifier:
       {
         create_strips_ms();

         Tri_Strips* pStrips = &m_tristrips;
         
         Tri_Strips tempStrips;
         //if (eTunnelSlowStripifier == mode)
         {
            // Sends individual tris to tunneler.
            int cur_ofs = 0;
            while (cur_ofs < m_tristrips.size())
            {
               Tri_Strips tri_strip;
               cur_ofs = m_tristrips.get_tris(tri_strip, cur_ofs);
               tempStrips.append(tri_strip, 0, tri_strip.size());
            }
            
            pStrips = &tempStrips;
         }            
         
         Strip_Tunneler tunneler(m_indexed_mesh, *pStrips);      
         
         m_tristrips = tunneler.tristrips();
         break;
      }
      case eIndividualTrisAsStrips:
      {
        for (int i = 0; i < m_indexed_mesh.num_tris(); i++)
        {
          m_tristrips.push_back(m_indexed_mesh.tri(i)[0], Tri_Strips::TRISTRIP_FLAGS_START);
          m_tristrips.push_back(m_indexed_mesh.tri(i)[1]);
          m_tristrips.push_back(m_indexed_mesh.tri(i)[2]);
        }
        break;
      }
      case eMSStripifier:
      {
        create_strips_ms();
        break;
      }
#if 0      
      case eACTCStripifier:
      {
        create_strips_actc();
        break;
      }
#endif      
      default:
      {
         BVERIFY(0);
      }
    }
          
    create_winding_bits();
  }

  const Indexed_Mesh& indexed_mesh(void) const
  {
    return m_indexed_mesh;
  }

  const Tri_Strips& tristrips(void) const
  {
    return m_tristrips;
  }
};
