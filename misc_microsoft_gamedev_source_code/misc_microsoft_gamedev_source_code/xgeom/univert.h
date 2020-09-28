//-----------------------------------------------------------------------------
// File: univert.h
//-----------------------------------------------------------------------------
#pragma once

// xcore
#include "math\generalVector.h"
#include "math\generalMatrix.h"
#include "math\halfFloat.h"
#include "hash\hash.h"

// local
#include "vertexElement.h"

//-----------------------------------------------------------------------------
// EUnivertElement
//-----------------------------------------------------------------------------
enum EUnivertElement
{
   ePos,
   eTangent,
   eBinormal,
   eNorm,
   eUV,
   eIndices,
   eWeights,
   eDiffuse,
   eIndex,
   
   eNumUnivertElements     
};
      
//-----------------------------------------------------------------------------            
// struct UnivertAttributes
//-----------------------------------------------------------------------------
struct UnivertAttributes
{
   bool  pos;
   int   numBasis;
   bool  norm;
   int   numUVSets;
   bool  indices;
   bool  weights;
   bool  diffuse;
   bool  idx;

   UnivertAttributes()
   {
      clear();
   }

   UnivertAttributes(const UnivertAttributes& b)
   {
      memcpy(this, &b, sizeof(b));
   }

   UnivertAttributes& operator= (const UnivertAttributes& b)
   {
      memcpy(this, &b, sizeof(b));
      return *this;
   }

   enum EAllClear { eAllClear };
   UnivertAttributes(EAllClear e)
   {
      e;
      clear();
   }

   enum EAllSet { eAllSet };
   UnivertAttributes(EAllSet e)
   {
      e;
      setAll();
   }

   UnivertAttributes(BStream& src)
   {
      src >> *this;
   }

   UnivertAttributes& clear(void)
   {
      Utils::ClearObj(*this);
      return *this;
   }

   UnivertAttributes& setAll(void);
   
   UnivertAttributes& set(const char* pDescStr);
   
   UnivertAttributes(const char* pDescStr)
   {
      set(pDescStr);
   }
   
   BFixedString256& getDescStr(BFixedString256& dst) const;
         
   int size(void) const;
   
   friend bool operator== (const UnivertAttributes& a, const UnivertAttributes& b)
   {
      return ((a.pos       == b.pos) &&
              (a.numBasis  == b.numBasis) &&
              (a.norm      == b.norm) &&
              (a.numUVSets == b.numUVSets) &&
              (a.indices   == b.indices) &&
              (a.weights   == b.weights) &&
              (a.diffuse   == b.diffuse) &&
              (a.idx       == b.idx));
   }
   
   friend bool operator!= (const UnivertAttributes& a, const UnivertAttributes& b)
   {
      return !(a == b);
   }

   friend UnivertAttributes operator& (const UnivertAttributes& a, const UnivertAttributes& b)
   {
      UnivertAttributes ret;
      ret.pos        = a.pos & b.pos;
      ret.numBasis   = Math::Min(a.numBasis, b.numBasis);
      ret.norm       = a.norm & b.norm;
      ret.numUVSets  = Math::Min(a.numUVSets, b.numUVSets);
      ret.indices    = a.indices & b.indices;
      ret.weights    = a.weights & b.weights;
      ret.diffuse    = a.diffuse & b.diffuse;
      ret.idx        = a.idx & b.idx;
      return ret;
   }

   friend UnivertAttributes operator| (const UnivertAttributes& a, const UnivertAttributes& b)
   {
      UnivertAttributes ret;
      ret.pos        = a.pos | b.pos;
      ret.numBasis   = Math::Max(a.numBasis, b.numBasis);
      ret.norm       = a.norm | b.norm;
      ret.numUVSets  = Math::Max(a.numUVSets, b.numUVSets);
      ret.indices    = a.indices | b.indices;
      ret.weights    = a.weights | b.weights;
      ret.diffuse    = a.diffuse | b.diffuse;
      ret.idx        = a.idx | b.idx;
      return ret;
   }

