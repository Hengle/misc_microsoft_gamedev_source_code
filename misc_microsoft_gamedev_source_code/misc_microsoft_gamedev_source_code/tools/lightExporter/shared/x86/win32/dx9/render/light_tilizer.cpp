//-----------------------------------------------------------------------------
// File: light_tilizer.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "light_tilizer.h"

#include "x86/win32/dx9/utils/tweaker_dx9.h"
#include "render_engine.h"

namespace gr
{
	static const int gMaxDynamicVBSize = 2 * 1200 * 6 * 15 * sizeof(TileColorVertex);
	
	static const int gTileWidth = 32;
	static const int gTileHeight = 32;

	LightTilizer::LightTilizer() :
		mLightTilerizer(1, 1, 1, 1),
		mBodyTilerizer(1, 1, 1, 1),
		mShadowOnlyTilerizer(1, 1, 1, 1),
		mBodyAndShadowTilerizer(1, 1, 1, 1),
		mTempTilerizer(1, 1, 1, 1),
		mTileVBOfs(0), mTileVBNumTris(0),
		mShadowTileVBOfs(0), mShadowTileVBNumTris(0),
		mGroundPlaneWorld(0, 1, 0, -10),
		mTileDepthFudge(32),
		mGroundPlaneClip(true),
		mSpotSphereRadiusFudge(1.14f), //085f),
		mSpotConeAngleFudge(1.04f),
		mSpotConeElements(12),
		mOmniSphereRadiusFudge(1.18f), //099f),
		mOmniSphereElementsA(6),
		mOmniSphereElementsB(6),
		mEffects(RenderEngine::effects()),
		mWireframeBounds(false),
		mShadowOnlyTilerizerValid(true),
		mViewTilerMode(eViewTileOff)
	{
	}

	LightTilizer::~LightTilizer()
	{
	}

	void LightTilizer::initDeviceObjects(void)
	{
		resize();

		mDynamicVB.resize(gMaxDynamicVBSize);
	}

	void LightTilizer::deleteDeviceObjects(void)
	{
	}

	void LightTilizer::invalidateDeviceObjects(void)
	{
		mDynamicVB.release();
	}

	void LightTilizer::restoreDeviceObjects(void)
	{
		resize();

		mDynamicVB.restore();
	}

	void LightTilizer::oneTimeSceneInit(void)
	{
		initTweakers();
	}

	void LightTilizer::startOfFrame(void)
	{
	}

	void LightTilizer::beginScene(const RenderViewport& renderViewport)
	{
		mRenderViewport = renderViewport;
		mGroundPlaneView = Plane::transformOrthonormal(mGroundPlaneWorld, renderViewport.camera().worldToView());
	}
		
	int LightTilizer::getTileWidth(void) const
	{
		return gTileWidth;
	}

	int LightTilizer::getTileHeight(void) const
	{
		return gTileHeight;
	}

	void LightTilizer::resize(void)
	{
		mLightTilerizer.resize(
			RenderEngine::bufferManager().getBackbufferWidth(), 
			RenderEngine::bufferManager().getBackbufferHeight(),
			gTileWidth,
			gTileHeight);
			
		mBodyTilerizer.resize(
			RenderEngine::bufferManager().getBackbufferWidth(), 
			RenderEngine::bufferManager().getBackbufferHeight(),
			gTileWidth,
			gTileHeight);
			
		mShadowOnlyTilerizer.resize(
			RenderEngine::bufferManager().getBackbufferWidth(), 
			RenderEngine::bufferManager().getBackbufferHeight(),
			gTileWidth,
			gTileHeight);
			
		mBodyAndShadowTilerizer.resize(
			RenderEngine::bufferManager().getBackbufferWidth(), 
			RenderEngine::bufferManager().getBackbufferHeight(),
			gTileWidth,
			gTileHeight
			);
			
		mTempTilerizer.resize(
			RenderEngine::bufferManager().getBackbufferWidth(), 
			RenderEngine::bufferManager().getBackbufferHeight(),
			gTileWidth,
			gTileHeight
			);
		
		mTileStats.resize(mLightTilerizer.cellsTotal());
	}

