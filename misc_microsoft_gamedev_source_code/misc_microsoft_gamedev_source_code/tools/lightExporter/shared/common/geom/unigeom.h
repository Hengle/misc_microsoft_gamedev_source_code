//-----------------------------------------------------------------------------
// File: unigeom.h
// Universal Geometry Format
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef UNIGEOM_H
#define UNIGEOM_H

#include "common/geom/univert.h"
#include "common/geom/indexed_tri.h"
#include "common/utils/unifier.h"
#include "common/math/vector.h"
#include "common/math/quat.h"
#include "common/utils/string.h"
#include "common/utils/stream.h"
#include "common/utils/logfile.h"

#include <algorithm>

namespace gr
{
	namespace Unigeom
	{
		#define SHADOW_MESH_NORMALS 0
		
		enum 
		{
			MaxBoneInfluences = Univert::MaxInfluences
		};

		enum 
		{ 
			GeomVersion = 0xABAB1013,
			AnimVersion = 0xDEDE1002
		};

		typedef int MaterialIndex;
		typedef int BoneIndex;
		typedef int TriIndex;
		typedef int ChannelIndex;
		typedef int KeyFrameIndex;
		typedef int TriVertIndex;
		typedef int VertIndex;
		typedef int MorphTargetIndex;
   
		class Map : Utils::RelativeOperators<Map>
		{
		public:
			typedef String<64> StringType;
			
			Map() : mChannel(InvalidIndex), mFlags(0)
			{
				mName.clear();
			}
			
			Map(const char* pName, ChannelIndex channel, int flags) : mChannel(channel), mFlags(flags)
			{
				mName.clear();
				mName = pName;
				mName.tolower();
			}

			void clear(void)
			{
				mName.clear();
				mChannel = 0;
				mFlags = 0;
			}

			const StringType& name(void) const	{ return mName; }
			ChannelIndex channel(void) const		{ return mChannel; }
			int flags(void) const								{ return mFlags; }

			void setName(const StringType& name)	{ mName = name; }
			void setChannel(ChannelIndex index)		{ mChannel = index; }
			void setFlags(int flags)							{ mFlags = flags; }

			bool operator== (const Map& b) const
			{
				return ((mFlags == b.mFlags) && (mChannel == b.mChannel) && (mName == b.mName));
			}

			bool operator< (const Map& b) const
			{
				if (mFlags < b.mFlags)
					return true;
				else if (mFlags == b.mFlags)
				{
					if (mChannel < b.mChannel)
						return true;
					else if (mChannel == b.mChannel)
					{
						if (mName < b.mName)
							return true;
					}
				}
				return false;
			}

			void debugCheck(void) const
			{
			}

			friend Stream& operator<< (Stream& dst, const Map& src)
			{
				return dst << src.mName << src.mChannel << src.mFlags;
			}

			friend Stream& operator>> (Stream& src, Map& dst)
			{
				return src >> dst.mName >> dst.mChannel >> dst.mFlags;
			}
			
			void log(LogFile& log) const
			{
				log.printf("Name: \"%s\", ChannelIndex: %i, Flags: %i\n",
					name().c_str(),
					channel(),
					flags());
			}
		
		protected:
			StringType mName;
			ChannelIndex mChannel;
			int mFlags;
		};

		template<int MaxSize>
		class MapCont
		{
		public:
			MapCont()
			{
			}

			void clear(void)
			{
				mNumMaps = 0;
				for (int i = 0; i < MaxSize; i++)
					mMaps[i].clear();
			}

			int maxSize(void) const
			{
				return MaxSize;
			}

			int size(void) const 
			{ 
				return mNumMaps; 
			}

			void resize(int size) 
			{ 
				mNumMaps = size;
			}
      			
						Map& operator[] (int i)				{ return mMaps[DebugRange(i, MaxSize)]; }
			const Map& operator[] (int i) const { return mMaps[DebugRange(i, MaxSize)]; }

			int addMap(const Map& map)
			{
				Assert(mNumMaps < MaxSize);
				mMaps[mNumMaps] = map;
				return mNumMaps++;
			}

			void debugCheck(void)
			{
			}

			friend Stream& operator<< (Stream& dst, const MapCont& src)
			{
				dst << src.mNumMaps;
				for (int i = 0; i < src.mNumMaps; i++)
					dst << src.mMaps[i];
				return dst;
			}

