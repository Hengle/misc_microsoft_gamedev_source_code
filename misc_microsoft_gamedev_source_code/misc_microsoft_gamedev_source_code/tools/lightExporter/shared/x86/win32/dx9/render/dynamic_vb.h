//-----------------------------------------------------------------------------
// File: dynamic_vb.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef DYNAMIC_VB_H
#define DYNAMIC_VB_H

#include "device.h"

namespace gr
{
	class DynamicVB
	{
	public:
		DynamicVB() :
			mLen(0), 
			mFVF(0),
			mCurOfs(0), 
			mpVertexBuffer(NULL)
		{
		}

		DynamicVB(UINT len, DWORD FVF = 0) :
			mLen(len), 
			mFVF(FVF),
			mCurOfs(0), 
			mpVertexBuffer(NULL)
		{
			restore();
		}
		
		~DynamicVB()
		{
			release();
		}

		void resize(UINT len, DWORD FVF = 0)
		{
			mLen = len;
			mFVF = FVF;
			mCurOfs = 0; 
			mpVertexBuffer = NULL;

			restore();
		}

		IDirect3DVertexBuffer9* getVB(void) const { return mpVertexBuffer; }
		
		operator IDirect3DVertexBuffer9* () const { return getVB(); }

		template<class T>	T* lock(int numBytes, int& ofs)
		{
			Verify(numBytes <= mLen);

			int left = mLen - mCurOfs;
			
			DWORD lockFlags = D3DLOCK_NOOVERWRITE;
			if (numBytes > left)
			{
				lockFlags = D3DLOCK_DISCARD;
				mCurOfs = 0;
			}

			void* pData;
			D3D::errCheck(DebugNull(mpVertexBuffer)->Lock(mCurOfs, numBytes, &pData, lockFlags));

			ofs = mCurOfs;

      mCurOfs += numBytes;

			return reinterpret_cast<T*>(pData);
		}
		
		void unlock(void)
		{
			D3D::errCheck(DebugNull(mpVertexBuffer)->Unlock());
		}
		
		template<class T> T* beginLock(int minBytes, int& maxBytes, int& ofs)
		{
			Assert(minBytes >= 1);
			Verify(minBytes <= mLen);
			
			maxBytes = mLen - mCurOfs;
			
			DWORD lockFlags = D3DLOCK_NOOVERWRITE;
			if (maxBytes < minBytes)
			{
				lockFlags = D3DLOCK_DISCARD;
				mCurOfs = 0;
				maxBytes = mLen;
			}
			
			Assert(maxBytes >= minBytes);

			void* pData;
			D3D::errCheck(DebugNull(mpVertexBuffer)->Lock(mCurOfs, maxBytes, &pData, lockFlags));

			ofs = mCurOfs;
      
			return reinterpret_cast<T*>(pData);
		}
		
		void endLock(int bytesWritten)
		{
			D3D::errCheck(DebugNull(mpVertexBuffer)->Unlock());
			
			mCurOfs += bytesWritten;
			
			Assert(mCurOfs <= mLen);
		}
		
		void release(void)
		{
			D3D::safeRelease(mpVertexBuffer);
		}

		void restore(void)
		{
			release();

			if (mLen)
			{
				mpVertexBuffer = D3D::createVertexBuffer(
					mLen, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,	mFVF, D3DPOOL_DEFAULT);
			}
		}

		int mLen;
		int mCurOfs;
		int mFVF;
		IDirect3DVertexBuffer9* mpVertexBuffer;
	};
	
} // namespace gr

#endif // DYNAMIC_VB_H
