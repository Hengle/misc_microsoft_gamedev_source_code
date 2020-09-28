//==============================================================================
// piecewise.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "aidebug.h"
#include "piecewise.h"

GFIMPLEMENTVERSION(BPiecewiseFunc, 1);
GFIMPLEMENTVERSION(BPiecewiseDataPoint, 1);

//==============================================================================
//==============================================================================
BPiecewiseFunc::BPiecewiseFunc()
{
   reset();
}


//==============================================================================
//==============================================================================
void BPiecewiseFunc::reset()
{
   mDataPoints.clear();
   mErrorOutput = 0.0f;
   mFlagClampInput = true;
   mFlagValid = false;
}


//==============================================================================
//==============================================================================
bool BPiecewiseFunc::loadFromXML(const BXMLNode &piecewiseFuncNode)
{
   // Validate the root node.
   reset();
   if (piecewiseFuncNode.getName() != "PiecewiseFunc")
      return (false);

   uint numDataPointNodes = static_cast<uint>(piecewiseFuncNode.getNumberChildren());
   mDataPoints.reserve(numDataPointNodes);
   for (uint i=0; i<numDataPointNodes; i++)
   {
      BXMLNode dataPointNode(piecewiseFuncNode.getChild(i));
      if (dataPointNode.getName() != "DataPoint")
         return (false);

      float input = 0.0f;
      if (!dataPointNode.getAttribValueAsFloat("Input", input))
         return (false);

      float output = 0.0f;
      if (!dataPointNode.getAttribValueAsFloat("Output", output))
         return (false);

      BPiecewiseDataPoint dataPoint;
      dataPoint.setInput(input);
      dataPoint.setOutput(output);
      addDataPoint(dataPoint);
   }

   // Set our validity flag.
   mFlagValid = determineValidity();
   BASSERTM(mFlagValid, "BPiecewiseFunc::loadFromXML() - mFlagValid is False.");

   // Success!
   return (true);
}


//==============================================================================
// Make sure you set mFlagValid to the result of determineValidity after you
// add all your data points to the piecewiseFunc.
//==============================================================================
void BPiecewiseFunc::addDataPoint(const BPiecewiseDataPoint& dataPoint)
{
   mDataPoints.add(dataPoint);
}


//==============================================================================
// To be considered valid we must:
// 1.)  Have at least MinimumDataPoints data points (which must also be >= 2).
// 2.)  Adjacent data point (iPrev, iCurrent) input values must be in ascending order except in the case
//      where the iPrev data point is inclusive and the iCurrent data point is exclusive.
//==============================================================================
bool BPiecewiseFunc::determineValidity() const
{
   // 1.)  Have at least MinimumDataPoints data points (which must also be >= 2).
   uint numDataPoints = mDataPoints.getSize();
   if (numDataPoints < 2)
      return (false);

   // 2.)  Adjacent data point (iPrev, iCurrent) input values must be in ascending order except in the case
   //      where the iPrev data point is inclusive and the iCurrent data point is exclusive.
   uint iFirst = 0;
   uint iLast = numDataPoints - 1;
   uint iPrev = iFirst;
   uint iCurrent = iFirst + 1;
   uint identicalInputs = 1;
   while (iCurrent <= iLast)
   {
      float prevInput = mDataPoints[iPrev].getInput();
      float currentInput = mDataPoints[iCurrent].getInput();
      if (currentInput < prevInput)
      {
         // Fail.
         return (false);
      }
      else if (currentInput == prevInput)
      {
         identicalInputs++;
         if (identicalInputs > 2)
            return (false);
      }
      else
      {
         identicalInputs = 1;
      }
      iPrev += 1;
      iCurrent += 1;
   }

   // We meet all the criteria to be valid.
   return (true);
}


//==============================================================================
//==============================================================================
float BPiecewiseFunc::evaluate(float input) const
{
   BASSERTM(mFlagValid, "Using a piecewise func that is not valid.  Returning mErrorOutput.");
   if (!mFlagValid)
      return (mErrorOutput);

   uint iFirst = 0;
   uint iLast = mDataPoints.getSize() - 1;

   // Handle out of range low.
   float firstInput = mDataPoints[iFirst].getInput();
   if (input < firstInput)
   {
      if (mFlagClampInput)
      {
         float firstOutput = mDataPoints[iFirst].getOutput();
         return (firstOutput);
      }
      else
         return (mErrorOutput);
   }

   // Handle out of range high.
   float lastInput = mDataPoints[iLast].getInput();
   if (input > lastInput)
   {
      if (mFlagClampInput)
      {
         float lastOutput = mDataPoints[iLast].getOutput();
         return (lastOutput);
      }
      else
         return (mErrorOutput);
   }

   // Find the two adjacent data points to which our input maps.
   uint iPrev = iFirst;
   uint iCurrent = iFirst + 1;
   while (iCurrent <= iLast)
   {
      float prevInput = mDataPoints[iPrev].getInput();
      float currentInput = mDataPoints[iCurrent].getInput();

      if (input <= currentInput)
      {
         float inputRange = currentInput - prevInput;
         float lerp = (inputRange > 0.0f) ? ((input - prevInput) / inputRange) : 1.0f;
         float prevOutput = mDataPoints[iPrev].getOutput();
         float currentOutput = mDataPoints[iCurrent].getOutput();
         float output = Math::Lerp(prevOutput, currentOutput, lerp);
         return (output);
      }

      iPrev += 1;
      iCurrent += 1;
   }

   // We should never, ever hit this.  But handle it anyway.
   BASSERTM(false, "Error in BPiecewiseFunc::evaluate.  The mFlagValid flag is true, but the func was unable to evaluate.  Returning mErrorOutput.");
   return (mErrorOutput);
}

//==============================================================================
// BPiecewiseFunc::save
//==============================================================================
bool BPiecewiseFunc::save(BStream* pStream, int saveType) const
{
   GFWRITECLASSARRAY(pStream, saveType, mDataPoints, uint16, 1000);
   GFWRITEVAR(pStream, float, mErrorOutput);
   GFWRITEBITBOOL(pStream, mFlagClampInput);
   GFWRITEBITBOOL(pStream, mFlagValid);
   return true;
}

//==============================================================================
// BPiecewiseFunc::load
//==============================================================================
bool BPiecewiseFunc::load(BStream* pStream, int saveType)
{  
   GFREADCLASSARRAY(pStream, saveType, mDataPoints, uint16, 1000);
   GFREADVAR(pStream, float, mErrorOutput);
   GFREADBITBOOL(pStream, mFlagClampInput);
   GFREADBITBOOL(pStream, mFlagValid);
   return true;
}

//==============================================================================
// BPiecewiseDataPoint::save
//==============================================================================
bool BPiecewiseDataPoint::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, float, mInput);
   GFWRITEVAR(pStream, float, mOutput);
   return true;
}

//==============================================================================
// BPiecewiseDataPoint::load
//==============================================================================
bool BPiecewiseDataPoint::load(BStream* pStream, int saveType)
{  
   GFREADVAR(pStream, float, mInput);
   GFREADVAR(pStream, float, mOutput);
   return true;
}

