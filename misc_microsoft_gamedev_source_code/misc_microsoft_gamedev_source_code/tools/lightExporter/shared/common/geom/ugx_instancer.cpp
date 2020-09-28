// File: ugx_instancer.cpp
#include "ugx_instancer.h"
#include "univert_packer.h"
#include "common/utils/logfile.h"
#include "common/geom/trilist_optimizer.h"

namespace gr
{
	void UGXInstancer::createTriVertBasisContributors(void)
	{
		if (!mUsedAttributes.numUVSets)
			return;
		
		IndexedTriVec tris(mGeom.numTris());
		for (int i = 0; i < mGeom.numTris(); i++)
			tris[i] = mGeom.tri(i).indexedTri();
				
		for (int i = 0; i < mUsedAttributes.numUVSets; i++)
		{
			mLog.printf("UGXInstancer::createTriVertBasisContributors: Building contributors for input UV channel %i\n", i);
			
			BasisVectorCreator::createTriVertBasisContributors(
				mTriVertBasisContributors[i],
				tris,
				mGeom.vertVec(),
				i);
		}
	}
	
	void UGXInstancer::dumpInputMaterials(void)
	{
		for (int i = 0; i < mGeom.numMaterials(); i++)
		{
			mLog.printf("Input Material: %i ", i);
			mGeom.material(i).log(mLog);
		}
	}
	
	void UGXInstancer::dumpOutputMaterials(void)
	{
		for (int i = 0; i < mOutGeom.numMaterials(); i++)
		{
			mLog.printf("Output Material: %i ", i);
			mOutGeom.material(i).log(mLog);
		}
	}
	
	void UGXInstancer::buildInit(void)
	{
		mLog.printf("UGXInstancer::buildInit: Unigeom stats: Shadow: %i, Tris: %i, Verts: %i,\n  Ave. Verts Per Tri: %f, Materials: %i, Bones: %i, Morph Targets: %i\n",
			mGeom.shadowGeom(),
			mGeom.numTris(),
			mGeom.numVerts(),
			float(mGeom.numVerts()) / mGeom.numTris(),
			mGeom.numMaterials(),
			mGeom.numBones(),
			mGeom.numMorphTargets());
		
		mUsedAttributes = mGeom.usedAttributes();
		
		// ensure the UV channels used by the materials don't exceed the # of detected channels
		
		for (int matIndex = 0; matIndex < mGeom.numMaterials(); matIndex++)
		{
			const Unigeom::Material& mat = mGeom.material(matIndex);
			
			for (int mapType = 0; mapType < Unigeom::Material::NumMapTypes; mapType++)
			{
				for (int mapIndex = 0; mapIndex < mat.numMaps(mapType); mapIndex++)
				{
					const Unigeom::Map& map = mat.map(mapType, mapIndex);
					const int uvChannel = map.channel();
					// FIXME: If this fires, the exported geom contains a used UV set of all 0's!
					Verify(uvChannel < mUsedAttributes.numUVSets);
				}
			}
		}
		
		mUsedAttributes.log(mLog);
		
		// determine if entire model is rigid only
		
		int rigidBoneIndex = -1;
		int maxBones = 0;
		
		for (int triIndex = 0; triIndex < mGeom.numTris(); triIndex++)
		{
			for (int trivertIndex = 0; trivertIndex < 3; trivertIndex++)
			{
				const int numBones = mGeom.vert(mGeom.tri(triIndex)[trivertIndex]).numActualInfluences();
				maxBones = Math::Max(maxBones, numBones);
				
				if (1 == numBones)
				{
					const int boneIndex = mGeom.triVert(triIndex, trivertIndex).boneIndex(0);
					if (rigidBoneIndex == -1)
						rigidBoneIndex = boneIndex;
					else if (rigidBoneIndex != boneIndex)
						rigidBoneIndex = -2;
				}
				else
					rigidBoneIndex = -2;
			}
		}
		
		mLog.printf("Max bones: %i\n", maxBones);
		
		if (rigidBoneIndex >= 0)
		{
			Assert(1 == maxBones);
			mOutGeom.setRigidOnly(true);
										
			mOutGeom.setRigidBoneIndex((rigidBoneIndex == Univert::DefaultBoneIndex) ? -1 : rigidBoneIndex);
			
			mLog.printf("Model is rigid only, index %i\n", rigidBoneIndex);
		}
		
		dumpInputMaterials();
		
		mOutGeom.IB().reserve(1024*1024*4);
		mOutGeom.VB().reserve(1024*1024*32);
	}
	
	class SegmentSectionTri
	{
		public:
			SegmentSectionTri(int triIndex, bool rigidOnly, int rigidBoneIndex, int numBones) :
				mTriIndex(triIndex),
				mRigidOnly(rigidOnly),
				mRigidBoneIndex(rigidBoneIndex),
				mNumBones(numBones)
			{
			}
				
			int triIndex(void) const				{ return mTriIndex; }
			bool rigidOnly(void) const			{ return mRigidOnly; }
			int rigidBoneIndex(void) const	{ return mRigidBoneIndex; }
			int numBones(void) const				{ return mNumBones; }
			