   UnivertAttributes& operator&= (const UnivertAttributes& b)
   {
      return *this = (*this & b);
   }

   UnivertAttributes& operator|= (const UnivertAttributes& b)
   {
      return *this = (*this | b);
   }

   friend BStream& operator<< (BStream& dest, const UnivertAttributes& a) 
   {
      dest << a.pos;
      dest << a.numBasis;
      dest << a.norm;
      dest << a.numUVSets;
      dest << a.indices;
      dest << a.weights;
      dest << a.diffuse;
      dest << a.idx;
      
      return dest;
   }

   friend BStream& operator>> (BStream& src, UnivertAttributes& a) 
   {
      src >> a.pos;
      src >> a.numBasis;
      src >> a.norm;
      src >> a.numUVSets;
      src >> a.indices;
      src >> a.weights;
      src >> a.diffuse;
      src >> a.idx;
      
      return src;
   }
   
   void log(BTextDispatcher& log) const
   {
      log.printf("UnivertAttributes: pos: %i, numBasis: %i, norm: %i, numUVSets: %i,\n  indices: %i, weights: %i, diffuse: %i, index: %i\n",
         pos, numBasis, norm, numUVSets, indices, weights, diffuse, idx);
   }
   
   static UnivertAttributes PosAttributes;
   static UnivertAttributes PosIndexAttributes;
   static UnivertAttributes PosNormAttributes;
};
   
//-----------------------------------------------------------------------------
// struct UnivertTolerances
//-----------------------------------------------------------------------------
struct UnivertTolerances
{
   float pTol;
   float nTol;
   float uvTol;
   float wTol;
   float dTol;

   UnivertTolerances(
         float p = .00125f, float n = 1.0f, float uv = 1.0f / 2048.0f, 
         float w = 1.0f, float d = 1.0f) :
      pTol(p),
      nTol(n),
      uvTol(uv),
      wTol(w),
      dTol(d)
   {
   }
};

//-----------------------------------------------------------------------------
// struct Univert
//-----------------------------------------------------------------------------
struct Univert
{
   typedef BVec3 PositionType;
   
   BVec3 p;
   BVec3 n;
   enum { MaxUVCoords = 4 };
   BVec2 uv[MaxUVCoords];
   
   enum { MaxBasisVecs = 2 };

   BVec4 t[MaxBasisVecs];
   BVec4 s[MaxBasisVecs];
   
   enum { MaxInfluences = 4 };
   enum { DefaultBoneIndex = 0xff };
   uint8 indices[MaxInfluences];
   BVecN<MaxInfluences> weights;

   BVec4 d;
   
   int idx;

   Univert()
   {
   }

   Univert(const BVec3& pos)
   {
      clear();
      p = pos;
   }
   
   Univert(const BVec3& pos, const BVec3& norm)
   {
      clear();
      p = pos;
      n = norm;
   }

   Univert(BStream& src)
   {
      src >> *this;
   }

   Univert(BStream& src, const UnivertAttributes& attr)
   {
      read(src, attr);
   }
   
   Univert(eClear dummy)
   {
      dummy;
      clear();
   }
   
   Univert(const UnivertAttributes& attr, const uint8* pSrc)
   {
      clear();
      unpack(attr, pSrc);
   }
   
   const BVec3& pos() const { return p; }
         BVec3& pos()       { return p; }
         
   const BVec3& norm() const { return n; }
         BVec3& norm()       { return n; }
         
   const int& index() const { return idx; }
         int& index()       { return idx; }            
               
   const BVec4& tangent(int set) const { return t[debugRangeCheck(set, MaxBasisVecs)]; }
         BVec4& tangent(int set)       { return t[debugRangeCheck(set, MaxBasisVecs)]; }
            
   const BVec4& binormal(int set) const { return s[debugRangeCheck(set, MaxBasisVecs)]; }
         BVec4& binormal(int set)       { return s[debugRangeCheck(set, MaxBasisVecs)]; }
                   
   const BVec2& texcoord(int set) const  { return uv[debugRangeCheck<int, int>(set, MaxUVCoords)]; }
         BVec2& texcoord(int set)        { return uv[debugRangeCheck<int, int>(set, MaxUVCoords)]; }

