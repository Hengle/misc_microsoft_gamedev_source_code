//-----------------------------------------------------------------------------
// File: tweaker.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef TWEAKER_H
#define TWEAKER_H

#include "common/math/math.h"
#include "common/utils/utils.h"
#include "common/utils/string.h"
#include "common/render/console.h"

namespace gr
{
	class TweakerEntry
	{
	public:
		TweakerEntry()
		{
		}
		
		virtual ~TweakerEntry()
		{
		}

		virtual void tick(bool first, int dir) = 0;

		virtual void print(char* pBuf) = 0;
	};

	class IntegralTweakerEntry : public TweakerEntry
	{
		enum EType
		{
			eInvalid = 0,
			eInt,		
			eFloat,
			eList,
			eBool
		};

		SmallString mName;
		EType mType;
		void* mpData;
		const char** mppList;
		bool mOneShot;
		int mArraySize;
		const int* mpArrayElementIndex;
		double mLow, mHigh, mInc;
		bool mReadOnly;
		bool mWrap;

		void clear(void)
		{
			mName.clear();
			mType = eInvalid;
			mpData = 0;
			mppList = NULL;
			mOneShot = false;
			mArraySize = 0;
			mpArrayElementIndex = NULL;
			mLow = -Math::fNearlyInfinite;
			mHigh = Math::fNearlyInfinite;
			mInc = 1.0f;
			mReadOnly = false;
		}

		template<class T> T& getData(void) const
		{
			if (mArraySize)
				return (reinterpret_cast<T*>(mpData))[DebugRange(*mpArrayElementIndex, mArraySize)];
			else
				return *reinterpret_cast<T*>(mpData);
		}

	public:
		// boolean
		IntegralTweakerEntry(
			const char* pName, 
			bool* pData, 
			int arraySize = 0, 
			const int* pArrayElementIndex = NULL,
			bool readOnly = false)
		{	
			clear();
			mName = pName;
			mType = eBool;
			mpData = DebugNull(pData);
			mOneShot = true;
			mArraySize = arraySize;
			mpArrayElementIndex = pArrayElementIndex;
			mReadOnly = readOnly;
			mWrap = false;
		}

		// integer
		IntegralTweakerEntry(
			const char* pName, 
			int* pData, 
			int low = 0,
			int high = 1000,
			int inc = 1,
			bool oneShot = false, 
			int arraySize = 0, 
			const int* pArrayElementIndex = NULL,
			bool wrap = true,
			bool readOnly = false)
		{	
			clear();
			mName = pName;
			mType = eInt;
			mpData = DebugNull(pData);
			mOneShot = oneShot;
			mArraySize = arraySize;
			mpArrayElementIndex = pArrayElementIndex;
			mLow = low;
			mHigh = high;
			mInc = inc;
			mReadOnly = readOnly;
			mWrap = wrap;
		}

		// float
		IntegralTweakerEntry(
			const char* pName, 
			float* pData, 
			float low = -1000.0f,
			float high = 1000.0f,
			float inc = FLT_MAX,
			bool oneShot = false, 
			int arraySize = 0, 
			const int* pArrayElementIndex = NULL,
			bool wrap = true,
			bool readOnly = false)
		{	
			clear();
			mName = pName;
			mType = eFloat;
			mpData = DebugNull(pData);
			mOneShot = oneShot;
			mArraySize = arraySize;
			mpArrayElementIndex = pArrayElementIndex;
			mLow = low;
			mHigh = high;
			if (FLT_MAX == inc)
				mInc = (mHigh - mLow) / 32.0f;
			else
				mInc = inc;
			mWrap = wrap;
			mReadOnly = readOnly;
		}