			void setRigidOnly(bool rigidOnly)						{ mRigidOnly = rigidOnly; }
			void setRigidBoneIndex(int rigidBoneIndex)	{ mRigidBoneIndex = rigidBoneIndex; }
			void setNumBones(int numBones)							{ mNumBones = numBones; }
			
			friend bool operator< (const SegmentSectionTri& lhs, const SegmentSectionTri& rhs)
			{
				if (static_cast<int>(lhs.mRigidOnly) < static_cast<int>(rhs.mRigidOnly))
					return true;
				else if (lhs.mRigidOnly == rhs.mRigidOnly)
				{
					if (lhs.mRigidOnly)
					{
						if (lhs.mRigidBoneIndex < rhs.mRigidBoneIndex)
							return true;
					}
					else if (lhs.mNumBones < rhs.mNumBones)
						return true;
				}
				
				return false;									
			}
			
			friend bool operator== (const SegmentSectionTri& lhs, const SegmentSectionTri& rhs)
			{
				return 
					(lhs.mTriIndex == rhs.mTriIndex) && 
					(lhs.mRigidOnly == rhs.mRigidOnly) &&
					(lhs.mRigidBoneIndex == rhs.mRigidBoneIndex) &&
					(lhs.mNumBones == rhs.mNumBones);
			}
			
			static bool SameCategory(const SegmentSectionTri& lhs, const SegmentSectionTri& rhs)
			{
				if (lhs.mRigidOnly == rhs.mRigidOnly)
				{
					if (lhs.mRigidOnly)
					{
						return lhs.mRigidBoneIndex == rhs.mRigidBoneIndex;
					}
					else
					{
						if (lhs.mNumBones <= 2)
							return lhs.mNumBones == rhs.mNumBones;
						else
							return rhs.mNumBones > 2;
					}
				}
				
				return false;
			}
			
		private:
			int mTriIndex;
			bool mRigidOnly;
			int mRigidBoneIndex;
			int mNumBones;
	};
	
	void UGXInstancer::createBuildSection(
		int first, 
		int numTris,
		int materialIndex,
		bool& allSectionsRigid,
		const std::vector<SegmentSectionTri>& segmentSectionTriVec)
	{
		const int last = first + numTris;
		
		BuildSection buildSection;
		for (int i = first; i < last; i++)
			buildSection.mTriIndices.push_back(segmentSectionTriVec.at(i).triIndex());
		
		const int maxSectionBones = segmentSectionTriVec[first].numBones();
#if SKIN_ONLY				
		const bool rigidOnly = false;//segmentSectionTriVec[first].rigidOnly();
		const int rigidBoneIndex = -1;//rigidOnly ? segmentSectionTriVec[first].rigidBoneIndex() : -1;
#else
		const bool rigidOnly = segmentSectionTriVec[first].rigidOnly();
		const int rigidBoneIndex = rigidOnly ? segmentSectionTriVec[first].rigidBoneIndex() : -1;
#endif				
						
		mLog.printf("Section %i: tris: %i, material: %i, maxBones: %i, rigidOnly: %i, rigidBoneIndex: %i\n",
			mBuildSections.size(),
			buildSection.mTriIndices.size(),
			materialIndex,
			maxSectionBones,
			rigidOnly,
			rigidBoneIndex
			);
												
		mBuildSections.push_back(buildSection);
		
		UGXGeom::Section section;
		
		section.setMaterialIndex(materialIndex);
		section.setMaxBones(maxSectionBones);
						
		if (rigidOnly)
		{ 
			Assert(maxSectionBones == 1);
			section.setRigidOnly(true);
												
			section.setRigidBoneIndex((rigidBoneIndex == Univert::DefaultBoneIndex) ? -1 : rigidBoneIndex);
		}
		else
			allSectionsRigid = false;
						
		mOutGeom.sections().push_back(section);								
	}
	
