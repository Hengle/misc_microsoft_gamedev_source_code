//============================================================================
//
//  File.h
//
//  Copyright 2002-2007 Ensemble Studios
//
//============================================================================
#pragma once
#include "filesystem.h"

//----------------------------------------------------------------------------
//  File Open Flags
//----------------------------------------------------------------------------
enum BFileOpenFlags
{
   BFILE_OPEN_NORMAL                         = 0,
   
   // BFILE_OPEN_APPEND: When opening for write access, existing files will be appended to instead of overwritten.
   BFILE_OPEN_APPEND                         = 1,
   
   // BFILE_OPEN_WRITEABLE: Open the file for write access
   BFILE_OPEN_WRITEABLE                      = 2,
   
   // BFILE_OPEN_READWRITE: Open the file for read/write access
   BFILE_OPEN_READWRITE                      = 4,
   
   // BFILE_OPEN_DISCARD_ON_CLOSE: Causes the file manager to discard the file's compressed data 
   // from the file cache when the file is closed. Use when you know for sure the file will only be ever read one time.
   BFILE_OPEN_DISCARD_ON_CLOSE               = 8,
   
   // BFILE_OPEN_BACKWARD_SEEKS: Allows backwards file seeks (forward seeks are always supported). 
   // The entire decompressed file will be present in memory when this is specified, until the file is closed.
   // If not specified, backwards seeks will fail when the file is being read from an archive.
   BFILE_OPEN_BACKWARD_SEEKS                 = 16,
         
   // BFILE_OPEN_IGNORE_ARCHIVES: Causes the file manager to ignore files in archives when finding the best file to open.
   BFILE_OPEN_IGNORE_ARCHIVES                = 32,
   
   // BFILE_OPEN_IGNORE_LOOSE: Causes the file manager to ignore loose files when finding the best file to open. 
   BFILE_OPEN_IGNORE_LOOSE                   = 64,
   
   // BFILE_OPEN_BYPASS_FILE_MANAGER: BFile will bypass the file manager's virtual file system and always 
   // create BWin32FileStream's. In this case, filenames must be valid absoluted/device pathnames (i.e. "game:\\blah.bin"), 
   // and the dirID parameter is ignored.
   BFILE_OPEN_BYPASS_FILE_MANAGER            = 128,
   
   // BFILE_OPEN_ENABLE_BUFFERING: Use buffered I/O. Useful if you will be doing a lot of small reads/writes.
   // This is done by placing a BBufferStream object between BFile and the underlying stream. 
   BFILE_OPEN_ENABLE_BUFFERING               = 256,
   
   // BFILE_OPEN_DO_NOT_WAIT_FOR_FILE: Do not wait for the file to finish loading from the archive. Return cFME_WOULD_HAVE_BLOCKED if blocking is required to access the file.
   BFILE_OPEN_DO_NOT_WAIT_FOR_FILE           = 512,

   // BFILE_OPEN_OVERWRITE_READONLY: When opening the file for write access, ensure the file is not read only before opening it.
   BFILE_OPEN_OVERWRITE_READONLY             = 1024,
   
   // BFILE_OPEN_OPTIMIZE_FOR_RANDOM_ACCESS: Hint that file will be randomly accessed.
   BFILE_OPEN_OPTIMIZE_FOR_RANDOM_ACCESS     = 2048,
   
   // BFILE_OPEN_OPTIMIZE_FOR_SEQUENTIAL_ACCESS: Hint that file will be sequentially accessed.
   BFILE_OPEN_OPTIMIZE_FOR_SEQUENTIAL_ACCESS = 4096,
   
   // Always try to open as a loose file, even if the file manager has been initialized with loose files disabled.
   BFILE_OPEN_FORCE_LOOSE                    = 8192,
};

//----------------------------------------------------------------------------
//  File Offset Positions
//----------------------------------------------------------------------------
enum BFileOffsetPositions
{
   BFILE_OFFSET_BEGIN = 0,
   BFILE_OFFSET_CURRENT,
   BFILE_OFFSET_END
};