			friend Stream& operator>> (Stream& src, MapCont& dst)
			{
				src >> dst.mNumMaps;
				for (int i = 0; i < dst.mNumMaps; i++)
					src >> dst.mMaps[i];
				return src;
			}
			
			void log(LogFile& log) const
			{
				log.printf("Num Textures: %i\n", mNumMaps);
				log.indent(1);
				for (int i = 0; i < mNumMaps; i++)
				{
					log.printf("Texture: %i\n", i);
					mMaps[i].log(log);
				}
				log.indent(-1);
			}
		
		protected:
			int mNumMaps;
			Map mMaps[MaxSize];
		};

		class Material : Utils::RelativeOperators<Material>
		{
		public:
			enum 
			{ 
				SpecPowerLow = 5, 
				SpecPowerHigh = 1000 
			};

			enum EMapType
			{
				eFirstMapType,
				
				eDiffuse = eFirstMapType,
				eSpecular,
				eBump,
				eEnv,
				eSelf,

				NumMapTypes
			};

			enum 
			{ 
				MaxMapsPerType = 4
			};

			typedef MapCont<MaxMapsPerType> MapContType;
      
			typedef String<64> StringType;
			
			Material()
			{
				clear();
			}

			Material(const char* pName)
			{
				clear();
				mName.clear();
				mName = pName;
			}
			
			Material(const Material& other)
			{
				*this = other;
			}

			void clear(void)
			{
				mName.clear();
				for (int i = 0; i < NumMapTypes; i++)
					mMaps[i].clear();
				mFlags = 0;
				mBumpiness = 1.0f;
				mSkin = false;
				mSpecLevel = 1.0f;
				mSpecPower = 20.0f;
				mEmissive = 0.0f;
				mDiffColor.set(1.0f);
				mSpecColor.set(1.0f);
				mSelfIntensity = 1.0f;
				mEnvIntensity = 1.0f;
			}

			const StringType& name(void) const { return mName; }
			void setName(const StringType& name) { mName = name; }

			int numMaps(int type) const { return mMaps[DebugRange(type, NumMapTypes)].size(); }
			void setNumMaps(int type, int numMaps) { mMaps[DebugRange(type, NumMapTypes)].resize(numMaps); }
			
			static const char* mapName(int type) 
			{
				switch (type)
				{
					case eDiffuse:	return "diffuse";
					case eSpecular:	return "specular";
					case eBump:			return "bump";
					case eEnv:			return "env";
					case eSelf:			return "self";
				}
				return "";
			}

			const Map& map(int type, int slot) const { return mMaps[DebugRange(type, NumMapTypes)][slot]; }
						Map& map(int type, int slot)				{ return mMaps[DebugRange(type, NumMapTypes)][slot]; }
			void setMap(int type, int slot, const Map& map) { mMaps[DebugRange(type, NumMapTypes)][slot] = map; }
			
			const MapContType& getMapCont(int type) const	{ return mMaps[DebugRange(type, NumMapTypes)]; }
						MapContType& getMapCont(int type)				{ return mMaps[DebugRange(type, NumMapTypes)]; }
						
			int flags(void) const { return mFlags; }
			float bumpiness(void) const { return mBumpiness; }
			bool skin(void) const { return mSkin; }
			float specLevel(void) const { return mSpecLevel; }
			float specPower(void) const { return mSpecPower; }
			float emissive(void) const { return mEmissive; }
			const Vec<3>& diffColor(void) const { return mDiffColor; }
			const Vec<3>& specColor(void) const { return mSpecColor; }
			float selfIntensity(void) const { return mSelfIntensity; }
			float envIntensity(void) const { return mEnvIntensity; }

			void setFlags(int flags)								{ mFlags = flags; }
			void setBumpiness(float bumpiness)			{ mBumpiness = bumpiness; }
			void setSkin(bool skin)									{ mSkin = skin; }
			void setSpecLevel(float level)					{ mSpecLevel = level; }
			void setSpecPower(float power)					{ mSpecPower = power; }
			void setEmissive(float emissive)				{ mEmissive = emissive; }
			void setDiffColor(const Vec<3>& color)	{ mDiffColor = color; }
			void setSpecColor(const Vec<3>& color)	{ mSpecColor = color; }
			void setSelfIntensity(float i)					{ mSelfIntensity = i; }
			void setEnvIntensity(float i)					{ mEnvIntensity = i; }
			