		// list
		IntegralTweakerEntry(
			const char* pName, 
			int* pData, 
			const char** ppList,
			int listSize,
			int arraySize = 0, 
			const int* pArrayElementIndex = NULL,
			bool wrap = true,
			bool readOnly = false)
		{	
			clear();
			mName = pName;
			mType = eList;
			mpData = DebugNull(pData);
			mOneShot = true;
			mArraySize = arraySize;
			mpArrayElementIndex = pArrayElementIndex;
			mLow = 0;
			mHigh = listSize - 1;
			mInc = 1;
			mppList = DebugNull(ppList);
			mWrap = wrap;
			mReadOnly = readOnly;
		}

		void setWrap(bool wrap)
		{
			mWrap = wrap;
		}

		bool wrap(void) const
		{
			return mWrap;
		}

		void setReadOnly(bool readOnly)
		{
			mReadOnly = readOnly;
		}

		bool readOnly(void) const 
		{
			return mReadOnly;
		}

		EType getType(void) const
		{
			return mType;
		}

		bool getBool(void) const
		{
			Assert(mType == eBool);
			return getData<bool>();
		}

		int getInt(void) const
		{
			Assert(mType == eInt);
			return getData<int>();
		}

		float getFloat(void) const
		{
			Assert(mType == eFloat);
			return getData<float>();
		}

		int getListIndex(void) const
		{
			Assert(mType == eList);
			return getData<int>();
		}

		virtual void tick(bool first, int dir)
		{
			if ((!dir) || (mReadOnly))
				return;
			
			switch (mType)
			{
				case eInt:
					if ((!mOneShot) || (first))
					{
						getData<int>() += dir * mInc;
						
						if (getData<int>() < mLow)
							getData<int>() = mWrap ? mHigh : mLow;
						else if (getData<int>() > mHigh)
							getData<int>() = mWrap ? mLow : mHigh;
					}
					break;
				case eFloat:
					if ((!mOneShot) || (first))
					{
						getData<float>() += dir * mInc;
						if (getData<float>() < mLow)
							getData<float>() = mWrap ? mHigh : mLow;
						else if (getData<float>() > mHigh)
							getData<float>() = mWrap ? mLow : mHigh;
					}
					break;
				case eList:
					if ((!mOneShot) || (first))
					{
						getData<int>() += dir;
						if (getData<int>() < 0.0f)
							getData<int>() = mWrap ? mHigh : 0.0f;
						else if (getData<int>() > mHigh)
							getData<int>() = mWrap ? 0.0f : mHigh;
					}
					break;
				case eBool:
					if (first)
					{
						if (mWrap)
							getData<bool>() = !getData<bool>();
						else
							getData<bool>() = dir > 0;
					}
					break;
			}
		}

		virtual void print(char* pBuf)
		{
			switch (mType)
			{
				case eInt:
					sprintf(pBuf, "%s: %i", mName.c_str(), getData<int>());
					break;
				case eFloat:
					sprintf(pBuf, "%s: %f", mName.c_str(), getData<float>());
					break;
				case eList:
					sprintf(pBuf, "%s: %s", mName.c_str(), mppList[DebugRangeIncl<int>(getData<int>(), mHigh)]);
					break;
				case eBool:
					sprintf(pBuf, "%s: %s", mName.c_str(), getData<bool>() ? "true" : "false");
					break;
			}
		}
	};

	class TweakerPage
	{
		SmallString mName;
		std::vector<TweakerEntry*> mTweakers;
		int mCurTweaker;
	
	public:
		TweakerPage(const char* pName) :
			mName(pName),
			mCurTweaker(0)
		{
		}

		~TweakerPage()
		{
			clear();
		}

		int numTweakers(void)
		{
			return static_cast<int>(mTweakers.size() + 1);
		}

		int curTweaker(void) const
		{
			return mCurTweaker;
		}

		// Takes ownership!
		void add(TweakerEntry* pEntry)
		{
			mTweakers.push_back(pEntry);
		}

		void clear(void)
		{
			Utils::DeletePointerVector(mTweakers);
			mTweakers.clear();
			mCurTweaker = 0;
		}