	// FIXME: This code can output sections which are too small to be efficiently rendered.
	void UGXInstancer::segmentSection(int materialIndex, int firstTri, int numTris)
	{
		Assert(numTris > 0);
		
		mLog.printf("UGXInstancer::createSections: materialIndex: %i, firstTri: %i, numTris: %i\n", materialIndex, firstTri, numTris);

		std::vector<SegmentSectionTri> segmentSectionTriVec;
		
		IntVec rigidHistogram(mGeom.numBones() + 1);
		int numSingleBoneSkinned = 0;
		
		for (int triIndex = firstTri; triIndex < firstTri + numTris; triIndex++)
		{
			int maxBones = 0;
			int triRigidBoneIndex = -1;
			for (int trivertIndex = 0; trivertIndex < 3; trivertIndex++)
			{
				const int numBones = mGeom.triVert(triIndex, trivertIndex).numActualInfluences();
				maxBones = Math::Max(maxBones, numBones);
				
				if (1 == numBones)
				{
					const int boneIndex = mGeom.vert(mGeom.tri(triIndex)[trivertIndex]).boneIndex(0);
					if (-1 == triRigidBoneIndex)
						triRigidBoneIndex = boneIndex;
					else if (triRigidBoneIndex != boneIndex)
						triRigidBoneIndex = -2;
				}
				else
					triRigidBoneIndex = -2;
			}
			
			segmentSectionTriVec.push_back(SegmentSectionTri(triIndex, triRigidBoneIndex >= 0, triRigidBoneIndex, maxBones));
			
			if (triRigidBoneIndex >= 0)
			{
				const int rigidBoneIndex = (Univert::DefaultBoneIndex == triRigidBoneIndex) ? 0 : (triRigidBoneIndex + 1);
				rigidHistogram.at(rigidBoneIndex) = rigidHistogram.at(rigidBoneIndex) + 1;
			}
			else if (1 == maxBones)
				numSingleBoneSkinned++;
		}
		
		const int RigidToSkinnedThreshold = 100;
		
		// FIXME: This code will reassign small, rigid only sections as skinned on rigid-only models
		// that don't have any skinned materials!
		if (!mOutGeom.rigidOnly())
		{
			// rigid tris in sections with too few tris should be skinned
			for (int i = 0; i < segmentSectionTriVec.size(); i++)
			{
				if (segmentSectionTriVec[i].rigidOnly())
				{	
					const int rigidBoneIndex = (Univert::DefaultBoneIndex == segmentSectionTriVec[i].rigidBoneIndex()) ? 0 : (segmentSectionTriVec[i].rigidBoneIndex() + 1);
					if (rigidHistogram.at(rigidBoneIndex) < RigidToSkinnedThreshold)
					{
						segmentSectionTriVec[i].setRigidOnly(false);
						segmentSectionTriVec[i].setRigidBoneIndex(-2);
						segmentSectionTriVec[i].setNumBones(1);
					}
				}
			}
		}
		
		std::sort(segmentSectionTriVec.begin(), segmentSectionTriVec.end());

		int totalTrisProcessed = 0;
		
		bool allSectionsRigid = true;
		
		int first = 0;
		for (int cur = 1; cur <= segmentSectionTriVec.size(); cur++)
		{
			if ((cur == segmentSectionTriVec.size()) || 
					(!SegmentSectionTri::SameCategory(segmentSectionTriVec.at(first), segmentSectionTriVec.at(cur))))
			{
				int numTrisLeft = cur - first;
																
				while (numTrisLeft)
				{
					const int numTrisToProcess = Math::Min<int>(numTrisLeft, UGXGeom::Section::MaxTris);
					
					createBuildSection(first, numTrisToProcess, materialIndex, allSectionsRigid, segmentSectionTriVec);
					
					totalTrisProcessed += numTrisToProcess;
					
					first += numTrisToProcess;
					numTrisLeft -= numTrisToProcess;
				}
			}
		}
		
		if (!allSectionsRigid)
			mOutGeom.setAllSectionsRigid(false);
										
		Verify(totalTrisProcessed == numTris);
	}
	
	void UGXInstancer::findSections(void)
	{
		mOutGeom.setAllSectionsRigid(true);
		
		int prevMaterialIndex = -1;
		int firstTri = 0;
		
		// assumes tris are already sorted by material indices
		for (int i = 0; i <= mGeom.numTris(); i++)
		{
			const int triMaterialIndex = mGeom.shadowGeom() ? 0 : mGeom.tri(i).materialIndex();
			const int curMaterialIndex = (i < mGeom.numTris()) ? triMaterialIndex : -2;
			
			const int curNumTris = i - firstTri;
			
			//if ((prevMaterialIndex != curMaterialIndex) || (curNumTris >= UGXGeom::Section::MaxTris))
			if (prevMaterialIndex != curMaterialIndex)
			{
				if (-1 != prevMaterialIndex)
				{
					segmentSection(prevMaterialIndex, firstTri, DebugNull(curNumTris));
					
					firstTri = i;
				}

				prevMaterialIndex = curMaterialIndex;
			}
		}
		
		mLog.printf("AllSectionsRigid: %i\n", mOutGeom.allSectionsRigid());
	}
	
