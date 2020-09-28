// lighting_engine.cpp
#include "lighting_engine.h"
#include "shader_manager.h"
#include "device.h"
#include "x86/core/timer_x86.h"
#include "common/geom/unigeom.h"

namespace gr
{
	const float gGlareBufferScale = 64.0f;

	static const int gMaxDynamicVBSize = 1200 * 6 * 15 * sizeof(TileVertex);
	
	static const int gTileWidth = 32;
	static const int gTileHeight = 32;

  static const float gQuadXOrder[] = { 0,1,1, 0,1,0 };
  static const float gQuadYOrder[] = { 0,0,1, 0,1,1 };

	// Also update shaders.fx!
	static const float gSpecPowerLow = Unigeom::Material::SpecPowerLow;
	static const float gSpecPowerHigh = Unigeom::Material::SpecPowerHigh;

  LightingEngine gLightingEngine;

	LightingEngine::LightingEngine() :
		mDisplayGamma(2.2f),
		mExposure(0.0f),
		mCurToneMapper(ePhotoToneMapper),
		mGroundPlaneWorld(0, 1, 0, -10),
		mTilerizer(1, 1, 1, 1)
  {
  }
  
  LightingEngine::~LightingEngine()
  {
  }

	int LightingEngine::getTileWidth(void) const
	{
		return gTileWidth;
	}

	int LightingEngine::getTileHeight(void) const
	{
		return gTileHeight;
	}

	void LightingEngine::resizeTilerizer(void)
	{
		mTilerizer.resize(
			gShaderManager.getBackbufferWidth(), 
			gShaderManager.getBackbufferHeight(),
			gTileWidth,
			gTileHeight);
	}

  void LightingEngine::init(void)
  {
		resizeTilerizer();

		mDynamicVB.resize(gMaxDynamicVBSize);
  }
  
  void LightingEngine::deinit(void)
  {
  }

  void LightingEngine::startOfFrame(void)
  {
  }

  void LightingEngine::release(void)
  {
		mDynamicVB.release();
  }
  
  void LightingEngine::restore(void)
  {
		resizeTilerizer();

		mDynamicVB.restore();
  }
	
  void LightingEngine::beginScene(void)
  {
		gShaderManager.setRenderTarget(0, gShaderManager.getSurface(eDBuffer));
    gShaderManager.setRenderTarget(1, gShaderManager.getSurface(eCBuffer));
    gShaderManager.setRenderTarget(2, gShaderManager.getSurface(eNBuffer));
		gShaderManager.setRenderTarget(3, gShaderManager.getSurface(eTBuffer));
    
    // assumes viewport is entire texture!

    D3D::clear(D3DCLEAR_TARGET);
  }

	void LightingEngine::setMaterialParams(
		float bumpZScale,
		float metalness,
		float specPower,
		float specInten,
		bool skin)
	{
		gShaderManager.setEffectFloat("gBumpZScale", 1.0f / bumpZScale );
		gShaderManager.setEffectVector("gMaterialParams", 
			Vec4(
				metalness, 
				skin, 
				Math::Clamp((specPower - gSpecPowerLow) / (gSpecPowerHigh - gSpecPowerLow), 0.0f, 1.0f),
				specInten * .5f));
	}

	void LightingEngine::setGroundPlane(const Plane& p)
	{
		mGroundPlaneWorld = p;
	}
	
	const Plane& LightingEngine::getGroundPlane(void) const
	{
		return mGroundPlaneWorld;
	}
  
  void LightingEngine::endScene(void)
  {
    gShaderManager.setRenderTarget(0, gShaderManager.getSurface(eKBuffer));
    gShaderManager.setRenderTarget(1, NULL);
    gShaderManager.setRenderTarget(2, NULL);
		gShaderManager.setRenderTarget(3, NULL);
		    
    // should set viewport here!
  }

