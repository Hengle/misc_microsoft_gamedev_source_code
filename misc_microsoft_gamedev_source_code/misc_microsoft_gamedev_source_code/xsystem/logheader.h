//=============================================================================
// logheader.h
//
// Copyright (c) 1999 Ensemble Studios
//=============================================================================

#ifndef _LOGHEADER_H_
#define _LOGHEADER_H_

//=============================================================================
class BLogHeader
{
   public:
      enum
      {
         cMaxHeaderNameLength=128,
         cHeaderInvalid=-1,
         cMaxFilenameLength=256
      };

      // Header stuff
      char     msTitle[cMaxHeaderNameLength];
      long     miLogFile;
      long     miIndentLevel;
      long     miParentHeader;
      bool     mbGameTimeStamp : 1;
      bool     mbTimeStamp : 1;
      bool     mbTitleStamp : 1;
      bool     mbFileAndLineStamp : 1;
      bool     mbValid : 1;
      bool     mbConsoleOutput : 1;

}; // BLogHeader

//=============================================================================

#endif // _LOGHEADER_H_

//=============================================================================
// eof: logheader.h
//=============================================================================