	void UGXInstancer::createOutputMaterials(void)
	{
		mUVChannelMap.resize(mGeom.numMaterials());
		
		for (int materialIndex = 0; materialIndex < mGeom.numMaterials(); materialIndex++)
		{
			const Unigeom::Material& oldMat = mGeom.material(materialIndex);
			Unigeom::Material newMat(oldMat);
			
			if (mGeom.shadowGeom())
			{
				for (int i = 0; i < Unigeom::Material::MaxMapsPerType; i++)
					newMat.setNumMaps(i, 0);
			}
			else
			{

// Diffuse: 1 Spec: 1 Bump: 0 UVChannels: 0
// Diffuse: 1 Spec: 1 Bump: 1 UVChannels: 0,1
// Diffuse: 1 Spec: 1 Bump: 2 UVChannels: 0,1,2
// Diffuse: 2 Spec: 1 Bump: 0 UVChannels: 0,1
// Diffuse: 2 Spec: 1 Bump: 1 UVChannels: 0,1,2
// Diffuse: 2 Spec: 1 Bump: 2 UVChannels: 0,1,2,3			

				for (int i = 0; i < Unigeom::Material::MaxMapsPerType; i++)
				{
					if (newMat.numMaps(i) > 2)
					{
						newMat.setNumMaps(i, 2);
						mLog.printf("WARNING: Material %i: Truncating map type %i (%s) down to 2 textures!\n",
							materialIndex, Unigeom::Material::mapName(i));
					}
				}
				
				// Is this right?
				if (newMat.numMaps(Unigeom::Material::eSpecular) < 1)
				{
					newMat.setNumMaps(Unigeom::Material::eSpecular, 1);
					
					mLog.printf("WARNING: Material %i: Forcing specular map to 1 texture!\n", materialIndex);
				}
							
				if (newMat.numMaps(Unigeom::Material::eDiffuse) < 2)
				{
					newMat.setNumMaps(Unigeom::Material::eDiffuse, 2);
					mLog.printf("WARNING: Material %i: Forcing diffuse map to 2 textures!\n", materialIndex);
				}
				
				IntVec& uvChannelMap = mUVChannelMap[materialIndex];
				
				int nextFreeChannel = 0;
				for (int i = 0; i < newMat.numMaps(Unigeom::Material::eDiffuse); i++)
				{
					uvChannelMap.push_back(oldMat.map(Unigeom::Material::eDiffuse, i).channel());
					
					newMat.map(Unigeom::Material::eDiffuse, i).setChannel(nextFreeChannel);
					
					nextFreeChannel++;
				}
				
				for (int i = 0; i < newMat.numMaps(Unigeom::Material::eBump); i++)
				{
					uvChannelMap.push_back(oldMat.map(Unigeom::Material::eBump, i).channel());
				
					newMat.map(Unigeom::Material::eBump, i).setChannel(nextFreeChannel);
					
					nextFreeChannel++;
				}
				
				newMat.map(Unigeom::Material::eSpecular, 0).setChannel(0);
				
				if ((oldMat.numMaps(Unigeom::Material::eDiffuse) >= 1) &&
						(oldMat.numMaps(Unigeom::Material::eSpecular) >= 1))
				{
					if (oldMat.map(Unigeom::Material::eDiffuse, 0).channel() != 
							oldMat.map(Unigeom::Material::eSpecular, 0).channel())
					{
						mLog.printf("WARNING: Material %i: Specular map's UV channel does not match the first diffuse map's!\n", materialIndex);
					}
				}
				
				mLog.printf("UV channel map:\n");
				for (int i = 0; i < uvChannelMap.size(); i++)
					mLog.printf("	 Out: %i In: %i\n", i, uvChannelMap[i]);
			}
			
			mOutGeom.materials().push_back(newMat);
		}
		
		dumpOutputMaterials();
	}
	
	void UGXInstancer::sortSections(void)
	{
		for (int sectionIndex = 0; sectionIndex < mBuildSections.size(); sectionIndex++)
		{
			BuildSection& buildSection = mBuildSections[sectionIndex];
			const UGXGeom::Section& section = mOutGeom.section(sectionIndex);
			
			buildSection.mBaseVertPackOrder = createBaseVertexPackOrder(section);
			buildSection.mMorphVertPackOrder = createMorphVertexPackOrder(section);
		}
		
		for (int i = 0; i < mBuildSections.size() - 1; i++)
		{
			BuildSection& buildSectionA = mBuildSections[i];
			UGXGeom::Section& sectionA = mOutGeom.section(i);
				
			for (int j = i + 1; j < mBuildSections.size(); j++)
			{
				BuildSection& buildSectionB = mBuildSections[j];
				UGXGeom::Section& sectionB = mOutGeom.section(j);
				
				bool swap = false;
				if (buildSectionA.mBaseVertPackOrder > buildSectionB.mBaseVertPackOrder)
					swap = true;
				else if (buildSectionA.mBaseVertPackOrder == buildSectionB.mBaseVertPackOrder)
				{
					if (buildSectionA.mMorphVertPackOrder > buildSectionB.mMorphVertPackOrder)
						swap = true;
					else if (buildSectionA.mMorphVertPackOrder == buildSectionB.mMorphVertPackOrder)
					{
						if (sectionA.materialIndex() > sectionB.materialIndex())
							swap = true;
					}
				}
				
				if (swap)
				{
					std::swap(buildSectionA, buildSectionB);
					std::swap(sectionA, sectionB);
				}
			}
		}
	}
	
	void UGXInstancer::createSectionRawMesh(
		TriMergeVertUnifier& triMergeVertUnifier,
		IndexedTriVec& rawTris,
		Unifier<int>& unigeomVertUnifier,
		const int sectionIndex)
	{
		BuildSection& buildSection = mBuildSections[sectionIndex];
		UGXGeom::Section& section = mOutGeom.section(sectionIndex);
		const Unigeom::Material& oldMat = mGeom.material(section.materialIndex());
		const Unigeom::Material& newMat = mOutGeom.material(section.materialIndex());
		
		mLog.printf("createSectionRawMesh: Remerging verts in section %i, %i bump map(s)\n", sectionIndex, newMat.numMaps(Unigeom::Material::eBump));
		
		rawTris.resize(buildSection.mTriIndices.size());
	
		const int numTris = static_cast<int>(buildSection.mTriIndices.size());
		
		for (int triIndex = 0; triIndex < numTris; triIndex++)
		{
			const unigeomTriIndex = buildSection.mTriIndices[triIndex];
			const IndexedTri& tri = mGeom.tri(unigeomTriIndex).indexedTri();
			
			IndexedTri outTri;
									
			for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
			{
				const int unigeomVertIndex = tri[triVertIndex];
				
				unigeomVertUnifier.insert(unigeomVertIndex);
				
				TriMergeVert triMergeVert(unigeomVertIndex);
												
				for (int bumpIndex = 0; bumpIndex < newMat.numMaps(Unigeom::Material::eBump); bumpIndex++)
				{
					const int oldUVChannel = oldMat.map(Unigeom::Material::eBump, bumpIndex).channel();
					
					for (int axis = 0; axis < 2; axis++)
					{
						triMergeVert.basisTris(bumpIndex, axis) = 
							mTriVertBasisContributors[DebugRange<int, int>(oldUVChannel, Univert::MaxUVCoords)].at(unigeomTriIndex).
								axisTris(axis, triVertIndex);
					}
				}
				
				outTri[triVertIndex] = triMergeVertUnifier.insert(triMergeVert).first;
			}
			
			rawTris[triIndex] = outTri.canonicalize();
		}	
	}
	