	void LightingEngine::makeSSQ(P4Vertex* quadVerts, bool viewVector)
  {
    const Matrix44& screenToView = gShaderManager.getMatrix(eScreenToView);
    
    memset(quadVerts, 0, sizeof(P4Vertex) * 6);
        
    for (int i = 0; i < 6; i++)
    {
      const float x = gQuadXOrder[i];
      const float y = gQuadYOrder[i];

      Vec4 screenPos(
        Vec4(gShaderManager.getViewportX(), gShaderManager.getViewportY(), 0, 1.0f) +
        Vec4::multiply(Vec4(gShaderManager.getViewportWidth(), gShaderManager.getViewportHeight(), 0, 0), Vec4(x, y, 0, 0))
        );
                  
      const Vec4 viewPos((screenPos * screenToView).toPoint());

      quadVerts[i].position = (Vec4(-.5f, -.5f, 0.0f, 0.0f) + screenPos);
      quadVerts[i].uv[0] = Vec<4>(x, y, 0, 1.0f);
      quadVerts[i].uv[1] = viewVector ? viewPos : Vec<4>(0,0,0,1);
      quadVerts[i].uv[2] = Vec<4>(0,0,0,1);
      quadVerts[i].uv[3] = Vec<4>(0,0,0,1);
    }
  }

