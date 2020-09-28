// File: texture_space_basis.h
#pragma once

// From Max SDK:
// Returns first two rows of UVW->Model matrix or GTS->model.
inline void MakeTextureSpaceBasis(BVec3* pBasisVecs, const BVec2* pT, const BVec3* pV) 
{
   const float uva = pT[1][0] - pT[0][0];
   const float uvb = pT[2][0] - pT[0][0];

   const float uvc = pT[1][1] - pT[0][1];
   const float uvd = pT[2][1] - pT[0][1];

   const float uvk = uvb * uvc - uva * uvd;

   const BVec3 v1(pV[1] - pV[0]);
   const BVec3 v2(pV[2] - pV[0]);

   if (uvk != 0.0f) 
   {
      pBasisVecs[0] = (uvc * v2 - uvd * v1) / uvk;
      pBasisVecs[1] = (uva * v2 - uvb * v1) / uvk;
   }
   else 
   {
      if (uva != 0.0f)
         pBasisVecs[0] = v1 / uva;
      else if (uvb != 0.0f)
         pBasisVecs[0] = v2 / uvb;
      else
         pBasisVecs[0] = BVec3(0.0f, 0.0f, 0.0f);

      if (uvc != 0.0f)
         pBasisVecs[1] = v1 / uvc;
      else if (uvd != 0.0f)
         pBasisVecs[1] = v2 / uvd;
      else
         pBasisVecs[1] = BVec3(0.0f, 0.0f, 0.0f);
   }

   pBasisVecs[1] *= -1.0f;
}

// false on failure
inline bool MakeTextureSpaceBasisAlt(BVec3* pBasisVecs, const BVec2* pT, const BVec3* pV) 
{
   bool success = true;

   BVec3 edge01( pV[1][0] - pV[0][0], pT[1][0] - pT[0][0], pT[1][1] - pT[0][1] );
   BVec3 edge02( pV[2][0] - pV[0][0], pT[2][0] - pT[0][0], pT[2][1] - pT[0][1] );
   BVec3 cp(edge01 % edge02);
   if( fabs(cp[0]) > 1e-8 )
   {
      pBasisVecs[0][0] = -cp[1] / cp[0];        
      pBasisVecs[1][0] = -cp[2] / cp[0];
   }
   else
      success = false;

   edge01 = BVec3( pV[1][1] - pV[0][1], pT[1][0] - pT[0][0], pT[1][1] - pT[0][1] );
   edge02 = BVec3( pV[2][1] - pV[0][1], pT[2][0] - pT[0][0], pT[2][1] - pT[0][1] );
   cp = edge01 % edge02;
   if( fabs(cp[0]) > 1e-8 )
   {
      pBasisVecs[0][1] = -cp[1] / cp[0];
      pBasisVecs[1][1] = -cp[2] / cp[0];
   }
   else
      success = false;

   edge01 = BVec3( pV[1][2] - pV[0][2], pT[1][0] - pT[0][0], pT[1][1] - pT[0][1] );
   edge02 = BVec3( pV[2][2] - pV[0][2], pT[2][0] - pT[0][0], pT[2][1] - pT[0][1] );
   cp = edge01 % edge02;
   if( fabs(cp[0]) > 1e-8 )
   {
      pBasisVecs[0][2] = -cp[1] / cp[0];
      pBasisVecs[1][2] = -cp[2] / cp[0];
   }
   else
      success = false;
      
   return success;
}

