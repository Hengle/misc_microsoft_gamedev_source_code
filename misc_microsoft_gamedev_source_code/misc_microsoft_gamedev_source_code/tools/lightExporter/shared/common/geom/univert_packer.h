// File: univert_packer.h
#pragma once
#ifndef UNIVERT_PACKER_H
#define UNIVERT_PACKER_H

#include "vertex_element.h"
#include "common/geom/univert.h"
#include "common/utils/logfile.h"
#include "common/utils/stream.h"

namespace gr
{
	struct UnivertPacker
	{
		UnivertPacker()
		{
			clear();
		}
		
		void clear(void)
		{
			mPos = VertexElement::eFLOAT3;
			mBasis = VertexElement::eFLOAT4;
			mBasisScales = VertexElement::eFLOAT2;
			mNorm = VertexElement::eFLOAT3;
			mUV = VertexElement::eFLOAT2;
			mIndices = VertexElement::eUBYTE4;
			mWeights = VertexElement::eFLOAT4;
			mDiffuse = VertexElement::eFLOAT4;
			mIndex = VertexElement::eSHORT2;
			mPackOrder.clear();
			mDeclOrder.clear();
		}
		
		bool operator== (const UnivertPacker& rhs) const
		{
			return 
				(mPos					== rhs.mPos)					&&
				(mBasis				== rhs.mBasis)				&&
				(mBasisScales == rhs.mBasisScales)	&&
				(mNorm				== rhs.mNorm)					&&
				(mUV					== rhs.mUV)						&&
				(mIndices			== rhs.mIndices)			&&
				(mWeights			== rhs.mWeights)			&&
				(mDiffuse			== rhs.mDiffuse)			&&
				(mIndex				== rhs.mIndex)				&&
				(mPackOrder		== rhs.mPackOrder)    &&
				(mDeclOrder		== rhs.mDeclOrder);
		}
		
		bool operator!= (const UnivertPacker& rhs) const
		{
			return !(*this == rhs);
		}
		
		VertexElement::EType pos(void) const			{ return mPos; }
		VertexElement::EType basis(void) const		{ return mBasis; }
		VertexElement::EType basisScales(void) const		{ return mBasisScales; }
		VertexElement::EType norm(void) const			{ return mNorm; }
		VertexElement::EType UV(void) const				{ return mUV; }
		VertexElement::EType indices(void) const	{ return mIndices; }
		VertexElement::EType weights(void) const	{ return mWeights; }
		VertexElement::EType diffuse(void) const	{ return mDiffuse; }
		VertexElement::EType index(void) const		{ return mIndex; }
		const BigString& packOrder(void) const		{ return mPackOrder; }
		const BigString& declOrder(void) const		{ return mDeclOrder; }
		
		void setPos(VertexElement::EType pos)					{ mPos = pos; }
		void setBasis(VertexElement::EType basis)			{ mBasis = basis; }
		void setBasisScales(VertexElement::EType basisScales) { mBasisScales = basisScales; }
		void setNorm(VertexElement::EType norm)				{ mNorm = norm; }
		void setUV(VertexElement::EType UV)						{ mUV = UV; }
		void setIndices(VertexElement::EType indices) { mIndices = indices; }
		void setWeights(VertexElement::EType weights) { mWeights = weights; }
		void setDiffuse(VertexElement::EType diffuse) { mDiffuse = diffuse; }
		void setIndex(VertexElement::EType index)			{ mIndex = index; }
		
