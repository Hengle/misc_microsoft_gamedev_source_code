//============================================================================
//
//  StaticBitStateArray.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

class BStaticBitStateArray
{
public:
   BStaticBitStateArray();
   ~BStaticBitStateArray();

   void	init(int numElements,bool defaultState=true);
   void  destroy(void);
   void	setAll(bool onOff);
   void	setState(int index, bool state);
   void	setSpanState(int index, int stride, bool state);
   bool	isSet(int index);

   int		giveNextTRUE(int startIndex);
   int		giveNextFALSE(int startIndex);

   //we leave it up to the calling functions to decide when to do this
   //extreamly helpful when you know that you're going to walk a great deal of 
   //the array
   void     doPrecache();

private:
   inline  void	   setContainerState(__int64 &value, bool state, __int64 mask);
   __int64           makeSpanBitMask(byte start, byte end);


   int		mNumElements;
   int		mNumPrimary;
   __int64	*mpArray;
};