	void UGXInstancer::createOptimizedMeshes(void)
	{
		for (int sectionIndex = 0; sectionIndex < mBuildSections.size(); sectionIndex++)
		{
			BuildSection& buildSection = mBuildSections[sectionIndex];
			UGXGeom::Section& section = mOutGeom.section(sectionIndex);
						
			TriMergeVertUnifier triMergeVertUnifier;
			IndexedTriVec rawTris;
			Unifier<int> unigeomVertUnifier;
			
			createSectionRawMesh(triMergeVertUnifier,	rawTris, unigeomVertUnifier, sectionIndex);
									
			const int numTris = static_cast<int>(buildSection.mTriIndices.size());
			const int numVerts = triMergeVertUnifier.size();
			
			buildSection.mFirstOutTri = static_cast<int>(mOutputTris.size());
			buildSection.mFirstOutVert = static_cast<int>(mOutputVerts.size());
							
			mLog.printf("  Optimizing indices\n");
			
			TriListOptimizer listOptimizer(rawTris);
			const IntVec& optimizedTriOrder = listOptimizer.triOrder();
			
			IntVec oldToNew(numVerts);
			std::fill(oldToNew.begin(), oldToNew.end(), -1);
			
			for (int triIndex = 0; triIndex < listOptimizer.numTris(); triIndex++)
			{
				const int reorderedTriIndex = DebugRange(optimizedTriOrder[triIndex], numTris);
				
				IndexedTri newTri;
				for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
				{
					const int oldVertIndex = rawTris[reorderedTriIndex][triVertIndex];
					int newVertIndex = oldToNew[DebugRange(oldVertIndex, oldToNew.size())];
					if (-1 == newVertIndex)
					{
						newVertIndex = static_cast<int>(mOutputVerts.size()) - buildSection.mFirstOutVert;
						oldToNew[oldVertIndex] = newVertIndex;
						mOutputVerts.push_back(triMergeVertUnifier[oldVertIndex]);
					}
															
					newTri[triVertIndex] = newVertIndex;
				}
			
				mOutputTris.push_back(newTri);	
			}
			
			Assert((mOutputVerts.size() - buildSection.mFirstOutVert) == numVerts);
			
			buildSection.mNumOutTris = listOptimizer.numTris();
			buildSection.mNumOutVerts = static_cast<int>(mOutputVerts.size()) - buildSection.mFirstOutVert;
			Assert(buildSection.mNumOutVerts <= buildSection.mTriIndices.size() * 3);
										
			mLog.printf("  Input Tris: %i, Input Verts: %i\n", buildSection.mTriIndices.size(), unigeomVertUnifier.size());
			mLog.printf("  Output Verts: %i, Input/Output Vertex Ratio: %f\n", buildSection.mNumOutVerts,
				float(buildSection.mNumOutVerts) / unigeomVertUnifier.size());
		}
	}
		
	Univert UGXInstancer::createBaseOutputVertex(
		const TriMergeVert& triMergeVert, 
		const IntVec& uvChannelMap,
		int numBumpMaps,
		const std::vector<BasisVectorCreator::TriBasisVec>& triBasisVec,
		const Unigeom::Material& oldMat)
	{
		const Univert& src = mGeom.vert(triMergeVert.unigeomVertex());
		
		Univert dst(src);

		for (int dstUVChannel = 0; dstUVChannel < uvChannelMap.size(); dstUVChannel++)
		{
			const int srcUVChannel = uvChannelMap[dstUVChannel];
			dst.texcoord(dstUVChannel) = src.texcoord(srcUVChannel);
		}
		
		for (int bumpIndex = 0; bumpIndex < numBumpMaps; bumpIndex++)
		{
			BasisVectorCreator::generateBasis(
				dst.tangent(bumpIndex),
				dst.binormal(bumpIndex),
				dst.n,
				triMergeVert.basisTris(bumpIndex, 0),
				triMergeVert.basisTris(bumpIndex, 1),
				triBasisVec[oldMat.map(Unigeom::Material::eBump, bumpIndex).channel()]
			);
		}
		
		for (int i = 0; i < Univert::MaxInfluences; i++)
		{
			int boneIndex = dst.boneIndex(i);
			if (boneIndex == Univert::DefaultBoneIndex)
				boneIndex = -1;
			
			dst.boneIndex(i) = boneIndex + 1;
		}
		
		return dst;
	}
	
