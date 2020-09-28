//============================================================================
//
//  File: ecfArc.cpp
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "xcorelib.h"
#include "utils\consoleAppHelper.h"
#include "consoleOutput.h"
#include "file\win32FindFiles.h"
#include "file\win32FileUtils.h"
#include "utils\commandLineParser.h"
#include "utils\consoleAppHelper.h"
#include "stream\cfileStream.h"
#include "utils\endianSwitch.h"
#include "resource\ecfUtils.h"
#include "resource\ecfHeaderIDs.h"
#include "resource\resourceTag.h"
#include "file\win32FileStream.h"
#include "hash\digitalSignature.h"
#include "stream\dynamicStream.h"
#include "stream\byteStream.h"
#include "containers\hashMap.h"
#include "ecfArchiver.h"

#define TEMP_FILENAME "___tmp___.!tmp!"

#define PROGRAM_TITLE "ECFARC" 
#define ARCHIVE_EXTENSION "ERA"

//-------------------------------------------------

enum eProcessingMode
{
   cModeInvalid,
   
   // Input: key phase, output: .key file, .inc public key
   // -keyGen -secretKeyPhrase "blah" -outfile EnsembleKey
   cModeKeyGen,
   
   // Input: archive filename, input files, .key file, Output: ERA archive
   // -create -outfile blah.era -file *.* -deep -secretKeyFile blah.key
   // Accepts listing file
   cModeCreate,
   
   // Input: archive ERA filenames
   // -view -file *.era
   // Accepts listing file
   cModeView,
         
   // Input: archive ERA filenames, .inc public key(s)
   // -verify -publicKeyFile blah.inc -file *.era
   // Accepts listing file
   cModeVerify,
   
   // Input: archive ERA filenames, output path
   // -extract -file *.era -outpath C:\blah  -publicKeyFile blah.inc
   // Accepts listing file
   cModeExtract,
};

class BCmdLineParams
{
public:
   BCommandLineParser::BStringArray mFileStrings;
   BCommandLineParser::BStringArray mExcludeStrings;
   BCommandLineParser::BStringArray mExcludeList;
   
   BString mOutputPath;
   BString mOutputFilename;
   BString mAppendString;
   
   BString mSourceFilename;
   BString mListFilename;
   BString mPassword;
   BString mSecretKeyFilename;
   BCommandLineParser::BStringArray mPublicKeyFilenames;
   
   BString mBasePath;
   
   bool mOutSameDirFlag;
   bool mDeepFlag;
   
   bool mNoOverwrite;
   bool mIgnoreErrors;

   bool mRemoveDups;
   bool mIgnoreMissingFiles;
   bool mCheckOut;

   int mProcessingMode;

   BCmdLineParams() :
      mOutSameDirFlag(false),
      mDeepFlag(false),

      mNoOverwrite(false),
      mIgnoreErrors(false),
      mCheckOut(false),
      mRemoveDups(false),
      mIgnoreMissingFiles(false),
      mProcessingMode(cModeInvalid)
   {
   }

   bool parse(BCommandLineParser::BStringArray& args)
   {
      const BCLParam clParams[] =
      {
         {"file",                cCLParamTypeBStringArrayPtr, &mFileStrings },
         {"password",            cCLParamTypeBStringPtr, &mPassword },
         {"secretKeyFile",       cCLParamTypeBStringPtr, &mSecretKeyFilename },
         {"publicKeyFile",       cCLParamTypeBStringArrayPtr, &mPublicKeyFilenames },
         {"outpath",             cCLParamTypeBStringPtr, &mOutputPath },
         {"sourcefile",          cCLParamTypeBStringPtr, &mSourceFilename },
         {"outfile",             cCLParamTypeBStringPtr, &mOutputFilename },
         {"listfile",            cCLParamTypeBStringPtr, &mListFilename },
         {"append",              cCLParamTypeBStringPtr, &mAppendString },
         {"outsamedir",          cCLParamTypeFlag, &mOutSameDirFlag },
         {"nooverwrite",         cCLParamTypeFlag, &mNoOverwrite },
         {"deep",                cCLParamTypeFlag, &mDeepFlag },
         {"removeDups",          cCLParamTypeFlag, &mRemoveDups },
         {"ignoreMissing",       cCLParamTypeFlag, &mIgnoreMissingFiles },
         {"ignoreerrors",        cCLParamTypeFlag, &mIgnoreErrors },
         {"checkout",            cCLParamTypeFlag, &mCheckOut },
         {"excludeSubstring",    cCLParamTypeBStringArrayPtr, &mExcludeStrings },
         {"exclude",             cCLParamTypeBStringArrayPtr, &mExcludeList },

         {"keyGen",              cCLParamTypeInt, &mProcessingMode, 0, NULL, cModeKeyGen },
         {"view",                cCLParamTypeInt, &mProcessingMode, 0, NULL, cModeView },
         {"create",              cCLParamTypeInt, &mProcessingMode, 0, NULL, cModeCreate },
         {"verify",              cCLParamTypeInt, &mProcessingMode, 0, NULL, cModeVerify },
         {"extract",             cCLParamTypeInt, &mProcessingMode, 0, NULL, cModeExtract },
         {"basePath",            cCLParamTypeBStringPtr, &mBasePath },
         
         { NULL } 
      };

      BCommandLineParser parser(clParams);

      const bool success = parser.parse(args, false, false);

      if (!success)
      {
         gConsoleOutput.error("%s\n", parser.getErrorString());
         return false;
      }

      if (parser.getUnparsedParams().size())
      {
         gConsoleOutput.error("Invalid parameter: %s\n", args[parser.getUnparsedParams()[0]].getPtr());
         return false;
      }

      return true;
   }

