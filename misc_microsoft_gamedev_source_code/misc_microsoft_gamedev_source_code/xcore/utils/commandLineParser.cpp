//============================================================================
//
// File: commandLineParser.cpp
// Copyright (c) 2005-2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "string\strHelper.h"
#include "commandLineParser.h"

BCommandLineParser::BCommandLineParser(const BCLParam* pParams, uint numParams) :
   mpParams(pParams),
   mNumParams(numParams),
   mFlagParamsInvertByDefault(false)
{
   if (!mNumParams)
   {
      while (mpParams[mNumParams].mpParamStr)
      {
         mNumParams++;
      }
   }
   
   BDEBUG_ASSERT(mNumParams);
}

bool BCommandLineParser::parse(int argc, const char * const argv[], bool ignoreUnrecognizedParams)
{
   BStringArray strArray;
   createBStringArray(strArray, argc, argv);
   return parse(strArray, false, ignoreUnrecognizedParams);
}

bool BCommandLineParser::readStringFile(const char* pFilename, BCommandLineParser::BStringArray& stringArray)
{
   FILE* pFile = NULL;
   if (fopen_s(&pFile, pFilename, "r"))
      return false;
   
   while (!feof(pFile))
   {
      char buf[1024];
      if (!fgets(buf, sizeof(buf), pFile))
         break;

      BString line(buf);

      line.trimLeft(" \t\n\r");
      line.trimRight(" \t\n\r");

      if (line.isEmpty())
         continue;
      
      stringArray.pushBack(line);
   }

   fclose(pFile);

   return true;
}

bool BCommandLineParser::parse(BStringArray& args, bool removeRecognizedParams, bool ignoreUnrecognizedParams)
{
   mErrorString.empty();
   mUnparsedParamIndices.clear();
   
   bool status = true;
   
   int argc = args.getSize();
   
   int argIndex = 1;
   while (argIndex < argc)
   {
      const uint curArgIndex = argIndex;         
      argIndex++;            
      
      if (args[curArgIndex].isEmpty())
         continue;
      
      const char* pArg = args[curArgIndex].getPtr();
      
      const uint numArgsLeft = argc - argIndex;
      
      if ((pArg[0] != '-') && (pArg[0] != '/'))
      {
         mUnparsedParamIndices.pushBack(curArgIndex);
         continue;
      }
                        
      uint paramIndex;
      for (paramIndex = 0; paramIndex < mNumParams; paramIndex++)
      {
         const BCLParam& param = mpParams[paramIndex];
         
         if (_stricmp(pArg + 1, param.mpParamStr) == 0)
            break;
      }
      
      if (paramIndex == mNumParams)
      {
         if (!ignoreUnrecognizedParams)
         {
            formatError(B("Unrecognized parameter: %s"), BStrConv::toB(pArg));
            status = false;
         }
         continue;
      }
      
      if (removeRecognizedParams)
         args[curArgIndex].empty();
      
      const BCLParam& param = mpParams[paramIndex];
      
      BASSERT(param.mpData);
      
      if (!param.mpData)
         continue;
         
      if (param.mpSpecifiedFlag)
         *param.mpSpecifiedFlag = true;
         
      switch (param.mType)
      {
         case cCLParamTypeIgnore: 
         {
            break;
         }
         case cCLParamTypeFlag:
         {
            bool enabledFlag = true;
            
            if (mFlagParamsInvertByDefault)
               enabledFlag = !*reinterpret_cast<bool*>(param.mpData);
            
            uchar c = pArg[strlen(pArg) - 1];
            if (c == '-')
               enabledFlag = false;
            else if (c == '+')
               enabledFlag = true;
               
            *reinterpret_cast<bool*>(param.mpData) = enabledFlag;
            break;
         }
         case cCLParamTypeBool:
         {
            *reinterpret_cast<bool*>(param.mpData) = param.mBoolValue;
            break;
         }
         case cCLParamTypeInt:
         {  
            *reinterpret_cast<int*>(param.mpData) = param.mIntValue;
            break;
         }
         case cCLParamTypeFloat:
         {
            *reinterpret_cast<float*>(param.mpData) = param.mFloatValue;
            break;
         }
         case cCLParamTypeIntPtr:
         case cCLParamTypeFloatPtr:
         case cCLParamTypeBStringPtr:
         {
            const uint numVals = param.mNumDataValues ? param.mNumDataValues : 1;
            
            if (numArgsLeft < numVals)
            {
               formatError(B("Parameter \"%s\" expects %i value(s)!"), BStrConv::toB(param.mpParamStr), numVals);
               return false;
            }
            
            for (uint i = 0; i < numVals; i++)
            {
               if (param.mType == cCLParamTypeFloatPtr)
                  reinterpret_cast<float*>(param.mpData)[i] = static_cast<float>(atof(args[argIndex].getPtr()));
               else if (param.mType == cCLParamTypeIntPtr)
                  reinterpret_cast<int*>(param.mpData)[i] = atoi(args[argIndex].getPtr());
               else if (param.mType == cCLParamTypeBStringPtr)
               {
                  BString paramVal(args[argIndex].getPtr());

                  if ((paramVal.length() >= 2) && (paramVal.getChar(0) == '@'))
                  {
                     paramVal.crop(1, paramVal.length() - 1);

                     BStringArray strArray;
                     
                     if (readStringFile(paramVal, strArray))
                        gConsoleOutput.printf("Successfully read string file: %s\n", paramVal.getPtr());
                     else
                     {
                        formatError("Failed reading string file: %s\n", paramVal.getPtr());
                        return false;
                     }
                     
                     if (strArray.getSize() > 1)
                        gConsoleOutput.warning("Too many strings in string file \"%s\", only using first string!\n", paramVal.getPtr());
                     else if (strArray.isEmpty())
                     {
                        formatError("String file is empty: %s\n", paramVal.getPtr());
                        return false;
                     }
                     
                     paramVal = strArray[0];
                  }
                  
                  reinterpret_cast<BString*>(param.mpData)[i] = paramVal;
               }
               
               if (removeRecognizedParams)
                  args[argIndex].empty();
                  
               argIndex++;
            }
                                                         
            break;
         }
         case cCLParamTypeBStringArrayPtr:
         {
            if (numArgsLeft < 1)
            {
               formatError("Parameter \"%s\" expects 1 value!", param.mpParamStr);
               return false;
            }
            
            BCommandLineParser::BStringArray* pArray = reinterpret_cast<BCommandLineParser::BStringArray*>(param.mpData);
            
            BString paramVal(args[argIndex].getPtr());
            
            if ((paramVal.length() >= 2) && (paramVal.getChar(0) == '@'))
            {
               paramVal.crop(1, paramVal.length() - 1);
               
               if (readStringFile(paramVal, *pArray))
                  gConsoleOutput.printf("Successfully read string file: %s\n", paramVal.getPtr());
               else
               {
                  formatError("Failed reading string file: %s\n", paramVal.getPtr());
                  return false;
               }
            }
            else
            {
               pArray->pushBack(paramVal);
            }
            
            if (removeRecognizedParams)
               args[argIndex].empty();
               
            argIndex++;
            
            break;
         }
         default:
         {
            BDEBUG_ASSERT(0);
         }
      }
   }
   
   return status;
}

void BCommandLineParser::createBStringArray(BStringArray& strArray, int argc, const char * const argv[])
{
   strArray.resize(0);

   for (int i = 0; i < argc; i++)
      strArray.pushBack(argv[i]);
}

void BCommandLineParser::formatError(const BCHAR_T* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   
   mErrorString.formatArgs(pMsg, args);
   
   va_end(args);
}