	BigString UGXInstancer::createBaseVertexPackOrder(const UGXGeom::Section& section)
	{
		const Unigeom::Material& newMat = mOutGeom.material(section.materialIndex());
		const IntVec& uvChannelMap = mUVChannelMap[section.materialIndex()];
			
		BigString packOrder;
		
		if (1 == mGeom.numMorphTargets())
		{
			packOrder += "P";
			
#if SHADOW_MESH_NORMALS
			packOrder += "N";
#endif			
			
			if (!mGeom.shadowGeom())
			{
#if !SHADOW_MESH_NORMALS			
				packOrder += "N";
#endif				
			
				for (int i = 0; i < newMat.numMaps(Unigeom::Material::eBump); i++)
					packOrder += BigString(eVarArg, "B%iX%i", i, i);
			}
		}
		
		if (!section.rigidOnly())
			packOrder += "S";
			
		if (!mGeom.shadowGeom())
		{
			const int numUVCoords = static_cast<int>(uvChannelMap.size());
			for (int i = 0; i < numUVCoords; i++)
				packOrder += BigString(eVarArg, "T%i", i);
		}
			
		return packOrder;
	}
	
	BigString UGXInstancer::createMorphVertexPackOrder(const UGXGeom::Section& section)
	{
		BigString packOrder;
		if (1 == mGeom.numMorphTargets())
			return packOrder;
								
		packOrder += "P";
		
#if SHADOW_MESH_NORMALS
		packOrder += "N";
#endif		
			
		if (!mGeom.shadowGeom())
		{
#if !SHADOW_MESH_NORMALS		
			packOrder += "N";
#endif			
		
			const Unigeom::Material& newMat = mOutGeom.material(section.materialIndex());	
			for (int i = 0; i < newMat.numMaps(Unigeom::Material::eBump); i++)
				packOrder += BigString(eVarArg, "B%i", i);
		}
		
		return packOrder;
	}
	
	void UGXInstancer::createTriBasisVecs(
		std::vector<BasisVectorCreator::TriBasisVec>& triBasisVec, 
		int keyFrameIndex)
	{
		triBasisVec.resize(mUsedAttributes.numUVSets);
		for (int bumpIndex = 0; bumpIndex < mUsedAttributes.numUVSets; bumpIndex++)
		{
			triBasisVec[bumpIndex].resize(mGeom.numTris());
	
			for (int triIndex = 0; triIndex < mGeom.numTris(); triIndex++)
			{
				Vec3 verts[3];
				Vec2 texcoords[3];
				
				for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
				{
					const Univert& unigeomVert = mGeom.triVert(triIndex, triVertIndex);

					if (0 == keyFrameIndex)
						verts[triVertIndex] = unigeomVert.p;
					else
						verts[triVertIndex] = 
							mGeom.morphTarget(keyFrameIndex)[mGeom.tri(triIndex)[triVertIndex]].p;
						
					texcoords[triVertIndex] = unigeomVert.texcoord(bumpIndex);
				}
				
				triBasisVec[bumpIndex][triIndex].set(verts, texcoords);
			}
		}
	}
		
	void UGXInstancer::createBaseOutputGeom(void)
	{
		UnivertPacker vertPacker;
		vertPacker.setBasis(VertexElement::eDEC3N);
		//vertPacker.setBasisScales(VertexElement::eHALFFLOAT2);
		vertPacker.setBasisScales(VertexElement::eFLOAT2);		
		vertPacker.setNorm(VertexElement::eDEC3N);
		vertPacker.setIndices(VertexElement::eUBYTE4);
		vertPacker.setWeights(VertexElement::eUBYTE4N);
		
		std::vector<BasisVectorCreator::TriBasisVec> triBasisVec;
		createTriBasisVecs(triBasisVec, 0);
			
		for (int sectionIndex = 0; sectionIndex < mBuildSections.size(); sectionIndex++)
		{
			const BuildSection& buildSection = mBuildSections[sectionIndex];
			UGXGeom::Section& section = mOutGeom.section(sectionIndex);
			
			const Unigeom::Material& oldMat = mGeom.material(section.materialIndex());
			const Unigeom::Material& newMat = mOutGeom.material(section.materialIndex());
			
			const IntVec& uvChannelMap = mUVChannelMap[section.materialIndex()];

			const int numBaseBumpMaps = mGeom.shadowGeom() ? 0 : newMat.numMaps(Unigeom::Material::eBump);
					
			const BigString& packOrder = buildSection.mBaseVertPackOrder;
			
			vertPacker.setPackOrder(packOrder);
			
			section.setBaseVertPacker(vertPacker);
											
			const int outVertexSize = vertPacker.size();
			const int outVertexBytes = outVertexSize * buildSection.mNumOutVerts;
			
			mLog.printf("createBaseOutputGeom: Section %i, Used Elements \"%s\", Vertex Size: %i, VB Size: %i\n", 
				sectionIndex, 
				packOrder.c_str(),
				outVertexSize,
				outVertexBytes);
				
			vertPacker.log(mLog);
			
			const int PackedVertexBufferSize = 512;
			Assert(outVertexSize <= PackedVertexBufferSize);
			uchar packedVertexBuf[PackedVertexBufferSize];
			memset(packedVertexBuf, 0xCC, sizeof(packedVertexBuf));
			
			section.setVBOfs(static_cast<int>(mOutGeom.VB().size()));
			section.setVBBytes(outVertexBytes);
			section.setVertSize(outVertexSize);
			section.setNumVerts(buildSection.mNumOutVerts);
					
			for (int outVertIndex = 0; outVertIndex < buildSection.mNumOutVerts; outVertIndex++)
			{
				Univert outVert(createBaseOutputVertex(
					mOutputVerts.at(buildSection.mFirstOutVert + outVertIndex),
					uvChannelMap,
					numBaseBumpMaps,
					triBasisVec,
					oldMat));
					
				// packer will skip unnecessary outVert elements
				vertPacker.pack(packedVertexBuf, outVert);

				mOutGeom.VB().insert(mOutGeom.VB().end(), &packedVertexBuf[0], &packedVertexBuf[outVertexSize]);
			}
		}
	}
	
