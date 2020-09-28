//==============================================================================
// xcontentStream.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once

#include "win32FileStream.h"

#ifdef XBOX

//==============================================================================
// 
//==============================================================================
class BXContentStream : public BWin32FileStream
{
   public:

      BXContentStream() :
         BWin32FileStream(),
         mXContentOpened(false)
      {
      }

      ~BXContentStream() { close(); }

      // opens a read-only stream of an existing file
      virtual bool open(DWORD userIndex, const XCONTENT_DATA& content, uint flags)
      {
         BDEBUG_ASSERT(content.DeviceID != XCONTENTDEVICE_ANY);
         if (content.DeviceID == XCONTENTDEVICE_ANY)
            return false;

         DWORD dwRet;
         DWORD dwContentFlags = XCONTENTFLAG_OPENALWAYS;

         if (flags & cSFOpenExisting)
         {
            BOOL fUserIsCreator = FALSE;
            dwRet = XContentGetCreator(userIndex, &content, &fUserIsCreator, NULL, NULL);
            if (dwRet != ERROR_SUCCESS)
            {
               // set an error code in the stream for later reference
               return false;
            }
            else if (!fUserIsCreator)
            {
               return false;
            }

            dwContentFlags = XCONTENTFLAG_OPENEXISTING;
         }
         else if (flags & cSFWritable)
            dwContentFlags = XCONTENTFLAG_CREATEALWAYS;

         DWORD dwDisposition = 0;
         dwRet = XContentCreate(userIndex, "phx", &content, dwContentFlags, &dwDisposition, NULL, NULL);
         if (dwRet != ERROR_SUCCESS)
         {
            return false;
         }
         else if (flags & cSFOpenExisting && dwDisposition != XCONTENT_OPENED_EXISTING)
         {
            return false;
         }

         BString filename;
         filename.set(content.szFileName, XCONTENT_MAX_FILENAME_LENGTH);

         BString fullFilename;
         fullFilename.format("phx:\\%s", filename.getPtr());

         bool retval = BWin32FileStream::open(fullFilename, flags, &gWin32LowLevelFileIO);

         // set this after we open the stream
         // this prevents us from calling XContentClose in the close method
         // during the stream's open method
         mXContentOpened = true;

         return retval;
      }

      virtual bool close()
      {
         bool retval = BWin32FileStream::close();

         if (retval && mXContentOpened)
         {
            DWORD dwRet = XContentClose("phx", NULL);
            if (dwRet != ERROR_SUCCESS)
            {
               // set the internal error code
               retval = false;
            }

            mXContentOpened = false;
         }

         return retval;
      }

   protected:
      bool mXContentOpened : 1;
};

#endif // XBOX