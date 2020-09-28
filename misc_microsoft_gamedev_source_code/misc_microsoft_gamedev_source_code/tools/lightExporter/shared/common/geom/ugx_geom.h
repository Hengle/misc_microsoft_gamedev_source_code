// File: ugx_geom.h
#pragma once
#ifndef UGX_GEOM_H
#define UGX_GEOM_H

#include "common/geom/unigeom.h"
#include "common/math/vector.h"
#include "common/math/hypersphere.h"
#include "common/utils/logfile.h"
#include "common/geom/univert_packer.h"

namespace gr
{
	namespace UGXGeom
	{
		enum
		{	
			GeomVersion = 0xECDA1015,
		};
		
		class Section
		{
		public:
			enum { MaxTris = 8192 };
			
			Section() :
				mMaterialIndex(0),
				mMaxBones(0),
				mRigidOnly(false),
				mRigidBoneIndex(INT_MAX),
				mIBOfs(0),
				mVBOfs(0),
				mVBBytes(0),
				mVertSize(0),
				mNumVerts(0),
				mMorphVBOfs(0),
				mMorphVBBytes(0),
				mMorphVertSize(0),
				mNumTris(0)
			{
			}
									
			friend Stream& operator<< (Stream& dst, const Section& src)
			{
				return	dst << src.mMaterialIndex 
										<< src.mMaxBones 
										<< src.mRigidOnly
										<< src.mRigidBoneIndex 
										<< src.mIBOfs 
										<< src.mVBOfs 
										<< src.mVBBytes
										<< src.mVertSize
										<< src.mNumVerts										
										<< src.mMorphVBOfs 
										<< src.mMorphVBBytes
										<< src.mMorphVertSize
										<< src.mNumTris
										<< src.mBaseVertPacker
										<< src.mMorphVertPacker;
			}
			
			friend Stream& operator>> (Stream& src, Section& dst)
			{
				return	src >> dst.mMaterialIndex 
										>> dst.mMaxBones
										>> dst.mRigidOnly
										>> dst.mRigidBoneIndex 
										>> dst.mIBOfs 
										>> dst.mVBOfs
										>> dst.mVBBytes 
										>> dst.mVertSize
										>> dst.mNumVerts										
										>> dst.mMorphVBOfs
										>> dst.mMorphVBBytes
										>> dst.mMorphVertSize
										>> dst.mNumTris
										>> dst.mBaseVertPacker
										>> dst.mMorphVertPacker;
			}
		
			int materialIndex(void) const { return mMaterialIndex; }
			void setMaterialIndex(int materialIndex) { mMaterialIndex = materialIndex; }
			
			int maxBones(void) const { return mMaxBones; }
			void setMaxBones(int maxBones) { mMaxBones = maxBones; }
			
			bool rigidOnly(void) const { return mRigidOnly; }
			void setRigidOnly(bool rigidOnly) { mRigidOnly = rigidOnly; }
			
			int rigidBoneIndex(void) const { return mRigidBoneIndex; }
			void setRigidBoneIndex(int rigidBoneIndex) { mRigidBoneIndex = rigidBoneIndex; }
			
			int IBOfs(void) const { return mIBOfs; }
			void setIBOfs(int ofs) { mIBOfs = ofs; }
			
			int VBOfs(void) const { return mVBOfs; }
			void setVBOfs(int ofs) { mVBOfs = ofs; }
			
			int VBBytes(void) const { return mVBBytes; }
			void setVBBytes(int bytes) { mVBBytes = bytes; }
			
			int vertSize(void) const { return mVertSize; }
			void setVertSize(int size) { mVertSize = size; }
			
			int numVerts(void) const { return mNumVerts; }
			void setNumVerts(int numVerts) { mNumVerts = numVerts; }
												
			int morphVBOfs(void) const { return mMorphVBOfs; }
			void setMorphVBOfs(int ofs) { mMorphVBOfs = ofs; }
			
			int morphVBBytes(void) const { return mMorphVBBytes; }
			void setMorphVBBytes(int bytes) { mMorphVBBytes = bytes; }
			
			int morphVertSize(void) const { return mMorphVertSize; }
			void setMorphVertSize(int size) { mMorphVertSize = size; }
			
			int numTris(void) const { return mNumTris; }			
			void setNumTris(int numTris) { mNumTris = numTris; }
									
			const UnivertPacker& baseVertPacker(void) const { return mBaseVertPacker; }
			void setBaseVertPacker(const UnivertPacker& baseVertPacker) { mBaseVertPacker = baseVertPacker; }
			
			const UnivertPacker& morphVertPacker(void) const { return mMorphVertPacker; }
			void setMorphVertPacker(const UnivertPacker& morphVertPacker) { mMorphVertPacker = morphVertPacker; }
			
