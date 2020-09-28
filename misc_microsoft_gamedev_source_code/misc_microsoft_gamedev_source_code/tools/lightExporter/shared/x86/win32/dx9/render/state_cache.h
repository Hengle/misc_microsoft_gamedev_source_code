//-----------------------------------------------------------------------------
// File: state_cache.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef STATE_CACHE_H
#define STATE_CACHE_H

#include <d3d9.h>
#include <algorithm>

#include "common/core/core.h"
#include "x86/win32/core/win_hdr.h"
#include "common/utils/utils.h"

namespace gr
{
	template<int Size>
	struct StateTracker
	{
		enum { NumStates = Size };
				
		typedef DWORD StateIndex;
		typedef DWORD StateValue;
		
		StateTracker()
		{
			Utils::ClearObj(*this);

			invalidate();
		}

		void invalidate(void) 
		{
			mNumChangedStates = 0;
			
			Utils::ClearObj(mDirtyFlags);
			
			for (int i = 0; i < NumStates; i++)
				mStates[i].mCurrent = mStates[i].mDesired = InvalidStateValue;
		}

		void set(StateIndex state, StateValue value)
		{
			DebugRange(state, NumStates);

			const int dirtyFlag = (mDirtyFlags[state >> 3] >> (state & 7)) & 1;
			mDirtyFlags[state >> 3] |= (1 << (state & 7));

			mChangedStates[DebugRange(mNumChangedStates, NumStates + 1)] = state;
			mNumChangedStates += (1 ^ dirtyFlag);

			mStates[state].mDesired = value;
		}

		// Can't be dirty!
		StateValue get(StateIndex state) const
		{
			Assert(!dirty());
			return mStates[DebugRange(state, NumStates)].mCurrent;
		}

		bool dirty(void) const
		{
			return 0 != mNumChangedStates;
		}
		
		template<class CheckFunc>
		void check(CheckFunc checkFunc)
		{
			Assert(!dirty());
			for (int i = 0; i < NumStates; i++)
				if (getCurStateValue(i) != InvalidStateValue)
					checkFunc(i, getCurStateValue(i));
		}

		template<class SetFunc>
		void flush(SetFunc setFunc)
		{
			for (int i = 0; i < numDirtyStates(); i++)
			{
				const StateIndex state = getDirtyState(i);
				if (isStateDirty(state))
					setFunc(state, getDesiredStateValue(state));
				syncState(state);
			}
			clearChangedStates();
		}

	private:
		enum { InvalidStateValue = 0xBEAD9934 };

		struct 
		{
			StateValue mCurrent;
			StateValue mDesired;
		} mStates[NumStates];
				
		int mNumChangedStates;
		StateIndex mChangedStates[NumStates + 1];

		uint8 mDirtyFlags[(NumStates + 7) >> 3];
    
		int numDirtyStates(void) const
		{
			return mNumChangedStates;
		}

		StateIndex getDirtyState(int i) const
		{
			return mChangedStates[DebugRange(i, mNumChangedStates)];
		}

		StateValue getCurStateValue(StateIndex i) const
		{
			return mStates[DebugRange(i, NumStates)].mCurrent;
		}

		StateValue getDesiredStateValue(StateIndex i) const
		{
			return mStates[DebugRange(i, NumStates)].mDesired;
		}

		bool isStateDirty(StateIndex state) const
		{
			DebugRange(state, NumStates);
			return ((mStates[state].mDesired != mStates[state].mCurrent) || (mStates[state].mCurrent == InvalidStateValue));
		}

		void syncState(StateIndex state)
		{
			DebugRange(state, NumStates);
			mStates[state].mCurrent = mStates[state].mDesired;
			mDirtyFlags[state >> 3] = 0;
		}

		void clearChangedStates(void)
		{
			mNumChangedStates = 0;
		}
	};

	class StateCache
	{
	public:
		// # of render states
		enum { NumRenderStates				= 256 };
		enum { NumRenderStatesLog2		= 8 };

		// # of texture stage states
		enum { NumTextureStates				= 32 };
		enum { NumTextureStagesLog2   = 5 };

		// # of sampler state states
		enum { NumSamplerStates				= 16 };
		enum { NumSamplerStatesLog2		= 4 };
		
		// # of texture stages
		//enum { NumTextures						= 8 };
		//enum { NumTexturesLog2				= 3 };
		
		enum { NumTextures						= 16 };
		enum { NumTexturesLog2				= 4 };
		
		// # of samplers
		enum { NumSamplers						= 16 };
		enum { NumSamplersLog2				= 4  };
						
		StateCache();
		
		void flush(void);
		void invalidate(void);
		void check(void);
				
		void setRenderState(D3DRENDERSTATETYPE State, DWORD Value)
		{
			mRenderStateTracker.set(State, Value);
		}

		void setTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
		{
			DebugRange<int, int>(Stage, NumTextures);
			DebugRange<int, int>(Type, NumTextureStates);
			mTextureStateTracker.set(Type | (Stage << NumTexturesLog2), Value);
		}

		void setSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
		{
			DebugRange<int, int>(Sampler, NumSamplers);
			DebugRange<int, int>(Type, NumSamplerStates);
			mSamplerStateTracker.set(Type | (Sampler << NumSamplersLog2), Value);
		}

		void setTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture)
		{
			if (pTexture != mpCurTexture[DebugRange(Stage, NumTextures)])
				mTexturesChanged = true;
			
			mpDesiredTexture[DebugRange(Stage, NumTextures)] = pTexture;
		}
					
	private:
		enum { InvalidTexturePointerVal = 0xFFFFFFFF };
		
		static void renderStateSetFunc(DWORD state, DWORD value);
		static void textureStateSetFunc(DWORD state, DWORD value);
		static void samplerStateSetFunc(DWORD state, DWORD value);
		static void renderStateCheckFunc(DWORD state, DWORD value);
		static void textureStateCheckFunc(DWORD state, DWORD value);
		static void samplerStateCheckFunc(DWORD state, DWORD value);
		
		void flushTextures(void);
		void invalidateTextures(void);
		void checkTextures(void);
		
		StateTracker<NumRenderStates>									mRenderStateTracker;
		StateTracker<NumTextureStates * NumTextures>	mTextureStateTracker;
		StateTracker<NumSamplerStates * NumSamplers>	mSamplerStateTracker;
						
		IDirect3DBaseTexture9* mpCurTexture[NumTextures];
		IDirect3DBaseTexture9* mpDesiredTexture[NumTextures];
		bool mTexturesChanged;
	};
} // namespace gr

#endif // STATE_CACHE_H