   uint8    boneIndex(uint i) const { return indices[debugRangeCheck<int, int>(i, MaxInfluences)]; }
   uint8&   boneIndex(uint i)       { return indices[debugRangeCheck<int, int>(i, MaxInfluences)]; }

   float    boneWeight(uint i) const   { return weights[debugRangeCheck<int, int>(i, MaxInfluences)]; }
   float&   boneWeight(uint i)         { return weights[debugRangeCheck<int, int>(i, MaxInfluences)]; }
   
   const BVec4& diffuse(void) const { return d; }
         BVec4& diffuse(void)       { return d; }
   
   void clearBoneIndicesAndWeights(void)
   {
      indices[0] = indices[1] = indices[2] = indices[3] = DefaultBoneIndex;
      weights[0] = 1.0f; 
      weights[1] = 0.0f;
      weights[2] = 0.0f;
      weights[3] = 0.0f;
   }
         
   void clear(void)
   {
      Utils::ClearObj(*this);
      
      indices[0] = indices[1] = indices[2] = indices[3] = DefaultBoneIndex;
      weights[0] = 1.0f;
   }

   operator size_t() const
   {
      return hashFast(this, sizeof(*this));
   }

   operator BHash() const
   {
      return BHash(this, sizeof(*this));
   }

   operator BVec3() const
   {
      return p;
   }
   
   //Univert& operator= (const BVec3& newP) { p = newP; return *this; }
         
   // Assumes weights are sorted!
   int numActualInfluences(void) const
   {
      int i;
      for (i = 0; i < MaxInfluences; i++)
         if (0.0f == boneWeight(i))
            break;
      return i;
   }

   bool operator== (const Univert& b) const
   {
      if ((p != b.p) || 
          (n != b.n) ||
          (d != b.d))
         return false;

      for (int i = 0; i < MaxBasisVecs; i++)
         if ((s[i] != b.s[i]) || (t[i] != b.t[i]))
            return false;

      for (int i = 0; i < MaxUVCoords; i++)
         if (uv[i] != b.uv[i])
            return false;
      
      for (int i = 0; i < MaxInfluences; i++)
         if ((indices[i] != b.indices[i]) || 
               (weights[i] != b.weights[i]))
            return false;

      if (idx != b.idx)
         return false;
      
      return true;
   }

   // FIXME: Make sure this always uses lex vector less
   bool operator< (const Univert& b) const
   {
#define COMP(x) if (x <b.x) return true; else if (x != b.x) return false; 
      COMP(p)
      COMP(n)
      COMP(d)
#undef COMP

      for (int i = 0; i < MaxBasisVecs; i++)
      {
         if (s[i] < b.s[i])
            return true;
         else if (s[i] != b.s[i])
            return false;
      }

      for (int i = 0; i < MaxBasisVecs; i++)
      {
         if (t[i] < b.t[i])
            return true;
         else if (t[i] != b.t[i])
            return false;
      }

      for (int i = 0; i < MaxUVCoords; i++)
      {
         if (uv[i] < b.uv[i])
            return true;
         else if (uv[i] != b.uv[i])
            return false;
      }
      
      for (int i = 0; i < MaxInfluences; i++)
      {
         if (indices[i] < b.indices[i])
            return true;
         else if (indices[i] != b.indices[i])
            return false;
      }

      for (int i = 0; i < MaxInfluences; i++)
      {
         if (weights[i] < b.weights[i])
            return true;
         else if (weights[i] != b.weights[i])
            return false;
      }

      if (idx < b.idx)
         return true;
   
      return false;
   }

   template <class T>
   static T snap(const T& x, float tol) 
   {
      if (tol < 1.0f)
         return tol * (x * (1.0f / tol)).floor();
      return x;
   }