			void log(LogFile& l) const
			{
				l.printf("Material Index: %i\n", mMaterialIndex);
				l.printf("Max Bones: %i\n", mMaxBones);
				l.printf("Rigid only: %i\n", mRigidOnly);
				l.printf("Rigid bone index: %i\n", mRigidBoneIndex);
				l.printf("Index buffer offset: %i\n", mIBOfs);
				l.printf("Num tris: %i\n", mNumTris);
				l.printf("Vertex buffer offset: %i\n", mVBOfs);
				l.printf("Vertex buffer bytes: %i\n", mVBBytes);
				l.printf("Base vertex size: %i\n", mVertSize);
				l.printf("Morph vertex buffer offset: %i\n", mMorphVBOfs);
				l.printf("Morph vertex buffer bytes: %i\n", mMorphVBBytes);
				l.printf("Morph vertex size: %i\n", mMorphVertSize);
								
				l.printf("Base vertex packer:\n");
				l.indent(1);
				mBaseVertPacker.log(l);
				l.indent(-1);
				
				l.printf("Morph vertex packer:\n");
				l.indent(1);
				mMorphVertPacker.log(l);
				l.indent(-1);
			}
						
		private:
			int mMaterialIndex;
			int mMaxBones;
			bool mRigidOnly;
			int mRigidBoneIndex;
			
			int mIBOfs;					// in indices, not bytes!
			int mNumTris;		
			
			int mVBOfs;
			int mVBBytes;
			int mVertSize;
			int mNumVerts;
			
			int mMorphVBOfs;
			int mMorphVBBytes;
			int mMorphVertSize;
			
			UnivertPacker mBaseVertPacker;
			UnivertPacker mMorphVertPacker;
		};
		
		typedef std::vector<Section> SectionVec;
		
		class Keyframe
		{
		public:
			Keyframe() :
				mTime(0)
			{
			}
			
			friend Stream& operator<< (Stream& dst, const Keyframe& src)
			{
				dst << src.mTime;
				return dst.writeVecRaw(src.mVerts);
			}
			
			friend Stream& operator>> (Stream& src, Keyframe& dst)
			{
				src >> dst.mTime;
				return src.readVecRaw(dst.mVerts);
			}
			
			float time(void) const { return mTime; }
			void setTime(float time) { mTime = time; }
			
			const UCharVec& verts(void) const {	return mVerts; }
						UCharVec& verts(void)				{	return mVerts; }		
						
			int size(void) const
			{
				return verts().size();
			}
						
			void log(LogFile& l) const
			{
				l.printf("Time: %f\n", mTime);
				
				l.printf("Vertex buffer bytes: %i\n", mVerts.size());
			}
								
		private:
			float mTime;
			UCharVec mVerts;
		};
		
		typedef std::vector<Keyframe> KeyframeVec;
		
		class Geom
		{
		public:
			typedef ushort IndexType;
			
			Geom() :
				mBoundingSphere(eClear),
				mBounds(eClear),
				mRigidOnly(false),
				mRigidBoneIndex(0),
				mShadowGeom(false),
				mAllSectionsRigid(false)
			{
			}
			
			friend Stream& operator<< (Stream& dst, const Geom& src)
			{
				return	dst << GeomVersion
										<< src.mShadowGeom
										<< src.mBoundingSphere
										<< src.mBounds
										<< src.mMaterials
										<< src.mBones
										<< src.mBoneBounds
										<< src.mKeyframes
										<< src.mSections
										<< src.mVB
										<< src.mIB
										<< src.mRigidOnly
										<< src.mRigidBoneIndex 
										<< src.mAllSectionsRigid
										<< GeomVersion;
			}
			
			friend Stream& operator>> (Stream& src, Geom& dst)
			{
				int ver;
				src >> ver;
				Verify(GeomVersion == ver);
				
				src >> dst.mShadowGeom
						>> dst.mBoundingSphere
						>> dst.mBounds
						>> dst.mMaterials
						>> dst.mBones
						>> dst.mBoneBounds
						>> dst.mKeyframes
						>> dst.mSections
						>> dst.mVB
						>> dst.mIB
						>> dst.mRigidOnly
						>> dst.mRigidBoneIndex 
						>> dst.mAllSectionsRigid
						>> ver;
				
				Verify(GeomVersion == ver);						
				
				return src;
			}
			
			const Sphere& boundingSphere(void) const { return mBoundingSphere; }
			void setBoundingSphere(const Sphere& sphere) { mBoundingSphere = sphere; }
			
			const AABB& bounds(void) const { return mBounds; }
			void setBounds(const AABB& bounds) { mBounds = bounds; }
						
			int numMaterials(void) const { return static_cast<int>(mMaterials.size()); }
			const Unigeom::Material& material(int i) const { return mMaterials[DebugRange(i, mMaterials.size())]; }
			const Unigeom::MaterialVec& materials(void) const { return mMaterials; }
						Unigeom::MaterialVec& materials(void)				{ return mMaterials; }
									
			int numBones(void) const { return static_cast<int>(mBones.size()); }
			const Unigeom::Bone& bone(int i) const { return mBones[DebugRange(i, numBones())]; }
			const Unigeom::BoneVec& bones(void) const	{ return mBones; }
						Unigeom::BoneVec& bones(void)				{ return mBones; }

			int numBoneBounds(void) const { return static_cast<int>(mBoneBounds.size()); }
			const AABB& boneBounds(int i) const { return mBoneBounds[DebugRange(i, numBoneBounds())]; }
						AABB& boneBounds(int i)				{ return mBoneBounds[DebugRange(i, numBoneBounds())]; }

