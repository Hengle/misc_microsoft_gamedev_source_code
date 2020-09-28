//-----------------------------------------------------------------------------
// File: univert.h
// Universal Vertex Type
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef UNIVERT_H

#include "common/math/vector.h"
#include "common/utils/utils.h"
#include "common/utils/hash.h"
#include "common/utils/stream.h"
#include "common/utils/logfile.h"
#include "common/utils/string.h"
#include "common/geom/vertex_element.h"

namespace gr
{
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
	
	struct UnivertAttributes
	{
		bool pos;
		int numBasis;
		bool norm;
		int numUVSets;
		bool indices;
		bool weights;
		bool diffuse;
		bool index;

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
			clear();
		}

		enum EAllSet { eAllSet };
		UnivertAttributes(EAllSet e)
		{
			setAll();
		}

		UnivertAttributes(Stream& src)
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
		
		BigString& getDescStr(BigString& dst) const;
				
		int size(void) const;

		friend UnivertAttributes operator& (const UnivertAttributes& a, const UnivertAttributes& b)
		{
			UnivertAttributes ret;
			ret.pos = a.pos & b.pos;
			ret.numBasis = Math::Min(a.numBasis, b.numBasis);
			ret.norm = a.norm & b.norm;
			ret.numUVSets = Math::Min(a.numUVSets, b.numUVSets);
			ret.indices = a.indices & b.indices;
			ret.weights = a.weights & b.weights;
			ret.diffuse = a.diffuse & b.diffuse;
			ret.index = a.index & b.index;
			return ret;
		}

		friend UnivertAttributes operator| (const UnivertAttributes& a, const UnivertAttributes& b)
		{
			UnivertAttributes ret;
			ret.pos = a.pos | b.pos;
			ret.numBasis = Math::Max(a.numBasis, b.numBasis);
			ret.norm = a.norm | b.norm;
			ret.numUVSets = Math::Max(a.numUVSets, b.numUVSets);
			ret.indices = a.indices | b.indices;
			ret.weights = a.weights | b.weights;
			ret.diffuse = a.diffuse | b.diffuse;
			ret.index = a.index | b.index;
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

		friend Stream& operator<< (Stream& dest, const UnivertAttributes& a) 
		{
			dest.writeObj(a);
			return dest;
		}

		friend Stream& operator>> (Stream& src, UnivertAttributes& a) 
		{
			src.readObj(a);
			return src;
		}
		
		void log(LogFile& log) const
		{
			log.printf("UnivertAttributes: pos: %i, numBasis: %i, norm: %i, numUVSets: %i,\n  indices: %i, weights: %i, diffuse: %i, index: %i\n",
				pos, numBasis, norm, numUVSets, indices, weights, diffuse, index);
		}
	};
			
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

	struct Univert
	{
		Vec<3> p;
		
		enum { MaxBasisVecs = 2 };

		Vec<4> t[MaxBasisVecs];
		Vec<4> s[MaxBasisVecs];
		
		Vec<3> n;
		
		enum { MaxUVCoords = 4 };
		Vec<2> uv[MaxUVCoords];

		enum { MaxInfluences = 4 };
		enum { DefaultBoneIndex = 0xff };
		uint8 indices[MaxInfluences];
		Vec<MaxInfluences> weights;

		Vec<4> d;
		
		int index;

		Univert()
		{
		}

		Univert(const Vec<3>& pos)
		{
			clear();
			p = pos;
		}

		Univert(Stream& src)
		{
			src >> *this;
		}

		Univert(Stream& src, const UnivertAttributes& attr)
		{
			read(src, attr);
		}
		
		Univert(EClear dummy)
		{
			clear();
		}
		
		const Vec<4>& tangent(int set) const { return t[DebugRange(set, MaxBasisVecs)]; }
					Vec<4>& tangent(int set)				{ return t[DebugRange(set, MaxBasisVecs)]; }
					
		const Vec<4>& binormal(int set) const { return s[DebugRange(set, MaxBasisVecs)]; }
					Vec<4>& binormal(int set)				{ return s[DebugRange(set, MaxBasisVecs)]; }
		        			 
		const Vec<2>& texcoord(int set) const	{ return uv[DebugRange<int, int>(set, MaxUVCoords)]; }
					Vec<2>& texcoord(int set)				{ return uv[DebugRange<int, int>(set, MaxUVCoords)]; }

		uint8		boneIndex(uint i) const	{ return indices[DebugRange<int, int>(i, MaxInfluences)]; }
		uint8&	boneIndex(uint i)				{ return indices[DebugRange<int, int>(i, MaxInfluences)]; }

		float		boneWeight(uint i) const	{ return weights[DebugRange<int, int>(i, MaxInfluences)]; }
		float&	boneWeight(uint i)				{ return weights[DebugRange<int, int>(i, MaxInfluences)]; }
				