		bool empty(void) const
		{
			return packOrder().empty();
		}

#if 0		
		void log(LogFile& log) const
		{
			log.printf("UnivertPacker (all elements):\n");
			log.printf("         mPos: %s\n", VertexElement::name(mPos));
      log.printf("       mBasis: %s\n", VertexElement::name(mBasis));
      log.printf("  basisScale: %s\n", VertexElement::name(mBasisScales));
      log.printf("        mNorm: %s\n", VertexElement::name(mNorm));
      log.printf("          mUV: %s\n", VertexElement::name(mUV));
      log.printf("     mIndices: %s\n", VertexElement::name(mIndices));
      log.printf("     mWeights: %s\n", VertexElement::name(mWeights));
      log.printf("     mDiffuse: %s\n", VertexElement::name(mDiffuse));
      log.printf("       mIndex: %s\n", VertexElement::name(mIndex));
		}
#endif
		
		void log(LogFile& log) const
		{
			log.printf("UnivertPacker: PackOrder: \"%s\" DeclOrder: \"%s\"\n", mPackOrder.c_str(), mDeclOrder.c_str());
			
			const char* pPackOrder = mPackOrder;
			while (*pPackOrder)
			{
				const char c = toupper(*pPackOrder++);
									
				switch (c)
				{
					case ePOS_SPEC:
						log.printf("         mPos: %s\n", VertexElement::name(mPos));
						break;
					case eBASIS_SPEC:
					{
						const int n = DebugRange<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
						log.printf("       mBasis[%02i]: %s\n", n, VertexElement::name(mBasis));
						break;
					}
					case eBASIS_SCALE_SPEC:
					{
						const int n = DebugRange<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
						log.printf("  basisScale[%02i]: %s\n", n, VertexElement::name(mBasisScales));
						break;
					}
					case eNORM_SPEC:
						log.printf("        mNorm: %s\n", VertexElement::name(mNorm));
						break;
					case eTEXCOORDS_SPEC:
					{
						const int n = DebugRange<int, int>(*pPackOrder++ - '0', Univert::MaxUVCoords);
						log.printf("      mUV[%02i]: %s\n", n, VertexElement::name(mUV));
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
						Assert(false);
				}
			}
		}
	
		void* pack(void* pDst, const Univert& vert) const
		{
			const char* pPackOrder = mPackOrder;
			while (*pPackOrder)
			{
				const char c = toupper(*pPackOrder++);
									
				switch (c)
				{
					case ePOS_SPEC:
						pDst = VertexElement::pack(pDst, mPos, Vec<4>(vert.p, 1.0f));
						break;
					case eBASIS_SPEC:
					{
						const int n = DebugRange<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
						pDst = VertexElement::pack(pDst, mBasis, vert.tangent(n));
						pDst = VertexElement::pack(pDst, mBasis, vert.binormal(n));
						break;
					}
					case eBASIS_SCALE_SPEC:
					{
						const int n = DebugRange<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
						pDst = VertexElement::pack(pDst, mBasisScales, 
							Vec<4>(vert.tangent(n)[3], vert.binormal(n)[3], 0, 0));
						break;
					}
					case eNORM_SPEC:
						pDst = VertexElement::pack(pDst, mNorm, Vec<4>(vert.n, 0.0f));
						break;
					case eTEXCOORDS_SPEC:
					{
						const int n = DebugRange<int, int>(*pPackOrder++ - '0', Univert::MaxUVCoords);
						pDst = VertexElement::pack(pDst, mUV, Vec<4>(vert.texcoord(n)));
						break;
					}
					case eSKIN_SPEC:
						pDst = VertexElement::pack(pDst, mIndices, Vec<4>(vert.boneIndex(0), vert.boneIndex(1), vert.boneIndex(2), vert.boneIndex(3)));
						pDst = VertexElement::pack(pDst, mWeights, Vec<4>(vert.boneWeight(0), vert.boneWeight(1), vert.boneWeight(2), vert.boneWeight(3)));
						break;
					case eDIFFUSE_SPEC:
						pDst = VertexElement::pack(pDst, mDiffuse, vert.d);
						break;
					case eINDEX_SPEC:
						pDst = VertexElement::pack(pDst, mIndex, Vec<4>(vert.index, 0, 0, 0));
						break;
					default:
						Assert(false);
				}
			}
			
			return pDst;
		}
		
		struct ElementStats
		{
			int numPos;
			int numBasis;
			int numBasisScales;
			int numNorm;
			int numTexCoords;
			int numSkin;
			int numDiffuse;
			int numIndex;
			
			ElementStats()
			{
				Utils::ClearObj(*this);
			}
		};
		
		ElementStats getStats(void) const
		{
			ElementStats ret;
									
			const char* pPackOrder = mPackOrder;
			while (*pPackOrder)
			{
				const char c = toupper(*pPackOrder++);
																
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
						Assert(false);
				}
			}
			
			return ret;
		}
		