   Univert gridSnap(const UnivertTolerances& tol) const
   {
      Univert ret;
      ret.p = snap(p, tol.pTol);
      ret.n = snap(n, tol.nTol);
      
      for (int i = 0; i < MaxBasisVecs; i++)
      {
         ret.s[i] = snap(s[i], tol.nTol);
         ret.t[i] = snap(t[i], tol.nTol);
      }

      for (int i = 0; i < MaxUVCoords; i++)
         ret.uv[i] = snap(uv[i], tol.uvTol);

      ret.weights = snap(weights, tol.wTol);

      Utils::Copy(indices, indices + MaxInfluences, ret.indices);

      ret.d = snap(d, tol.dTol);
      return ret;
   }

   Univert select(const UnivertAttributes& attr) const
   {
      Univert ret;
      ret.clear();
      
      if (attr.pos)     { ret.p = p; }
      
      Utils::Copy(s, s + attr.numBasis, ret.s);
      Utils::Copy(t, t + attr.numBasis, ret.t);
      
      if (attr.norm)    { ret.n = n; }
      
      if (attr.weights) { ret.weights = weights; }
      if (attr.indices) { Utils::Copy(indices, indices + MaxInfluences, ret.indices); }
      if (attr.diffuse) { ret.d = d; }
      if (attr.idx)     { ret.idx = idx; }

      Utils::Copy(uv, uv + attr.numUVSets, ret.uv);
      
      return ret;
   }
         
   uint8* pack(const UnivertAttributes& attr, uint8* pDest) const;
   
   const uint8* unpack(const UnivertAttributes& attr, const uint8* pSrc);
               
   UnivertAttributes usedAttributes(void) const
   {
      UnivertAttributes ret(UnivertAttributes::eAllClear);
      if (!p.isZero()) ret.pos = true;
      
      for (int i = MaxBasisVecs - 1; i >= 0; i--)
      {
         if ((!s[i].isZero()) || (!t[i].isZero())) 
         {
            ret.numBasis = i + 1;
            break;
         }
      }

      if (!n.isZero()) ret.norm = true;
      //if (!weights.isZero()) ret.weights = true;
      
      if (BVecN<MaxInfluences>(1.0f, 0.0f, 0.0f, 0.0f) != weights) ret.weights = true;
      
      for (int i = 0; i < MaxInfluences; i++)
         if (indices[i] != DefaultBoneIndex)
         {
            ret.indices = true;
            break;
         }

      if (idx)
         ret.idx = true;
                     
      for (int i = MaxUVCoords - 1; i >= 0; i--)
         if (!uv[i].isZero())
         {
            ret.numUVSets = i + 1;
            break;
         }  

      return ret;
   }

   friend BStream& operator<< (BStream& dest, const Univert& a)
   {
      dest << a.p;
               
      for (int i = 0; i < MaxBasisVecs; i++)
      {
         dest << a.s[i];
         dest << a.t[i];
      }

      dest << a.n;

      for (int i = 0; i < MaxInfluences; i++)
      {      
         dest << a.weights[i];
         dest << a.indices[i];
      }
      
      dest << a.d;
      dest << a.idx;

      for (int i = 0; i < MaxUVCoords; i++)
         dest << a.uv[i];

      return dest;
   }

   friend BStream& operator>> (BStream& src, Univert& a)
   {
      src >> a.p;
               
      for (int i = 0; i < MaxBasisVecs; i++)
      {
         src >> a.s[i];
         src >> a.t[i];
      }

      src >> a.n;

      for (int i = 0; i < MaxInfluences; i++)
      {      
         src >> a.weights[i];
         src >> a.indices[i];
      }
      
      src >> a.d;
      src >> a.idx;

      for (int i = 0; i < MaxUVCoords; i++)
         src >> a.uv[i];

      return src;
   }

   BStream& write(BStream& dest, const UnivertAttributes& attr) const
   {
      if (attr.pos)     { dest << p; }
               
      for (int i = 0; i < attr.numBasis; i++)
      {
         dest << s[i];
         dest << t[i];
      }

      if (attr.norm)    { dest << n; }

      if (attr.weights)
      {
         for (int i = 0; i < MaxInfluences; i++)
            dest << weights[i];
      }
      
      if (attr.indices)
      {
         for (int i = 0; i < MaxInfluences; i++)
            dest << indices[i];
      }
      
      if (attr.diffuse) { dest << d; }
      if (attr.idx)     { dest << idx; }

      for (int i = 0; i < attr.numUVSets; i++)
         dest << uv[i];
         
      return dest;            
   }

