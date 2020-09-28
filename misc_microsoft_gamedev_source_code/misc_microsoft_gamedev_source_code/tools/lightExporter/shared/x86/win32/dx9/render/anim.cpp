// File: anim.cpp
#include "common/math/quat.h"
#include "anim.h"

namespace gr
{
	Anim::Anim() : 
		mLoadedAnim(false), 
		mUseQuats(true),
		mLoop(false)
	{
	}

	Anim::~Anim()
	{
	}

	void Anim::clear(void)
	{
		mAnim.clear();
		mLoadedAnim = false;
	}
	
	bool Anim::load(Stream& stream)
	{
		clear();
				
		if (mAnim.read(stream))
		{
			clear();
			return true;
		}

		if (!mAnim.numKeyFrames())
			clear();
		else
			mLoadedAnim = true;
							
		return false;
	}

	bool Anim::valid(void) const
	{
		return mLoadedAnim;
	}

	int Anim::numKeyFrames(void) const
	{
		return mAnim.numKeyFrames();
	}

	int Anim::numBones(void) const
	{
		if (!mLoadedAnim)
			return 0;
		return mAnim[0].size();
	}

	float Anim::duration(void) const
	{
		if (!mLoadedAnim)
			return 0.0f;
		return mAnim[numKeyFrames() - 1].time();
	}

	float Anim::keyFrameTime(int i) const
	{
		if (!mLoadedAnim)
			return 0.0f;
		return mAnim[i].time();
	}

	// interpolates local_bone->root matrices
	void Anim::interpolateKeyFrames(Matrix44* pDst, float time) const
	{
		int frameA = 0, frameB = 0;

		if ((duration() > 0.0f) && (time > 0.0f))
		{
			if (mLoop)
				time = Math::fPosMod(time, duration());
								
			if (time >= duration())
				frameA = frameB = mAnim.numKeyFrames() - 1;
			else
			{
				int low = 1;
				int high = mAnim.numKeyFrames() - 1;
				int mid = 1;
								
				while (low <= high)
				{
					mid = (low + high) >> 1;
					DebugRange(mid, 1, mAnim.numKeyFrames());
					
					if (time < mAnim[mid].time())
					{
						if (time >= mAnim[mid - 1].time())
							break;
						
						high = mid - 1;
					}
					else
					{
						low = mid + 1;
					}
				}

#if DEBUG
				for (int i = 0; i < mAnim.numKeyFrames(); i++)
					if (time < mAnim[i].time())
						break;				
				Assert(mid == i);
#endif				

				frameA = Math::Max(0, mid - 1);
				frameB = mid;
			}
		}

		DebugRange(frameA, mAnim.numKeyFrames());
		DebugRange(frameB, mAnim.numKeyFrames());
		
		const float frameATime = mAnim[frameA].time();
		const float frameBTime = mAnim[frameB].time();
		Assert(frameBTime >= frameATime);
		Assert(time >= frameATime);

		const float duration = frameBTime - frameATime;
				
		const float interpFactor = duration ? ((time - frameATime) / duration) : 0.0f;
				
		DebugRangeIncl(interpFactor, 0.0f, 1.0f);

		const Unigeom::KeyFrame& ka = mAnim[frameA];
		const Unigeom::KeyFrame& kb = mAnim[frameB];
		if (mUseQuats)
		{
			for (int i = 0; i < numBones(); i++)
			{
				const Unigeom::QForm& qa = ka[i];
				const Unigeom::QForm& qb = kb[i];

				const Quat q(Quat::slerp(qa.getQ(), qb.getQ(), interpFactor));
				const Vec3 t(Vec3::lerp(qa.getT(), qb.getT(), interpFactor));

				q.toMatrix(pDst[i + 1]).setTranslate(t, 1.0f);
			}
		}
		else
		{
#if QFORM_STORE_MATRIX
			for (int i = 0; i < numBones(); i++)
			{
				const Unigeom::QForm& qa = ka[i];
				const Unigeom::QForm& qb = kb[i];

				pDst[i + 1].setRow(0, qa.getX(0));
				pDst[i + 1].setRow(1, qa.getX(1));
				pDst[i + 1].setRow(2, qa.getX(2));
				pDst[i + 1].setRow(3, Vec4(qa.getX(3), 1.0f));
			}
#else
			Verify(false);
#endif
		}
	}
} // namespace gr

