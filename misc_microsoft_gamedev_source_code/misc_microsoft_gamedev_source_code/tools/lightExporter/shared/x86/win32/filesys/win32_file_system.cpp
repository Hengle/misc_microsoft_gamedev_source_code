// File: win32_file_system.cpp
#include "win32_file_system.h"

namespace gr
{
	Win32FileSystem gWin32FileSystem;
	FileSystem& gFileSystem = gWin32FileSystem;
	
	#define PATH_SEPERATOR "\\"
	#define ALT_PATH_SEPERATOR "/"	
	
	#define PATH_SEPERATOR_CHAR '\\'
	#define ALT_PATH_SEPERATOR_CHAR '/'	
	
	Win32FileSystem::Win32FileSystem() : FileSystem()
	{
	}
	
	Win32FileSystem::~Win32FileSystem()
	{
	}
					
	void Win32FileSystem::clearSearchPaths(void)
	{
		Status("Win32FileSystem::clearSearchPaths\n");
		
		mSearchPaths.clear();
	}
	
	void Win32FileSystem::addSearchPath(const BigString& path)
	{
		Status("Win32FileSystem::addSearchPath: Adding path \"%s\"\n", path.c_str());
		
		mSearchPaths.push_back(path);
	}
	
	int Win32FileSystem::numSearchPaths(void) const 
	{
		return mSearchPaths.size();
	}
	
	const BigString& Win32FileSystem::searchPath(int index) const 
	{
		return mSearchPaths[DebugRange(index, numSearchPaths())];
	}
	
	int Win32FileSystem::doesFileExist(BigString& fullFilename, const BigString& rawFilename) const
	{
		fullFilename = rawFilename;
		
		for (int i = 0; i < mSearchPaths.size(); i++)
		{
			const BigString filename(BigString(mSearchPaths[i]).convertToPathname() + rawFilename);
			
			FILE* pFile = fopen(filename, "rb");
			if (pFile)
			{
				fclose(pFile);
				fullFilename = filename;				
				return i;
			}
		}
		
		return -1;		
	}

	std::pair<BigString, int> Win32FileSystem::resolveFilename(const BigString& rawFilename) const
	{
		BigString fullFilename;
		const int pathIndex = doesFileExist(fullFilename, rawFilename);
		return std::make_pair(fullFilename, pathIndex);
	}
	
	Stream* Win32FileSystem::createStream(const BigString& rawFilename)
	{
		const std::pair<BigString, int> resolveResult(resolveFilename(rawFilename));
		if (resolveResult.second < 0)
			return NULL;
		
		const BigString& filename = resolveResult.first;
		
		FILEStream* pStream = new FILEStream(filename, true, false);
		if (pStream->errorStatus())
		{
			delete pStream;
			return NULL;
		}
		
		return pStream;
	}
		
} // namespace gr

