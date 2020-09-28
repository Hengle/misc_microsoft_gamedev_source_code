// tone_mapper.h
#ifndef TONE_MAPPER_H
#define TONE_MAPPER_H

#include "effects.h"

namespace gr
{
	enum EToneMapper
	{
		ePhotoToneMapper,
		eLinearToneMapper,
		eRawToneMapper,
		eDiffuseAccum,
		eSpecAccum,

		eViewNormals,
		eViewReflection,
		eViewAlbedo,
		eViewFrontSpec,
		eViewSideSpec,
		eViewSpecPower,
		eViewEmissive,
		eViewEmissiveAlpha,
		eViewEnv,
		eViewEnvAlpha,
				    
		eNumToneMappers
	};

	class ToneMapper
	{

	public:
		ToneMapper();
		~ToneMapper();
		
		void initDeviceObjects(void);
		void deleteDeviceObjects(void);
		void invalidateDeviceObjects(void);
		void restoreDeviceObjects(void);
		void oneTimeSceneInit(void);

		void initTweakers(void);
	
		void toneMap(float exposure);
		
	private:
		ToneMapper(const ToneMapper& b);
		ToneMapper& operator= (const ToneMapper& b);

		Effects& mEffects;

		int mType;

		float mDisplayGamma;
		float mExposure;
		float mRelExposure;
		
		float mGreyFStops;
		float mKneeLow;
		float mKneeHigh;
		float mGlareInten;

		// FIXME: Should be app controllable!
		enum { MaxStreakElements = 8 };
		float mStreakOfs;
		float mStreakExposure;
		int mStreakElements;
		int mNumStreakPasses[MaxStreakElements];
		float mStreakFalloff[MaxStreakElements];
		float mStreakInten[MaxStreakElements];
		float mStreakR[MaxStreakElements];
		float mStreakG[MaxStreakElements];
		float mStreakB[MaxStreakElements];
		float mStreakElementOfs[MaxStreakElements];
		int mTweakerStreakIndex;
		float mStreakOverallInten;

		// FIXME: Should be app controllable!
		float mBloomExposure;
		int mNumBloomPasses;
		float mBloomSigma;
		float mBloomR;
		float mBloomG;
		float mBloomB;
		
		float mBrightMaskOfs;
		
		IDirect3DTexture9* mpGlareMaskTexture;

		void photoToneMapDownsample(ETexture srcBuffer, ETexture dstBuffer);
		void photoToneMapLowPass(float relStops, ETexture dstBuffer);
		ETexture photoToneMapStreak(
			int numPasses,
			float dir,
			float falloff,
			float inten, float exposure,
			ETexture firstBuffer,
			ETexture buffer0,
			ETexture buffer1);
		void photoToneMapAccum(
			ETexture srcBuffer,
			ETexture dstBuffer,
			bool clearDst = false,
			float r = 1.0f, float g = 1.0f, float b = 1.0f);
		void photoToneMapFinal(ETexture glareBuffer);
		void photoToneMap(void);
		
		float knee(float x, float f);
		float findF(float x, float y);
		void setToneMapParams(void);

		void createGlareMaskTexture(void);
		
		void test(void);
	};

} // namespace gr

#endif // TONE_MAPPER_H