			bool operator== (const Material& b) const
			{
				if (mName != b.mName)
					return false;
        
				for (int i = 0; i < NumMapTypes; i++)
				{
					if (mMaps[i].size() != b.mMaps[i].size())
						return false;
					
					for (int j = 0; j < mMaps[i].size(); j++)
						if (mMaps[i][j] != b.mMaps[i][j])
							return false;
				}
				
				if (mFlags != b.mFlags)
					return false;
								
#define COMP(a) do { if (a != b.a) return false; } while(0)
				COMP(mBumpiness);
				COMP(mSkin);
				COMP(mSpecPower);
				COMP(mSpecLevel);
				COMP(mEmissive);
				COMP(mDiffColor);
				COMP(mSpecColor);
				COMP(mSelfIntensity);
				COMP(mEnvIntensity);
#undef COMP
				       
				return true;
			}

			bool operator< (const Material& b) const
			{
				if (mName < b.mName)
					return true;
				else if (mName != b.mName)
					return false;

				for (int i = 0; i < NumMapTypes; i++)
				{
					if (mMaps[i].size() < b.mMaps[i].size())
						return false;
					else if (mMaps[i].size() != b.mMaps[i].size())
						return false;
          					
					for (int j = 0; j < mMaps[i].size(); j++)
					{
						if (mMaps[i][j] < b.mMaps[i][j])
							return true;
						else if (mMaps[i][j] != b.mMaps[i][j])
							return false;
					}
				}
													
				if (mFlags < b.mFlags)
					return true;
				else if (mFlags != b.mFlags)
					return false;
				
				#define COMP(a) do { if (a < b.a) return true; else if (a != b.a) return false; } while(0)
					COMP(mBumpiness);
					COMP(mSkin);
					COMP(mSpecPower);
					COMP(mSpecLevel);
					COMP(mEmissive);
					COMP(mDiffColor);
					COMP(mSpecColor);
					COMP(mSelfIntensity);
					COMP(mEnvIntensity);
				#undef COMP
				
				return false;
			}

			friend Stream& operator<< (Stream& dst, const Material& src)
			{
				dst << src.mName;
				for (int i = 0; i < NumMapTypes; i++)
					dst << src.mMaps[i];
				return dst << src.mFlags << src.mBumpiness << src.mSkin << src.mSpecLevel << src.mSpecPower << src.mEmissive << src.mDiffColor << src.mSpecColor << src.mSelfIntensity << src.mEnvIntensity;
			}
			
			friend Stream& operator>> (Stream& src, Material& dst)
			{
				src >> dst.mName;
				for (int i = 0; i < NumMapTypes; i++)
					src >> dst.mMaps[i];
				return src >> dst.mFlags >> dst.mBumpiness >> dst.mSkin >> dst.mSpecLevel >> dst.mSpecPower >> dst.mEmissive >> dst.mDiffColor >> dst.mSpecColor >> dst.mSelfIntensity >> dst.mEnvIntensity;
			}
			
			void log(LogFile& log) const
			{
				log.printf("Material Name: \"%s\", Flags: %i, Bumpiness: %f\n",
					mName.c_str(),
					mFlags,
					mBumpiness);
					
				log.printf("Skin: %i, SpecLevel: %f, SpecPower: %f\n",
					mSkin,
					mSpecLevel,
					mSpecPower);					
					
				log.printf("Emissive: %f\n",
					mEmissive);
					
				log.printf("Diffuse: %f %f %f\n",
					mDiffColor[0], mDiffColor[1], mDiffColor[2]);
					
				log.printf("Specular: %f %f %f\n",
					mSpecColor[0], mSpecColor[1], mSpecColor[2]);															
				
				for (int i = 0; i < NumMapTypes; i++)
				{
					log.printf("Map %i (%s):\n", i, mapName(static_cast<EMapType>(i)));
					log.indent(1);
					mMaps[i].log(log);
					log.indent(-1);
				}
				
				log.printf("Self Intensity: %f, Env. Intensity: %f\n", mSelfIntensity, mEnvIntensity);
			}

			void debugCheck(void) const
			{
			}