	void LightTilizer::initTweakers(void)
	{
		static const char* pViewTilerizerModes[] = { "Off", "Light", "Body", "Shadow", "Body and Shadow" };
		gSharedTweaker.createEntry(eTilizer, new IntegralTweakerEntry("View tilizer cells", &mViewTilerMode, pViewTilerizerModes, eNumModes, 0, NULL, true, false));
		
		gSharedTweaker.createEntry(eTilizer, new IntegralTweakerEntry("Tile depth bias", &mTileDepthFudge, -1000, 1000, 1));
		gSharedTweaker.createEntry(eTilizer, new IntegralTweakerEntry("Omni sphere radius bias", &mOmniSphereRadiusFudge, 1.0f, 2.0f, .005f, 0, NULL, false));
		gSharedTweaker.createEntry(eTilizer, new IntegralTweakerEntry("Omni sphere elements A", &mOmniSphereElementsA, 3, 25, 1, true, 0, NULL, false));
		gSharedTweaker.createEntry(eTilizer, new IntegralTweakerEntry("Omni sphere elements B", &mOmniSphereElementsB, 4, 25, 1, true, 0, NULL, false));
		gSharedTweaker.createEntry(eTilizer, new IntegralTweakerEntry("Spot sphere radius bias", &mSpotSphereRadiusFudge, 1.0f, 2.0f, .005f, 0, NULL, false));
		gSharedTweaker.createEntry(eTilizer, new IntegralTweakerEntry("Spot cone angle bias", &mSpotConeAngleFudge, 1.0f, 2.0f, .005f, 0, NULL, false));
		gSharedTweaker.createEntry(eTilizer, new IntegralTweakerEntry("Spot elements", &mSpotConeElements, 4, 25, 1, true, 0, NULL, false));
		gSharedTweaker.createEntry(eTilizer, new IntegralTweakerEntry("Ground plane clipping", &mGroundPlaneClip));
		gSharedTweaker.createEntry(eTilizer, new IntegralTweakerEntry("Wireframe body bounds", &mWireframeBounds));
	}

	void LightTilizer::setGroundPlane(const Plane& p)
	{
		mGroundPlaneWorld = p;
	}
	
	const Plane& LightTilizer::getGroundPlane(void) const
	{
		return mGroundPlaneWorld;
	}
	
	const Plane& LightTilizer::getGroundPlaneView(void) const
	{
		return mGroundPlaneView;
	}

	void LightTilizer::clearStats(void)
	{
		for (uint i = 0; i < mTileStats.size(); i++)
			mTileStats[i].clear();
	}
		
	// true if culled
	bool LightTilizer::createPolyhedron(const Light& light)
	{
		Vec4 viewPos(light.mPos * mRenderViewport.camera().worldToView());
		Vec4 viewDir(Matrix44::transformNormal(light.mDir, mRenderViewport.camera().worldToView()));

		if (light.mType == eSpot)
		{
			//static int e = 12;
			//static float oe = 1.085f;
			//static float re = 1.085f;
				
			mPolyhedron.createCappedCone(viewPos, viewDir, light.mSpotOuter * mSpotConeAngleFudge, light.mRadius * mSpotSphereRadiusFudge, mSpotConeElements);
		}
		else
		{
			//static int a = 9, b = 9;
			//static float re = 1.085f;
			//static int a = 6, b = 6;
			//static float re = 1.099f;
			mPolyhedron.createSphere(viewPos, light.mRadius * mOmniSphereRadiusFudge, mOmniSphereElementsA, mOmniSphereElementsB);
		}

		mRawPolyhedron = mPolyhedron;
		
		if (mGroundPlaneClip)
		{
			mPolyhedron.clipAgainstPlane(mTempPolyhedron, mGroundPlaneView);
			if (mTempPolyhedron.empty())
				return true;
		
			mTempPolyhedron.clipAgainstFrustum(mPolyhedron, mRenderViewport.camera().viewFrustum());
			if (mPolyhedron.empty())
				return true;
		}
		else
		{
			ConvexPolyhedron(mPolyhedron).clipAgainstFrustum(mPolyhedron, mRenderViewport.camera().viewFrustum());
			if (mPolyhedron.empty())
				return true;
		}

		return false;
	}
			
	void LightTilizer::rasterizeLightPolyhedron(void)
	{
		mLightTilerizer.clearBounds();
		mLightTilerizer.clearCells();
								
		const Matrix44& viewToScreen = mRenderViewport.viewToScreenMatrix();
		
		for (int i = 0; i < mPolyhedron.numTris(); i++)
		{
			Vec4 v[3];
						
			for (int j = 0; j < 3; j++)
				v[j] = Vec4(mPolyhedron.vert(mPolyhedron.tri(i)[j]), 1.0f) * viewToScreen;
				
			mLightTilerizer.rasterizeScreenspaceTri(v, mRenderViewport);
		}
	}
	
