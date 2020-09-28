// File: univertPacker.inl

template<bool Packed, bool BigEndian>
inline UnivertPacker<Packed, BigEndian>::UnivertPacker()
{
   BCOMPILETIMEASSERT(sizeof(UnivertPacker) == sizeof(UnivertPacker<!Packed, BigEndian>));
   BCOMPILETIMEASSERT((sizeof(UnivertPacker) & 3) == 0);
   clear();
}

template<bool Packed, bool BigEndian>
inline void UnivertPacker<Packed, BigEndian>::clear(void)
{
   mPos           = VertexElement::eFLOAT3;
   mBasis         = VertexElement::eFLOAT4;
   mBasisScales   = VertexElement::eFLOAT2;
   mTangent       = VertexElement::eFLOAT4;
   mNorm          = VertexElement::eFLOAT3;

   for (uint i = 0; i < cMaxUV; i++)
      mUV[i] = VertexElement::eFLOAT2;

   mIndices       = VertexElement::eUBYTE4;
   mWeights       = VertexElement::eFLOAT4;
   mDiffuse       = VertexElement::eFLOAT4;
   mIndex         = VertexElement::eSHORT2;
   mPackOrder.clear();
   mDeclOrder.clear();
}

template<bool Packed, bool BigEndian>
inline bool UnivertPacker<Packed, BigEndian>::operator== (const UnivertPacker& rhs) const
{
   for (uint i = 0; i < cMaxUV; i++)
      if (mUV[i] != rhs.mUV[i])
         return false;

   return 
      (mPos             == rhs.mPos)         &&
      (mBasis           == rhs.mBasis)       &&
      (mBasisScales     == rhs.mBasisScales) &&
      (mTangent         == rhs.mTangent)     &&          
      (mNorm            == rhs.mNorm)        &&
      (mIndices         == rhs.mIndices)     &&
      (mWeights         == rhs.mWeights)     &&
      (mDiffuse         == rhs.mDiffuse)     &&
      (mIndex           == rhs.mIndex)       &&
      (mPackOrder       == rhs.mPackOrder)   &&
      (mDeclOrder       == rhs.mDeclOrder);
}

template<bool Packed, bool BigEndian>
inline bool UnivertPacker<Packed, BigEndian>::empty(void) const
{
   return packOrder().getEmpty();
}

template<bool Packed, bool BigEndian>
inline void UnivertPacker<Packed, BigEndian>::log(BTextDispatcher& log) const
{
   log.printf("UnivertPacker: PackOrder: \"%s\" DeclOrder: \"%s\"\n", mPackOrder.c_str(), mDeclOrder.c_str());

   const char* pPackOrder = mPackOrder;
   while (*pPackOrder)
   {
      const char c = static_cast<char>(toupper(*pPackOrder++));

      switch (c)
      {
      case ePOS_SPEC:
         log.printf("         mPos: %s\n", VertexElement::name(mPos));
         break;
      case eBASIS_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
            log.printf("       mBasis[%02i]: %s\n", n, VertexElement::name(mBasis));
            break;
         }
      case eBASIS_SCALE_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
            log.printf("  basisScale[%02i]: %s\n", n, VertexElement::name(mBasisScales));
            break;
         }
      case eTANGENT_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
            log.printf("    mTangent[%02i]: %s\n", n, VertexElement::name(mTangent));
            break;
         }
      case eNORM_SPEC:
         log.printf("        mNorm: %s\n", VertexElement::name(mNorm));
         break;
      case eTEXCOORDS_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', cMaxUV); 
            log.printf("      mUV[%02i]: %s\n", n, VertexElement::name(mUV[n]));
            break;
         }
      case eSKIN_SPEC:
         log.printf("     mIndices: %s\n", VertexElement::name(mIndices));
         log.printf("     mWeights: %s\n", VertexElement::name(mWeights));
         break;
      case eDIFFUSE_SPEC:
         log.printf("     mDiffuse: %s\n", VertexElement::name(mDiffuse));
         break;
      case eINDEX_SPEC:
         log.printf("       mIndex: %s\n", VertexElement::name(mIndex));
         break;
      default:
         BASSERT(false);
      }
   }
}