		protected:
			StringType mName;
			MapContType mMaps[NumMapTypes];
			int mFlags;
			float mBumpiness;
			bool mSkin;
			float mSpecLevel;
			float mSpecPower;
			float mEmissive;
			Vec<3> mDiffColor;
			Vec<3> mSpecColor;
			float mSelfIntensity;
			float mEnvIntensity;
		};
		
		typedef std::vector<Material> MaterialVec;
						
		class Tri
		{
		public:
			Tri()
			{
				clear();
			}
									
			void clear(void)
			{
				mMaterialIndex = InvalidIndex;
				Utils::ClearObj(mVertIndex);
			}

			MaterialIndex materialIndex(void) const 
			{ 
				return mMaterialIndex; 
			}

			void setMaterialIndex(MaterialIndex index)				
			{ 
				mMaterialIndex = index; 
			}
			
			VertIndex  operator[] (TriVertIndex i) const	{ return mVertIndex[DebugRange(i, 3)]; }
			VertIndex& operator[] (TriVertIndex i)				{ return mVertIndex[DebugRange(i, 3)]; }
			
			void debugCheck(void) const
			{
			}
			
			Stream& write(Stream& dst) const
			{
				return dst << mMaterialIndex << mVertIndex[0] << mVertIndex[1] << mVertIndex[2];
			}
			
			Stream& read(Stream& src) 
			{
				return src >> mMaterialIndex >> mVertIndex[0] >> mVertIndex[1] >> mVertIndex[2];
			}

			friend Stream& operator<< (Stream& dst, const Tri& src)
			{
				return src.write(dst);
			}

			friend Stream& operator>> (Stream& src, Tri& dst)
			{
				return dst.read(src);
			}
			
			static bool MaterialIndexComp(const Tri& lhs, const Tri& rhs)
			{
				return lhs.mMaterialIndex < rhs.mMaterialIndex;
			}
			
			IndexedTri indexedTri(void) const
			{
				return IndexedTri(mVertIndex[0], mVertIndex[1], mVertIndex[2]);
			}

		protected:
			MaterialIndex mMaterialIndex;
			VertIndex mVertIndex[3];
		};
				
		class QForm
		{
		public:
			QForm()
			{
			}

			QForm(const Vec<4>& q, const Vec<3>& t) : mQ(q), mT(t)
			{
#if QFORM_STORE_MATRIX						
				mX[0].setZero();
				mX[1].setZero();
				mX[2].setZero();
				mX[3].setZero();
#endif				
			}

#if QFORM_STORE_MATRIX			
			QForm(const Vec<4>& q, const Vec<3>& t, const Vec<3>& r0, const Vec<3>& r1, const Vec<3>& r2, const Vec<3>& r3
					) : 
				mQ(q), mT(t)
			{
				mX[0] = r0;
				mX[1] = r1;
				mX[2] = r2;
				mX[3] = r3;
			}
#endif

			void clear(void)
			{
				mQ.set(0, 0, 0, 1);
				mT.setZero();
#if QFORM_STORE_MATRIX			
				mX[0].setZero();
				mX[1].setZero();
				mX[2].setZero();
				mX[3].setZero();
#endif				
			}

			const Vec<4>& getQ(void) const { return mQ; }
			const Vec<3>& getT(void) const { return mT; }

#if QFORM_STORE_MATRIX			
			const Vec<3>& getX(int i) const { return mX[DebugRange(i, 4)]; }
#endif

			void setQ(const Vec<4>& q) { mQ = q; }
			
			void setT(const Vec<4>& t) { mT = t; }

#if QFORM_STORE_MATRIX			
			void setX(const Vec<3>& r0, const Vec<3>& r1, const Vec<3>& r2, const Vec<3>& r3) 
			{
				mX[0] = r0;
				mX[1] = r1;
				mX[2] = r2;
				mX[3] = r3;
			}
#endif

			Matrix44 getMatrix(void) const
			{
				Matrix44 m(Quat::createMatrix(getQ()));
				m.setTranslate(getT(), 1.0f);
				return m;
			}

			bool operator== (const QForm& b) const
			{
				return (mQ == b.mQ) && (mT == b.mT);
			}

			bool operator!= (const QForm& b) const
			{
				return !(*this == b);
			}