	void LightTilizer::rasterizeBodyShadow(const Light& light, const BodyBounds& bodyBounds)
	{
		if (!light.mShadows)
			return;
		
		mShadowOnlyTilerizer.clearBounds();
		mShadowOnlyTilerizer.clearCells();
		mShadowOnlyTilerizerValid = true;
		
		const Matrix44& worldToView = mRenderViewport.camera().worldToView();
		
		Vec3Vec verts;
		verts.reserve(4);
		
		Vec3Vec shadowVerts;
		shadowVerts.reserve(4);
		
		const Vec3 viewLightPos(light.mPos * worldToView);
		
		// HACK HACK
		const Plane groundPlaneView(
			Plane::transformOrthonormal(Plane(0, 1, 0, .6f), mRenderViewport.camera().worldToView())
			);
				
		int numFailed = 0;
		
		for (int i = 0; i < bodyBounds.numNodes(); i++)
		{
			const AABB& bounds = bodyBounds[i].getBounds();
			const Matrix44& world = bodyBounds[i].getBoundsToWorld();
			
			const Matrix44 view(world * worldToView);
						
			for (int axis = 0; axis < 3; axis++)
			{
				const int axis1 = (axis + 1) % 3;
				const int axis2 = (axis + 2) % 3;
				
				for (int side = 0; side < 2; side++)
				{
					verts.resize(4);
					
					for (int k = 0; k < 4; k++)
					{
						Vec4 t;
						t[axis] = bounds[side][axis];
						t[axis1] = bounds[k >> 1][axis1];
						t[axis2] = bounds[(k & 1) ^ (k >> 1)][axis2];
						t[3] = 1.0f;
						
						verts[k] = t * view;
					}
															
					if ((light.mType == eSpot) || (light.mType == eOmnidirectional))
					{
						shadowVerts.resize(4);
						
						for (int k = 0; k < 4; k++)
						{
							EResult res = Intersection::ray3Plane(
								shadowVerts[k], groundPlaneView, 
								Ray3(viewLightPos, (verts[k] - viewLightPos).normalize()),
								2000.0f
								);
								
							if (SUCCESS != res)
							{
								numFailed++;
								break;
							}
						}
																								
						if (mWireframeBounds)
						{
							RenderEngine::drawLine(Vec4(shadowVerts[0], 1) * mRenderViewport.camera().viewToWorld(),	Vec4(shadowVerts[1], 1) * mRenderViewport.camera().viewToWorld(), Vec3(1,1,1));
							RenderEngine::drawLine(Vec4(shadowVerts[1], 1) * mRenderViewport.camera().viewToWorld(),	Vec4(shadowVerts[2], 1) * mRenderViewport.camera().viewToWorld(), Vec3(1,1,1));
							RenderEngine::drawLine(Vec4(shadowVerts[2], 1) * mRenderViewport.camera().viewToWorld(),	Vec4(shadowVerts[3], 1) * mRenderViewport.camera().viewToWorld(), Vec3(1,1,1));
							RenderEngine::drawLine(Vec4(shadowVerts[3], 1) * mRenderViewport.camera().viewToWorld(),	Vec4(shadowVerts[0], 1) * mRenderViewport.camera().viewToWorld(), Vec3(1,1,1));
						}
						
						if (4 == k)
							mShadowOnlyTilerizer.rasterizeViewspacePoly(shadowVerts, mRenderViewport, false);
					}
				}
			}							
		}
		
		// HACK HACK
		if (numFailed > (bodyBounds.numNodes() * 6 * .75f))
			mShadowOnlyTilerizerValid = false;
	}
	