template<bool Packed, bool BigEndian>
inline void* UnivertPacker<Packed, BigEndian>::pack(void* pDst, const Univert& vert, bool bigEndian) const
{
   const char* pPackOrder = mPackOrder;
   while (*pPackOrder)
   {
      const char c = static_cast<char>(toupper(*pPackOrder++));

      switch (c)
      {
      case ePOS_SPEC:
         pDst = VertexElement::pack(pDst, mPos, BVecN<4>(vert.p, 1.0f), bigEndian);
         break;
      case eBASIS_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
            pDst = VertexElement::pack(pDst, mBasis, vert.tangent(n), bigEndian);
            pDst = VertexElement::pack(pDst, mBasis, vert.binormal(n), bigEndian);
            break;
         }
      case eBASIS_SCALE_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
            pDst = VertexElement::pack(pDst, mBasisScales, 
               BVecN<4>(vert.tangent(n)[3], vert.binormal(n)[3], 0, 0), bigEndian);
            break;
         }
      case eTANGENT_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
            pDst = VertexElement::pack(pDst, mTangent, vert.tangent(n), bigEndian);
            break;
         }            
      case eNORM_SPEC:
         pDst = VertexElement::pack(pDst, mNorm, BVecN<4>(vert.n, 0.0f), bigEndian);
         break;
      case eTEXCOORDS_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', Univert::MaxUVCoords);
            pDst = VertexElement::pack(pDst, mUV[n], BVecN<4>(vert.texcoord(n)), bigEndian);
            break;
         }
      case eSKIN_SPEC:
         pDst = VertexElement::pack(pDst, mIndices, BVecN<4>(vert.boneIndex(0), vert.boneIndex(1), vert.boneIndex(2), vert.boneIndex(3)), bigEndian);
         pDst = VertexElement::pack(pDst, mWeights, BVecN<4>(vert.boneWeight(0), vert.boneWeight(1), vert.boneWeight(2), vert.boneWeight(3)), bigEndian);
         break;
      case eDIFFUSE_SPEC:
         pDst = VertexElement::pack(pDst, mDiffuse, vert.d, bigEndian);
         break;
      case eINDEX_SPEC:
         pDst = VertexElement::pack(pDst, mIndex, BVecN<4>(static_cast<float>(vert.idx), 0, 0, 0), bigEndian);
         break;
      default:
         BASSERT(false);
      }
   }

   return pDst;
}

template<bool Packed, bool BigEndian>
inline int UnivertPacker<Packed, BigEndian>::countVertexElements(EVertElementSpec spec) const
{
   int num = 0;

   const char* pPackOrder = mPackOrder;
   while (*pPackOrder)
   {
      const char c = static_cast<char>(toupper(*pPackOrder++));

      if (spec == c)
         num++;

      switch (c)
      {
         case eBASIS_SPEC:
         case eBASIS_SCALE_SPEC:
         case eTEXCOORDS_SPEC:
         {
            pPackOrder++;
            break;
         }
      }
   }

   return num;
}

template<bool Packed, bool BigEndian>
inline typename UnivertPacker<Packed, BigEndian>::ElementStats UnivertPacker<Packed, BigEndian>::getStats(void) const
{
   ElementStats ret;

   const char* pPackOrder = mPackOrder;
   while (*pPackOrder)
   {
      const char c = static_cast<char>(toupper(*pPackOrder++));

      switch (c)
      {
      case ePOS_SPEC:
         {
            ret.numPos++;
            break;
         }
      case eBASIS_SPEC:
         {
            pPackOrder++;
            ret.numBasis++;
            break;
         }
      case eBASIS_SCALE_SPEC:
         {
            pPackOrder++;
            ret.numBasisScales++;
            break;
         }
      case eTANGENT_SPEC:
         {
            pPackOrder++;
            ret.numTangent++;
            break;
         }
      case eNORM_SPEC:
         {
            ret.numNorm++;
            break;
         }
      case eTEXCOORDS_SPEC:
         {
            pPackOrder++;
            ret.numTexCoords++;
            break;
         }
      case eSKIN_SPEC:
         {
            ret.numSkin++;
            break;
         }
      case eDIFFUSE_SPEC:
         {
            ret.numDiffuse++;
            break;
         }
      case eINDEX_SPEC:
         {
            ret.numIndex++;
            break;
         }
      default:
         BASSERT(false);
      }
   }

   return ret;
}