	void LightingEngine::renderSSQ(bool viewVector)
	{
		P4Vertex quadVerts[6];
		makeSSQ(quadVerts, viewVector);
	
		gShaderManager.setVertexDeclaration(eP4Declaration);
			
		D3D::drawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadVerts, sizeof(quadVerts[0]));
	}

	void LightingEngine::beginSSQPasses(void)
	{
		D3D::disableCulling();
    D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    D3D::setRenderState(D3DRS_ZWRITEENABLE, FALSE);
		D3D::clearTextures();
	}

	void LightingEngine::endSSQPasses(void)
	{
		gShaderManager.setRenderTarget(0, gShaderManager.getSurface(eKBuffer));
		gShaderManager.setRenderTarget(1, NULL);
		gShaderManager.setRenderTarget(2, NULL);
		gShaderManager.setRenderTarget(3, NULL);

    D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    D3D::setRenderState(D3DRS_ZWRITEENABLE, TRUE);
    
    D3D::enableCulling();

    gShaderManager.setVertexShader(eInvalidVS);
    gShaderManager.setPixelShader(eInvalidPS);
		gShaderManager.setVertexDeclaration(eInvalidDeclaration);
  
    D3D::clearTextures();

		for (int i = 0; i < D3D::NumSamplers; i++)
    {
      D3D::setWrapAddressing(i);
      D3D::setLinearFiltering(i);
    }
	}

  void LightingEngine::beginLight(void)
  {
    gShaderManager.setRenderTarget(0, gShaderManager.getSurface(eDABuffer));
		gShaderManager.setRenderTarget(1, gShaderManager.getSurface(eSABuffer));
    
    D3D::clear(0L, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0L);

		D3D::disableCulling();

    D3D::setRenderState(D3DRS_ZWRITEENABLE, FALSE);
		D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
		
		mViewFrustum.set(gShaderManager.getMatrix(eViewToProj));

		mGroundPlaneView = Plane::transformOrthonormal(mGroundPlaneWorld, gShaderManager.getMatrix(eWorldToView));
  }
  
	// true if culled
	bool LightingEngine::createPolyhedron(
		const LightParams& params,
		const Vec4& viewPos,
		const Vec4& viewDir)
	{
		if (params.mType == eSpot)
		{
			static int e = 12;
			static float oe = 1.085f;
			static float re = 1.085f;
				
			mPolyhedron.createCappedCone(viewPos, viewDir, params.mSpotOuter * oe, params.mRadius * re, e);
		}
		else
		{
			static int a = 9, b = 9;
			static float re = 1.085f;
			mPolyhedron.createSphere(viewPos, params.mRadius * re, a, b);
		}

		mPolyhedron.clipAgainstPlane(mTempPolyhedron, mGroundPlaneView);
    if (mTempPolyhedron.empty())
			return true;

		mTempPolyhedron.clipAgainstFrustum(mPolyhedron, mViewFrustum);
		if (mPolyhedron.empty())
			return true;

		return false;
	}

	// true if culled
	bool LightingEngine::findTiles(void)
	{
		mTilerizer.clearBounds();
		mTilerizer.clearCells();

		const Matrix44 viewToScreen(gShaderManager.getMatrix(eViewToProj) * gShaderManager.getMatrix(eProjToScreen));
		const int width = gShaderManager.getBackbufferWidth() - 1;
		const int height = gShaderManager.getBackbufferHeight() - 1;

		static bool backfaceCull = true;

		for (int i = 0; i < mPolyhedron.numTris(); i++)
		{
			Vec3 v[3];

			for (int j = 0; j < 3; j++)
			{
				const Vec4 t(Vec4(mPolyhedron.vert(mPolyhedron.tri(i)[j]), 1.0f) * viewToScreen);
				
				const float oow = (t.w > 0.0f) ? (1.0f / t.w) : 0.0f;

				v[j][0] = t.x * oow;
				v[j][1] = t.y * oow;
				v[j][2] = oow;

				v[j][0] = Math::Clamp<float>(v[j][0], 0, width);
				v[j][1] = Math::Clamp<float>(v[j][1], 0, height);
			}

			Plane p;
			bool badTri = p.setFromTriangle(Vec3(v[0][0], v[0][1], 0), Vec3(v[1][0], v[1][1], 0),	Vec3(v[2][0], v[2][1], Math::fTinyEpsilon));
			
			if (!badTri)
			{
				if ((!backfaceCull) || (p.normal()[2] < 0.0f))
       		mTilerizer.rasterizeTri(v);
			}
		}

		return 0 == mTilerizer.getNumSetTiles();
	}

	void LightingEngine::fillTileVB(int& vbOfs, int& numTris)
	{
		int numTiles = mTilerizer.getNumSetTiles();
		const int VerticesPerTile = 6;
		const int TrisPerTile = 2;
		int numVertexBytes = numTiles * VerticesPerTile * sizeof(TileVertex);

		numTris = numTiles * TrisPerTile;

		TileVertex* pVerts = mDynamicVB.lock<TileVertex>(numVertexBytes, vbOfs);
		TileVertex* pDstVerts = pVerts;

		const float wFudge = 16;
		const Vec4 cz(gShaderManager.getMatrix(eViewToProj).getColumn(2));
		const Vec4 cw(gShaderManager.getMatrix(eViewToProj).getColumn(3));
		const Vec4 wToZ(cz.z, cz.w, cw.z, cw.w);
   
		for (int ty = mTilerizer.getTop(); ty <= mTilerizer.getBottom(); ty++)
		{
			for (int tx = mTilerizer.getLeft(); tx <= mTilerizer.getRight(); tx++)
			{
				if (mTilerizer.isCellOccupied(tx, ty))
				{
					double w = (1.0f / mTilerizer.getCellLimits(tx, ty).lowestZ()) + wFudge;
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
		
		Assert((pDstVerts - pVerts) == numTiles * VerticesPerTile);

		mDynamicVB.unlock();
	}

	void LightingEngine::drawOmniSpotLight(const LightParams& params)
	{
		Vec4 viewPos(params.mPos * gShaderManager.getMatrix(eWorldToView));
		Vec4 viewDir(Matrix44::transformNormal(params.mDir, gShaderManager.getMatrix(eWorldToView)));
		
		if (createPolyhedron(params, viewPos, viewDir))
			return;

		if (findTiles())
			return;

		gShaderManager.setRenderTarget(0, gShaderManager.getSurface(eTBuffer));
		gShaderManager.setRenderTarget(1, gShaderManager.getSurface(eNBuffer));
		gShaderManager.setRenderTarget(0, gShaderManager.getSurface(eDABuffer));
		gShaderManager.setRenderTarget(1, gShaderManager.getSurface(eSABuffer));
						    
		D3D::clearTextures();
								
		gShaderManager.beginEffectTechnique((params.mType == eSpot) ? "SpotLight" : "OmniLight");

		gShaderManager.setEffectTexture(eCBuffer);
		gShaderManager.setEffectTexture(eDBuffer);
		gShaderManager.setEffectTexture(eNBuffer);
		gShaderManager.setEffectTexture(eTBuffer);
		gShaderManager.setEffectTexture(eDABuffer);
		gShaderManager.setEffectTexture(eSABuffer);
				
		gShaderManager.setEffectVector("gOmniViewPos", viewPos);
		gShaderManager.setEffectVector("gOmniDiffuse", params.mColor0);
		gShaderManager.setEffectVector("gOmniSpecular", params.mColor1);

		float gOmniOOFalloffRange = 1.0f / (params.mFalloffOuter - params.mFalloffInner);
		gShaderManager.setEffectFloat("gOmniFalloffInner", params.mFalloffInner * gOmniOOFalloffRange);
		gShaderManager.setEffectFloat("gOmniOOFalloffRange", gOmniOOFalloffRange);
		gShaderManager.setEffectFloat("gOmniFalloffPower", params.mFalloffPower);
		gShaderManager.setEffectFloat("gOmniOORadius", (1.0f / params.mRadius) * gOmniOOFalloffRange);
		
		if (params.mType == eSpot)
		{
			gShaderManager.setEffectVector("gSpotViewDirection", viewDir); 

			float angleMul = 1.0f / ((cos(params.mSpotInner * .5f) - cos(params.mSpotOuter * .5f)));
			float angleAdd = 1 - cos(params.mSpotInner * .5f) * angleMul;
			float anglePow = params.mSpotPower;

			gShaderManager.setEffectVector("gSpotParams", Vec4(angleMul, angleAdd, anglePow, 0)); 
		}

		const Vec4 tileMul(gTileWidth, gTileHeight, 0, 0);
		const Vec4 tileAdd(0, 0, 0, 0);
		const Vec4 ooScreenDim(1.0f / gShaderManager.getBackbufferWidth(), 1.0f / gShaderManager.getBackbufferHeight(), 0, 0);
		
		gShaderManager.setEffectVector("gTileMul", tileMul);
		gShaderManager.setEffectVector("gTileAdd", tileAdd);
		gShaderManager.setEffectVector("gOOScreenDim", ooScreenDim);
		gShaderManager.setEffectMatrix(eScreenToView);
		gShaderManager.setEffectMatrix(eScreenToProj);
		
		//gShaderManager.setEffectFloat("gWFudge", wFudge);
		//gShaderManager.setEffectVector("gWToZ", wToZ);
		
#define BENCH 0

#if BENCH
		IDirect3DQuery9* pQuery = D3D::createQuery(D3DQUERYTYPE_OCCLUSION);
		
		pQuery->Issue(D3DISSUE_BEGIN);
		
		uint32 cpus = Time::ReadCycleCounter();
#endif    

		int vbOfs, numTris;
		fillTileVB(vbOfs, numTris);

		gShaderManager.setVertexDeclaration(eTileVertexDeclaration);
		
		D3D::setStreamSource(0, mDynamicVB, vbOfs, sizeof(TileVertex));

		D3D::drawPrimitive(D3DPT_TRIANGLELIST, 0, numTris);
				
#if BENCH
		uint32 cpue = Time::ReadCycleCounter();

		pQuery->Issue(D3DISSUE_END);
		
		DWORD qres;
		while (pQuery->GetData(&qres, sizeof(qres), D3DGETDATA_FLUSH) == S_FALSE) ;
		
		//double t = (double(e - s) / 1666666666.0) / float(N);
		double cput = double(cpue - cpus) / 1666666666.0;
		//double mps = (gShaderManager.getViewportWidth() * gShaderManager.getViewportHeight()) / t;

		TraceAlways("CPU %f, Pixels: %u\n", 
			cput,
			qres
		);

		pQuery->Release();
#endif
								
		gShaderManager.endEffectTechnique();
	}
  
  void LightingEngine::drawLight(const LightParams& params)
  {
    switch (params.mType)
		{
			case eDirectional:
				break;
			case eOmnidirectional:
			case eSpot:
				drawOmniSpotLight(params);
				break;
		}
	}
  
  void LightingEngine::endLight(void)
  {
		endSSQPasses();
  }

  void LightingEngine::finalAccumPass(const FinalPassParams& params)
  {
		const Vec4 viewHemiAxis(Matrix44::transformNormal(params.mHemiAxis, gShaderManager.getMatrix(eWorldToView)).normalize());
				
		gShaderManager.beginEffectTechnique("FinalPass");

		gShaderManager.setEffectTexture(eDABuffer);
		gShaderManager.setEffectTexture(eSABuffer);
		gShaderManager.setEffectTexture(eCBuffer);
		gShaderManager.setEffectTexture(eNBuffer);
		gShaderManager.setEffectTexture(eTBuffer);
        		
		gShaderManager.setEffectVector("gFinalPassHemiTop", params.mHemiTop);
		gShaderManager.setEffectVector("gFinalPassHemiBottom", params.mHemiBottom);
		gShaderManager.setEffectVector("gFinalPassHemiAxis", viewHemiAxis);
		gShaderManager.setEffectVector("gFinalPassSkinColor", params.mSkinColor);
		
		gShaderManager.setRenderTarget(0, gShaderManager.getSurface(eDABuffer));

		renderSSQ(true);
				
		gShaderManager.endEffectTechnique();
  }
	
  float LightingEngine::knee(float x, float f)
  {
    return log(x * f + 1) / f;
  }

	float LightingEngine::findF(float x, float y)
  {
    float fLow = 0.0f;
    float fHigh = 16.0f;
    
		for (int i = 0; i < 48; i++)
    {
      const float f2 = .5f * (fLow + fHigh);
      const float y2 = knee(x, f2);
      if (y2 < y)
        fHigh = f2;
      else
        fLow = f2;
    }

    return (fLow + fHigh) * .5f;
  }

	void LightingEngine::setToneMapParams(const FinalPassParams& params)
	{
		const float greyFStops = mPhotoToneMapParams.mGreyFStops;
		const float m = pow(2.0f, 2.47393f + mExposure);
		const float kl = pow(2.0f, mPhotoToneMapParams.mKneeLow);
		const float f = findF(pow(2.0f, mPhotoToneMapParams.mKneeHigh) - kl, pow(2.0f, greyFStops) - kl);
		const float g = 1.0f / mDisplayGamma;
		const float r = pow(pow(2.0f, -greyFStops), g);
		const float oof = 1.0f / f;
				
		gShaderManager.setEffectFloat("gToneMapM", m);
		gShaderManager.setEffectFloat("gToneMapKl", kl);
		gShaderManager.setEffectFloat("gToneMapF", f);
		gShaderManager.setEffectFloat("gToneMapG", g);
		gShaderManager.setEffectFloat("gToneMapR", r);
		gShaderManager.setEffectFloat("gToneMapOOF", oof);
	}

	void LightingEngine::photoToneMapExpose(const FinalPassParams& params)
	{
		gShaderManager.beginEffectTechnique("ToneMapPhoto");
		
		gShaderManager.setEffectTexture(eDABuffer);

		setToneMapParams(params);
		
		gShaderManager.setRenderTarget(0, gShaderManager.getSurface(eDABuffer));

		renderSSQ();
						
		gShaderManager.endEffectTechnique();

		D3D::clearTextures();
	}
	
	void LightingEngine::photoToneMapDownsample(const FinalPassParams& params)
	{
		static float O = .5f;

		gShaderManager.beginEffectTechnique("Resample2");
				
    gShaderManager.setEffectFloat("gResample2HalfPixelX", O / gShaderManager.getBackbufferWidth());
		gShaderManager.setEffectFloat("gResample2HalfPixelY", O / gShaderManager.getBackbufferHeight());
		gShaderManager.setEffectFloat("gResample2PixelScale", .25f * 1.0f / gGlareBufferScale);
		
		gShaderManager.setRenderTarget(0, gShaderManager.getSurface(eG2Buffer));

		D3D::setTextureClampPoint(0, gShaderManager.getTexture(eDABuffer));

		renderSSQ();

    gShaderManager.endEffectTechnique();

		D3D::clearTextures();

		// -----

		gShaderManager.beginEffectTechnique("Resample2");
				
    gShaderManager.setEffectFloat("gResample2HalfPixelX", O / gShaderManager.getBackbufferWidth());
		gShaderManager.setEffectFloat("gResample2HalfPixelY", O / gShaderManager.getBackbufferHeight());
		gShaderManager.setEffectFloat("gResample2PixelScale", .25f);
		
		gShaderManager.setRenderTarget(0, gShaderManager.getSurface(eG0Buffer));

		D3D::setTextureClampPoint(0, gShaderManager.getTexture(eG2Buffer));

		renderSSQ();

    gShaderManager.endEffectTechnique();

		D3D::clearTextures();
	}

	void LightingEngine::photoToneMapLowPass(const FinalPassParams& params)
	{
		static int nq = 1;
		static float ofs = .5f;
		static float BLOOM_SIGMA = 2.0f;
		static bool linear = true;

		for (int q = 0; q < nq; q++)
		{
			//float d = 2.0f * (q + 1);
			float d = pow(2.0f, q + 1);
			
			const int BLOOM_DIMENSION = 8;
			
			D3DXVECTOR2 offsets[BLOOM_DIMENSION];
			float coefficients[BLOOM_DIMENSION];

			gShaderManager.beginEffectTechnique("BloomFilter");

			gShaderManager.setRenderTarget(0, gShaderManager.getSurface(eG1Buffer));
			
			float t = 0;
			for(int i=0; i < BLOOM_DIMENSION; i++)
			{
				coefficients[i]  = (float) exp(-(i*i)/(2.0f*BLOOM_SIGMA*BLOOM_SIGMA));
				if (coefficients[i] < .000125f)
					coefficients[i] = 0.0f;

				t += coefficients[i];
								
				offsets[i].x = (ofs + (d * i)) / gShaderManager.getRenderTargetWidth();
				offsets[i].y = 0;
			}
			      			
			for(int i=0; i < BLOOM_DIMENSION; i++)
			{
				coefficients[i] /= t;
				coefficients[i] *= .5f;
				if (coefficients[i] < .000125f)
					coefficients[i] = 0.0f;
			}

			gShaderManager.effects()->SetValue("gBloomFilterTapScale", coefficients, sizeof(float)*BLOOM_DIMENSION);
			gShaderManager.effects()->SetValue("gBloomFilterTapOfs", offsets, sizeof(D3DXVECTOR2)*BLOOM_DIMENSION);

			if (linear)
				D3D::setTextureClampLinear(0, gShaderManager.getTexture(eG0Buffer));
			else
				D3D::setTextureClampPoint(0, gShaderManager.getTexture(eG0Buffer));
	    
			renderSSQ();

			gShaderManager.endEffectTechnique();

			//-----

			gShaderManager.beginEffectTechnique("BloomFilter");

			gShaderManager.setRenderTarget(0, gShaderManager.getSurface(eG0Buffer));
			
			for(int i=0; i < BLOOM_DIMENSION; i++)
			{
				offsets[i].y = (ofs + (d * i)) / gShaderManager.getRenderTargetHeight();
				offsets[i].x = 0;
			}

			gShaderManager.effects()->SetValue("gBloomFilterTapScale", coefficients, sizeof(float)*BLOOM_DIMENSION);
			gShaderManager.effects()->SetValue("gBloomFilterTapOfs", offsets, sizeof(D3DXVECTOR2)*BLOOM_DIMENSION);
			
			if (linear)
				D3D::setTextureClampLinear(0, gShaderManager.getTexture(eG1Buffer));
			else
				D3D::setTextureClampPoint(0, gShaderManager.getTexture(eG1Buffer));
	    
			renderSSQ();

			gShaderManager.endEffectTechnique();

			D3D::clearTextures();
		}
	}

	void LightingEngine::photoToneMapFinal(const FinalPassParams& params)
	{
		gShaderManager.beginEffectTechnique("ToneMapPhotoFinal");

		setToneMapParams(params);
		
		gShaderManager.setEffectFloat("gToneMapFinalGlareBufferScale", gGlareBufferScale);

		gShaderManager.setRenderTarget(0, gShaderManager.getSurface(eKBuffer));

		gShaderManager.setEffectTexture(eDABuffer);
		gShaderManager.setEffectTexture(eG0Buffer);

		renderSSQ();
						
		gShaderManager.endEffectTechnique();

		D3D::clearTextures();
	}

	void LightingEngine::photoToneMap(const FinalPassParams& params)
	{
		photoToneMapExpose(params);
		photoToneMapDownsample(params);
		photoToneMapLowPass(params);
		photoToneMapFinal(params);
	}

  void LightingEngine::toneMapPass(const FinalPassParams& params)
  {
		switch (mCurToneMapper)
		{
			case ePhotoToneMapper:
			{
				photoToneMap(params);
				return;
			}
			case eLinearToneMapper:
			{
				const float m = pow(2.0f, mExposure);
				const float g = 1.0f / mDisplayGamma;

				gShaderManager.beginEffectTechnique("ToneMapLinear");

				gShaderManager.setEffectTexture(eDABuffer);
								
				gShaderManager.setEffectFloat("gToneMapM", m);
				gShaderManager.setEffectFloat("gToneMapG", g);
				break;
			}
			case eRawToneMapper:
			{
				gShaderManager.beginEffectTechnique("ToneMapRaw");

				gShaderManager.setEffectTexture(eDABuffer);
												
				break;
			}
			case eViewNormals:
			{
				gShaderManager.beginEffectTechnique("ViewBufferRGB");
				D3D::setTextureClampPoint(0, gShaderManager.getTexture(eNBuffer));
				break;
			}
			case eViewAlbedo:
			{
				gShaderManager.beginEffectTechnique("ViewBufferRGB");
				D3D::setTextureClampPoint(0, gShaderManager.getTexture(eCBuffer));
				break;
			}
			case eViewFrontGloss:
			{
				gShaderManager.beginEffectTechnique("ViewBufferComponent");
				D3D::setTextureClampPoint(0, gShaderManager.getTexture(eCBuffer));
				gShaderManager.setEffectVector("gViewBufferComponentDot", Vec4(0,0,0,1));
				break;
			}
			case eViewSideGloss:
			{
				gShaderManager.beginEffectTechnique("ViewBufferComponent");
				D3D::setTextureClampPoint(0, gShaderManager.getTexture(eTBuffer));
				gShaderManager.setEffectVector("gViewBufferComponentDot", Vec4(0,0,0,1));
				break;
			}
			case eViewMetal:
			{
				gShaderManager.beginEffectTechnique("ViewBufferComponent");
				D3D::setTextureClampPoint(0, gShaderManager.getTexture(eTBuffer));
				gShaderManager.setEffectVector("gViewBufferComponentDot", Vec4(1,0,0,0));
				break;
			}
			case eViewSpecPower:
			{
				gShaderManager.beginEffectTechnique("ViewBufferComponent");
				D3D::setTextureClampPoint(0, gShaderManager.getTexture(eTBuffer));
				gShaderManager.setEffectVector("gViewBufferComponentDot", Vec4(0,1,0,0));
				break;
			}
			case eViewOcclusion:
			{
				gShaderManager.beginEffectTechnique("ViewBufferComponent");
				D3D::setTextureClampPoint(0, gShaderManager.getTexture(eTBuffer));
				gShaderManager.setEffectVector("gViewBufferComponentDot", Vec4(0,0,1,0));
				break;
			}
			case eViewSkin:
			{
				gShaderManager.beginEffectTechnique("ViewBufferComponent");
				D3D::setTextureClampPoint(0, gShaderManager.getTexture(eNBuffer));
				gShaderManager.setEffectVector("gViewBufferComponentDot", Vec4(0,0,0,1));
				break;
			}
		}

		gShaderManager.setRenderTarget(0, gShaderManager.getSurface(eKBuffer));
		
		renderSSQ();
				
		gShaderManager.endEffectTechnique();
  }
	
	void LightingEngine::beginFinal(const FinalPassParams& params)
  {
		beginSSQPasses();

    finalAccumPass(params);

    toneMapPass(params);
  }
  
  void LightingEngine::endFinal(void)
  {
		endSSQPasses();

#if 0
		{
			int vbOfs, numTris;
			fillTileVB(vbOfs, numTris);

			if (numTris)
			{

				gShaderManager.beginEffectTechnique("WireframeTiles");

				gShaderManager.setVertexDeclaration(eTileVertexDeclaration);
				
				D3D::setStreamSource(0, mDynamicVB, vbOfs, sizeof(TileVertex));

				D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);		

				D3D::disableCulling();
				D3D::setRenderState(D3DRS_ZWRITEENABLE, FALSE);
				D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);

				D3D::drawPrimitive(D3DPT_TRIANGLELIST, 0, numTris);

				D3D::setRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
				D3D::setRenderState(D3DRS_ZWRITEENABLE, TRUE);
				D3D::enableCulling();

				D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);		

				gShaderManager.endEffectTechnique();

				gShaderManager.setVertexShader(eInvalidVS);
				gShaderManager.setPixelShader(eInvalidPS);
				gShaderManager.setVertexDeclaration(eInvalidDeclaration);
			}
		}
#endif
  }

	PhotoToneMapParams& LightingEngine::getPhotoToneMapParams(void) 
	{ 
		return mPhotoToneMapParams; 
	}
  
} // namespace gr