			const AABBVec& boneBoundsVec(void) const { return mBoneBounds; }
						AABBVec& boneBoundsVec(void)				{ return mBoneBounds; }
			
			int numKeyframes(void) const { return static_cast<int>(mKeyframes.size()); }
			const Keyframe& keyframe(int i) const { return mKeyframes[DebugRange(i, numKeyframes())]; }
						Keyframe& keyframe(int i)				{ return mKeyframes[DebugRange(i, numKeyframes())]; }
			const KeyframeVec& keyframes(void) const	{ return mKeyframes; }
						KeyframeVec& keyframes(void)				{ return mKeyframes; }
						
			int numSections(void) const { return static_cast<int>(mSections.size()); }
			const Section& section(int i) const { return mSections[DebugRange(i, numSections())]; }
						Section& section(int i)				{ return mSections[DebugRange(i, numSections())]; }
			const SectionVec& sections(void) const	{ return mSections; }
						SectionVec& sections(void)				{ return mSections; }
								
			const UCharVec& VB(void) const	{ return mVB; }
						UCharVec& VB(void)				{ return mVB; }
						
			const UShortVec& IB(void) const	{ return mIB; }
						UShortVec& IB(void)				{ return mIB; }						
									
			bool rigidOnly(void) const { return mRigidOnly; }
			void setRigidOnly(bool rigidOnly) { mRigidOnly = rigidOnly; }

			int rigidBoneIndex(void) const { return mRigidBoneIndex; }
			void setRigidBoneIndex(int rigidBoneIndex) { mRigidBoneIndex = rigidBoneIndex; }
			
			bool shadowGeom(void) const { return mShadowGeom; }
			void setShadowGeom(bool shadowGeom) { mShadowGeom = shadowGeom; }
			
			bool allSectionsRigid(void) const { return mAllSectionsRigid; }
			void setAllSectionsRigid(bool allSectionsRigid) { mAllSectionsRigid = allSectionsRigid; }
									
			void log(LogFile& l) const
			{
				l.printf("********************* UGX Data Version 0x%X Log Dump: \n", GeomVersion);
							
				l.indent(1);
				
				l.printf("Shadow geom: %i\n", mShadowGeom);
				
				l.printf("Rigid only: %i Rigid bone index: %i AllSectionsRigid: %i\n", mRigidOnly, mRigidBoneIndex, mAllSectionsRigid);
				
				l.printf("Bounding sphere:\n");
				mBoundingSphere.log(l);
				
				l.printf("AABB:\n");
				mBounds.log(l);
				
				l.printf("Vertex buffer size: %i bytes\n", mVB.size());
				l.printf("Index buffer size: %i bytes\n", mIB.size() * sizeof(mIB[0]));
								
				l.printf("---- Materials: %i\n", numMaterials());
				l.indent(1);
				for (int i = 0; i < numMaterials(); i++)
				{
					l.printf("Material: %i\n", i);
					l.indent(1);
					material(i).log(l);
					l.indent(-1);
				}
				l.indent(-1);
				
				l.printf("---- Bones: %i\n", numBones());
				l.indent(1);
				for (int i = 0; i < numBones(); i++)
				{
					l.printf("Bone: %i\n", i);
					l.indent(1);
					bone(i).log(l);
					
					l.printf("Bounds:\n");
					l.indent(1);
					boneBounds(i).log(l);
					l.indent(-1);
					
					l.indent(-1);
				}
				l.indent(-1);					
				
				l.printf("---- Sections: %i\n", numSections());
				l.indent(1);
				for (int i = 0; i < numSections(); i++)
				{
					l.printf("Section: %i\n", i);
					l.indent(1);
					section(i).log(l);
					l.indent(-1);
				}
				l.indent(-1);
					
				l.printf("---- Keyframes: %i\n", numKeyframes());
				l.indent(1);
				for (int i = 0; i < numKeyframes(); i++)
				{
					l.printf("Keyframe: %i\n", i);
					l.indent(1);
					keyframe(i).log(l);
					l.indent(-1);
				}
				l.indent(-1);
					
				l.indent(-1);
			}
			
			void clear(void)
			{
				mBoundingSphere.clear();
				mMaterials.clear();
				mBones.clear();
				mBoneBounds.clear();
				mKeyframes.clear();
				mSections.clear();
				mVB.clear();
				mIB.clear();
				mRigidOnly = false;
				mRigidBoneIndex = 0;
				mShadowGeom = false;
			}
					
		private:
			Sphere mBoundingSphere;
			AABB mBounds;
			
			Unigeom::MaterialVec mMaterials;
			Unigeom::BoneVec mBones;
			AABBVec mBoneBounds;
			KeyframeVec mKeyframes;
											
			SectionVec mSections;
			
			UCharVec mVB;
			UShortVec mIB;
			
			bool mRigidOnly;
			int mRigidBoneIndex;
			
			bool mAllSectionsRigid;
			
			bool mShadowGeom;
		};
		
	} // namespace UGXGeom

} // namespace gr

#endif // UGX_GEOM_H
