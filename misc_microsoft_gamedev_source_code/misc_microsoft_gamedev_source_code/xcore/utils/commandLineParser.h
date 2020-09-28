//============================================================================
//
// File: commandLineParser.h
// Copyright (c) 2005-2007, Ensemble Studios
//
//============================================================================
#pragma once

enum eCLParamType
{
   cCLParamTypeIgnore,
   
   // By default, the bool pointed to by mpData will be set to true, unless setFlagParamsInvertByDefault(true) is called, then the current value is inverted.
   // If the param ends in "-", the flag is set to false, and if it ends in "+" it's set to true.
   cCLParamTypeFlag,

   // Copies bool/int/float value from BCLParam struct and writes value to mpData when param is specified. 
   // IMPORTANT: The actual value written does not come from the command line!
   cCLParamTypeBool, 
   cCLParamTypeInt,
   cCLParamTypeFloat,

   // Parses bool/int/float from the command line, writes value to mpData.
   cCLParamTypeIntPtr,
   cCLParamTypeFloatPtr,
   cCLParamTypeBStringPtr,
   
   // mpData must be a pointer to a BCommandLineParser::BStringArray.
   cCLParamTypeBStringArrayPtr,

   cCLParamTypeMax
};

struct BCLParam
{
   const char* mpParamStr;

   eCLParamType mType;

   void* mpData;
   uint mNumDataValues;
         
   bool* mpSpecifiedFlag;
   
   // purposely not a union 
   uint mIntValue;
   bool mBoolValue;
   float mFloatValue;
};

class BCommandLineParser
{
public:
   typedef BDynamicArray<BString> BStringArray;
   
   BCommandLineParser(const BCLParam* pParams, uint numParams = 0);
   
   bool parse(int argc, const char * const argv[], bool ignoreUnrecognizedParams = false);
   bool parse(BStringArray& args, bool removeRecognizedParams, bool ignoreUnrecognizedParams);
         
   const char* getErrorString(void) { return mErrorString.getPtr(); }

   const BDynamicArray<uint>& getUnparsedParams(void) const { return mUnparsedParamIndices; }
   
   void setFlagParamsInvertByDefault(bool value) { mFlagParamsInvertByDefault = value; }
   
   static void createBStringArray(BStringArray& strArray, int argc, const char * const argv[]);
   
private:
   const BCLParam* mpParams;
   uint mNumParams;

   BDynamicArray<uint> mUnparsedParamIndices;
   BString mErrorString;

   bool mFlagParamsInvertByDefault;
   
   BCommandLineParser(const BCommandLineParser&);
   BCommandLineParser& operator= (const BCommandLineParser&);
   void formatError(const char* pMsg, ...);
   bool readStringFile(const char* pFilename, BCommandLineParser::BStringArray& stringArray);
}; 