		void clear(void)
		{
			Utils::ClearObj(*this);
			
			indices[0] = indices[1] = indices[2] = indices[3] = DefaultBoneIndex;
			weights[0] = 1.0f;
		}

		operator size_t() const
		{
			return ComputeHash(this, sizeof(*this));
		}

		operator Hash() const
		{
			return Hash(this, sizeof(*this));
		}

		operator Vec<3>() const
		{
			return p;
		}

		// Assumes weights are sorted!
		int numActualInfluences(void) const
		{
			for (int i = 0; i < MaxInfluences; i++)
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

			if (index != b.index)
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

			if (index < b.index)
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

			std::copy(indices, indices + MaxInfluences, ret.indices);

			ret.d = snap(d, tol.dTol);
			return ret;
		}

		Univert select(const UnivertAttributes& attr) const
		{
			Univert ret;
			ret.clear();
			
			if (attr.pos)			{ ret.p = p; }
			
			std::copy(s, s + attr.numBasis, ret.s);
			std::copy(t, t + attr.numBasis, ret.t);
			
			if (attr.norm)		{ ret.n = n; }
			
			if (attr.weights) { ret.weights = weights; }
			if (attr.indices) { std::copy(indices, indices + MaxInfluences, ret.indices); }
			if (attr.diffuse) { ret.d = d; }
			if (attr.index)		{	ret.index = index; }

			std::copy(uv, uv + attr.numUVSets, ret.uv);
			
			return ret;
		}
				
		uint8* pack(const UnivertAttributes& attr, uint8* pDest) const
		{
			if (attr.pos)			{ writeObj(pDest, p); }
			
			for (int i = 0; i < attr.numBasis; i++)
			{
				writeObj(pDest, s[i]); 
				writeObj(pDest, t[i]);
			}

			if (attr.norm)		{ writeObj(pDest, n); }
			
			if (attr.weights) { writeObj(pDest, weights); }
			if (attr.indices) { writeObj(pDest, indices); }
			if (attr.diffuse) { writeObj(pDest, d); }
			if (attr.index)		{	writeObj(pDest, index); }

			for (int i = 0; i < attr.numUVSets; i++)
				writeObj(pDest, uv[i]);
			
			return pDest;
		}

		const uint8* unpack(const UnivertAttributes& attr, const uint8* pSrc)
		{
			if (attr.pos)			{ readObj(pSrc, p); }
			
			for (int i = 0; i < attr.numBasis; i++)
			{
				readObj(pSrc, s[i]); 
				readObj(pSrc, t[i]);
			}

			if (attr.norm)		{ readObj(pSrc, n); }
			
			if (attr.weights) { readObj(pSrc, weights); }
			if (attr.indices) { readObj(pSrc, indices); }
			if (attr.diffuse) { readObj(pSrc, d); }
			if (attr.index)		{	readObj(pSrc, index); }

			for (int i = 0; i < attr.numUVSets; i++)
				readObj(pSrc, uv[i]);

			return pSrc;
		}

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
			
			if (Vec<MaxInfluences>(1.0f, 0.0f, 0.0f, 0.0f) != weights) ret.weights = true;
			
			for (int i = 0; i < MaxInfluences; i++)
				if (indices[i] != DefaultBoneIndex)
				{
					ret.indices = true;
					break;
				}

			if (index)
				ret.index = true;
        						
			for (int i = MaxUVCoords - 1; i >= 0; i--)
				if (!uv[i].isZero())
				{
					ret.numUVSets = i + 1;
					break;
				}	

			return ret;
		}

		friend Stream& operator<< (Stream& pDest, const Univert& a)
		{
			pDest.writeObj(a);
			return pDest;
		}

		friend Stream& operator>> (Stream& src, Univert& a)
		{
			src.readObj(a);
			return src;
		}

		Stream& write(Stream& dest, const UnivertAttributes& attr) const
		{
			if (attr.pos)			{ dest.writeObj(p); }
						
			for (int i = 0; i < attr.numBasis; i++)
			{
				dest.writeObj(s[i]);
				dest.writeObj(t[i]);
			}

			if (attr.norm)		{ dest.writeObj(n); }
			
			if (attr.weights) { dest.writeObj(weights); }
			if (attr.indices) { dest.writeObj(indices); }
			if (attr.diffuse)	{ dest.writeObj(d); }
			if (attr.index)		{	dest.writeObj(index); }

			for (int i = 0; i < attr.numUVSets; i++)
				dest.writeObj(uv[i]);
      return dest;				
		}