	Univert UGXInstancer::createMorphOutputVertex(
		int keyFrameIndex,
		const TriMergeVert& triMergeVert, 
		int numBumpMaps,
		const std::vector<BasisVectorCreator::TriBasisVec>& triBasisVec,
		const Unigeom::Material& oldMat)
	{
		Univert dst(eClear);

		const Unigeom::MorphVert& morphVert = mGeom.morphTarget(keyFrameIndex)[triMergeVert.unigeomVertex()];		
		dst.p = morphVert.p;
		dst.n = morphVert.n;
				
		for (int bumpIndex = 0; bumpIndex < numBumpMaps; bumpIndex++)
		{
			BasisVectorCreator::generateBasis(
				dst.tangent(bumpIndex),
				dst.binormal(bumpIndex),
				dst.n,
				triMergeVert.basisTris(bumpIndex, 0),
				triMergeVert.basisTris(bumpIndex, 1),
				triBasisVec[oldMat.map(Unigeom::Material::eBump, bumpIndex).channel()]
			);
		}
		
		return dst;
	}
	
	void UGXInstancer::findAABB(void)
	{
		AABB bounds(AABB::eInitExpand);

		for (int keyFrameIndex = 0; keyFrameIndex < mGeom.numMorphTargets(); keyFrameIndex++)			
		{
			for (int vertIndex = 0; vertIndex < mOutputVerts.size(); vertIndex++)
			{
				const TriMergeVert& triMergeVert = mOutputVerts[vertIndex];
				
				const Unigeom::MorphVert& morphVert = mGeom.morphTarget(keyFrameIndex)[triMergeVert.unigeomVertex()];
				
				bounds.expand(morphVert.p);
			}
		}
		
		mOutGeom.setBounds(bounds);
		printf("Overall bounds: (%f %f %f) - (%f %f %f)\n",
			bounds[0][0], bounds[0][1], bounds[0][2],
			bounds[1][0], bounds[1][1], bounds[1][2]);
			
		mOutGeom.setBoundingSphere(Sphere(bounds.center(), bounds.diagonal().len() * .5f));
	}
	
	void UGXInstancer::findBoneBounds(void)
	{
		//for (int i = 0; i < mGeom.numBones(); i++)
		//	mOutGeom.bones().push_back(mGeom.bone(i));
			
		if (!mGeom.numBones())
			return;
			
		mOutGeom.boneBoundsVec().resize(mGeom.numBones());
		
		for (int i = 0; i < mGeom.numBones(); i++)
			mOutGeom.boneBounds(i) = AABB(AABB::eInitExpand);
				
		for (int keyFrameIndex = 0; keyFrameIndex < mGeom.numMorphTargets(); keyFrameIndex++)
		{
			for (int vertIndex = 0; vertIndex < mOutputVerts.size(); vertIndex++)
			{
				const TriMergeVert& triMergeVert = mOutputVerts[vertIndex];
				
				const Univert& baseVert = mGeom.vert(triMergeVert.unigeomVertex());
				const Unigeom::MorphVert& morphVert = mGeom.morphTarget(keyFrameIndex)[triMergeVert.unigeomVertex()];
				
				const float MinWeight = 1.0f/64.0f;
				
				for (int i = 0; i < baseVert.numActualInfluences(); i++)
				{
					const int boneIndex = baseVert.boneIndex(i);
					const float boneWeight = baseVert.boneWeight(i);
					
					if ((Univert::DefaultBoneIndex == boneIndex) || (boneWeight < MinWeight))
						continue;
						
					Assert(boneIndex >= 0);
					Assert(boneIndex < mGeom.numBones());
					
					const Matrix44 bindToBone(mGeom.bone(boneIndex).modelToBone().getMatrix());
														
					mOutGeom.boneBounds(boneIndex).expand(Vec4(morphVert.p, 1.0f) * bindToBone);
				}
			}
		}
	}
	