   void printHelp(void)
   {
      //      --------------------------------------------------------------------------------
      gConsoleOutput.printf("Usage: ecfArc <options>\n");
      gConsoleOutput.printf("\n");
      gConsoleOutput.warning("IMPORTANT: .SECRETKEY files must be kept SECRET. DO NOT ship\n");
      gConsoleOutput.warning(".SECRETKEY files with the game, with patches, etc.\n");
      gConsoleOutput.warning(".INC files contain a 20 byte public key. They should be\n");
      gConsoleOutput.warning("compiled into the game's executable.\n");
      gConsoleOutput.printf("\n");
      gConsoleOutput.printf("Options:\n");
      gConsoleOutput.printf(" -file filename             Source filename (wildcards okay), may be\n");
      gConsoleOutput.printf("                            specified multiple times.\n");
      gConsoleOutput.printf(" -outPath path              Output path\n");
      gConsoleOutput.printf(" -outFile filename          Output filename\n");
      gConsoleOutput.printf(" -secretKeyFile filename    Digital signature secret key filename\n");
      gConsoleOutput.printf(" -publicKeyFile filename    Digital signature public key filename\n");
      gConsoleOutput.printf(" -sourceFile filename       Source filename\n");
      gConsoleOutput.printf(" -append string             Append string to filename\n");
      gConsoleOutput.printf(" -outSameDir                Write output files to source path\n");
      gConsoleOutput.printf(" -deep                      Recurse subdirectories\n");
      gConsoleOutput.printf(" -removeDups                Remove duplicate files from filelist\n");
      gConsoleOutput.printf(" -ignoreMissing             Ignore missing files\n");
      
      gConsoleOutput.printf(" -nooverwrite               Don't overwrite existing files\n");
      gConsoleOutput.printf(" -ignoreErrors              Don't stop on failed files\n");
      gConsoleOutput.printf(" -checkOut                  Use P4 to check out output file if read only\n");
      gConsoleOutput.printf(" -excludeSubstring substr   Exclude files that contain substr (multiple OK)\n");
      gConsoleOutput.printf(" -exclude wildcard          Exclude files with wildcard (multiple OK)\n");
      gConsoleOutput.printf(" -password                  keyGen mode: Secret key phrase\n");
      gConsoleOutput.printf("                            create/view/verify/extract modes: Encryption password\n");
      gConsoleOutput.printf(" -listFile filename         File containing files to process\n");
      gConsoleOutput.printf(" -basePath pathname         Archive base directory (used to determine archive filenames\n");
      
      gConsoleOutput.printf("\nModes:\n");
      gConsoleOutput.printf(" -keyGen                    Generate digital signature public/secret key files\n");
      gConsoleOutput.printf("                            Must also specify: -password and -outfile\n");
      gConsoleOutput.printf(" -create                    Create archive. Must also specify -file, -outFile \n");
      gConsoleOutput.printf("                            and -secretKeyFile\n");
      gConsoleOutput.printf(" -view                      View archive. Must also specify -file and -publicKeyFile\n");                
      gConsoleOutput.printf(" -verify                    Verify archive. Must also specify -file -publicKeyFile\n");               
      gConsoleOutput.printf(" -extract                   Extract archive. Must also specify -file, -publicKeyFile\n");
      gConsoleOutput.printf("                            and -outPath\n");    
      gConsoleOutput.printf("\nTo specify a text file's contents for any parameter accepting a\n");
      gConsoleOutput.printf("string, proceed the filename with a '@' character. Example:\n");
      gConsoleOutput.printf("\"-password @1.txt\" will read the password from file \"1.txt\".\n");
      
      BConsoleAppHelper::printHelp();
   }
}; // class BCmdLineParams