			void debugCheck(void) const
			{
			}

			friend Stream& operator<< (Stream& dst, const QForm& src)
			{
				return dst << src.mQ << src.mT 
#if QFORM_STORE_MATRIX							
				<< src.mX[0] << src.mX[1] << src.mX[2] << src.mX[3]
#endif				
				;
			}

			friend Stream& operator>> (Stream& src, QForm& dst)
			{
				return src >> dst.mQ >> dst.mT 
#if QFORM_STORE_MATRIX							
				>> dst.mX[0] >> dst.mX[1] >> dst.mX[2] >> dst.mX[3]
#endif				
				;
			}
			
			void log(LogFile& l) const
			{
				l.printf("Rotation: ");
				mQ.log(l);
				l.printf("Translation: ");
				mT.log(l);
			}

		protected:
			Vec<4> mQ;
			Vec<3> mT;
#if QFORM_STORE_MATRIX			
			Vec<3> mX[4];
#endif			
		};
				
		class Bone
		{
		public:
			typedef String<64> StringType;
			
			Bone() : mParentIndex(InvalidIndex)
			{
				mModelToBone.clear();
				mName.clear();
			}

			Bone(const char* pName, const QForm& modelToBone, BoneIndex parentIndex = InvalidIndex) :
				mParentIndex(parentIndex), mModelToBone(modelToBone)
			{
				mName.clear();
				mName = pName;
			}

			const StringType& name(void) const		{ return mName; }
			BoneIndex parentBoneIndex(void) const { return mParentIndex; }
			const QForm& modelToBone(void) const	{ return mModelToBone; }

			void setName(const StringType& name)				{ mName = name; }
			void setParentBoneIndex(BoneIndex index)		{ mParentIndex = index; }
			void modelToBone(const QForm& modelToBone)	{ mModelToBone = modelToBone; }
			
			bool operator== (const Bone& b) const
			{
				if (mName != b.mName)
					return false;
				else if (mParentIndex != b.mParentIndex)
					return false;
				else if (mModelToBone != b.mModelToBone)
					return false;
				return true;
			}

			bool operator< (const Bone& b) const
			{
				if (mParentIndex < b.mParentIndex)
					return true;
				else if (mParentIndex == b.mParentIndex)
				{
					if (mName < b.mName)
						return true;
				}
				return false;
			}

			void debugCheck(void) const
			{
			}

			friend Stream& operator<< (Stream& dst, const Bone& src)
			{
				return dst << src.mName << src.mParentIndex << src.mModelToBone;
			}

			friend Stream& operator>> (Stream& src, Bone& dst)
			{
				return src >> dst.mName >> dst.mParentIndex >> dst.mModelToBone;
			}
			
			void log(LogFile& l) const
			{
				l.printf("Name: \"%s\"\n", mName.c_str());
				l.printf("Parent index: %i\n", mParentIndex);
				l.printf("Model To Bone QForm:\n");
				l.indent(1);
				mModelToBone.log(l);
				l.indent(-1);
			}

		protected:
			StringType mName;
			BoneIndex mParentIndex;
			QForm mModelToBone;
		};

		typedef std::vector<QForm>	QFormVec;
		typedef std::vector<Bone>		BoneVec;
		typedef std::vector<Tri>		TriVec;

		template<class Type>
		inline void TraverseBoneList(
				const Type& bones, 
				IntVec& newToOldRemap, 
				BoneIndex curBoneIndex)
		{
			IntVec children;
			for (int i = 0; i < bones.size(); i++)
				if (bones[i].parentBoneIndex() == curBoneIndex)
					children.push_back(i);

			int numChildren = children.size();

			bool swapFlag;
			do
			{
				swapFlag = false;
				for (int i = 0; i < numChildren - 1; i++)
				{
					int& a = children.at(i);
					int& b = children.at(i + 1);
					if (bones[b] < bones[a])
					{
						std::swap(a, b);
						swapFlag = true;
						Assert(!(bones[b] < bones[a]));
					}
				}
			} while (swapFlag);

			for (int i = 0; i < numChildren; i++)
			{
				newToOldRemap.push_back(children[i]);
			}

			for (int i = 0; i < numChildren; i++)
				TraverseBoneList(bones, newToOldRemap, children[i]);
		}

