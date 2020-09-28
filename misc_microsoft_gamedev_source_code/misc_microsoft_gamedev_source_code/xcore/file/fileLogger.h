//==============================================================================
// fileLogger.h
//
// Copyright (c) 2007, Ensemble Studios
//==============================================================================

#pragma once

class IFileLogger
{
   public:
      virtual ~IFileLogger() {}
      virtual bool openWriteable(long dirID, const char* pFilename, uint flags) = 0;
      virtual uint writeEx(const void* pBuffer, uint numBytes) = 0;
      virtual bool close() = 0;
      virtual void flush() = 0;
      virtual uint fprintf(const char* format, ...) = 0;
      virtual uint64 getOffset() const = 0;
      virtual bool isOpen() const = 0;
};