		int size(void) const
		{
			int size = 0;
			
			const char* pPackOrder = mPackOrder;
			while (*pPackOrder)
			{
				const char c = toupper(*pPackOrder++);
																
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
					case eNORM_SPEC:
						size += VertexElement::size(mNorm);
						break;
					case eTEXCOORDS_SPEC:
					{
						pPackOrder++;
						size += VertexElement::size(mUV);
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
						Assert(false);
				}
			}
			
			return size;
		}
		
		void setPackOrder(const char* pPackOrder)
		{
			mPackOrder = pPackOrder;
		}
		
		void setDeclOrder(const char* pDeclOrder)
		{
			mDeclOrder = pDeclOrder;
		}
						
		void setDeclOrder(
			int firstPos = 0,
			const IntVec& basisMap = IntVec(),
			const IntVec& basisScalesMap = IntVec(),
			int firstNorm = 0,
			const IntVec& uvMap = IntVec(),
			int firstSkin = 0,
			int firstDiffuse = 0,
			int firstIndex = 0)
		{
			mDeclOrder.clear();
			
			const char* pPackOrder = mPackOrder;
			while (*pPackOrder)
			{
				const char c = toupper(*pPackOrder++);
																
				switch (c)
				{
					case ePOS_SPEC:
					{
						mDeclOrder += BigString(eVarArg, "%c%x", ePOS_SPEC, firstPos);
						firstPos++;
						break;
					}
					case eBASIS_SPEC:
					{
						const int n = DebugRange<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
						mDeclOrder += BigString(eVarArg, "%c%x", eBASIS_SPEC, basisMap.at(n));
						break;
					}
					case eBASIS_SCALE_SPEC:
					{
						const int n = DebugRange<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
						mDeclOrder += BigString(eVarArg, "%c%x", eBASIS_SCALE_SPEC, basisScalesMap.at(n));
						break;
					}
					case eNORM_SPEC:
					{
						mDeclOrder += BigString(eVarArg, "%c%x", eNORM_SPEC, firstNorm);
						firstNorm++;
						break;
					}
					case eTEXCOORDS_SPEC:
					{
						const int n = DebugRange<int, int>(*pPackOrder++ - '0', Univert::MaxUVCoords);						
						mDeclOrder += BigString(eVarArg, "%c%x", eTEXCOORDS_SPEC, uvMap.at(n));						
						break;
					}
					case eSKIN_SPEC:
					{
						mDeclOrder += BigString(eVarArg, "%c%x", eSKIN_SPEC, firstSkin);
						firstSkin++;
						break;
					}
					case eDIFFUSE_SPEC:
					{
						mDeclOrder += BigString(eVarArg, "%c%x", eDIFFUSE_SPEC, firstDiffuse);
						firstDiffuse++;
						break;
					}
					case eINDEX_SPEC:
					{
						mDeclOrder += BigString(eVarArg, "%c%x", eINDEX_SPEC, firstIndex);
						firstIndex++;
						break;
					}
					default:
						Assert(false);
				}
			}
		}
		