	void UGXInstancer::createMorphOutputGeom(void)
	{
		if (1 == mGeom.numMorphTargets())
			return;
			
		UnivertPacker vertPacker;
		vertPacker.setPos(VertexElement::eUDEC3N);
		vertPacker.setBasis(VertexElement::eDEC3N);
		vertPacker.setNorm(VertexElement::eDEC3N);
						
		mOutGeom.keyframes().resize(mGeom.numMorphTargets());
		
		int totalKeyFrameVertexBytes = 0;
				
		for (int keyFrameIndex = 0; keyFrameIndex < mGeom.numMorphTargets(); keyFrameIndex++)
		{
			mLog.printf("createMorphOutputGeom: Processing morph target %i\n", keyFrameIndex);
			
			UGXGeom::Keyframe& keyframe = mOutGeom.keyframe(keyFrameIndex);
			keyframe.setTime(mGeom.morphTarget(keyFrameIndex).time());
						
			std::vector<BasisVectorCreator::TriBasisVec> triBasisVec;
			createTriBasisVecs(triBasisVec, keyFrameIndex);
									
			keyframe.verts().clear();
			if (totalKeyFrameVertexBytes)
				keyframe.verts().reserve(totalKeyFrameVertexBytes);
			
			for (int sectionIndex = 0; sectionIndex < mBuildSections.size(); sectionIndex++)
			{
				const BuildSection& buildSection = mBuildSections[sectionIndex];
				UGXGeom::Section& section = mOutGeom.section(sectionIndex);
				
				const Unigeom::Material& oldMat = mGeom.material(section.materialIndex());
				const Unigeom::Material& newMat = mOutGeom.material(section.materialIndex());
								
				const int numBaseBumpMaps = mGeom.shadowGeom() ? 0 : newMat.numMaps(Unigeom::Material::eBump);
						
				const BigString& packOrder = buildSection.mMorphVertPackOrder;
				Assert(!packOrder.empty());
				
				vertPacker.setPackOrder(packOrder);
																				
				const int outVertexSize = vertPacker.size();
				const int outVertexBytes = outVertexSize * buildSection.mNumOutVerts;
				
				if (keyFrameIndex == 0)
				{
					mLog.printf("	 Section %i, Used Elements \"%s\", Vertex Size: %i, VB Size: %i\n", 
						sectionIndex, 
						packOrder.c_str(),
						outVertexSize,
						outVertexBytes);
				
					vertPacker.log(mLog);
					
					totalKeyFrameVertexBytes += outVertexBytes;
					
					section.setMorphVBOfs(static_cast<int>(keyframe.verts().size()));
					section.setMorphVBBytes(outVertexBytes);
					section.setMorphVertPacker(vertPacker);
					section.setMorphVertSize(outVertexSize);
				}
				
				const int PackedVertexBufferSize = 512;
				Assert(outVertexSize <= PackedVertexBufferSize);
				uchar packedVertexBuf[PackedVertexBufferSize];
				memset(packedVertexBuf, 0xCC, sizeof(packedVertexBuf));
																		
				for (int outVertIndex = 0; outVertIndex < buildSection.mNumOutVerts; outVertIndex++)
				{
					Univert outVert(createMorphOutputVertex(
						keyFrameIndex,
						mOutputVerts.at(buildSection.mFirstOutVert + outVertIndex),
						numBaseBumpMaps,
						triBasisVec,
						oldMat));
						
					outVert.p = mOutGeom.bounds().toNormPos(outVert.p);
											
					// packer will skip unnecessary outVert elements
					vertPacker.pack(packedVertexBuf, outVert);

					keyframe.verts().insert(keyframe.verts().end(), &packedVertexBuf[0], &packedVertexBuf[outVertexSize]);
				}
			}				
			
			if (0 == keyFrameIndex)
				mLog.printf("  Bytes per keyframe: %i bytes\n", totalKeyFrameVertexBytes);
		}
	}
	
	void UGXInstancer::createOutputIndices(void)
	{
		mOutGeom.IB().resize(mOutputTris.size() * 3);
		
		for (int triIndex = 0; triIndex < mOutputTris.size(); triIndex++)
		{
			mOutGeom.IB()[triIndex * 3 + 0] = mOutputTris[triIndex][0];
			mOutGeom.IB()[triIndex * 3 + 1] = mOutputTris[triIndex][1];
			mOutGeom.IB()[triIndex * 3 + 2] = mOutputTris[triIndex][2];
		}
		
		for (int sectionIndex = 0; sectionIndex < mBuildSections.size(); sectionIndex++)
		{
			const BuildSection& buildSection = mBuildSections[sectionIndex];
			UGXGeom::Section& section = mOutGeom.section(sectionIndex);
		
			section.setNumTris(buildSection.mNumOutTris);// * 3);
			//section.setIBOfs(buildSection.mFirstOutTri * 3 * sizeof(UGXGeom::Geom::IndexType));
			section.setIBOfs(buildSection.mFirstOutTri * 3);
		}
	}
	
	void UGXInstancer::createOutputBones(void)
	{
		for (int i = 0; i < mGeom.numBones(); i++)
			mOutGeom.bones().push_back(mGeom.bone(i));
	}
	
	void UGXInstancer::createOutputGeom(void)
	{
		createBaseOutputGeom();
		
		findAABB();
		
		findBoneBounds();
		
		createMorphOutputGeom();
		
		createOutputIndices();
		
		createOutputBones();
		
		mOutGeom.setShadowGeom(mGeom.shadowGeom());
	}

	void UGXInstancer::build(void)
	{
		mLog.printf("UGXInstancer::build: Data Format Version %X\n", UGXGeom::GeomVersion);
		
		buildInit();
		
		findSections();
			
		createTriVertBasisContributors();

		createOutputMaterials();		
		
		sortSections();
		
		createOptimizedMeshes();
		
		createOutputGeom();
	}

} // namespace gr




