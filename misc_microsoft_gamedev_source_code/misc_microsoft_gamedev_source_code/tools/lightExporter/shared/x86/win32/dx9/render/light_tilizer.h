//-----------------------------------------------------------------------------
// File: light_tilizer.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef LIGHT_TILIZER_H
#define LIGHT_TILIZER_H

#include "common/geom/convex_polyhedron.h"
#include "common/render/camera.h"

#include "dynamic_vb.h"
#include "light.h"
#include "render_viewport.h"
#include "effects.h"
#include "tilerizer.h"
#include "vertex_decl_manager.h"
#include "body_bounds.h"

namespace gr
{
	class LightTilizer
	{
	public:
		LightTilizer();
		~LightTilizer();

		void initDeviceObjects(void);
		void deleteDeviceObjects(void);
		void invalidateDeviceObjects(void);
		void restoreDeviceObjects(void);
		void oneTimeSceneInit(void);
		void startOfFrame(void);
		
		void beginScene(const RenderViewport& renderViewport);

		// true if culled
		bool setLight(const Light& params, const BodyBounds& bodyBounds, bool gatherStats);
						
		void clearStats(void);

		void displayTiles(void);
				
		int getTileWidth(void) const;
		int getTileHeight(void) const;

		void renderTiles(void) const;
		void renderShadowTiles(void) const;

		void setGroundPlane(const Plane& p);
		const Plane& getGroundPlane(void) const;
		const Plane& getGroundPlaneView(void) const;

		const ConvexPolyhedron& getPolyhedron(void) const {	return mPolyhedron;	}
					ConvexPolyhedron& getPolyhedron(void)				{	return mPolyhedron;	}	
		
		const ConvexPolyhedron& getRawPolyhedron(void) const	{ return mRawPolyhedron; }
					ConvexPolyhedron& getRawPolyhedron(void)				{ return mRawPolyhedron; }
		
		void setBodyBounds(const BodyBounds& bodyBounds);
		
	private:
		Effects& mEffects;
		Tilerizer mLightTilerizer;		// light tiles (updated per light)
		Tilerizer mBodyTilerizer;			// body tiles (updated per frame)
		Tilerizer mShadowOnlyTilerizer;		// shadow only tiles (updated per light)
		Tilerizer mBodyAndShadowTilerizer;				// union of body and shadow tiles (updated per light)
		Tilerizer mTempTilerizer;
		ConvexPolyhedron mPolyhedron;
		ConvexPolyhedron mTempPolyhedron;
		ConvexPolyhedron mRawPolyhedron;
		DynamicVB mDynamicVB;
		RenderViewport mRenderViewport;
		
		float mTileDepthFudge;
		int mTileVBOfs, mTileVBNumTris;
		int mShadowTileVBOfs, mShadowTileVBNumTris;
		
		bool mGroundPlaneClip;

		Plane mGroundPlaneWorld;
		Plane mGroundPlaneView;

		float mSpotSphereRadiusFudge;
		float mSpotConeAngleFudge;
		int mSpotConeElements;
		float mOmniSphereRadiusFudge;
		int mOmniSphereElementsA;
		int mOmniSphereElementsB;
		
		bool mShadowOnlyTilerizerValid;
		bool mWireframeBounds;
						
		enum EViewTileModes
		{
			eViewTileOff,
			eViewTileLight,
			eViewTileBody,
			eViewTileShadow,
			eViewTileBodyAndShadow,
			eNumModes
		};
		
		int mViewTilerMode;
		
		struct TileStats
		{
			uchar mNumLights;

			TileStats()
			{
				clear();
			}

			void clear(void)
			{
				mNumLights = 0;
			}
		};

		std::vector<TileStats> mTileStats;
								
		void fillTileVB(const Light& light, bool gatherStats);
		
		void fillTileCellsVB(
			int& vbOfs, int& numTris, 
			const Tilerizer& tilerizer,
			const TileStats* pTileStats);
		
		D3DCOLOR colorRamp(float v, float vMin, float vMax);
		
		void renderWireframeTiles(int vbOfs, int numTris);
		bool createPolyhedron(const Light& params);
		void resize(void);
		void initTweakers(void);
		void initTileRenderEffectConstants(void);
		void rasterizeLightPolyhedron(void);
		void rasterizeBodyShadow(const Light& light, const BodyBounds& bodyBounds);
		void createTileVerts(TileVertex* pDstVerts, const Tilerizer& tilerizer,	bool gatherStats);
	};

} // namespace gr

#endif // LIGHT_TILIZER_H