   BStream& read(BStream& src, const UnivertAttributes& attr) 
   {
      clear();
      if (attr.pos)     { src >> p; }
               
      for (int i = 0; i < attr.numBasis; i++)
      {
         src >> s[i];
         src >> t[i];
      }

      if (attr.norm)    { src >> n; }

      if (attr.weights)
      {
         for (int i = 0; i < MaxInfluences; i++)
            src >> weights[i];
      }          

      if (attr.indices)         
      {
         for (int i = 0; i < MaxInfluences; i++)
            src >> indices[i];
      }         
      
      if (attr.diffuse) { src >> d; }
      if (attr.idx)     { src >> idx; }

      for (int i = 0; i < attr.numUVSets; i++)
         src >> uv[i];
         
      return src;          
   }

   static int size(const UnivertAttributes& attr) 
   {
      int ret = 0;
      Univert dummy;
   
      if (attr.pos)     { ret += sizeof(dummy.p); }
      
      ret += (sizeof(dummy.s[0]) + sizeof(dummy.t[0])) * attr.numBasis;
      
      if (attr.norm)    { ret += sizeof(dummy.n); }
      
      if (attr.weights) { ret += sizeof(dummy.weights); }
      if (attr.indices) { ret += sizeof(dummy.indices); }
      if (attr.diffuse) { ret += sizeof(dummy.d); }
      if (attr.idx)     { ret += sizeof(dummy.idx); }

      ret += attr.numUVSets * sizeof(dummy.uv[0]);
      return ret;
   }

   void debugCheck(void) const
   {
      p.debugCheck();
      for (int i = 0; i < MaxBasisVecs; i++)
      {
         t[i].debugCheck();
         s[i].debugCheck();
      }

      n.debugCheck();

      for (int i = 0; i < MaxUVCoords; i++)
         uv[i].debugCheck();
         
      weights.debugCheck();
      d.debugCheck();
   }
   
   Univert& operator+= (const Univert& b)
   {
      p += b.p;
      
      for (int i = 0; i < MaxBasisVecs; i++)
      {
         t[i] += b.t[i];
         s[i] += b.s[i];
      }
      
      n += b.n;
      
      for (int i = 0; i < MaxUVCoords; i++)
         uv[i] += b.uv[i];
      
      weights += b.weights;
      
      d += b.d;
      
      return *this;         
   }
   
   Univert& operator-= (const Univert& b)
   {
      p -= b.p;

      for (int i = 0; i < MaxBasisVecs; i++)
      {
         t[i] -= b.t[i];
         s[i] -= b.s[i];
      }

      n -= b.n;
      
      for (int i = 0; i < MaxUVCoords; i++)
         uv[i] -= b.uv[i];

      weights -= b.weights;

      d -= b.d;

      return *this;         
   }

   Univert& operator*= (float scaler)
   {
      p *= scaler;

      for (int i = 0; i < MaxBasisVecs; i++)
      {
         t[i] *= scaler;
         s[i] *= scaler;
      }

      n *= scaler;
      
      for (int i = 0; i < MaxUVCoords; i++)
         uv[i] *= scaler;

      weights *= scaler;

      d *= scaler;
      
      return *this;
   }

   friend Univert operator+ (const Univert& a, const Univert& b)
   {
      Univert ret(a);
      ret += b;
      return ret;
   }
   
   friend Univert operator- (const Univert& a, const Univert& b)
   {
      Univert ret(a);
      ret -= b;
      return ret;
   }

   friend Univert operator* (const Univert& a, float scaler)
   {
      Univert ret(a);
      ret *= scaler;
      return ret;
   }
   
