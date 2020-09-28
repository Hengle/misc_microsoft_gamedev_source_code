// File: anim.h
#pragma once
#ifndef ANIM_H
#define ANIM_H

#include "common/geom/unigeom.h"

namespace gr
{

	class Anim
	{
	public:
		Anim();
		~Anim();

		void clear(void);
		
		bool load(Stream& stream);
		bool valid(void) const;

		int numBones(void) const;
		int numKeyFrames(void) const;
		float duration(void) const;
		bool loop(void) const { return mLoop; }
		void setLoop(bool loop) { mLoop = loop; }
		float keyFrameTime(int i) const;
				
		void interpolateKeyFrames(Matrix44* pDst, float time) const;
						
	private:
		Unigeom::Anim mAnim;
		bool mLoadedAnim;
		bool mUseQuats;
		bool mLoop;
	};
	
} // namespace gr
	
#endif // ANIM_H