		void display(void)
		{
			for (int i = 0; i < numTweakers(); i++)
			{
				BigString buf;
				
				const char* pColor = "#cF6DCDC";
				if (i == 0)
				{
					buf = mName;
					pColor = "#cFFFF20";
				}
				else
				{
					mTweakers[DebugRange<int>(i - 1, mTweakers.size())]->print(buf);
				}

				if (i == mCurTweaker)
					pColor = "#c0000FF";

				gConsole.printf("%s%s", pColor, buf.c_str());
			}
		}

		void tick(bool first, int xDir, int yDir)
		{
			if (xDir)
			{
				const int index = mCurTweaker - 1;
				if (index >= 0)
				{
					Assert(index < mTweakers.size());
          DebugNull(mTweakers[index])->tick(first, xDir);
				}
			}
			else if ((yDir) && (first))
			{
				mCurTweaker += yDir;
				
				if (mCurTweaker >= numTweakers())
					mCurTweaker = 0;
				else if (mCurTweaker < 0)
					mCurTweaker = numTweakers() - 1;
			}
		}
	};

	class Tweaker
	{
		std::vector<TweakerPage*> mPages;
    int mCurPage;

	public:
		Tweaker()
		{
		}

		~Tweaker()
		{
			clear();
		}

		TweakerPage& page(int pageIndex) const
		{
			return *DebugNull(mPages[DebugRange<int>(pageIndex, mPages.size())]);
		}

		void clear(void)
		{
			Utils::DeletePointerVector(mPages);
			mPages.clear();
			mCurPage = 0;
		}

    // Takes ownership!
		// Returns page number.
		int add(TweakerPage* pPage)
		{
			mPages.push_back(pPage);
			return static_cast<int>(mPages.size()) - 1;
		}

		void del(TweakerPage* pPage)
		{
			if (mPages.empty())
				return;

			for (uint i = 0; i < mPages.size(); i++)
				if (pPage == mPages[i])
					break;

			if (i == mPages.size())
				return;

			delete pPage;
			mPages.erase(mPages.begin() + i);

			mCurPage = 0;
		}

		int numPages(void) const
		{
			return static_cast<int>(mPages.size());
		}

		int curPage(void) const
		{
			return mCurPage;
		}

		void display(void)
		{
			if (!numPages())
				return;

			mPages[DebugRange(mCurPage, numPages())]->display();
		}

		void tick(bool first, int xDir, int yDir)
		{
			if (!numPages())
				return;

			if ((first) && 
					(xDir != 0) && 
					(mPages[DebugRange(mCurPage, numPages())]->curTweaker() == 0))
			{
				mCurPage += xDir;
				if (mCurPage < 0)
					mCurPage = numPages() - 1;
				else if (mCurPage >= numPages())
					mCurPage = 0;
			}
			else
				mPages[DebugRange(mCurPage, numPages())]->tick(first, xDir, yDir);
		}
	};

	enum ESharedTweakers
	{
		eMisc,
		
		eNumCommonTweakers
	};

	class SharedTweaker
	{
	protected:
		Tweaker mTweaker;

	public:
		SharedTweaker()
		{
		}

		virtual ~SharedTweaker()
		{
		}

		virtual Tweaker& get(void) 
		{ 
			return mTweaker; 
		}

		virtual void init(void)
		{
			createPage("Misc", eMisc);
		}

		virtual void clear(void)
		{
			mTweaker.clear();
		}
    		
		virtual void createPage(const char* pName, int desiredPageIndex)
		{
			const int pageIndex = mTweaker.add(new TweakerPage(pName));
			Assert(pageIndex == desiredPageIndex);
		}

		virtual TweakerEntry* createEntry(int pageIndex, TweakerEntry* pTweaker)
		{
			mTweaker.page(pageIndex).add(pTweaker);
			return pTweaker;
		}

		virtual void clearPage(int pageIndex)
		{
			if (pageIndex >= mTweaker.numPages())
				return;
			mTweaker.page(pageIndex).clear();
		}
	};

	extern SharedTweaker& gSharedTweaker;

} // namespace gr

#endif // TWEAKER_H

