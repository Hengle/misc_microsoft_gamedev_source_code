// win32_file_system.h
#pragma once
#ifndef WIN32_FILE_SYSTEM_H
#define WIN32_FILE_SYSTEM_H

#include "common/filesys/file_system.h"
	
namespace gr
{
	class Win32FileSystem : public FileSystem
	{
	public:
		Win32FileSystem();
		virtual ~Win32FileSystem();
				
		virtual void clearSearchPaths(void);
				
		virtual void addSearchPath(const BigString& path);
				
		virtual int numSearchPaths(void) const;
				
		virtual const BigString& searchPath(int index) const;
		
		// -1 if not found, otherwise returns search path index
		virtual int doesFileExist(BigString& fullFilename, const BigString& rawFilename) const;

		virtual std::pair<BigString, int> resolveFilename(const BigString& rawFilename) const;
		
		virtual Stream* createStream(const BigString& rawFilename);
	private:
		std::vector<BigString> mSearchPaths;		
	};
	
} // namespace gr

#endif // WIN32_FILE_SYSTEM_H 