template<bool Packed, bool BigEndian>
inline int UnivertPacker<Packed, BigEndian>::size(void) const
{
   int size = 0;

   const char* pPackOrder = mPackOrder;
   while (*pPackOrder)
   {
      const char c = static_cast<char>(toupper(*pPackOrder++));

      switch (c)
      {
      case ePOS_SPEC:
         size += VertexElement::size(mPos);
         break;
      case eBASIS_SPEC:
         {
            pPackOrder++;
            size += VertexElement::size(mBasis) * 2;
            break;
         }
      case eBASIS_SCALE_SPEC:
         {
            pPackOrder++;
            size += VertexElement::size(mBasisScales);
            break;
         }
      case eTANGENT_SPEC:
         {
            pPackOrder++;
            size += VertexElement::size(mTangent);
            break;
         }
      case eNORM_SPEC:
         size += VertexElement::size(mNorm);
         break;
      case eTEXCOORDS_SPEC:
         {
            const uint n = *pPackOrder++ - '0';
            debugRangeCheck<int, int>(n, cMaxUV);
            size += VertexElement::size(mUV[n]);
            break;
         }
      case eSKIN_SPEC:
         size += VertexElement::size(mIndices);
         size += VertexElement::size(mWeights);
         break;
      case eDIFFUSE_SPEC:
         size += VertexElement::size(mDiffuse);
         break;
      case eINDEX_SPEC:
         size += VertexElement::size(mIndex);
         break;
      default:
         BASSERT(false);
      }
   }

   return size;
}

template<bool Packed, bool BigEndian>
inline void UnivertPacker<Packed, BigEndian>::setPackOrder(const char* pPackOrder)
{
   mPackOrder = pPackOrder;
}

template<bool Packed, bool BigEndian>
inline void UnivertPacker<Packed, BigEndian>::setDeclOrder(const char* pDeclOrder)
{
   mDeclOrder = pDeclOrder;
}

