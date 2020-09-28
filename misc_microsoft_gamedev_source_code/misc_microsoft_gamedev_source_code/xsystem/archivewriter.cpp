#include "xsystem.h"
#include "regexp.h"
#include "xmlreader.h"
#include "archive.h"
#include "config.h"
#include "econfigenum.h"

#pragma warning (disable: 4127)

#define DOOUTPUT if(input.mSupressOutput==false)printf

//-----------------------------------------------------------------------------
// globals
//-----------------------------------------------------------------------------
#ifdef XBOX
static const long gBufferSize=1;
#else
static const long gBufferSize=1024000;
#endif
static BYTE gBuffer[gBufferSize];

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CALLBACK findAndWriteFiles(const BString& Path, void* pParam)
{
   BArchiveWriterInput* pInput = (BArchiveWriterInput*)pParam;
   if(pInput == NULL)
      return(false);

   long numItems = pInput->mExcludeExts.getNumber();
   for(long i=0; i<numItems; i++)
   {
      //-- Don't add this file.
      if(Path.findLeft(pInput->mExcludeExts[i]) != -1)
      {
         if(pInput->mSupressOutput==false)
         {
            printf("findAndWriteFiles: SkippingExt %s -> %s\n", pInput->mExcludeExts[i].getPtr(), Path.getPtr());
         }
         return(true);
      }
   }

   BString fileName;
   strPathGetFilename(Path, fileName);

   numItems = pInput->mExcludeFiles.getNumber();
   for(long i=0; i<numItems; i++)
   {
      if(fileName.compare(pInput->mExcludeFiles[i], false) == 0)
      {
         if(pInput->mSupressOutput==false)
         {
            printf("findAndWriteFiles: SkippingFile %s -> %s\n", pInput->mExcludeFiles[i].getPtr(), Path.getPtr());
         }
         return(true);
      }
   }

	//-- If this is a xml file, then convert it to xmb and load that file.
	BString newPath = Path;
	bool hasXMLExt = false;
	numItems = pInput->mXmlToBinaryExtensions.getNumber();
	for(long i=0; i<numItems; i++)
	{
		if(strPathHasExtension(newPath, pInput->mXmlToBinaryExtensions[i]) == true)
		{
			hasXMLExt = true;
			break;
		}
	}

	//-- if we have the xml type ext, then we should xmb it.   Note: Only if its not excluded from the xmb process.
	if(hasXMLExt == true)
	{
		//-- Check to see if we should xmb this file.
		bool shouldConvert = true;
		numItems = pInput->mExcludeFromXMBConversion.getNumber();
		for(long i=0; i<numItems; i++)
		{
			Regexp regexp(pInput->mExcludeFromXMBConversion[i].getPtr(), Regexp::nocase);
			if (regexp.match(newPath.getPtr()))
			{
				shouldConvert = false;
				break;
			}
		}
	
		//-- convert to xmb.
		if(shouldConvert == true)
		{
			BString temp;
			temp.set(newPath);

			// load the file
			BXMLReader reader;
			if(!reader.loadFileSAX(cDirCurrent, temp.getPtr(), NULL, false))
			{
				return(false);
			}

			// save out the XMB
			//strPathRemoveExtension(temp);
			temp.append(B(".xmr"));
			if(reader.save(cDirCurrent, temp.getPtr()) == false)
			{
				return(false);
			}

			if(pInput->mbDeleteSourceFile == true)
			{
				gFileManager.setFileReadOnly(cDirCurrent, newPath, false);
				gFileManager.deleteFile(cDirCurrent, newPath);
			}
			
			//-- get the correct extension.
			newPath.set(temp);
			strPathRemoveExtension(newPath);
			newPath.append(B(".xmb"));

			//-- delete xmr file.
			gFileManager.deleteFile(cDirCurrent, temp);
		}
	}

   //-- Found good file, start writing to disk.
   BFile srcFile;
   bool ok = srcFile.openReadOnly(cDirCurrent, newPath);
   if(!ok)
      return(false);
   
   BArchiveFileEntry fileEntry;

   fileEntry.mFileName = newPath;
   fileEntry.mFileName.crop(pInput->mRoot.length(), newPath.length());
   pInput->mArchiveFile.getOffset(fileEntry.mdwOffset);
   gFileManager.getFileTime(Path, fileEntry.mFileTime);

   while(true)
   {
      DWORD dwreads = srcFile.readEx(gBuffer, gBufferSize);
      if (!dwreads)
         break;

      fileEntry.mdwLength += dwreads;
      if (pInput->mArchiveFile.write(gBuffer, dwreads) == false)
      {
         return(false);
      }
   }

   //-- archive length is the same as original length if uncompressed.
   fileEntry.mdwArchiveLength=fileEntry.mdwLength;
   pInput->mFileEntries.add(fileEntry);

	//-- if we are deleting the source files as we go, delete it now.
	srcFile.close();
	if(pInput->mbDeleteSourceFile == true)
	{
		gFileManager.setFileReadOnly(cDirCurrent, newPath, false);
		gFileManager.deleteFile(cDirCurrent, newPath);
	}

   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CALLBACK diffFiles(const BString& Path, void* pParam)
{
   BArchiveWriterInput* pInput = (BArchiveWriterInput*)pParam;
   if(pInput == NULL)
      return(false);
   if(pInput->mArchivesAreDifferent)
      return(true);

   BArchiveFileEntry entry;
   BDynamicSimArray<BArchiveFileEntry> entryList;
   
   // Make standardized for archive processing.
	BString standardizedPath;
	standardizedPath.standardizePathFrom(Path);

   long numItems = pInput->mExcludeExts.getNumber();
   for(long i=0; i<numItems; i++)
   {
      //-- Don't add this file.
      if(Path.findLeft(pInput->mExcludeExts[i]) != -1)
      {
         pInput->mArchivesAreDifferent = pInput->mpCompareArchive->findFile(standardizedPath, true, NULL);
         if(pInput->mArchivesAreDifferent)
         {
            if(pInput->mSupressOutput==false)
            {
               printf("diffFiles: Need to remake archive\n");
            }
         }
            
         return(true);
      }
   }

   BString fileName;
   strPathGetFilename(Path, fileName);

   numItems = pInput->mExcludeFiles.getNumber();
   for(long i=0; i<numItems; i++)
   {
      Regexp regexp(pInput->mExcludeFiles[i].getPtr(), Regexp::nocase);
      if (regexp.match(fileName.getPtr()))
      {
         pInput->mArchivesAreDifferent = pInput->mpCompareArchive->findFile(standardizedPath, false, NULL);
         if(pInput->mArchivesAreDifferent)
         {
            if(pInput->mSupressOutput==false)
            {
               printf("diffFiles: Need to remake archive.\n");
            }
         }
         return(true);
      }
   }

   //-- Found good file.
   pInput->mArchivesAreDifferent = !pInput->mpCompareArchive->findFile(standardizedPath, false, &entryList);
   if((pInput->mArchivesAreDifferent) || (entryList.getNumber() == 0))
   {
      pInput->mArchivesAreDifferent = true;
      if(pInput->mSupressOutput==false)
      {
         printf("diffFiles: Need to remake archive.\n");
      }
      return(true);
   }

   BFileTime fileTime;
   gFileManager.getFileTime(Path, fileTime);
   pInput->mArchivesAreDifferent = !entryList[0].mFileTime.equal(fileTime);
   if(pInput->mArchivesAreDifferent)
   {
      if(pInput->mSupressOutput==false)
      {
         printf("diffFiles: Need to remake archive.\n");
      }
   }
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CALLBACK findDirs(const BString& Path, void* pParam)
{
   BDynamicSimArray<BString>* pDirs = (BDynamicSimArray<BString>*) pParam;
   if(pDirs == NULL)
      return(false);

   //-- Add backslash if needed.
   BString newPath = Path;
   strPathAddBackSlash(newPath);
   pDirs->add(newPath);

   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BArchiveWriter::parseInput(BArchiveWriterInput& input, const BXMLNode* pNode)
{
   if(pNode->getName() != BString("bar"))
   {
      return(false);
   }

   //-- See if we should supress output. Default is to show output.
   long supressOutput = 0;
   pNode->getAttribValueAsLong("supressOutput", supressOutput);
   input.mSupressOutput = (supressOutput!=0) ? true : false;

   
   long numChildren = pNode->getNumberChildren();
   for(long i=0; i<numChildren; i++)
   {
      parseNode(input, pNode->getChild(i));
   }
   
   //-- Always add these excludes.
   input.mExcludeExts.add(BString(".bar"));
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BArchiveWriter::parseNode(BArchiveWriterInput& input, const BXMLNode* pNode)
{
   //-- Look for specific child node values.
   const BString& name = pNode->getName();
   if(name.compare(B("Root"))==0)
   {
      input.mRoot=pNode->getText();
   }
   else if(name.compare(B("ArchiveName"))==0)
   {
      input.mArchiveName=pNode->getText();
   }
   else if(name.compare(B("ExcludeDir"))==0)
   {
      input.mExcludeDirs.add(pNode->getText());
   }
   else if(name.compare(B("ExcludeExt"))==0)
   {
      input.mExcludeExts.add(pNode->getText());
   }
   else if(name.compare(B("ExcludeFile"))==0)
   {
      input.mExcludeFiles.add(pNode->getText());
   }
	else if(name.compare(B("XMLtoXMBExtension"))==0)
	{
		input.mXmlToBinaryExtensions.add(pNode->getText());
	}
	else if(name.compare(B("ExcludeFromXMBFileConversion"))==0)
	{
		input.mExcludeFromXMBConversion.add(pNode->getText());
	}
	else if(name.compare(B("DeleteSourceFiles"))==0)
	{
		input.mbDeleteSourceFile = true;
	}
   
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BArchiveWriter::build(BArchiveWriterInput& input)
{
   //-- Validate input.
   if((input.mArchiveName.isEmpty() == true) || (input.mRoot.isEmpty() == true))
   {
      return(false);
   }

   //-- Build the list of directories to put into archive.
   buildDirList(input);

   //-- If the file isn't different, dont rebuild it.
   if(diff(input) == false)
   {
      return(true);
   }
   
   //-- Open file for writing and reading.  Clearing out the file if it existed.
   bool ok = input.mArchiveFile.openReadWrite(cDirCurrent, input.mArchiveName, BFILE_OPEN_NORMAL);
   if(!ok)
   {
      return(false);
   }

   //-- Write out header.
   BArchiveHeader header;
   if(input.mArchiveFile.write(&header, sizeof(header)) == false)
   {
      return(false);
   }
   
   //-- Build the list of files we are going to add to the archive.
   buildFileList(input);
   
   //-- Save the current File Position. Will stick back into header later.
   DWORD fileTableOffset=0;
   input.mArchiveFile.getOffset(fileTableOffset);

   //-- Save out the Root.
   if(input.mArchiveFile.writeBString(input.mRoot) == false)
   {
      return(false);
   }
   
   //-- Save the File Entries.
   writeFileEntries(input);

   //-- Calc the calcAdler32
   DWORD dwAdler = 0;
   calcAdler32(input, dwAdler);

   //-- ReWrite the header with better information this time.
   input.mArchiveFile.setOffset(0);
   header.mdwAdler32 = dwAdler;
   header.mdwEntries = input.mFileEntries.getNumber();
   header.mdwManifestOffset = fileTableOffset;

   if (input.mArchiveFile.write(&header, sizeof(header)) == false)
   {
      return(false);
   }
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BArchiveWriter::diff(BArchiveWriterInput& input)
{
   //-- Validate input.
   if((input.mArchiveName.isEmpty() == true) || (input.mRoot.isEmpty() == true))
   {
      return(true);
   }

   BHandle scriptArchiveHandle = gFileManager.openArchive(cDirCurrent, input.mArchiveName);
   if(scriptArchiveHandle == 0)
      return(true);

   input.mpCompareArchive = gFileManager.getArchive(scriptArchiveHandle);
   if(input.mpCompareArchive == NULL)
      return(true);

   if(diffFileList(input) == false)
   {
      return(true);
   }

   gFileManager.closeArchive(scriptArchiveHandle);

   //-- See if the files are different.
   return(input.mArchivesAreDifferent);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BArchiveWriter::buildDirList(BArchiveWriterInput& input)
{
   //-- Find all dirs from the root path.
   BString searchPath = input.mRoot;
   
   //-- Add backslash at end, if there isn't one already.
   strPathAddBackSlash(searchPath);
   strPathAddBackSlash(input.mRoot);
   
   input.mDirectories.add(searchPath);
   if(gFileManager.findFiles(
               searchPath, BString("*.*"), 
               BFFILE_WANT_DIRS|BFFILE_RECURSE_SUBDIRS|BFFILE_DONT_SEARCH_ARCHIVES, findDirs, &input.mDirectories) == false)
   {
      return(false);
   }
   
   //-- Now remove any exclude dirs from the list.
   long numExcludes = input.mExcludeDirs.getNumber();
   BString dir;
   for(long i=0; i<numExcludes; i++)
   {
      dir = input.mExcludeDirs[i];
      strPathAddBackSlash(dir);
      dir.prepend(searchPath);
      if(input.mDirectories.removeValue(dir) == true)
      {
         DOOUTPUT("buildDirList:removing dir from list: %s\n", dir.getPtr());
      }
   }

   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BArchiveWriter::buildFileList(BArchiveWriterInput& input)
{
   BString searchPath;
   BString wildCard("*.*");

   long numDirs = input.mDirectories.getNumber();
   for(long i=0; i<numDirs; i++)
   {
      searchPath = input.mDirectories[i];
      if(gFileManager.findFiles(searchPath, wildCard, BFFILE_DONT_SEARCH_ARCHIVES|BFFILE_WANT_FILES, findAndWriteFiles, &input) == false)
         return(false);
   }
   
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BArchiveWriter::diffFileList(BArchiveWriterInput& input)
{
   BString searchPath;
   BString wildCard("*.*");

   long numDirs = input.mDirectories.getNumber();
   for(long i=0; i<numDirs; i++)
   {
      searchPath = input.mDirectories[i];
      if(gFileManager.findFiles(searchPath, wildCard, BFFILE_DONT_SEARCH_ARCHIVES|BFFILE_WANT_FILES, diffFiles, &input) == false)
         return(false);
   }
   
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BArchiveWriter::writeFileEntries(BArchiveWriterInput& input)
{
   long numFiles = input.mFileEntries.getNumber();



   //-- Write out the number of file entries.
   if(input.mArchiveFile.write(&numFiles, sizeof(numFiles)) == false)
   {
      return(false);
   }

   BArchiveFileEntry* pEntry = NULL;
   for(long i=0; i<numFiles; i++)
   {
      pEntry = &(input.mFileEntries[i]);
      if(pEntry == NULL)
      {
         return(false);
      }

      DOOUTPUT("adding %s", BStrConv::toA(pEntry->mFileName));
      
      if(input.mArchiveFile.write(&pEntry->mdwOffset, sizeof(pEntry->mdwOffset)) == false)
      {
         return(false);
      }
      
      DOOUTPUT(".");

      if(input.mArchiveFile.write(&pEntry->mdwLength, sizeof(pEntry->mdwLength)) == false)
      {
         return(false);
      }

      DOOUTPUT(".");
      
      if(input.mArchiveFile.write(&pEntry->mdwArchiveLength, sizeof(pEntry->mdwArchiveLength)) == false)
      {
         return(false);
      }

      DOOUTPUT(".");

      if(input.mArchiveFile.write(&pEntry->mFileTime, sizeof(pEntry->mFileTime)) == false)
      {
         return(false);
      }

      DOOUTPUT(".");

      if(input.mArchiveFile.writeBString(pEntry->mFileName) == false)
      {
         return(false);
      }

      DOOUTPUT(".done\n");
   }

   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BArchiveWriter::calcAdler32(BArchiveWriterInput& input, DWORD& dwAdler)
{
   return false;
#if 0
   //-- Make sure we are done writing to the file.
   FlushFileBuffers(input.mArchiveFile.getHandle());
   
   //-- calculate CRC starting after header
   input.mArchiveFile.setOffset(sizeof(BArchiveHeader));

   while (true)
   {
      DWORD dwreads = input.mArchiveFile.readEx(gBuffer, gBufferSize);
      if (!dwreads)
         break;

      dwAdler = adler32(dwAdler, (BYTE *)gBuffer, dwreads);
   }
   return(true);
#endif   
}
/*
 *	
 int main(int argc, char** argv)
 {
 if(argc != 2)
 {
 printf("xmbgen.exe requires one argument: the XML to convert.");
 return cERRBadParams;
 }

 // initialize XML stuff
 HRESULT result = CoInitialize(NULL);
 if(FAILED(result))
 {
 printf("COM not initialized");
 return cERRCOMNotInitialized;
 }

 BString temp;
 temp.set(argv[1]);

 // load the file
 BXMLReader reader;
 long dirID = -1;
 if(isExternal(temp.getPtr()))
 dirID = cDirAbsolutePath;
 else
 dirID = cDirCurrent;
 if(!reader.loadFileSAX(dirID, temp, NULL, false))
 {
 printf("File load failed.");
 return cERRLoadFailed;
 }

 // save out the XMB
 strPathRemoveExtension(temp);
 temp.append(B(".xmr"));
 if(reader.save(dirID, temp.getPtr()))
 {
 printf("XMB generated successfully.");
 return cERROk;
 }
 else
 {
 printf("File save failed.");
 return cERRSaveFailed;
 }
 }

 */