		template<class Type>
		inline void CanonicalizeBoneList(
				const Type& bones, 
				IntVec& newToOldRemap,
				IntVec& oldToNewRemap)
		{
			const int numBones = static_cast<int>(bones.size());

			newToOldRemap.clear();
			
			TraverseBoneList(bones, newToOldRemap, -1);

			Verify(newToOldRemap.size() == numBones);
			
			BoolVec accountedFor(numBones);
			for (int i = 0; i < numBones; i++)
			{
				const int oldIndex = newToOldRemap.at(i);
				Verify(!accountedFor.at(oldIndex));
				accountedFor.at(oldIndex) = true;
			}

			oldToNewRemap.resize(numBones);
			for (int i = 0; i < numBones; i++)
				oldToNewRemap[newToOldRemap[i]] = i;
		}

		struct MorphVert
		{
			Vec<3> p;
			Vec<3> n;

			friend Stream& operator<< (Stream& dst, const MorphVert& src)
			{
				return dst << src.p << src.n;
			}

			friend Stream& operator>> (Stream& src, MorphVert& dst)
			{
				return src >> dst.p >> dst.n;
			}
		};
		typedef std::vector<MorphVert> MorphVertVec;

		class MorphTarget
		{
		public:
			MorphTarget(float time = 0.0f, int numVerts = 0) :
				mTime(time),
				mVerts(numVerts)
			{
			}

			void clear(void)
			{
				mTime = 0;
				mVerts.clear();
			}

			float time(void) const
			{
				return mTime;
			}

			void setTime(float time) 
			{
				mTime = time;
			}

			void resize(int numVerts)
			{
				mVerts.resize(numVerts);
			}

			void insertVert(const MorphVert& vert)
			{
				mVerts.push_back(vert);
			}

			int size(void) const
			{
				return static_cast<int>(mVerts.size());
			}

			const MorphVert& operator[] (int i) const { return mVerts[DebugRange(i, size())]; }
						MorphVert& operator[] (int i)				{ return mVerts[DebugRange(i, size())]; }
      
			const MorphVertVec& getMorphVertVec(void) const
			{
				return mVerts;
			}

			friend Stream& operator<< (Stream& dst, const MorphTarget& src)
			{
				dst << src.mTime;
				return dst.writeVec(src.mVerts);
			}

			friend Stream& operator>> (Stream& src, MorphTarget& dst)
			{
				src >> dst.mTime;
				return src.readVec(dst.mVerts);
			}

			// FIXME
			void debugCheck(void)
			{
			}

		private:
			float mTime;
			MorphVertVec mVerts;	
		};
		
		typedef std::vector<MorphTarget> MorphTargetVec;
		
		class Geom
		{
		public:
			typedef MapUnifier<Material> MaterialUnifier;
            
			Geom() :
				mShadowGeom(false)
			{
			}

			~Geom()
			{
			}

      MaterialIndex insertMaterial(const Material& m)
			{
				return mMaterials.insert(m).first;
			}

			BoneIndex insertBone(const Bone& b)
			{
				mBones.push_back(b);
				return numBones() - 1;
      }

			TriIndex insertTri(const Tri& t)
			{
				mTris.push_back(t);
				return numTris() - 1;
			}

			VertIndex insertVert(const Univert& v)
			{
				mVerts.push_back(v);
				return numVerts() - 1;
			}

			MorphTargetIndex insertMorphTarget(const MorphTarget& m)
			{
				mMorphTargets.push_back(m);
				return numMorphTargets() - 1;
			}
			      
			int numMaterials(void) const { return mMaterials.size(); }
			const Material& material(MaterialIndex i) const { return mMaterials[i];	}

			int numTris(void) const { return static_cast<int>(mTris.size()); }
			const Tri& tri(TriIndex i) const	{ return mTris[i]; }
			
			int numBones(void) const { return static_cast<int>(mBones.size()); }
			const Bone& bone(BoneIndex i) const { return mBones[i]; }

			int numVerts(void) const { return static_cast<int>(mVerts.size()); }
			const Univert& vert(VertIndex i) const { return mVerts[DebugRange(i, numVerts())]; }
			
			const Univert& triVert(TriIndex i, TriVertIndex j) const 
			{
				return mVerts[
					DebugRange( mTris[DebugRange(i, numTris())][DebugRange(j, 3)], numVerts() )
					];
			}

