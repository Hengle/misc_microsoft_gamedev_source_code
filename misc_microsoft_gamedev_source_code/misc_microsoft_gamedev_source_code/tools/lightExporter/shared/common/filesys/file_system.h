// file_system.h
#pragma once
#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include "common/utils/stream.h"
#include "common/utils/string.h"

namespace gr
{
	class FileSystem
	{
	public:
		virtual ~FileSystem();
				
		virtual void clearSearchPaths(void) = 0;
				
		virtual void addSearchPath(const BigString& path) = 0;
				
		virtual int numSearchPaths(void) const = 0;
				
		virtual const BigString& searchPath(int index) const = 0;
		
		// -1 if not found, otherwise returns search path index
		virtual int doesFileExist(BigString& fullFilename, const BigString& rawFilename) const = 0;

		// returns resolved filename, path index
		virtual std::pair<BigString, int> resolveFilename(const BigString& rawFilename) const = 0;
		
		virtual Stream* createStream(const BigString& rawFilename) = 0;
	};
	
	extern FileSystem& gFileSystem;
					
} // namespace gr

#endif // FILE_SYSTEM_H