   Univert& normalize(const UnivertAttributes& attr)
   {
      for (int i = 0; i < attr.numBasis; i++)
      {
         s[i].tryNormalize();
         t[i].tryNormalize();
      }

      if (attr.norm)    
      { 
         n.tryNormalize(); 
      }

      if (attr.weights) 
      { 
         // TODO: Normalize weights to 1.0!
      }
      
      return *this;
   }
   
   static Univert lerp(const Univert& a, const Univert& b, float scaler)
   {
      return a + (b - a) * scaler;
   }
       

};

typedef BDynamicArray<Univert> UnivertVec;

//-----------------------------------------------------------------------------
// struct UnivertEqualTolHashCompare
//-----------------------------------------------------------------------------
struct UnivertEqualTolHashCompare
{  
   // parameters for stl hash table
   enum
   {  
      bucket_size = 4,  
      min_buckets = 8
   }; 

   UnivertEqualTolHashCompare(const UnivertTolerances& t) : mTol(t)
   {
   }
   
   size_t operator()(const Univert& val) const
   {  
      Univert snapped(val.gridSnap(mTol));
      return (size_t)snapped;
   }
   
   bool operator()(const Univert& a, const Univert& b) const
   {  
      Univert aSnapped(a.gridSnap(mTol));
      Univert bSnapped(b.gridSnap(mTol));
      return aSnapped < bSnapped;
   }

private:
   UnivertTolerances mTol;
};

inline UnivertAttributes& UnivertAttributes::setAll(void)
{
   pos = norm = indices = weights = diffuse = idx = true;
   numBasis = Univert::MaxBasisVecs;
   numUVSets = Univert::MaxUVCoords;
   return *this;
}

inline int UnivertAttributes::size(void) const 
{
   return Univert::size(*this);
}

inline BFixedString256& UnivertAttributes::getDescStr(BFixedString256& dst) const
{
   dst.clear();
   if (pos)
      dst += "P";
   for (int i = 0; i < numBasis; i++)
      dst += BFixedString256(cVarArg, "B%i", i);
   if (norm)
      dst += "N";
   for (int i = 0; i < numUVSets; i++)
      dst += BFixedString256(cVarArg, "T%i", i);
   if ((weights) || (indices))
      dst += "S";
   if (diffuse)
      dst += "D";
   if (idx)
      dst += "I";
   return dst;
}

inline UnivertAttributes& UnivertAttributes::set(const char* pDescStr)
{
   BASSERT(pDescStr);
   
   clear();
   
   while (*pDescStr)
   {
      const char c = static_cast<char>(toupper(*pDescStr++));
      switch (c)
      {
         case ePOS_SPEC:
         {
            pos = true;
            break;
         }
         case eBASIS_SPEC:
         case eTANGENT_SPEC:
         {
            const int n = debugRangeCheck(*pDescStr++ - '0', Univert::MaxBasisVecs);
            numBasis = Math::Max(numBasis, n + 1);
            break;
         }
         case eNORM_SPEC:
         {
            norm = true;
            break;
         }
         case eTEXCOORDS_SPEC:
         {
            const int n = debugRangeCheck(*pDescStr++ - '0', static_cast<int>(Univert::MaxUVCoords));
            numUVSets = Math::Max(numUVSets, n + 1);
            break;
         }
         case eSKIN_SPEC:
         {
            indices = true;
            weights = true;
            break;
         }
         case eDIFFUSE_SPEC:
         {
            diffuse = true;
            break;
         }
         case eINDEX_SPEC:
         {
            idx = true;
            break;
         }
         default:
            BASSERT(false);
      }
   }
   
   return *this;
}

//-----------------------------------------------------------------------------   
// class UnivertStreamer
//-----------------------------------------------------------------------------
class UnivertStreamer
{
public:
   UnivertStreamer()  
   {
      clear();
   }
   
   UnivertStreamer(const UnivertAttributes& attr)
   {
      setAttributes(attr);
   }
   
   void clear(void)
   {
      mPosOfs = -1;
      mNormOfs = -1;
      mUVOfs = -1;
      mBasisOfs = -1;
      mIndicesOfs = -1;
      mWeightsOfs = -1;
      mDiffuseOfs = -1;
      mIndexOfs = -1;
   }
   