	void LightTilizer::setBodyBounds(const BodyBounds& bodyBounds)
	{
		mBodyTilerizer.clearBounds();
		mBodyTilerizer.clearCells();
		
		const Matrix44& worldToView = mRenderViewport.camera().worldToView();
		
		Vec3Vec verts;
		verts.reserve(4);
			
		for (int i = 0; i < bodyBounds.numNodes(); i++)
		{
			const AABB& bounds = bodyBounds[i].getBounds();
			const Matrix44& world = bodyBounds[i].getBoundsToWorld();
			
			const Matrix44 view(world * worldToView);
						
			for (int axis = 0; axis < 3; axis++)
			{
				const int axis1 = (axis + 1) % 3;
				const int axis2 = (axis + 2) % 3;
				
				for (int side = 0; side < 2; side++)
				{
					verts.resize(4);
					
					for (int k = 0; k < 4; k++)
					{
						Vec4 t;
						t[axis] = bounds[side][axis];
						t[axis1] = bounds[k >> 1][axis1];
						t[axis2] = bounds[(k & 1) ^ (k >> 1)][axis2];
						t[3] = 1.0f;
						
						verts[k] = t * view;
					}

					if (mWireframeBounds)					
					{
						RenderEngine::drawLine(Vec4(verts[0], 1) * mRenderViewport.camera().viewToWorld(),	Vec4(verts[1], 1) * mRenderViewport.camera().viewToWorld(), Vec3(1,1,1));
						RenderEngine::drawLine(Vec4(verts[1], 1) * mRenderViewport.camera().viewToWorld(),	Vec4(verts[2], 1) * mRenderViewport.camera().viewToWorld(), Vec3(1,1,1));
						RenderEngine::drawLine(Vec4(verts[2], 1) * mRenderViewport.camera().viewToWorld(),	Vec4(verts[3], 1) * mRenderViewport.camera().viewToWorld(), Vec3(1,1,1));
						RenderEngine::drawLine(Vec4(verts[3], 1) * mRenderViewport.camera().viewToWorld(),	Vec4(verts[0], 1) * mRenderViewport.camera().viewToWorld(), Vec3(1,1,1));
					}
					
					mBodyTilerizer.rasterizeViewspacePoly(verts, mRenderViewport, false);
				}
			}							
		}
	}
		
	void LightTilizer::createTileVerts(
		TileVertex* pDstVerts,
		const Tilerizer& tilerizer,
		bool gatherStats)
	{
		const Vec4 cz(mRenderViewport.camera().viewToProj().getColumn(2));
		const Vec4 cw(mRenderViewport.camera().viewToProj().getColumn(3));
		const Vec4 wToZ(cz.z, cz.w, cw.z, cw.w);
   
		for (int ty = tilerizer.getTop(); ty <= tilerizer.getBottom(); ty++)
		{
			for (int tx = tilerizer.getLeft(); tx <= tilerizer.getRight(); tx++)
			{
				if (tilerizer.isCellOccupied(tx, ty))
				{
					if (gatherStats)
					{
						uchar& numLights = mTileStats[DebugRange(tx + ty * tilerizer.cellsX(), mTileStats.size())].mNumLights;
						numLights++;
					}

					double w = (1.0f / tilerizer.getCellLimits(tx, ty).lowestZ()) + mTileDepthFudge;
					float z = (w * wToZ.x + wToZ.y) / (w * wToZ.z + wToZ.w);

					pDstVerts[0].set(tx, ty, z);
					pDstVerts[3].set(tx, ty, z);
					pDstVerts[1].set(tx + 1, ty, z);
					pDstVerts[2].set(tx + 1, ty + 1, z);
					pDstVerts[4].set(tx + 1, ty + 1, z);
					pDstVerts[5].set(tx, ty + 1, z);
					pDstVerts += 6;
				}
			}
		}
	}
		
	void LightTilizer::fillTileVB(const Light& light, bool gatherStats)
	{	
		int totalTiles = mLightTilerizer.getNumSetTiles();
		
		if (light.mShadows)
		{
			if (mShadowOnlyTilerizerValid)
			{
				mTempTilerizer.unionOp(mBodyTilerizer, mShadowOnlyTilerizer);
				mBodyAndShadowTilerizer.andOp(mTempTilerizer, mLightTilerizer);
			}
			else
			{
				// FIXME: slow fallback path in case the crappy body projection code doesn't work
				mBodyAndShadowTilerizer.assignOp(mLightTilerizer);
			}
			
			totalTiles += mBodyAndShadowTilerizer.getNumSetTiles();
		}
									
		const int VerticesPerTile = 6;
		const int TrisPerTile = 2;
		int totalVertexBytes = totalTiles * VerticesPerTile * sizeof(TileVertex);

		mTileVBNumTris = mLightTilerizer.getNumSetTiles() * TrisPerTile;
		TileVertex* pVerts = mDynamicVB.lock<TileVertex>(totalVertexBytes, mTileVBOfs);

		createTileVerts(pVerts, mLightTilerizer, gatherStats);
		
		if (light.mShadows)
		{
			mShadowTileVBNumTris = mBodyAndShadowTilerizer.getNumSetTiles() * TrisPerTile;
			mShadowTileVBOfs = mTileVBOfs + mLightTilerizer.getNumSetTiles() * VerticesPerTile * sizeof(TileVertex);
			
			createTileVerts(pVerts + mLightTilerizer.getNumSetTiles() * VerticesPerTile, mBodyAndShadowTilerizer, false);
		}
		else
		{
			mShadowTileVBNumTris = 0;
			mShadowTileVBOfs = -1;
		}		
					
		mDynamicVB.unlock();
	}
	