		void setPackOrder(const UnivertAttributes& usedAttr, const char* pPackOrder = NULL)
		{
			DebugNull(pPackOrder);
			
			const BigString temp(mPackOrder);
			
			if (!pPackOrder)
				pPackOrder = temp.c_str();
				
			mPackOrder.clear();
			
			while (*pPackOrder)
			{
				const char c = toupper(*pPackOrder++);
								
				switch (c)
				{
					case ePOS_SPEC:
						if (usedAttr.pos)
							mPackOrder += BigString(eVarArg, "%c", ePOS_SPEC);
						break;
					case eBASIS_SPEC:
					{
						const int n = DebugRange<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
						if (n < usedAttr.numBasis)
							mPackOrder += BigString(eVarArg, "%c%i", eBASIS_SPEC, n);
						break;
					}
					case eBASIS_SCALE_SPEC:
					{
						const int n = DebugRange<int, int>(*pPackOrder++ - '0', Univert::MaxBasisVecs);
						if (n < usedAttr.numBasis)
							mPackOrder += BigString(eVarArg, "%c%i", eBASIS_SCALE_SPEC, n);
						break;
					}
					case eNORM_SPEC:
						if (usedAttr.norm)
							mPackOrder += BigString(eVarArg, "%c", eNORM_SPEC);
						break;
					case eTEXCOORDS_SPEC:
					{
						const int n = DebugRange<int, int>(*pPackOrder++ - '0', Univert::MaxUVCoords);
						if (n < usedAttr.numUVSets)
							mPackOrder += BigString(eVarArg, "%c%i", eTEXCOORDS_SPEC, n);
						break;
					}
					case eSKIN_SPEC:
						if ((usedAttr.weights) || (usedAttr.indices))
							mPackOrder += BigString(eVarArg, "%c", eSKIN_SPEC);
						break;
					case eDIFFUSE_SPEC:
						if (usedAttr.diffuse)
							mPackOrder += BigString(eVarArg, "%c", eDIFFUSE_SPEC);
						break;
					case eINDEX_SPEC:
						if (usedAttr.index)
							mPackOrder += BigString(eVarArg, "%c", eINDEX_SPEC);
						//break;
					default:
						Assert(false);
				}
			}
		}
		
		friend Stream& operator<< (Stream& dst, const UnivertPacker& src)
		{
			return dst << (char&)src.mPos 
				<< (char&)src.mBasis 
				<< (char&)src.mBasisScales 
				<< (char&)src.mNorm 
				<< (char&)src.mUV 
				<< (char&)src.mIndices 
				<< (char&)src.mWeights 
				<< (char&)src.mDiffuse 
				<< (char&)src.mIndex 
				<< src.mPackOrder 
				<< src.mDeclOrder;
		}
		
		friend Stream& operator>> (Stream& src, UnivertPacker& dst)
		{
			return src >> (char&)dst.mPos
				>> (char&)dst.mBasis 
				>> (char&)dst.mBasisScales 
				>> (char&)dst.mNorm 
				>> (char&)dst.mUV 
				>> (char&)dst.mIndices 
				>> (char&)dst.mWeights 
				>> (char&)dst.mDiffuse 
				>> (char&)dst.mIndex
				>> dst.mPackOrder
				>> dst.mDeclOrder;
		}
		
		

	protected:
		VertexElement::EType mPos;					// written as (x,y,z,1)
		VertexElement::EType mBasis;				// written as (x,y,z,s) where s is scale
		VertexElement::EType mBasisScales;	// written as (ts,bs,0,0)
		VertexElement::EType mNorm;				// written as (x,y,z,0)
		VertexElement::EType mUV;					// written as (u,v,0,0)
		VertexElement::EType mIndices;			// written as (a,b,c,d)
		VertexElement::EType mWeights;			// written as (a,b,c,d)
		VertexElement::EType mDiffuse;			// written as (r,g,b,a)
		VertexElement::EType mIndex;				// written as (i,0,0,0)
		
		BigString mPackOrder;	
		BigString mDeclOrder;	
	}; // struct UnivertPacker
		
} // namespace gr

#endif // UNIVERT_PACKER_H
