//============================================================================
// flashXFSFile.h
// Ensemble Studios (C) 2007
//============================================================================

#pragma once

#include "errno.h"
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class GXFSFile : public GFile
{  
   BString mFilename; 
   BFile mFile;
   int mErrorCode;

   void clearError(void) { mErrorCode = 0; }
   void setError(int errorCode) { mErrorCode = errorCode; }

public:
   GXFSFile(long dirID, const char* pFilename) :
      mFilename(pFilename),
         mErrorCode(0)
      {

         int i = mFilename.findLeft("file://");
         if (i != -1)
            mFilename.crop(i + 7, mFilename.length() - 1);

         mFilename.standardizePath();

         // fix up relatives paths if needed.
         if ( mFilename.findLeft("..\\") != -1)      // scope this so we don't have to do this for every file.
         {
            BDynamicArray<BString> pathParts;

            BString token;
            long strLen = mFilename.length();
            long loc = token.copyTok(mFilename, strLen, -1, B("\\"));
            while (loc != -1)
            {
               if (token == "..")
               {
                  // remove the previous path.
                  if (pathParts.getNumber() > 0)
                     pathParts.removeIndex(pathParts.getNumber()-1);
                  else
                     pathParts.add(token);   // if we can't go up from here, then leave the .. and let the filemanager handle it.
               }
               else if (token != ".")        // just omit this part
               {
                  pathParts.add(token);
               }

               loc = token.copyTok(mFilename, strLen, loc+1, B("\\"));
            }

            mFilename.set("");
            // now we reconstitute the path
            for (int i=0; i<pathParts.getNumber(); i++)
            {
               if (i!=0)
                  mFilename.append("\\");

               mFilename.append(pathParts[i]);
            }
         }

         bool success = mFile.openReadOnly(dirID, mFilename, BFILE_OPEN_BACKWARD_SEEKS);
         if (!success)
         {
#ifndef BUILD_FINAL
            BString temp;
            temp.format("Failed to load: %s", mFilename.getPtr());
            // fixme - put an assert here.
            BASSERTM(0, temp.getPtr());
#endif
            setError(ENOENT);
         }
      }

      virtual const char* GetFilePath()
      {
         return mFilename.getPtr();
      }

      virtual bool IsValid()
      {
         return mFile.isOpen();
      }

      // Return 1 if file's writable, otherwise 0                                         
      virtual bool IsWritable()
      {
         return 0;
      }

      virtual SInt        Tell ()
      {
         clearError();

         uint64 ofs;
         if (!mFile.getOffset(ofs))
         {
            setError(EIO);
            return 0;
         }
         return (SInt)ofs;
      }

      virtual SInt64      LTell ()
      {
         clearError();

         uint64 ofs;
         mFile.getOffset(ofs);
         return ofs;
      }

      // File size                                                                        
      virtual SInt        GetLength ()
      {
         clearError();

         uint64 size;
         if (!mFile.getSize(size))
         {
            setError(EIO);
            return 0;
         }
         return (SInt)size;
      }

      virtual SInt64      LGetLength ()
      {
         clearError();

         uint64 size;
         if (!mFile.getSize(size))
         {
            setError(EIO);
            return 0;
         }
         return size;
      }

      // Return errno-based error code                                                    
      // Useful if any other function failed                                              
      virtual SInt        GetErrorCode()
      {
         //EIO
         return mErrorCode;
      }


      // Blocking write, will write in the given number of bytes to the stream
      // Returns : -1 for error
      //           Otherwise number of bytes read 
      virtual SInt        Write(const UByte *pbufer, SInt numBytes)
      {
         setError(EPERM);
         return -1;
      }

      // Blocking read, will read in the given number of bytes or less from the stream
      // Returns : -1 for error
      //           Otherwise number of bytes read,
      //           if 0 or < numBytes, no more bytes available; end of file or the other side of stream is closed
      virtual SInt        Read(UByte *pbufer, SInt numBytes)
      {
         clearError();

         bool success = mFile.read(pbufer, (uint)numBytes);
         if (!success)
         {  
            setError(EIO);
            return -1;
         }
         return numBytes;
      }

      // Skips (ignores) a given # of bytes
      // Same return values as Read
      virtual SInt        SkipBytes(SInt numBytes)
      {
         clearError();

         bool success = mFile.setOffset(numBytes, BFILE_OFFSET_CURRENT);
         if (!success)
         {
            setError(EIO);
            return -1;
         }
         return numBytes;
      }

      // Returns the number of bytes available to read from a stream without blocking
      // For a file, this should generally be number of bytes to the end
      virtual SInt        BytesAvailable()
      {
         clearError();

         uint64 size;
         if (!mFile.getSize(size))
         {
            setError(EIO);
            return 0;
         }

         uint64 ofs;
         if (!mFile.getOffset(ofs))
         {
            setError(EIO);      
            return 0;
         }

         uint64 remaining = size - ofs;
         if (remaining > INT_MAX)
         {
            setError(EIO);
            return 0;
         }

         return (SInt)remaining;
      }

      virtual bool        Flush()
      {
         setError(EPERM);
         return 0;
      }

      GINLINE bool        IsEOF() { return !BytesAvailable(); }

      // Seeking                                                                              
      // Returns new position, -1 for error                                                   
      virtual SInt        Seek(SInt offset, SInt origin=Seek_Set)
      {
         SInt64 pos = LSeek(offset, origin);

         if (pos > INT_MAX)
         {
            setError(ERANGE);
            return -1;
         }

         return (SInt)pos;
      }

      virtual SInt64      LSeek(SInt64 offset, SInt origin=Seek_Set)                        
      {
         clearError();

         uint seek;
         switch (origin)
         {
         case Seek_Set:  seek = BFILE_OFFSET_BEGIN; break;
         case Seek_Cur:  seek = BFILE_OFFSET_CURRENT; break;
         case Seek_End:  
         default:
            BFATAL_FAIL("unsupported");
         }

         if (!mFile.setOffset(offset, seek))
         {
            setError(EIO);
            return -1;
         }

         unsigned long ofs;
         if (!mFile.getOffset(ofs))
         {
            setError(EIO);
            return -1;
         }

         return (SInt64)ofs;
      }

      // Seek simplification
      SInt                SeekToBegin()           {return Seek(0); }
      SInt                SeekToEnd()             {return Seek(0,Seek_End); }
      SInt                Skip(SInt numBytes)     {return Seek(numBytes,Seek_Cur); }

      // Resizing the file                                    
      // Return 0 for failure
      virtual bool        ChangeSize(SInt newSize)
      {
         setError(EPERM);
         return 0;
      }

      // Appends other file data from a stream
      // Return -1 for error, else # of bytes written
      virtual SInt        CopyFromStream(GFile *pstream, SInt byteSize)
      {
         setError(EPERM);
         return -1;
      }

      virtual bool        Close()
      {
         clearError();
         if (!mFile.close())
         {
            setError(EIO);
            return 0;
         }
         return 1;
      }
};