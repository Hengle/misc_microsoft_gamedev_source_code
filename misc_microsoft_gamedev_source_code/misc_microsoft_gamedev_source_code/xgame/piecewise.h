//==============================================================================
// piecewise.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

#include "gamefilemacros.h"

//==============================================================================
// class BPiecewiseDataPoint
//==============================================================================
class BPiecewiseDataPoint
{
public:
   
   float getInput() const { return (mInput); }
   float getOutput() const { return (mOutput); }
   void setInput(float input) { mInput = input; }
   void setOutput(float output) { mOutput = output; }
   void set(float input, float output) { mInput = input; mOutput = output; } 

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:
   float mInput;
   float mOutput;
};


//==============================================================================
// class BPiecewiseFunc
//==============================================================================
class BPiecewiseFunc
{
public:
   BPiecewiseFunc();
   void reset();
   bool loadFromXML(const BXMLNode &piecewiseFuncNode);
   float evaluate(float input) const;

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   bool getFlagClampInput() const { return (mFlagClampInput); }
   bool getFlagValid() const { return (mFlagValid); }
   void setFlagClampInput(bool v) { mFlagClampInput = v; }
   void setFlagValid(bool v) { mFlagValid = v; }
   void addDataPoint(const BPiecewiseDataPoint& dataPoint);
   bool determineValidity() const;

   BSmallDynamicSimArray<BPiecewiseDataPoint> mDataPoints;
   float mErrorOutput;
   bool mFlagClampInput : 1;
   bool mFlagValid : 1;
};