			int numMorphTargets(void) const { return static_cast<int>(mMorphTargets.size()); }
			const MorphTarget& morphTarget(int i) const { return mMorphTargets[DebugRange(i, numMorphTargets())]; }
						MorphTarget& morphTarget(int i)				{ return mMorphTargets[DebugRange(i, numMorphTargets())]; }

			const TriVec& triVec(void) const	{ return mTris; }
						TriVec& triVec(void)				{ return mTris; }

			const UnivertVec& vertVec(void) const	{ return mVerts; }
						UnivertVec& vertVec(void)				{ return mVerts; }

			const MorphTargetVec& morphTargetVec(void) const	{ return mMorphTargets; }
						MorphTargetVec& morphTargetVec(void)				{ return mMorphTargets; }
						
			bool shadowGeom(void) const { return mShadowGeom; }
			void setShadowGeom(bool shadowGeom) { mShadowGeom = shadowGeom; }

			// true on failure
			bool write(Stream& s) const
			{
				s << GeomVersion 
					<< numMaterials() 
					<< numBones() 
					<< numTris() 
					<< numVerts()
					<< numMorphTargets();
				
        s.writeVec(mMaterials);
				s.writeVec(mBones);
				s.writeVec(mTris);

				const UnivertAttributes usedAttr(usedAttributes());

				s << usedAttr;

				s << static_cast<int>(mVerts.size());
				for (int i = 0; i < mVerts.size(); i++)
					 mVerts[i].write(s, usedAttr);

				s.writeVec(mMorphTargets);
				
				s << mShadowGeom;
				
				s << GeomVersion;
				                
				return s.errorStatus();
			}

			UnivertAttributes usedAttributes(void) const
			{
				UnivertAttributes usedAttr(UnivertAttributes::eAllClear);

				for (int i = 0; i < mVerts.size(); i++)
					usedAttr |= mVerts[i].usedAttributes();

				return usedAttr;
			}

			void clear(void)
			{
				mMaterials.clear();
				mTris.clear();
				mBones.clear();
				mVerts.clear();
				mMorphTargets.clear();
				mShadowGeom = false;
			}

			// true on failure
			bool read(Stream& s) 
			{
				clear();

				enum { PrevGeomVersion = 0xABAB1012 };
				
				int ver = s.readValue<int>();
				if ((ver != GeomVersion) && (ver != PrevGeomVersion))
				{
					Error("Unigeom::Geom::readFromStream: Version not recognized!\n");
					return true;
				}

				int numM = 0, numB = 0, numT = 0, numV = 0, numMT = 0;
				
				s >> numM >> numB >> numT >> numV >> numMT;
								
				if ((numM < 0) || (numB < 0) || (numT < 0) || (numV < 0) || (numMT < 0))
				{
					Error("Unigeom::Geom::readFromStream: Header invalid!\n");
					clear();
					return true;
				}
						
				if (s.readValue<int>() != numM)
				{
					Error("Unigeom::Geom::readFromStream: File corrupted!\n");
					clear();
					return true;
				}

				for (int i = 0; i < numM; i++)
				{
					Material m;
					s >> m;
					MaterialIndex mi = mMaterials.insert(m).first;
					Assert(i == mi);
				}

				s.readVec(mBones, numB);
				if ((static_cast<int>(mBones.size()) != numB) || (s.errorStatus()))
				{
					Error("Unigeom::Geom::readFromStream: File corrupted!\n");
					clear();
					return true;
				}
								
				s.readVec(mTris, numT);
				if ((static_cast<int>(mTris.size()) != numT) || (s.errorStatus()))
				{
					Error("Unigeom::Geom::readFromStream: File corrupted!\n");
					clear();
					return true;
				}
				
				UnivertAttributes attr(s);
								
				int nv;
				s >> nv;
				Verify(numV == nv);
				
				mVerts.resize(numV);
				for (int i = 0; i < numV; i++)
					mVerts[i].read(s, attr);

				s.readVec(mMorphTargets, numMT);
				if ((static_cast<int>(mMorphTargets.size()) != numMT) || (s.errorStatus()))
				{
					Error("Unigeom::Geom::readFromStream: File corrupted!\n");
					clear();
					return true;
				}

				s >> mShadowGeom;
				
				ver = s.readValue<int>();
				if ((ver != GeomVersion) && (ver != PrevGeomVersion))
				{
					Error("Unigeom::Geom::readFromStream: File corrupted!\n");
					clear();
					return true;
				}
        															
				for (int i = 0; i < numM; i++)
					mMaterials[i].debugCheck();

				for (i = 0; i < numB; i++)
					mBones[i].debugCheck();

				for (int i = 0; i < numT; i++)
					mTris[i].debugCheck();

				for (int i = 0; i < numV; i++)
					mVerts[i].debugCheck();

				for (int i = 0; i < numMT; i++)
					mMorphTargets[i].debugCheck();
								
				return s.errorStatus();
			}