	void LightTilizer::initTileRenderEffectConstants(void)
	{
		const Vec4 tileMul(gTileWidth, gTileHeight, 0, 0);
		const Vec4 tileAdd(0, 0, 0, 0);
		const Vec4 ooScreenDim(1.0f / RenderEngine::bufferManager().getBackbufferWidth(), 1.0f / RenderEngine::bufferManager().getBackbufferHeight(), 0, 0);
		
		RenderEngine::effects().setVector("gTileMul", tileMul);
		RenderEngine::effects().setVector("gTileAdd", tileAdd);
		RenderEngine::effects().setVector("gOOScreenDim", ooScreenDim);
		RenderEngine::effects().setMatrix(eScreenToView);
		RenderEngine::effects().setMatrix(eScreenToProj);
		//mEffects.setFloat("gmTileDepthFudge", mTileDepthFudge);
		//RenderEngine::effects().setVector("gWToZ", wToZ);
	}

	void LightTilizer::renderTiles(void) const
	{
		RenderEngine::vertexDeclManager().setToDevice(eTileVertexDeclaration);

		D3D::setStreamSource(0, mDynamicVB, mTileVBOfs, sizeof(TileVertex));

		D3D::drawPrimitive(D3DPT_TRIANGLELIST, 0, mTileVBNumTris);
	}
	
	void LightTilizer::renderShadowTiles(void) const
	{
		Verify(-1 != mShadowTileVBOfs);
		
		RenderEngine::vertexDeclManager().setToDevice(eTileVertexDeclaration);

		D3D::setStreamSource(0, mDynamicVB, mShadowTileVBOfs, sizeof(TileVertex));

		D3D::drawPrimitive(D3DPT_TRIANGLELIST, 0, mShadowTileVBNumTris);
	}

	D3DCOLOR LightTilizer::colorRamp(float s, float sMin, float sMax)
	{
		s = Math::Clamp(s, sMin, sMax);

	  const float ds = sMax - sMin;

		float r = 0, g = 0, b = 0;

   	if (s < (sMin + 0.25f * ds)) 
		{
     	g = 4.0f * (s - sMin) / ds;
			b = 1.0f;
   	} 
		else if (s < (sMin + 0.5f * ds)) 
		{
			g = 1.0f;
      b = 1.0f + 4.0f * (sMin + 0.25f * ds - s) / ds;
   	} 
		else if (s < (sMin + 0.75f * ds)) 
		{
     	r = 4.0f * (s - sMin - 0.5f * ds) / ds;
			g = 1.0f;
   	} 
		else 
		{
			r = 1.0f;
     	g = 1.0f + 4.0f * (sMin + 0.75f * ds - s) / ds;
   	}

		const int ir = Math::FloatToIntTrunc(r * 255.0f + .5f);
		const int ig = Math::FloatToIntTrunc(g * 255.0f + .5f);
		const int ib = Math::FloatToIntTrunc(b * 255.0f + .5f);

		return D3DCOLOR_ARGB(255, ir, ig, ib);
	}
		
