// File: ugx_instancer.h
#pragma once
#ifndef UGX_INSTANCER_H
#define UGX_INSTANCER_H

#include "common/geom/unigeom.h"
#include "common/geom/basis_vector_creator.h"
#include "ugx_geom.h"

namespace gr
{
	class SegmentSectionTri;
	
	class UGXInstancer
	{
	public:
		UGXInstancer(const Unigeom::Geom& geom, LogFile& log) :
			mGeom(geom),
			mLog(log)
		{
			build();
		}
		
		const UGXGeom::Geom& geom(void) const
		{
			return mOutGeom;
		}
			
	private:
		// types
		
		struct BuildSection
		{
			IntVec mTriIndices;
			int mFirstOutTri;
			int mFirstOutVert;
			int mNumOutTris;
			int mNumOutVerts;
			BigString mBaseVertPackOrder;
			BigString mMorphVertPackOrder;
			
			BuildSection() :
				mFirstOutTri(0),
				mFirstOutVert(0),
				mNumOutTris(0),
				mNumOutVerts(0)
			{
			}
		};
		typedef std::vector<BuildSection> BuildSectionVec;
				
		struct TriMergeVert
		{
			TriMergeVert()
			{
			}
			
			TriMergeVert(int v) : mUnigeomVertex(v)
			{
			}
			
			TriMergeVert(EClear e)
			{
				clear();
			}
			
			void clear(void)
			{
				mUnigeomVertex = 0;
				for (int i = 0; i < Univert::MaxUVCoords; i++)
					for (int j = 0; j < 2; j++)
						mBasisTris[i][j].clear();
			}
			
			operator size_t() const
			{
				Hash hash(&mUnigeomVertex, sizeof(mUnigeomVertex));
								
				for (int set = 0; set < Univert::MaxUVCoords; set++)
					for (int axis = 0; axis < 2; axis++)
						hash.update(&mBasisTris[set][axis][0], static_cast<uint>(mBasisTris[set][axis].size()));
					
				return hash;
			}
			
			friend bool operator== (const TriMergeVert& a, const TriMergeVert& b)
			{
				if (a.mUnigeomVertex != b.mUnigeomVertex)
					return false;
				
				for (int set = 0; set < Univert::MaxUVCoords; set++)					
				{
					for (int axis = 0; axis < 2; axis++)
					{
						if (a.mBasisTris[set][axis].size() != b.mBasisTris[set][axis].size())
							return false;
							
						for (int i = 0; i < a.mBasisTris[set][axis].size(); i++)
						{
							if (a.mBasisTris[set][axis][i] != b.mBasisTris[set][axis][i])
								return false;
						}
					}
				}

				return true;
			}
			
			friend bool operator< (const TriMergeVert& a, const TriMergeVert& b)
			{
				if (a.mUnigeomVertex < b.mUnigeomVertex)
					return true;
				else if (a.mUnigeomVertex != b.mUnigeomVertex)
					return false;
					
				for (int set = 0; set < Univert::MaxUVCoords; set++)					
				{
					for (int axis = 0; axis < 2; axis++)
					{
						if (a.mBasisTris[set][axis].size() < b.mBasisTris[set][axis].size())
							return true;
						else if (a.mBasisTris[set][axis].size() != b.mBasisTris[set][axis].size())
							return false;
								
						for (int i = 0; i < a.mBasisTris[set][axis].size(); i++)
						{
							if (a.mBasisTris[set][axis][i] < b.mBasisTris[set][axis][i])
								return true;
							else if (a.mBasisTris[set][axis][i] != b.mBasisTris[set][axis][i])
								return false;
						}
					}
				}

				return false;
				}
			
			int unigeomVertex(void) const { return mUnigeomVertex; }
			int& unigeomVertex(void)			{ return mUnigeomVertex; }
			
			const IntVec& basisTris(int set, int axis) const	{ return mBasisTris[DebugRange(set, static_cast<int>(Univert::MaxUVCoords))][DebugRange(axis, 2)]; }
						IntVec& basisTris(int set, int axis)				{ return mBasisTris[DebugRange(set, static_cast<int>(Univert::MaxUVCoords))][DebugRange(axis, 2)]; }
						
		private:
			int mUnigeomVertex;
			IntVec mBasisTris[Univert::MaxUVCoords][2];
		};

		typedef std::vector<TriMergeVert> TriMergeVertVec;
		typedef Unifier<TriMergeVert> TriMergeVertUnifier;
		
		// member vars
		
		const Unigeom::Geom& mGeom;
				
		UnivertAttributes mUsedAttributes;
		BuildSectionVec mBuildSections;						
		UGXGeom::Geom mOutGeom;
		LogFile& mLog;
		BasisVectorCreator::TriVertBasisContributorsVec mTriVertBasisContributors[Univert::MaxUVCoords];
		TriMergeVertVec mOutputVerts;
		IndexedTriVec mOutputTris;
		std::vector<IntVec> mUVChannelMap;	// UVChannelMap[material][outputChannel] = inputChannel

		// methods
		
		void createTriVertBasisContributors(void);
		void dumpInputMaterials(void);
		void dumpOutputMaterials(void);
		
		void buildInit(void);
		
		void segmentSection(int materialIndex, int firstTri, int numTris);
		void findSections(void);
		
		void createOutputMaterials(void);
		
		void sortSections(void);
		
		void createSectionRawMesh(
			TriMergeVertUnifier& triMergeVertUnifier,
			IndexedTriVec& rawTris,
			Unifier<int>& unigeomVertUnifier,
			const int sectionIndex);
			
		void createOptimizedMeshes(void);
						
		Univert createBaseOutputVertex(
			const TriMergeVert& triMergeVert, 
			const IntVec& uvChannelMap,
			int numBumpMaps,
			const std::vector<BasisVectorCreator::TriBasisVec>& triBasisVec,
			const Unigeom::Material& oldMat);
			
		BigString createBaseVertexPackOrder(const UGXGeom::Section& section);
			
		BigString createMorphVertexPackOrder(const UGXGeom::Section& section);
			
		void createTriBasisVecs(
			std::vector<BasisVectorCreator::TriBasisVec>& triBasisVec, 
			int keyFrameIndex);
		
		void createBaseOutputGeom(void);
		
		Univert createMorphOutputVertex(
			int keyFrameIndex,
			const TriMergeVert& triMergeVert, 
			int numBumpMaps,
			const std::vector<BasisVectorCreator::TriBasisVec>& triBasisVec,
			const Unigeom::Material& oldMat);

		void findAABB(void);
		void findBoneBounds(void);
		void createMorphOutputGeom(void);
		void createOutputIndices(void);
		void createOutputGeom(void);
		void createOutputBones(void);
			
		void createBuildSection(
			int first, 
			int numTris, 
			int materialIndex,
			bool& allSectionsRigid,
			const std::vector<SegmentSectionTri>& segmentSectionVec);
								
		void build(void);
	};
		
} // namespace gr

#endif // UGX_INSTANCER_H