			void sortTrisByMaterial(void)
			{
				std::sort(mTris.begin(), mTris.end(), Tri::MaterialIndexComp);
			}
			
			friend Stream& operator<< (Stream& dst, const Geom& src)
			{
				src.write(dst);
				return dst;
			}

			friend Stream& operator>> (Stream& src, Geom& dst)
			{
				dst.read(src);
				return src;
			}
			      
		private:
			MaterialUnifier mMaterials;
			TriVec mTris;
			BoneVec mBones;
			UnivertVec mVerts;
			MorphTargetVec mMorphTargets;
			bool mShadowGeom;
		};

		class KeyFrame
		{
		public:
			KeyFrame(float time = 0.0f) : mTime(time)
			{
			}

			void clear(void)
			{
				mTime = 0.0f;
				mQForms.clear();
			}

			float time(void) const 
			{
				return mTime;
			}

			void setTime(float time)
			{
				mTime = time;
			}

			void resize(int size)
			{
				mQForms.resize(size);
			}

			void push_back(const QForm& q) 
			{
				mQForms.push_back(q);
			}

			int size(void) const
			{
				return static_cast<int>(mQForms.size());
			}

			const QForm& operator[] (int i) const { return mQForms[DebugRange(i, size())]; }
						QForm& operator[] (int i)				{ return mQForms[DebugRange(i, size())]; }

			const QFormVec& getQFormVec(void) const	{ return mQForms; }
						QFormVec& getQFormVec(void)				{ return mQForms; }

			bool write(Stream& s) const
			{
				s << mTime;
				s.writeVec(mQForms);
				return s.errorStatus();
			}

			bool read(Stream& s) 
			{
				s >> mTime;
				s.readVec(mQForms);
				return s.errorStatus();
			}

		protected:
			float mTime;
      QFormVec mQForms;
    };

		typedef std::vector<KeyFrame> KeyFrameVec;
		
		class Anim
		{
		public:
			Anim()
			{
			}

			~Anim()
			{
			}

			void clear(void)
			{
				mKeyFrames.clear();
			}

			void insertKeyFrame(const KeyFrame& keyFrame)
			{
        mKeyFrames.push_back(keyFrame);
			}

			int numKeyFrames(void) const
			{
				return static_cast<int>(mKeyFrames.size());
			}

			const KeyFrame& operator[] (KeyFrameIndex i) const { return mKeyFrames[DebugRange(i, numKeyFrames())]; }
			
			bool write(Stream& s) const
			{
				s.writeObj<int>(AnimVersion)
				 .writeObj(numKeyFrames());

				for (int i = 0; i < numKeyFrames(); i++)
					mKeyFrames[i].write(s);
				 
				return s.errorStatus();
			}

			// true on failure
			bool read(Stream& s) 
			{
				clear();

				if (s.readValue<int>() != AnimVersion)
				{
					Error("Unigeom::Anim::readFromStream: Version not recognized!\n");
					return true;
				}
				
				int numK = 0;
				s.readObj(numK);
				if (numK < 0)
				{
					Error("Unigeom::Anim::readFromStream: File corrupted!\n");
					clear();
					return true;
				}

				mKeyFrames.resize(numK);
				for (int i = 0; i < numK; i++)
				{
					if (mKeyFrames[i].read(s))
					{
						Error("Unigeom::Anim::readFromStream: File corrupted!\n");
						clear();
						return true;
					}
				}
				
				return s.errorStatus();
			}
      
		private:
			KeyFrameVec mKeyFrames;
		};
		
	} // namespace Unigeom

} // namespace gr

#endif // UNIGEOM_H