class BECFArc
{
   typedef BDynamicArray<BString> BStringArray;
   
   BCmdLineParams mCmdLineParams;

   uint mNumFilesProcessed;
   uint mNumFilesSkipped;
   uint mNumFailedFiles;

   BString mWorkDirectory;
  
   class BFilePath
   {
   public:
      BFilePath() { }

      BFilePath(const BString& basePathname, const BString& relPathname, const BString& filename) :
         mBasePathname(basePathname),
         mRelPathname(relPathname),
         mFilename(filename)
      {
      }         

      BFilePath(const BFileDesc& fileDesc) :
         mBasePathname(fileDesc.basePathname()),
         mRelPathname(fileDesc.relPathname()),
         mFilename(fileDesc.filename())
      {
      }         

      BString& basePathname(void) { return mBasePathname; }
      BString& relPathname(void) { return mRelPathname; }
      BString& filename(void) { return mFilename; }

      const BString& basePathname(void) const { return mBasePathname; }
      const BString& relPathname(void) const { return mRelPathname; }
      const BString& filename(void) const { return mFilename; }

      BString fullFilename(void) const
      {
         BString filename(mBasePathname);
         strPathAddBackSlash(filename, true);
         filename += mRelPathname;
         strPathAddBackSlash(filename, true);
         filename += mFilename;
         return filename;
      }

   private:
      BString mBasePathname;
      BString mRelPathname;
      BString mFilename;
   };
   
   enum BProcessFileStatus
   {
      cPFSFailed = -1,
      cPFSSucceeded = 0,
      cPFSSkipped = 1,
   };
   