		Stream& read(Stream& src, const UnivertAttributes& attr) 
		{
			clear();
			if (attr.pos)			{ src.readObj(p); }
						
			for (int i = 0; i < attr.numBasis; i++)
			{
				src.readObj(s[i]);
				src.readObj(t[i]);
			}

			if (attr.norm)		{ src.readObj(n); }
			
			if (attr.weights) { src.readObj(weights); }
			if (attr.indices) { src.readObj(indices); }
			if (attr.diffuse) { src.readObj(d); }
			if (attr.index)		{	src.readObj(index); }

			for (int i = 0; i < attr.numUVSets; i++)
				src.readObj(uv[i]);
      return src;				
		}

		static int size(const UnivertAttributes& attr) 
		{
			int ret = 0;
			Univert dummy;
		
			if (attr.pos)			{ ret += sizeof(dummy.p); }
			
			ret += (sizeof(dummy.s[0]) + sizeof(dummy.t[0])) * attr.numBasis;
			
			if (attr.norm)		{ ret += sizeof(dummy.n); }
			
			if (attr.weights)	{ ret += sizeof(dummy.weights); }
			if (attr.indices)	{ ret += sizeof(dummy.indices); }
			if (attr.diffuse)	{ ret += sizeof(dummy.d); }
			if (attr.index)		{	ret += sizeof(dummy.index); }

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
		
	private:
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

	typedef std::vector<Univert> UnivertVec;

	struct UnivertEqualTolHashCompare
	{	
		// parameters for hash table
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
		pos = norm = indices = weights = diffuse = index = true;
		numBasis = Univert::MaxBasisVecs;
		numUVSets = Univert::MaxUVCoords;
		return *this;
	}

	inline int UnivertAttributes::size(void) const 
	{
		return Univert::size(*this);
	}
	
	inline BigString& UnivertAttributes::getDescStr(BigString& dst) const
	{
		dst.clear();
		if (pos)
			dst += "P";
		for (int i = 0; i < numBasis; i++)
			dst += BigString(eVarArg, "B%i", i);
		if (norm)
			dst += "N";
		for (int i = 0; i < numUVSets; i++)
			dst += BigString(eVarArg, "T%i", i);
		if ((weights) || (indices))
			dst += "S";
		if (diffuse)
			dst += "D";
		if (index)
			dst += "I";
		return dst;
	}
	
	inline UnivertAttributes& UnivertAttributes::set(const char* pDescStr)
	{
		DebugNull(pDescStr);
		
		clear();
		
		while (*pDescStr)
		{
			const char c = toupper(*pDescStr++);
			switch (c)
			{
				case ePOS_SPEC:
					pos = true;
					break;
				case eBASIS_SPEC:
				{
					const int n = DebugRange(*pDescStr++ - '0', Univert::MaxBasisVecs);
					numBasis = Math::Max(numBasis, n + 1);
					break;
				}
				case eNORM_SPEC:
					norm = true;
					break;
				case eTEXCOORDS_SPEC:
				{
					const int n = DebugRange(*pDescStr++ - '0', static_cast<int>(Univert::MaxUVCoords));
					numUVSets = Math::Max(numUVSets, n + 1);
					break;
				}
				case eSKIN_SPEC:
					indices = true;
					weights = true;
					break;
				case eDIFFUSE_SPEC:
					diffuse = true;
					break;
				case eINDEX_SPEC:
					index = true;
					break;
				default:
					Assert(false);
			}
		}
	}

	class Unitri
	{
	public:
		Unitri()
		{
			clear();
		}

		void clear(void)
		{
			mVerts[0].clear();
			mVerts[1].clear();
			mVerts[2].clear();
		}

		const Univert& operator[] (int i) const { return mVerts[DebugRange(i, 3)]; }
					Univert& operator[] (int i)				{ return mVerts[DebugRange(i, 3)]; }

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

		Stream& write(Stream& dest, const UnivertAttributes& attr) const
		{
			mVerts[0].write(dest, attr);
			mVerts[1].write(dest, attr);
			mVerts[2].write(dest, attr);
			return dest;
		}

		Stream& read(Stream& src, const UnivertAttributes& attr) 
		{
			mVerts[0].read(src, attr);
			mVerts[1].read(src, attr);
			mVerts[2].read(src, attr);
			return src;
		}

		Stream& write(Stream& dst) const
		{
			return dst << mVerts[0] << mVerts[1] << mVerts[2];
		}

		Stream& read(Stream& dst)
		{
			return dst >> mVerts[0] >> mVerts[1] >> mVerts[2];
		}

		friend Stream& operator<< (Stream& dst, const Unitri& src)
		{
			dst << src.mVerts[0] << src.mVerts[1] << src.mVerts[2];
		}

		friend Stream& operator>> (Stream& src, Unitri& dst)
		{
			src >> dst.mVerts[0] >> dst.mVerts[1] >> dst.mVerts[2];
		}

	protected:
		Univert mVerts[3];
	};

	typedef std::vector<Unitri> UnitriVec;
		
} // end namespace gr

#endif // UNIVERT_H