//----------------------------------------------------------------------------
//  File Errors
//----------------------------------------------------------------------------
enum BFileErrors
{
   BFILE_NOT_OPEN                = -100,
   BFILE_NOT_CLOSED,
   BFILE_UNABLE_TO_OPEN_FILE,
   BFILE_UNABLE_TO_CREATE_FILE,
   BFILE_READ_FAILED,
   BFILE_WRITE_FAILED,
   BFILE_CLOSE_FAILED,
   BFILE_SEEK_FAILED,
   BFILE_FILE_TOO_LARGE,
   BFILE_GENERAL_ERROR,
   BFILE_WOULD_HAVE_WAITED,
   BFILE_READ_PAST_EOF,
   
   BFILE_NO_ERROR                = 0,
};

//----------------------------------------------------------------------------
//  Class BFile
//----------------------------------------------------------------------------
class BFile
{
   BFile(const BFile& file);
   const BFile& operator = (const BFile& file);
   
public:
   //-- Construction/Destruction
   BFile();
   ~BFile();
   
   bool  open(long dirID, const char* pFilename, uint flags = BFILE_OPEN_NORMAL);

   bool  openReadOnly (long dirID, const char* pFilename, uint flags = BFILE_OPEN_NORMAL)                      { return open(dirID, pFilename, flags); }
   
   // If you are going to read/write a lot of small objects, be sure to specify BFILE_OPEN_ENABLE_BUFFERING. Otherwise, the stream will be unbuffered!
   // Buffering will only benefit streams reading from loose files (Win32 or XFS).
   bool  openWriteable(long dirID, const char* pFilename, uint flags = BFILE_OPEN_NORMAL)                      { return open(dirID, pFilename, flags | BFILE_OPEN_WRITEABLE); };
   bool  openReadWrite(long dirID, const char* pFilename, uint flags = BFILE_OPEN_NORMAL | BFILE_OPEN_APPEND)  { return open(dirID, pFilename, flags | BFILE_OPEN_READWRITE); };
         
   bool  read         (void* pBuffer, uint numBytes);
   uint  readEx       (void* pBuffer, uint numBytes);
   // write will still return 0/1 from BFile
   bool  write        (const void* pBuffer, uint numBytes) { return (writeEx(pBuffer, numBytes) == numBytes); }
   uint  writeEx      (const void* pBuffer, uint numBytes);
   bool  close        ();
   void  flush        ();

   //-- File Pointer
   bool  setOffset(__int64 offset, uint fromPosition = BFILE_OFFSET_BEGIN);
   uint64 getOffset() const;
   bool  getOffset(DWORD& offset) const;
   bool  getOffset(unsigned __int64& offset) const;

   //-- Other Info
   bool  isOpen() const;
   bool  isReadable(void) const;
   bool  isWriteable(void) const;
   bool  isFullySeekable(void) const;

   bool  getSize    (DWORD& size) const;
   bool  getSize    (unsigned __int64& size) const;
   bool  getTime    (uint64& time) const;
   bool  getTime    (BFileTime& time) const;
   
   bool  getPath    (BString& path) const;
   bool  getPath    (BSimString& path) const;
      
   //-- Helper Functions
   bool    readBString       (BString& dstString);
   bool    writeBString      (const BString& string);
   
   bool    readBString       (BSimString& dstString);
   bool    writeBString      (const BSimString& string);
   
   const BYTE* getPtr(void);
   
   void  setOwnerThread(DWORD threadID);

   BFileErrors getLastError(void) const { return mLastError; }
   void setLastError(BFileErrors error) const;

   BStream* getStream(void) { return mpStream; }

   // Legacy crap.
   uint      fprintf(const char *format, ...);
   uint      fprintf(const WCHAR *format, ...);
   long        fputs(const char *pString);
   long        fputs(const WCHAR *pString);
   const char* fgets(char *string, DWORD dwMaxBytes);
      
private:
	long				mFMIndex;
	uint           mOpenFlags;
   
   BStream*       mpStream;  
   
   mutable BFileErrors    mLastError;    
   
   bool openDirect(long dirID, const char* pFilename, uint flags, uint streamFlags);
};