// The decl order string is canonicalized to contain indices after each specification, to simplify vertex decl creation.
// This must be called before attempting to create a decl through the vertex decl manager.               
template<bool Packed, bool BigEndian>
inline void UnivertPacker<Packed, BigEndian>::setDeclOrder(
   int firstPos,
   const IntVec& basisMap,
   const IntVec& basisScalesMap,
   int firstNorm,
   const IntVec& uvMap,
   int firstSkin,
   int firstDiffuse,
   int firstIndex)
{
   mDeclOrder.clear();

   const char* pPackOrder = mPackOrder;
   while (*pPackOrder)
   {
      const char c = static_cast<char>(toupper(*pPackOrder++));

      switch (c)
      {
         case ePOS_SPEC:
         {
            mDeclOrder += BFixedString256(cVarArg, "%c%x%x", ePOS_SPEC, firstPos, firstPos);
            firstPos++;
            break;
         }
         case eBASIS_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', basisMap.size());
            mDeclOrder += BFixedString256(cVarArg, "%c%x%x", eBASIS_SPEC, n, basisMap.at(n));
            break;
         }
         case eBASIS_SCALE_SPEC:
         {
            const int n = *pPackOrder++ - '0';
            debugRangeCheck<int, int>(n, basisScalesMap.at(n));
            mDeclOrder += BFixedString256(cVarArg, "%c%x%x", eBASIS_SCALE_SPEC, n, basisScalesMap.at(n));
            break;
         }
         case eTANGENT_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', basisMap.size());
            mDeclOrder += BFixedString256(cVarArg, "%c%x%x", eTANGENT_SPEC, n, basisMap.at(n));
            break;
         }
         case eNORM_SPEC:
         {
            mDeclOrder += BFixedString256(cVarArg, "%c%x%x", eNORM_SPEC, firstNorm, firstNorm);
            firstNorm++;
            break;
         }
         case eTEXCOORDS_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', uvMap.size());
            mDeclOrder += BFixedString256(cVarArg, "%c%x%x", eTEXCOORDS_SPEC, n, uvMap.at(n));                
            break;
         }
         case eSKIN_SPEC:
         {
            mDeclOrder += BFixedString256(cVarArg, "%c%x%x", eSKIN_SPEC, firstSkin, firstSkin);
            firstSkin++;
            break;
         }
         case eDIFFUSE_SPEC:
         {
            mDeclOrder += BFixedString256(cVarArg, "%c%x%x", eDIFFUSE_SPEC, firstDiffuse, firstDiffuse);
            firstDiffuse++;
            break;
         }
         case eINDEX_SPEC:
         {
            mDeclOrder += BFixedString256(cVarArg, "%c%x%x", eINDEX_SPEC, firstIndex, firstIndex);
            firstIndex++;
            break;
         }
         default:
            BASSERT(false);
      }
   }
}

template<bool Packed, bool BigEndian>
inline void UnivertPacker<Packed, BigEndian>::setPackOrder(const UnivertAttributes& usedAttr, const char* pPackOrder)
{
   const BFixedString256 temp(mPackOrder);

   if (!pPackOrder)
      pPackOrder = temp.c_str();

   mPackOrder.clear();

   while (*pPackOrder)
   {
      const char c = static_cast<char>(toupper(*pPackOrder++));

      switch (c)
      {
         case ePOS_SPEC:
         {
            if (usedAttr.pos)
               mPackOrder += BFixedString256(cVarArg, "%c", ePOS_SPEC);
            break;
         }
         case eBASIS_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
            if (n < usedAttr.numBasis)
               mPackOrder += BFixedString256(cVarArg, "%c%i", eBASIS_SPEC, n);
            break;
         }
         case eBASIS_SCALE_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
            if (n < usedAttr.numBasis)
               mPackOrder += BFixedString256(cVarArg, "%c%i", eBASIS_SCALE_SPEC, n);
            break;
         }
         case eTANGENT_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
            if (n < usedAttr.numBasis)
               mPackOrder += BFixedString256(cVarArg, "%c%i", eTANGENT_SPEC, n);
            break;
         }
         case eNORM_SPEC:
         {
            if (usedAttr.norm)
               mPackOrder += BFixedString256(cVarArg, "%c", eNORM_SPEC);
            break;
         }
         case eTEXCOORDS_SPEC:
         {
            const int n = debugRangeCheck<int, int>(*pPackOrder++ - '0', Univert::MaxUVCoords);
            if (n < usedAttr.numUVSets)
               mPackOrder += BFixedString256(cVarArg, "%c%i", eTEXCOORDS_SPEC, n);
            break;
         }
         case eSKIN_SPEC:
         {
            if ((usedAttr.weights) || (usedAttr.indices))
               mPackOrder += BFixedString256(cVarArg, "%c", eSKIN_SPEC);
            break;
         }
         case eDIFFUSE_SPEC:
         {
            if (usedAttr.diffuse)
               mPackOrder += BFixedString256(cVarArg, "%c", eDIFFUSE_SPEC);
            break;
         }
         case eINDEX_SPEC:
         {
            if (usedAttr.idx)
               mPackOrder += BFixedString256(cVarArg, "%c", eINDEX_SPEC);
            break;
         }
         default:
            BASSERT(false);
      }
   }
}