   // returns total size
   int setAttributes(const UnivertAttributes& attr)
   {
      clear();
      
      int ofs = 0;
      Univert dummy;
               
      if (attr.pos)     
      { 
         mPosOfs = ofs; 
         ofs += sizeof(dummy.p); 
      }
      
      if (attr.norm)    
      { 
         mNormOfs = ofs;
         ofs += sizeof(dummy.n);
      }
      
      if (attr.numUVSets)
      {
         mUVOfs = ofs;
         ofs += attr.numUVSets * sizeof(dummy.uv[0]);
      }
               
      if (attr.numBasis)
      {
         mBasisOfs = ofs;
         ofs += attr.numBasis * (sizeof(dummy.s[0]) + sizeof(dummy.t[0]));
      }
                        
      if (attr.indices) 
      { 
         mIndicesOfs = ofs;
         ofs += sizeof(dummy.indices);
      }
      
      if (attr.weights) 
      { 
         mWeightsOfs = ofs;
         ofs += sizeof(dummy.weights);
      }
      
      if (attr.diffuse) 
      { 
         mDiffuseOfs = ofs;
         ofs += sizeof(dummy.d);
      }
      
      if (attr.idx)     
      { 
         mIndexOfs = ofs;
         ofs += sizeof(dummy.idx);
      }
      
      BASSERT(ofs == attr.size());
      return ofs;
   }
               
   int posOfs(void) const { return mPosOfs; }
   int normOfs(void) const { return mNormOfs; }
   int uvOfs(void) const { return mUVOfs; }
   int basisOfs(void) const { return mBasisOfs; }
   int indicesOfs(void) const { return mIndicesOfs; }
   int weightsOfs(void) const { return mWeightsOfs; }
   int diffuseOfs(void) const { return mDiffuseOfs; }
   int indexOfs(void) const { return mIndexOfs; }
   
   static uint8* pack(const Univert& v, const UnivertAttributes& attr, uint8* pDest) 
   {
      if (attr.pos)     { writeObj(pDest, v.p); }
      if (attr.norm)    { writeObj(pDest, v.n); }
      for (int i = 0; i < attr.numUVSets; i++) writeObj(pDest, v.uv[i]);      

      for (int i = 0; i < attr.numBasis; i++)
      {
         writeObj(pDest, v.s[i]); 
         writeObj(pDest, v.t[i]);
      }

      if (attr.indices) { writeObj(pDest, v.indices); }
      if (attr.weights) { writeObj(pDest, v.weights); }
      
      if (attr.diffuse) { writeObj(pDest, v.d); }
      if (attr.idx)     { writeObj(pDest, v.idx); }

      return pDest;
   }

   static const uint8* unpack(Univert& v, const UnivertAttributes& attr, const uint8* pSrc)
   {
      if (attr.pos)        { readObj(pSrc, v.p); }
      if (attr.norm)    { readObj(pSrc, v.n); }
      for (int i = 0; i < attr.numUVSets; i++) readObj(pSrc, v.uv[i]);
      
      for (int i = 0; i < attr.numBasis; i++)
      {
         readObj(pSrc, v.s[i]); 
         readObj(pSrc, v.t[i]);
      }
      
      if (attr.indices) { readObj(pSrc, v.indices); }
      if (attr.weights) { readObj(pSrc, v.weights); }
      
      if (attr.diffuse) { readObj(pSrc, v.d); }
      if (attr.idx)     { readObj(pSrc, v.idx); }
      
      return pSrc;
   }         
  
private:
   int mPosOfs;
   int mNormOfs;
   int mUVOfs;
   int mBasisOfs;
   int mIndicesOfs;
   int mWeightsOfs;
   int mDiffuseOfs;
   int mIndexOfs;
   
   template<class T>
   static void writeObj(uint8*& pDest, const T& t)
   {
      memcpy(pDest, &t, sizeof(T));
      pDest += sizeof(T);
   }