	void LightTilizer::fillTileCellsVB(
		int& vbOfs, int& numTris, 
		const Tilerizer& tilerizer,
		const TileStats* pTileStats)
	{
		vbOfs = 0;
		numTris = 0;

		int numActiveTiles = 0;
		int maxLights = 0;
		
		if (pTileStats)
		{		
			for (int ty = 0; ty < mLightTilerizer.cellsY(); ty++)
			{
				for (int tx = 0; tx < mLightTilerizer.cellsX(); tx++)
				{
					const uchar& numLights = pTileStats[DebugRange(tx + ty * mLightTilerizer.cellsX(), mTileStats.size())].mNumLights;
					if (numLights)
						numActiveTiles++;
						
					maxLights = Math::Max<int>(maxLights, numLights);
				}
			}
		}
		else
		{
			numActiveTiles = tilerizer.getNumSetTiles();
		}
				
		if (!numActiveTiles)
			return;
				
		int numTiles = numActiveTiles;
		const int VerticesPerTile = 6;
		const int TrisPerTile = 2;
		int numVertexBytes = numTiles * VerticesPerTile * sizeof(TileColorVertex);

		numTris = numTiles * TrisPerTile;

		TileColorVertex* pVerts = mDynamicVB.lock<TileColorVertex>(numVertexBytes, vbOfs);
		TileColorVertex* pDstVerts = pVerts;
		 		
		for (int ty = 0; ty < tilerizer.cellsY(); ty++)
		{
			for (int tx = 0; tx < tilerizer.cellsX(); tx++)
			{
				bool draw = false;
				
				D3DCOLOR c = D3DCOLOR_ARGB(255, 255, 255, 255);
					
				if (pTileStats)
				{
					const uchar& numLights = pTileStats[DebugRange(tx + ty * mLightTilerizer.cellsX(), mTileStats.size())].mNumLights;
					if (numLights)
					{
						draw = true;
						c = colorRamp(numLights, 0, maxLights);
					}
				}
				else
				{
					draw = tilerizer.isCellOccupied(tx, ty);
				}
				
				if (draw)
				{
					const float z = 0;	
					pDstVerts[0].set(tx, ty, z, c);
					pDstVerts[3].set(tx, ty, z, c);
					pDstVerts[1].set(tx + 1, ty, z, c);
					pDstVerts[2].set(tx + 1, ty + 1, z, c);
					pDstVerts[4].set(tx + 1, ty + 1, z, c);
					pDstVerts[5].set(tx, ty + 1, z, c);
					pDstVerts += 6;
				}
			}
		}
		
		Assert((pDstVerts - pVerts) == numTiles * VerticesPerTile);

		mDynamicVB.unlock();
	}
	
	void LightTilizer::renderWireframeTiles(int vbOfs, int numTris)
	{
		mEffects.beginTechnique("WireframeTiles");

		RenderEngine::vertexDeclManager().setToDevice(eTileColorVertexDeclaration);
		
		D3D::setStreamSource(0, mDynamicVB, vbOfs, sizeof(TileColorVertex));

		D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);		

		D3D::disableCulling();
		
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		
		D3D::drawPrimitive(D3DPT_TRIANGLELIST, 0, numTris);

		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

		D3D::enableCulling();

		D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);		

		mEffects.endTechnique();

		D3D::setVertexShader(NULL);
		D3D::setPixelShader(NULL);
		RenderEngine::vertexDeclManager().setToDevice(eInvalidDeclaration);
	}

	void LightTilizer::displayTiles(void)
	{
		if (eViewTileOff == mViewTilerMode)
			return;
			
		int vbOfs, numTris;
		switch (mViewTilerMode)
		{
			case eViewTileLight:
				fillTileCellsVB(vbOfs, numTris, mLightTilerizer, &mTileStats[0]);
				break;
			case eViewTileBody:
				fillTileCellsVB(vbOfs, numTris, mBodyTilerizer, NULL);
				break;
			case eViewTileShadow:
				fillTileCellsVB(vbOfs, numTris, mShadowOnlyTilerizer, NULL);
				break;
			case eViewTileBodyAndShadow:
				fillTileCellsVB(vbOfs, numTris, mBodyAndShadowTilerizer, NULL);
				break;
		}
		
		renderWireframeTiles(vbOfs, numTris);
	}

	bool LightTilizer::setLight(const Light& light, const BodyBounds& bodyBounds, bool gatherStats)
	{
		if (createPolyhedron(light))
			return true;
					
		rasterizeLightPolyhedron();
		if (0 == mLightTilerizer.getNumSetTiles())
			return true;
			
		rasterizeBodyShadow(light, bodyBounds);
		
		fillTileVB(light, gatherStats);
		
		initTileRenderEffectConstants();
				
		return false;
	}

} // namespace gr








