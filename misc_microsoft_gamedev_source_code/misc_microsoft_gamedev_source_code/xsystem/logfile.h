//=============================================================================
// logfile.h
//
// Copyright (c) 1999-2008 Ensemble Studios
//=============================================================================

#pragma once

//=============================================================================
//
//=============================================================================
class BLogFile
{
public:
   enum
   {
      cLogLineLength = 1024,
      cMaxRollingLines = 1024
   };

   BLogFile();

   // File stuff
   BSimString     mFileName;

   BStream*       mpStream;
   BStream*       mpDeflateStream;
   BStream*       mpRawStream;

   char*          mLogLines;

   long           mPostWriteAction;
   long           miRollingLength;
   long           miLineNum;

   bool           mbLineNumbering : 1;
   bool           mbValid : 1;

   // Parameters for delayed open files (basically for error/warning logs for now)
   bool           mbDelayedOpen : 1;
   bool           mbDelayedOpenAppend : 1;
   bool           mbDelayedOpenHeaderStamp : 1;

}; // BLogFile