   bool processExcludeList(void)
   {
      for (uint i = 0; i < mCmdLineParams.mExcludeStrings.getSize(); i++)
         mCmdLineParams.mExcludeStrings[i].toLower();
         
      for (uint i = 0; i < mCmdLineParams.mExcludeList.getSize(); i++)
      {
         BString excludeSpec(mCmdLineParams.mExcludeList[i]);
         
         excludeSpec.standardizePath();
         
         if ((excludeSpec.findLeft('*') == -1) && (excludeSpec.findLeft('?') == -1))
         {
            DWORD fileAttributes = GetFileAttributes(excludeSpec);

            if ((fileAttributes != INVALID_FILE_ATTRIBUTES) && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
               excludeSpec += "\\*";
         }               
         
         if ((excludeSpec.findLeft('\\') != -1) || (excludeSpec.findLeft(':') != -1) || (excludeSpec.findLeft("..") != -1))
         {
            BString absoluteExcludeStr;
            if (!strPathMakeAbsolute(excludeSpec, absoluteExcludeStr))
            {
               gConsoleOutput.error("Failed converting exclude string to absolute path: %s\n", mCmdLineParams.mExcludeList[i].getPtr());
               return false;
            }
            
            absoluteExcludeStr.standardizePath();
            
            excludeSpec = absoluteExcludeStr;
         }
         
         if ((excludeSpec.findLeft('*') == -1) && (excludeSpec.findLeft('?') == -1))
         {
            DWORD fileAttributes = GetFileAttributes(excludeSpec);

            if ((fileAttributes != INVALID_FILE_ATTRIBUTES) && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
               excludeSpec += "\\*";
         }               
         
         mCmdLineParams.mExcludeList[i] = excludeSpec;
      }
      
      return true;
   }

   bool processParams(BCommandLineParser::BStringArray& args)
   {
      if (args.getSize() < 2)
      {
         mCmdLineParams.printHelp();
         return false;
      }

      if (!mCmdLineParams.parse(args))
         return false;

      mCmdLineParams.mExcludeList.pushBack(TEMP_FILENAME);
      mCmdLineParams.mExcludeList.pushBack("*.secretkey");
      
      if (!processExcludeList())
         return false;

      return true;
   }

   bool findFiles(BDynamicArray<BFilePath>& sourceFiles)
   {
      sourceFiles.clear();

      int findFilesFlag = BFindFiles::FIND_FILES_WANT_FILES;
      if (mCmdLineParams.mDeepFlag)
         findFilesFlag |= BFindFiles::FIND_FILES_RECURSE_SUBDIRS;

      for (uint fileIndex = 0; fileIndex < mCmdLineParams.mFileStrings.getSize(); fileIndex++)
      {         
         const BString& fileString = mCmdLineParams.mFileStrings[fileIndex];

         TCHAR fullpath[_MAX_PATH];
         LPTSTR pFilePart = NULL;

         DWORD result = GetFullPathName(fileString.getPtr(), _MAX_PATH, fullpath, &pFilePart);
         if (0 == result)            
         {
            gConsoleOutput.error("Can't resolve path: %s\n", fileString);
            return false;
         }

         BString path;
         BString filename;
         path.set(fullpath, pFilePart - fullpath);
         filename.set(pFilePart);

         const DWORD fullAttributes = GetFileAttributes(fullpath);

         if ((fullAttributes != INVALID_FILE_ATTRIBUTES) && (fullAttributes & FILE_ATTRIBUTE_DIRECTORY))
         {
            // If the full path points to a directory then change the filename part to be a wildcard.
            path.set(fullpath);
            filename.set(B("*.*"));
         }
         else
         {
            // Remove backslash from end of path
            const BCHAR_T lastChar = path.getPtr()[path.length() - 1];
            if ((lastChar == B('\\')) || (lastChar == B('/')))
            {
               path.set(path.getPtr(), path.length() - 1);
            }
         }

         BFindFiles findFiles(path, filename, findFilesFlag);

         if (!findFiles.success())
         {
            gConsoleOutput.error("Can't find files: %s\n", fileString);
            return false;
         }
         else if (!findFiles.numFiles())
         {
            if ((filename.findLeft('*') != -1) || (filename.findLeft('?') != -1))
            {
               gConsoleOutput.warning("No files found matching wildcard specification: %s\n", fileString.getPtr());
            }
            else
            {
               gConsoleOutput.error("File not found: %s\n", fileString.getPtr());
               return false;
            }
         }

         for (uint fileIndex = 0; fileIndex < findFiles.numFiles(); fileIndex++)
            sourceFiles.pushBack(BFilePath(findFiles.getFile(fileIndex)));
      }
            
      return true;
   }
      
   bool isExcludedFile(const BString& filename)
   {
      uint i;
      for (i = 0; i < mCmdLineParams.mExcludeStrings.getSize(); i++)
         if (strstr(filename.getPtr(), mCmdLineParams.mExcludeStrings[i].getPtr()) != NULL)
            return true;

      BString baseFilename;
      strPathGetFilename(filename, baseFilename);
                  
      for (i = 0; i < mCmdLineParams.mExcludeList.getSize(); i++)
      {
         if (mCmdLineParams.mExcludeList[i].findLeft('\\') != -1)
         {
            if (wildcmp(mCmdLineParams.mExcludeList[i].getPtr(), filename.getPtr()))
               return true;
         }
         else
         {
            if (wildcmp(mCmdLineParams.mExcludeList[i].getPtr(), baseFilename.getPtr()))
               return true;
         }               
      }
      
      return false;
   }
   
   bool readFileList(const BString& listingFilename, BStringArray& srcFilenames)
   {
      FILE* pFile = NULL;
      if (fopen_s(&pFile, listingFilename, "r"))
      {
         gConsoleOutput.error("Unable to open listing file: %s\n", listingFilename.getPtr());
         return false;
      }

      while (!feof(pFile))
      {
         char buf[1024];
         if (!fgets(buf, sizeof(buf), pFile))
            break;

         BString line(buf);
            
         line.trimLeft(" \t\n\r");
         line.trimRight(" \t\n\r");

         if (line.isEmpty())
            continue;

         line.standardizePath();

         BString path;            
         strPathMakeAbsolute(line, path);
         
         path.standardizePath();

         if (isExcludedFile(path))
         {
            gConsoleOutput.printf("Skipping excluded file: %s\n", path.getPtr());
            mNumFilesSkipped++;
            continue;
         }

         srcFilenames.pushBack(path);
      }

      fclose(pFile);
      
      return true;
   }

   bool generateFileList(BStringArray& srcFilenames)
   {
      srcFilenames.clear();
               
      if (!mCmdLineParams.mListFilename.isEmpty())
      {
         if (!readFileList(mCmdLineParams.mListFilename, srcFilenames))
            return false;
      }

      BDynamicArray<BFilePath> sourceFiles;
      if (!findFiles(sourceFiles))
         return false;

      for (uint fileIndex = 0; fileIndex < sourceFiles.size(); fileIndex++)
      {
         const BFilePath& srcFilePath = sourceFiles[fileIndex];

         BString srcFullFilename(srcFilePath.fullFilename());

         srcFullFilename.standardizePath();
         
         if (isExcludedFile(srcFullFilename))
         {
            gConsoleOutput.printf("Skipping excluded file: %s\n", srcFullFilename.getPtr());
            mNumFilesSkipped++;
            continue;
         }
        
         srcFilenames.pushBack(srcFullFilename);
      }
      
      if (mCmdLineParams.mRemoveDups)
      {
         typedef BHashMap<BString> BStringHashMap;
         BStringHashMap stringHashMap;
         
         BStringArray newSrcFilenames;
         
         for (uint i = 0; i < srcFilenames.getSize(); i++)
         {
            const BString& filename = srcFilenames[i];
            BStringHashMap::InsertResult result = stringHashMap.insert(filename);
            
            if (result.second)
               newSrcFilenames.pushBack(filename);
            else
               gConsoleOutput.printf("Removing duplicate file: %s\n", filename.getPtr());
         }
         
         newSrcFilenames.swap(srcFilenames);
      }

      return true;
   }

   void clear(void)
   {
      mWorkDirectory.empty();
      
      mNumFilesProcessed = 0;
      mNumFilesSkipped = 0;
      mNumFailedFiles = 0;
   }
   
   bool processKeyGen(void)
   {
      // -keyGen -secretKeyPhrase "blah" -outfile EnsembleKey
      
      if (mCmdLineParams.mPassword.isEmpty())
      {
         gConsoleOutput.error("Must specify digital signature secret key pass phrase (-password)!\n");
         return false;
      }
      
      if (mCmdLineParams.mOutputFilename.isEmpty())
      {
         gConsoleOutput.error("Must specify output filename (-outFile)!\n");
         return false;
      }
                  
      BDigitalSignature digitalSignature;
      BSHA1 publicKey;
      BDynamicStream secretKeyData;
      
      if (!digitalSignature.createKeys(publicKey, secretKeyData, mCmdLineParams.mPassword, 1024))
      {
         gConsoleOutput.error("BDigitalSignature::createKeys failed! Key phrase was probably too short.\n");
         return false;
      }
      
      BString outputFilename;
      outputFilename = mCmdLineParams.mOutputFilename;
      outputFilename += ".inc";
      
      FILE* pFile;
      if (fopen_s(&pFile, outputFilename, "w"))
      {
         gConsoleOutput.printf("Unable to open file: %s\n", outputFilename.getPtr());
         return false;
      }
      
      fputs("// Digital signature public key generated by ECFARC - DO NOT HAND EDIT\n", pFile);
      fputs("unsigned char PUBLIC_KEY_NAME[] = {\n", pFile);
            
      for (uint i = 0; i < publicKey.size(); i++)
         fprintf(pFile, "0x%02X%c", publicKey[i], (i < (publicKey.size() - 1)) ? ',' : ' ');
         
      fprintf(pFile, "\n};\n");
      
      if (EOF == fclose(pFile))
      {
         gConsoleOutput.error("Unable to write to file: %s\n", outputFilename.getPtr());
         return false;
      }      
      pFile = NULL;
      
      gConsoleOutput.printf("Wrote digital signature public key file: %s\n", outputFilename.getPtr());
      
      BECFFileBuilder ecfFileBuilder;
      ecfFileBuilder.setID((uint)cSignatureSecretKeyECFHeaderID);
      
      ecfFileBuilder.addChunk(0, secretKeyData.getBuf());
      
      outputFilename = mCmdLineParams.mOutputFilename;
      outputFilename += ".secretKey";
      
      BWin32FileStream outputStream;
      if (!outputStream.open(outputFilename, cSFWritable | cSFSeekable))
      {
         gConsoleOutput.printf("Unable to create file: %s\n", outputFilename.getPtr());  
         return false;
      }
      
      if (!ecfFileBuilder.writeToStream(outputStream))
      {  
         gConsoleOutput.printf("Unable to write to file: %s\n", outputFilename.getPtr());
         return false;
      }
      
      if (!outputStream.close())
      {
         gConsoleOutput.printf("Unable to write to file: %s\n", outputFilename.getPtr());
         return false;  
      }
      
      gConsoleOutput.printf("Wrote digital signature secret key file: %s\n", outputFilename.getPtr());
                  
      return true;     
   }
   
   bool loadSecretKey(const char* pFilename, BDynamicStream& secretKeyStream)
   {
      BByteArray secretKeyData;
      if (!BWin32FileUtils::readFileData(pFilename, secretKeyData))
      {
         gConsoleOutput.error("loadSecretKey: Unable to read digital signature secret key file: %s\n", pFilename);
         return false;
      }

      BECFFileReader ecfReader;
      ecfReader.setDataBuffer(BConstDataBuffer(secretKeyData.getPtr(), secretKeyData.getSize()));
      if ((!ecfReader.check()) || (ecfReader.getID() != cSignatureSecretKeyECFHeaderID) || (!ecfReader.getNumChunks()))
      {
         gConsoleOutput.error("loadSecretKey: Digital signature secret file is invalid: %s\n", pFilename);
         return false;
      }  

      uint len = 0;
      const void* p = ecfReader.getChunkDataByIndex(0, len);
      if (!p)
      {
         gConsoleOutput.error("loadSecretKey: Digital signature secret key file is invalid: %s\n", pFilename);
         return false;  
      }
      
      if (secretKeyStream.writeBytes(p, len) != len)
      {
         gConsoleOutput.error("loadSecretKey: Failed creating secret key stream!\n");
         return false;
      }
      
      secretKeyStream.seek(0);
      
      return true;
   }      
   
   bool loadPublicKey(const char* pFilename, BSHA1& publicKey)
   {
      BCFileStream fileStream;
      if (!fileStream.open(pFilename))
      {
         gConsoleOutput.error("Failed opening public key file: %s\n", pFilename);
         return false;
      }
      
      uint lineNumber = 0;
      BString sigLine;
      for ( ; ; )
      {
         BString str;
         
         if (!fileStream.readLine(str))
         {
            if (fileStream.errorStatus())
            {
               gConsoleOutput.error("Failed reading from public key file: %s\n", pFilename);
               return false;
            }
            
            break;
         }
         
         if (lineNumber > 4)
         {
            gConsoleOutput.error("Public key file has too many lines: %s\n", pFilename);
            return false;
         }
                  
         if (lineNumber == 0)
         {
            if (strstr(str, "public key") == NULL)
            {
               gConsoleOutput.error("Not a valid public key file: %s\n", pFilename);
               return false;
            }
         }
         else if (lineNumber == 2)
            sigLine = str;
         
         lineNumber++;
      }
      
      sigLine.trimLeft("\n\r\t ");
      sigLine.trimRight("\n\r\t ");
      
      if ((sigLine.isEmpty()) || (sigLine.findLeft("0x") != 0))
      {  
         gConsoleOutput.error("Not a valid public key file: %s\n", pFilename);
         return false;
      }
                  
      int buf[20];
      for (uint i = 0; i < 20; i++)
         buf[i] = -1;
      
      int status = sscanf_s(
         sigLine.getPtr(),
         "0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X", 
         &buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5], &buf[6], &buf[7],
         &buf[8], &buf[9], &buf[10], &buf[11], &buf[12], &buf[13], &buf[14], &buf[15],
         &buf[16], &buf[17], &buf[18], &buf[19]);
      
      if (status != 20)
      {
         gConsoleOutput.error("Not a valid public key file: %s\n", pFilename);
         return false;
      }         
      
      for (uint i = 0; i < 20; i++)
      {
         if ((buf[i] < 0) || (buf[i] > 255))
         {
            gConsoleOutput.error("Not a valid public key file: %s\n", pFilename);
            return false;
         }
         
         publicKey[i] = (uchar)buf[i];
      }
                  
      return true;
   }
   
   bool removeMissingFiles(BStringArray& filenames)
   {
      uint numMissingFiles = 0;
      
      for (uint i = 0; i < filenames.getSize(); i++)
      {
         const BString& srcFilename = filenames[i];
         
         WIN32_FILE_ATTRIBUTE_DATA srcFileAttribs;

         if (!GetFileAttributesEx(srcFilename, GetFileExInfoStandard, &srcFileAttribs))
         {
            gConsoleOutput.warning("Unable to get attributes of file: %s\n", srcFilename.getPtr());
            numMissingFiles++;
            
            filenames.removeIndex(i, true);
            i--;
         }
      }
      
      if (numMissingFiles)
         gConsoleOutput.warning("%u missing files\n", numMissingFiles);
         
      return true;
   }
   
   struct BFileTypeStats
   {
      BFileTypeStats() : mTotalFiles(0), mTotalBytes(0), mLargestFile(0) { }
      
      uint     mTotalFiles;
      uint64   mTotalBytes;
      uint64   mLargestFile;
   };
   
   void printFileStats(const BStringArray& srcFiles)
   {
      typedef BHashMap<BString, BFileTypeStats> BStringMap;
      BStringMap extMap;
      
      for (uint i = 0; i < srcFiles.getSize(); i++)
      {
         BString ext;
         strPathGetExtension(srcFiles[i], ext);
         ext.toLower();
         
         BStringMap::InsertResult result = extMap.insert(ext, BFileTypeStats());
         BFileTypeStats& stats = result.first->second;
         
         stats.mTotalFiles++;
         
         WIN32_FILE_ATTRIBUTE_DATA data;
         if (GetFileAttributesEx(srcFiles[i], GetFileExInfoStandard, &data))
         {
            uint64 fileSize = Utils::CreateUInt64(data.nFileSizeLow, data.nFileSizeHigh);
            stats.mTotalBytes += fileSize;
            stats.mLargestFile = Math::Max(stats.mLargestFile, fileSize);
         }
         else
            gConsoleOutput.warning("Unable to find file: %s\n", srcFiles[i].getPtr());
      }
      
      for (BStringMap::const_iterator it = extMap.begin(); it != extMap.end(); ++it)
      {
         const BString& ext = it->first;
         const BFileTypeStats& stats = it->second;
         
         gConsoleOutput.printf("Extension: %s, Total Files: %u, Total bytes: %I64u, Largest file: %I64u, Ave. file size: %f\n", 
            ext.getPtr(), stats.mTotalFiles, stats.mTotalBytes, stats.mLargestFile, (double)stats.mTotalBytes / stats.mTotalFiles);
      }
   }
   
   bool processCreate(void)
   {
      if ((mCmdLineParams.mFileStrings.isEmpty()) && (mCmdLineParams.mListFilename.isEmpty()))
      {
         gConsoleOutput.error("No files specified to process!\n");
         return false;  
      }
      
      if (mCmdLineParams.mSecretKeyFilename.isEmpty())
      {
         gConsoleOutput.error("Must specify digital signature secret key filename (-secretKeyFile)!\n");
         return false;
      }     
      
      BDynamicStream secretKeyStream;
      if (!loadSecretKey(mCmdLineParams.mSecretKeyFilename, secretKeyStream))
         return false;
      
      if (mCmdLineParams.mOutputFilename.isEmpty())
      {
         gConsoleOutput.error("Must specify output archive filename (-outFile)!\n");
         return false;
      }
      
      BStringArray srcFilenames;
      if (!generateFileList(srcFilenames))
         return false;
      
      if (mCmdLineParams.mIgnoreMissingFiles)
      {
         if (!removeMissingFiles(srcFilenames))
            return false;  
      }
         
      if (srcFilenames.empty())
      {
         gConsoleOutput.error("No files found to process!\n");
         return false;  
      }
         
      if (!BConsoleAppHelper::checkOutputFileAttribs(mCmdLineParams.mOutputFilename, mCmdLineParams.mCheckOut))
         return false;
      
      BECFArchiver ecfArchiver;
      
      bool status = ecfArchiver.build(mCmdLineParams.mBasePath.isEmpty() ? mWorkDirectory : mCmdLineParams.mBasePath, mCmdLineParams.mOutputFilename, TEMP_FILENAME, srcFilenames, secretKeyStream, 2, mCmdLineParams.mPassword.length() ? &mCmdLineParams.mPassword : NULL);
      if (!status)
      {
         gConsoleOutput.error("Archive creation failed!\n");
         return false;
      }
      
      mNumFilesProcessed += srcFilenames.getSize();
      
      printFileStats(srcFilenames);
                  
      return true;
   }
   
   bool processVerify(BECFArchiver::eVerifyMode verifyMode)
   {
      if (verifyMode == BECFArchiver::cExtract)
      {
         if (mCmdLineParams.mOutputPath.isEmpty())
         {
            gConsoleOutput.error("Must specify output path (-outPath)!\n");
            return false;
         }
      }
      
      if (mCmdLineParams.mFileStrings.isEmpty())
      {
         gConsoleOutput.error("No archive files specified to process!\n");
         return false;  
      }
      
      BStringArray srcFilenames;
      if (!generateFileList(srcFilenames))
         return false;
         
      if (srcFilenames.empty())
      {
         gConsoleOutput.error("No archive files found!\n");
         return false;  
      }
                                 
      if (mCmdLineParams.mPublicKeyFilenames.isEmpty())
      {
         gConsoleOutput.error("Must specify at least one digital signature public key filename (-publicKeyFile)!\n");
         return false;
      }     
            
      BDynamicArray<BSHA1> signaturePublicKeyArray(mCmdLineParams.mPublicKeyFilenames.getSize());
      for (uint i = 0; i < mCmdLineParams.mPublicKeyFilenames.getSize(); i++)
      {
         if (!loadPublicKey(mCmdLineParams.mPublicKeyFilenames[i], signaturePublicKeyArray[i]))
            return false;
      }  
      
      uint numSuccessfulFiles = 0;
      uint numFailedFiles = 0;  
      uint numSkippedFiles = 0;
    
      for (uint fileIndex = 0; fileIndex < srcFilenames.getSize(); fileIndex++)
      {
         const BString& srcFilename = srcFilenames[fileIndex];
         
         gConsoleOutput.printf("Processing archive: %s\n", srcFilename.getPtr());
         
         BECFArchiver ecfArchiver;
      
         BECFArchiver::BVerifyParams verifyParams;
         verifyParams.mpSrcFilename = srcFilename.getPtr();
         verifyParams.mpDstPath = mCmdLineParams.mOutputPath.getPtr();
         verifyParams.mpSignaturePublicKeyArray = &signaturePublicKeyArray;
         verifyParams.mpEncryptionPassword = mCmdLineParams.mPassword.length() ? &mCmdLineParams.mPassword : NULL;
         verifyParams.mMode = verifyMode;
         verifyParams.mIgnoreChunkProcessingErrors = mCmdLineParams.mIgnoreErrors;
         verifyParams.mCheckOut = mCmdLineParams.mCheckOut;
         verifyParams.mNoOverwrite = mCmdLineParams.mNoOverwrite;
                  
         bool status = ecfArchiver.verify(verifyParams, numSuccessfulFiles, numFailedFiles, numSkippedFiles);
         
         if (!status)
         {
            gConsoleOutput.error("Failed verifying archive: %s\n", srcFilename.getPtr());
            mNumFailedFiles++;
            
            if (!mCmdLineParams.mIgnoreErrors)
               return false;
         }
         else
         {
            if (numFailedFiles)
               gConsoleOutput.printf("File processed with %u failed files.\n", numFailedFiles);
            else
               gConsoleOutput.printf("File successfully processed with no errors.\n");
            
            mNumFilesProcessed++;
         }
         
         gConsoleOutput.printf("\n");
      }
      
      gConsoleOutput.printf("Total archive files process: %u, failed: %u, skipped: %u\n", numSuccessfulFiles, numFailedFiles, numSkippedFiles);
                
      return true;
   }

   bool findWorkDirectory(void)
   {
      char buf[MAX_PATH];
      if (!GetModuleFileNameA(GetModuleHandle(NULL), buf, sizeof(buf)))
         return false;

      mWorkDirectory = buf;
      mWorkDirectory.toLower();

      int i = mWorkDirectory.findRight("\\tools\\ecfarc");
      if (i == -1)
         i = mWorkDirectory.findRight("\\tools\\ecfarc");
      if (i == -1)
         return false;

      mWorkDirectory.crop(0, i);

      return true;
   }

public:
   BECFArc()
   {
      clear();
   }

   bool process(BCommandLineParser::BStringArray& args)
   {
      clear();

      if (!findWorkDirectory())
      {
         gConsoleOutput.error("Unable to find work directory!\n");
         return false;
      }

      if (!processParams(args))
         return false;

      if (cModeInvalid == mCmdLineParams.mProcessingMode)
      {
         gConsoleOutput.error("Must specify processing mode (-keyGen, -create, etc.)!\n");
         return false;  
      }

      bool status = false;
      switch (mCmdLineParams.mProcessingMode)
      {
         case cModeKeyGen:    status = processKeyGen(); break;
         case cModeCreate:    status = processCreate(); break;
         case cModeVerify:    status = processVerify(BECFArchiver::cFull); break;
         case cModeView:      status = processVerify(BECFArchiver::cQuick); break;
         case cModeExtract:   status = processVerify(BECFArchiver::cExtract); break;
      }
      
      gConsoleOutput.printf("Files processed successfully: %i, skipped: %i, failed: %i\n", mNumFilesProcessed, mNumFilesSkipped, mNumFailedFiles);
      gConsoleOutput.printf("Total errors: %u, Total warnings: %u\n", BConsoleAppHelper::getTotalErrorMessages(), BConsoleAppHelper::getTotalWarningMessages());

      return status;    
   }
};

static int mainInternal(int argC, const char** argV)
{
   XCoreCreate();

   BConsoleAppHelper::setup();

   BCommandLineParser::BStringArray args;
   if (!BConsoleAppHelper::init(args, argC, argV))
   {
      BConsoleAppHelper::deinit();

      XCoreRelease();
      return EXIT_FAILURE;
   }

   gConsoleOutput.printf(PROGRAM_TITLE " Compiled %s %s\n", __DATE__, __TIME__);
   
   BECFArc ecfArc;
   const bool success = ecfArc.process(args);

   BConsoleAppHelper::deinit();

   XCoreRelease();
   return success ? EXIT_SUCCESS : EXIT_FAILURE;  
}

int main(int argC, const char* argV[])
{
   int status;

#ifndef BUILD_DEBUG   
   __try
#endif   
   {
      status = mainInternal(argC, argV);
   }
#ifndef BUILD_DEBUG   
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      fprintf(stderr, PROGRAM_TITLE ": Unhandled exception!");
      return 1;
   }
#endif   

   return status;
}