   template<class T>
   static void readObj(const uint8*& pSrc, T& t)
   {
      memcpy(&t, pSrc, sizeof(T));
      pSrc += sizeof(T);
   }
};

inline uint8* Univert::pack(const UnivertAttributes& attr, uint8* pDest) const
{
   return UnivertStreamer::pack(*this, attr, pDest);
}

inline const uint8* Univert::unpack(const UnivertAttributes& attr, const uint8* pSrc)
{
   return UnivertStreamer::unpack(*this, attr, pSrc);
}

//-----------------------------------------------------------------------------   
// class Unitri
//-----------------------------------------------------------------------------   
class Unitri
{
public:
   Unitri()
   {
      clear();
   }
   
   Unitri(const Univert& a, const Univert& b, const Univert& c, int index = 0)
   {
      mVerts[0] = a;
      mVerts[1] = b;
      mVerts[2] = c;
      mIndex = index;
   }
   
   Unitri(int index)
   {
      clear();
      mIndex = index;
   }

   void clear(void)
   {
      mVerts[0].clear();
      mVerts[1].clear();
      mVerts[2].clear();
      mIndex = 0;
   }

   const Univert& operator[] (int i) const { return mVerts[debugRangeCheck(i, 3)]; }
         Univert& operator[] (int i)       { return mVerts[debugRangeCheck(i, 3)]; }

   void debugCheck(void)
   {
      mVerts[0].debugCheck();
      mVerts[1].debugCheck();
      mVerts[2].debugCheck();
   }

   UnivertAttributes usedAttributes(void) const 
   {
      return mVerts[0].usedAttributes() | mVerts[1].usedAttributes() | mVerts[2].usedAttributes();
   }
   
   int  index(void) const  { return mIndex; }
   int& index(void)        { return mIndex; }

   BStream& write(BStream& dest, const UnivertAttributes& attr) const
   {
      mVerts[0].write(dest, attr);
      mVerts[1].write(dest, attr);
      mVerts[2].write(dest, attr);
      dest << mIndex;
      return dest;
   }

   BStream& read(BStream& src, const UnivertAttributes& attr) 
   {
      mVerts[0].read(src, attr);
      mVerts[1].read(src, attr);
      mVerts[2].read(src, attr);
      src >> mIndex;
      return src;
   }

   BStream& write(BStream& dst) const
   {
      return dst << mVerts[0] << mVerts[1] << mVerts[2] << mIndex;
   }

   BStream& read(BStream& dst)
   {
      return dst >> mVerts[0] >> mVerts[1] >> mVerts[2] >> mIndex;
   }

   friend BStream& operator<< (BStream& dst, const Unitri& src)
   {
      return src.write(dst);
   }

   friend BStream& operator>> (BStream& src, Unitri& dst)
   {
      return dst.read(src);
   }
   
   Univert fromBarycentric(const BVec2& barycentric) const
   {
      return mVerts[0] + (mVerts[1] - mVerts[0]) * barycentric[0] + (mVerts[2] - mVerts[0]) * barycentric[1];
   }      
   
   Unitri& normalize(const UnivertAttributes& attr)
   {
      mVerts[0].normalize(attr);
      mVerts[1].normalize(attr);
      mVerts[2].normalize(attr);
      return *this;
   }
   
   BVec3 normal(bool tryNormalize = true) const
   {
      BVec3 n((mVerts[2].pos() - mVerts[0].pos()) % (mVerts[0].pos() - mVerts[1].pos()));
      if (tryNormalize)
         n.tryNormalize();
      return n;
   }
   
   float area(void) const
   {
      return normal(false).len() * .5f;
   }
   
   Unitri flip(void) const
   {  
      return Unitri(mVerts[2], mVerts[1], mVerts[0]);
   }
   
   Unitri& setVertsToIndex(int index) 
   {
      mVerts[0].index() = index;
      mVerts[1].index() = index;
      mVerts[2].index() = index;
      return *this;
   }
   
protected:
   Univert mVerts[3];
   int mIndex;
};
   
typedef BDynamicArray<Unitri> UnitriVec;

