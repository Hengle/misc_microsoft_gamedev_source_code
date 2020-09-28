//------------------------------------------------------------------------------------------------------------------------
//
//  File: tacticProcess.cpp
//
//  Copyright (c) 2008, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#include "xcore.h"
#include "xcoreLib.h"

// xcore
#include "consoleOutput.h"
#include "file\win32FindFiles.h"
#include "file\win32FileUtils.h"
#include "utils\commandLineParser.h"
#include "utils\consoleAppHelper.h"
#include "xmlreader.h"
#include "granny\win32\granny.h"
#include "math\matrix.h"
#include "math\quat.h"
#include "stream\cfileStream.h"

#define TOOL_NAME "tacticProcess"

#define PROCESSED_TACTIC_EXTENSION "tactic2"

//=====================================================================================================================
//=====================================================================================================================
class BCmdLineParams
{
public:
   BString mFileString;
   BString mOutputFilenameString;
   bool mDeepFlag;
   bool mNoOverwrite;
   bool mIgnoreErrors;
   bool mFileStats;
   bool mCheckOut;
      
   BCmdLineParams() :
      mDeepFlag(false),
      mFileString(""),
      mNoOverwrite(false),
      mIgnoreErrors(false),
      mFileStats(false),
      mCheckOut(false)
   {
   }

   bool parse(BCommandLineParser::BStringArray& args)
   {
      const BCLParam clParams[] =
      {
         {"file",          cCLParamTypeBStringPtr, &mFileString },
         {"outfile",       cCLParamTypeBStringPtr, &mOutputFilenameString },
         {"nooverwrite",   cCLParamTypeFlag, &mNoOverwrite },
         {"deep",          cCLParamTypeFlag, &mDeepFlag },
         {"ignoreerrors",  cCLParamTypeFlag, &mIgnoreErrors },
         {"filestats",     cCLParamTypeFlag, &mFileStats },
         {"checkout",      cCLParamTypeFlag, &mCheckOut },
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
      gConsoleOutput.printf("Usage: gr2ugx <options>\n");
      gConsoleOutput.printf("Options:\n");
      
      gConsoleOutput.printf(" -file filename    Specify source filename (wildcards okay)\n");
      gConsoleOutput.printf(" -outfile filename Specify output filename\n");
      
      gConsoleOutput.printf(" -deep             Recurse subdirectories\n");
      
      gConsoleOutput.printf(" -nooverwrite      Don't overwrite existing files\n");
      gConsoleOutput.printf(" -ignoreerrors     Don't stop on failed files\n");
      gConsoleOutput.printf(" -checkout         Use P4 to check out output file if read only\n");
      
      BConsoleAppHelper::printHelp();
   }
}; // class BCmdLineParams


//=====================================================================================================================
//=====================================================================================================================
class BTacticProcess
{
   BCmdLineParams mCmdLineParams;

   uint mNumFilesProcessed;
   uint mNumFilesSkipped;
   uint mNumFailedFiles;
   uint mNumNonTacticFiles;

   BXMLNode objectsRoot;
   BCFileStream mOutputFile;

   //=====================================================================================================================
   struct BFileStats 
   {
      uint64 mTotalSize;
      uint mNumFiles;

      void update(uint64 size)
      {
         mTotalSize += size;            
         mNumFiles++;
      }
   };

   BFileStats mOverallFileStats;

   //=====================================================================================================================
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

   BDynamicArray<BFilePath> mSourceFiles;
  
   enum BConvertFileStatus
   {
      cCFSFailed = -1,
      cCFSSucceeded = 0,
      cCFSSkipped = 1,
   };


   //=====================================================================================================================
   bool getAnimNode(const char* pVisFilename, const char* pAnimName, BXMLReader& reader, BXMLNode& node)
   {
      uint loadFlags = XML_READER_IGNORE_BINARY;

      BString filepath;
      filepath.set("..\\..\\art\\");
      filepath.append(pVisFilename);

      bool ok = reader.load(-1, filepath.asNative(), loadFlags);
      if (!ok || !reader.getRootNode())
      {
         gConsoleOutput.error("Failed loading vis file: %s\n", filepath.asNative());
         return false;
      }

      BXMLNode& root = reader.getRootNode();

      BXMLNode firstModelNode;
      if (!root.getChild("Model", &firstModelNode))
         return false;

      for (int i = 0; i < firstModelNode.getNumberChildren(); i++)
      {
         BXMLNode& child = firstModelNode.getChild(i);
         if (child.getName().compare(B("anim")) == 0)
         {
            BString typeName;
            if (child.getAttribValue("Type", &typeName))
            {
               if (typeName.compare(pAnimName) == 0)
               {
                  node = child;
                  return true;
               }
            }
         }
      }

      return false;
   }

   //=====================================================================================================================
   static void convertCoordinateSystem(granny_file_info* pGrannyFileInfo, bool flipWinding)      
   {
      #define POST_GRANNY_EXPORT_TOOL_NAME "ESPostExport"
      
      if (strcmp(pGrannyFileInfo->ArtToolInfo->FromArtToolName, POST_GRANNY_EXPORT_TOOL_NAME) == 0)
         return;

      // Transform from the art tool's coordinate system to the game's coordinate system.
      granny_real32 affine3[3];
      granny_real32 linear3x3[9];
      granny_real32 inverseLinear3x3[9];
      const BVec3 forward(0,0,1);
      const BVec3 right(-1,0,0);
      const BVec3 cOriginVector(0,0,0);
      const BVec3 cYAxisVector(0,1,0);

      GrannyComputeBasisConversion(pGrannyFileInfo, pGrannyFileInfo->ArtToolInfo->UnitsPerMeter / 64.0f, 
         (float*)&cOriginVector, (float*)&right, (float*)&cYAxisVector, (float*)&forward,
         affine3, linear3x3, inverseLinear3x3);

      GrannyTransformFile(pGrannyFileInfo, affine3, linear3x3, inverseLinear3x3, 1e-5f, 1e-5f, (flipWinding ? GrannyReorderTriangleIndices : 0) | GrannyRenormalizeNormals);
   }

   //=====================================================================================================================
   bool getInitialPlacementTransform(const char* pAnimFilename, granny_transform& transform)
   {
      if (!GrannyFileCRCIsValid(pAnimFilename))
      {  
         gConsoleOutput.error("File CRC check failed: \"%s\"\n", pAnimFilename);
         return false;
      }
                  
      granny_file* pGrannyFile = GrannyReadEntireFile(pAnimFilename);
      if(!pGrannyFile)
      {
         gConsoleOutput.error("Unable to read file \"%s\"\n", pAnimFilename);
         return false;
      }

      granny_file_info* pFileInfo = GrannyGetFileInfo(pGrannyFile);
      if(!pFileInfo)
      {
         gConsoleOutput.error("Unable to read file \"%s\"\n", pAnimFilename);
                  
         GrannyFreeFile(pGrannyFile);
         return false;
      }

      convertCoordinateSystem(pFileInfo, true);

      transform = pFileInfo->Animations[0]->TrackGroups[0]->InitialPlacement;
      return true;
   }

   //=====================================================================================================================
   bool processAnims(const char* pAttackerAnimFilename, const char* pTargetAnimFilename, BQuat& targetOffsetQuat, BVec4& targetOffsetPos)
   {
      // Get anim
      BString attackerAnimFilepath;
      attackerAnimFilepath.set("..\\..\\art\\");
      attackerAnimFilepath.append(pAttackerAnimFilename);
      attackerAnimFilepath.append(".gr2");

      granny_transform attackerAnimTransform;
      granny_transform targetAnimTransform;
      if (!getInitialPlacementTransform(attackerAnimFilepath.asNative(), attackerAnimTransform))
      {
         return false;
      }

      // Target anim optional
      if (!pTargetAnimFilename)
      {
         GrannyMakeIdentity(&targetAnimTransform);
      }
      else 
      {
         BString targetAnimFilepath;
         targetAnimFilepath.set("..\\..\\art\\");
         targetAnimFilepath.append(pTargetAnimFilename);
         targetAnimFilepath.append(".gr2");
         if (!getInitialPlacementTransform(targetAnimFilepath.asNative(), targetAnimTransform))
            return false;
      }

      BMatrix44 attackerAnimMatrix;
      GrannyBuildCompositeTransform4x4(&attackerAnimTransform, (granny_real32*)&attackerAnimMatrix);
      BMatrix44 attackerAnimMatrixInvert(attackerAnimMatrix);
      attackerAnimMatrixInvert.invert();

      BMatrix44 targetAnimMatrix;
      GrannyBuildCompositeTransform4x4(&targetAnimTransform, (granny_real32*)&targetAnimMatrix);
      BMatrix44 targetAnimMatrixInvert(targetAnimMatrix);
      targetAnimMatrixInvert.invert();

      // O = T * A^-1
      BMatrix44 targetOffsetMatrix;
      targetOffsetMatrix = targetAnimMatrix * attackerAnimMatrixInvert;

      D3DXQUATERNION quat;
      D3DXQuaternionRotationMatrix(&quat, (const D3DXMATRIX*)(targetOffsetMatrix.getPtr()));
      targetOffsetPos = targetOffsetMatrix.getRow(3);
      targetOffsetQuat = BVec4(quat.x, quat.y, quat.z, quat.w);

      return true;
   }

   //=====================================================================================================================
   bool processAnimNodes(BXMLNode& attackerAnimNode, BXMLNode& targetAnimNode)
   {
      /*
      <anim type="Fatality_MarkIMarine_01" exitAction="Freeze" tweenTime="5" tweenToAnimation="">
         <asset type="Anim">
            <file>unsc\infantry\mark1Suit_01\Fatality_MarkIMarine_01A</file>
            <weight>1</weight>
      */

      bool foundAnyTargetAnims = false;
      int targetChildIndex = 0;
      int attackerChildIndex = 0;
      BXMLNode targetAnimChild;
      int currentTargetAnimChildIndex = 0;
      bool useTargetAnim = true;
      if (!targetAnimNode.getValid())
         useTargetAnim = false;

      // Go through each anim in the attacker
      for (int i = 0; i < attackerAnimNode.getNumberChildren(); i++)
      {
         BString typeName;
         BXMLNode& assetChild = attackerAnimNode.getChild(i);
         if (assetChild.getName().compare(B("asset")) == 0)
         {
            if (assetChild.getAttribValue("Type", &typeName))
            {
               if (typeName.compare("anim") == 0)
               {
                  int weight = 1;
                  BXMLNode attackerWeightNode;
                  if (assetChild.getChild("weight", &attackerWeightNode))
                  {
                     attackerWeightNode.getTextAsInt(weight);
                  }

                  BXMLNode fileNode;
                  if (assetChild.getChild("file", &fileNode))
                  {
                     BString attackerAnimFilename;
                     fileNode.getText(attackerAnimFilename);

                     // If not using target anim, don't bother looking up the anim name
                     if (!useTargetAnim)
                     {
                        // Process anims
                        BQuat targetOffsetQuat;
                        BVec4 targetOffsetPos;
                        processAnims(attackerAnimFilename.asNative(), NULL, targetOffsetQuat, targetOffsetPos);

                        // Add asset to output file
                        mOutputFile.printf("            <asset attackerAssetIndex=\"%d\" targetAssetIndex=\"%d\" orientOffset=\"%f %f %f %f\" posOffset=\"%f %f %f\" weight=\"%d\"></asset>\n",
                           attackerChildIndex, targetChildIndex - 1,
                           targetOffsetQuat.mVec[0], targetOffsetQuat.mVec[1], targetOffsetQuat.mVec[2], targetOffsetQuat.mVec[3],
                           targetOffsetPos[0], targetOffsetPos[1], targetOffsetPos[2],
                           weight);
                     }
                     else
                     {
                        // Get next target anim asset node
                        bool animFound = false;

                        while ((targetChildIndex < targetAnimNode.getNumberChildren()) && !animFound)
                        {
                           BXMLNode& targetChild = targetAnimNode.getChild(targetChildIndex);
                           if (targetChild.getAttribValue("Type", &typeName) &&
                               (typeName.compare("anim") == 0))
                           {
                              targetAnimChild = targetChild;
                              currentTargetAnimChildIndex = targetChildIndex;
                              animFound = true;
                              foundAnyTargetAnims = true;
                           }

                           targetChildIndex++;
                        }
                        // Use most recently found target anim
                        if (foundAnyTargetAnims)
                        {
                           BXMLNode targetFileNode;
                           if (targetAnimChild.getChild("file", &targetFileNode))
                           {
                              BString targetAnimFilename;
                              targetFileNode.getText(targetAnimFilename);

                              // Process anims
                              BQuat targetOffsetQuat;
                              BVec4 targetOffsetPos;
                              processAnims(attackerAnimFilename.asNative(), targetAnimFilename.asNative(), targetOffsetQuat, targetOffsetPos);

                              // Add asset to output file
                              mOutputFile.printf("            <asset attackerAssetIndex=\"%d\" targetAssetIndex=\"%d\" orientOffset=\"%f %f %f %f\" posOffset=\"%f %f %f\" weight=\"%d\"></asset>\n",
                                 attackerChildIndex, currentTargetAnimChildIndex,
                                 targetOffsetQuat.mVec[0], targetOffsetQuat.mVec[1], targetOffsetQuat.mVec[2], targetOffsetQuat.mVec[3],
                                 targetOffsetPos[0], targetOffsetPos[1], targetOffsetPos[2],
                                 weight);
                           }
                        }
                     }
                     attackerChildIndex++;
                  }
               }
            }
         }
      }

      return true;
   }

   //=====================================================================================================================
   bool processTactic(BXMLNode& tacticRoot, BString& tacticName, BString& attackerObjectName, BString& attackerVisName)
   {
      bool tacticPrint = false;
      for (int i = 0; i < tacticRoot.getNumberChildren(); i++)
      {
         BXMLNode& actionChild = tacticRoot.getChild(i);
         if (actionChild.getName().compare(B("action")) == 0)
         {
            bool actionPrint = false;
            for (int j = 0; j < actionChild.getNumberChildren(); j++)
            {
               // Get name
               BXMLNode nameNode;
               BString actionName;
               if (actionChild.getChild("Name", &nameNode))
                  nameNode.getText(actionName);
               else
                  actionName.empty();

               // Go through fatalities
               BXMLNode& fatalityChild = actionChild.getChild(j);
               if (fatalityChild.getName().compare(B("fatality")) == 0)
               {
                  //
                  if (!tacticPrint)
                  {
                     mOutputFile.printf("   <Tactic attacker=\"%s\" tactic=\"%s\">\n", attackerObjectName.asNative(), tacticName.asNative());
                     tacticPrint = true;
                  }

                  // Fatality target, anims
                  BString targetName;
                  BString attackerAnimName;
                  BString targetAnimName;
                  fatalityChild.getText(targetName);
                  fatalityChild.getAttribValue("attacker", &attackerAnimName);
                  bool hasTargetAnim = fatalityChild.getAttribValue("target", &targetAnimName);

                  // Get target vis file reference from object
                  bool foundVis = false;
                  BString targetVisName;
                  targetVisName.empty();
                  for (int k = 0; k < objectsRoot.getNumberChildren(); k++)
                  {
                     BString objectName;
                     BXMLNode& objectChild = objectsRoot.getChild(k);
                     if (objectChild.getAttribValue("name", &objectName))
                     {
                        if (objectName.compare(targetName) == 0)
                        {
                           BXMLNode visNode;
                           if (objectChild.getChild("Visual", &visNode))
                           {
                              visNode.getText(targetVisName);
                              foundVis = true;
                              break;
                           }
                        }
                     }
                  }

                  if (!foundVis)
                     continue;

                  // Get fatality anim nodes for the attacker and target from their vis files
                  BXMLReader attackerVisReader;
                  BXMLNode attackerAnimNode;
                  if (!getAnimNode(attackerVisName.asNative(), attackerAnimName.asNative(), attackerVisReader, attackerAnimNode))
                     continue;
                  BXMLReader targetVisReader;
                  BXMLNode targetAnimNode;
                  if (hasTargetAnim)
                  {
                     if (!getAnimNode(targetVisName.asNative(), targetAnimName.asNative(), targetVisReader, targetAnimNode))
                        continue;
                  }

                  if (!actionPrint)
                  {
                     BString tempStr;
                     actionChild.getText(tempStr);
                     mOutputFile.printf("      <action>%s\n", actionName.asNative());
                     actionPrint = true;
                  }

                  mOutputFile.printf("         <target>%s\n", targetName.asNative());

                  if (!processAnimNodes(attackerAnimNode, targetAnimNode))
                     continue;

                  mOutputFile.printf("         </target>\n");
               }
            }

            if (actionPrint)
            {
               mOutputFile.printf("      </action>\n");
            }
         }
      }

      if (tacticPrint)
      {
         mOutputFile.printf("   </Tactic>\n");
         return true;
      }
      else
         return false;
   }

   //=====================================================================================================================
   BConvertFileStatus convertFile(const char* pSrcFilename)
   {
      BConvertFileStatus status;
      status = BTacticProcess::cCFSSucceeded;

      BXMLReader reader;
      uint loadFlags = XML_READER_IGNORE_BINARY;
      bool ok = reader.load(-1, pSrcFilename, loadFlags);
      if (!ok || !reader.getRootNode())
      {
         gConsoleOutput.error("XML parse failed for file: %s\n", pSrcFilename);
         return cCFSFailed;
      }
      BXMLNode& root = reader.getRootNode();

      BString tacticFilepath(pSrcFilename);
      BString tacticFilename;
      strPathGetFilename(tacticFilepath, tacticFilename);

      // Get list of attacker vis files using this tactic
      BDynamicArray<BString> attackerObjectNames;
      BDynamicArray<BString> attackerVisFiles;
      attackerObjectNames.clear();
      attackerVisFiles.clear();
      for (int k = 0; k < objectsRoot.getNumberChildren(); k++)
      {
         BString objectName;
         BXMLNode& objectChild = objectsRoot.getChild(k);
         BXMLNode tacticsNode;
         if (objectChild.getChild("Tactics", &tacticsNode))
         {
            BString tacticsName;
            tacticsNode.getText(tacticsName);
            if (tacticsName.compare(tacticFilename) == 0)
            {
               BXMLNode visNode;
               if (objectChild.getChild("Visual", &visNode))
               {
                  BString attackerVisName;
                  BString attackerObjectName;
                  visNode.getText(attackerVisName);
                  objectChild.getAttribValue("name", &attackerObjectName);
                  attackerVisFiles.add(attackerVisName);
                  attackerObjectNames.add(attackerObjectName);
               }
            }
         }
      }

      // For each attacker using this tactic, process it
      for (int attackerIndex = 0; attackerIndex < attackerVisFiles.getNumber(); attackerIndex++)
      {
         processTactic(root, tacticFilename, attackerObjectNames[attackerIndex], attackerVisFiles[attackerIndex]);
      }

      return status;
   }   

   //=====================================================================================================================
   bool processParams(BCommandLineParser::BStringArray& args)
   {
      /*
      if (args.getSize() < 2)
      {
         mCmdLineParams.printHelp();
         return false;
      }
      */

      if (!mCmdLineParams.parse(args))
         return false;
         
      if (mCmdLineParams.mFileString.isEmpty())
      {
         mCmdLineParams.mFileString.set("..\\..\\data\\tactics\\*.tactics");
      }         
            
      return true;
   }

   //=====================================================================================================================
   bool findFiles(void)
   {
      mSourceFiles.clear();

      int findFilesFlag = BFindFiles::FIND_FILES_WANT_FILES;
      if (mCmdLineParams.mDeepFlag)
         findFilesFlag |= BFindFiles::FIND_FILES_RECURSE_SUBDIRS;

      TCHAR fullpath[_MAX_PATH];
      LPTSTR pFilePart = NULL;

      DWORD result = GetFullPathName(mCmdLineParams.mFileString.getPtr(), _MAX_PATH, fullpath, &pFilePart);
      if (0 == result)            
      {
         gConsoleOutput.error("Can't resolve path: %s\n", mCmdLineParams.mFileString.getPtr());
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
         gConsoleOutput.error("Can't find files: %s\n", mCmdLineParams.mFileString.getPtr());
         return false;
      }

      for (uint fileIndex = 0; fileIndex < findFiles.numFiles(); fileIndex++)
         mSourceFiles.pushBack(BFilePath(findFiles.getFile(fileIndex)));

      return true;
   }

   //=====================================================================================================================
   bool processFiles(void)
   {
      if (!mSourceFiles.size())
      {
         gConsoleOutput.error("No files found to process.\n");
         return false;
      }

      // Load objects.xml
      uint loadFlags = XML_READER_IGNORE_BINARY;
      BXMLReader objectsReader;
      bool objectsOK = objectsReader.load(-1, "..\\..\\data\\objects.xml", loadFlags);
      if (!objectsOK || !objectsReader.getRootNode())
      {
         gConsoleOutput.error("Failed loading objects.xml");
         return false;
      }
      objectsRoot = objectsReader.getRootNode();

      // Get destination data filename
      BString dataFilename("..\\..\\data\\tactics\\fatalityData.xml");
      if (!mCmdLineParams.mOutputFilenameString.isEmpty())
         dataFilename = mCmdLineParams.mOutputFilenameString;

      // Check noOverwrite flag and file existence
      WIN32_FILE_ATTRIBUTE_DATA dstFileAttributes;
      const BOOL dstFileExists = GetFileAttributesEx(dataFilename.asNative(), GetFileExInfoStandard, &dstFileAttributes);
      if (dstFileExists)
      {
         if (mCmdLineParams.mNoOverwrite)
         {
            gConsoleOutput.printf("Skipping already existing file: %s\n", dataFilename.getPtr());
            mNumFilesSkipped++;
            return false;
         }
      }

      // Checkout destination file if needed and specified
      if (!BConsoleAppHelper::checkOutputFileAttribs(dataFilename.asNative(), mCmdLineParams.mCheckOut))
      {

         return false;
      }

      // Open dst file
      if (!mOutputFile.open(dataFilename, cSFWritable))
      {
         gConsoleOutput.error("Unable to open file: %s\n", dataFilename.getPtr());
         return false;  
      }
      mOutputFile.printf("<FatalityData>\n");
      

      // Process files
      for (uint fileIndex = 0; fileIndex < mSourceFiles.size(); fileIndex++)
      {
         const BFilePath& srcFilePath = mSourceFiles[fileIndex];
         BString srcRelPathname(srcFilePath.relPathname());
         BString srcFullFilename(srcFilePath.fullFilename());

         BString srcPath;
         BString srcFilename;
         strPathSplit(srcFullFilename, srcPath, srcFilename);

         char srcFName[_MAX_FNAME];
         char srcExt[_MAX_EXT];
         _splitpath_s(srcFilename.getPtr(), NULL, 0, NULL, 0, srcFName, sizeof(srcFName), srcExt, sizeof(srcExt));
         
         if ((_stricmp(srcExt, ".tactics") != 0) && (_stricmp(srcExt, "tactics") != 0))
         {
            mNumNonTacticFiles++;
            continue;
         }

         gConsoleOutput.printf("Processing file %u of %u: %s\n", 1 + fileIndex, mSourceFiles.size(), srcFullFilename.getPtr());                                                      
         BConvertFileStatus success = convertFile(srcFullFilename);

         if (success == cCFSFailed)
         {
            mNumFailedFiles++;

            gConsoleOutput.error("Failed processing file: %s\n", srcFullFilename.getPtr());

            if (!mCmdLineParams.mIgnoreErrors)
               return false;
         }
         else if (success == cCFSSkipped)
         {
            mNumFilesSkipped++;
         }
         else
         {
            mNumFilesProcessed++;
         }
      }

      // Finish and close destination file
      mOutputFile.printf("</FatalityData>\n");
      if (!mOutputFile.close())
      {
         gConsoleOutput.error("Unable to write to file: %s\n", dataFilename.getPtr());
         return false;  
      }

      return true;
   }

   //=====================================================================================================================
   void clear(void)
   {
      mSourceFiles.clear();

      mNumFilesProcessed = 0;
      mNumFilesSkipped = 0;
      mNumFailedFiles = 0;
      mNumNonTacticFiles = 0;

      Utils::ClearObj(mOverallFileStats);
   }
  
public:
   BTacticProcess() 
   {
      clear();
   }
      
   //=====================================================================================================================
   bool process(BCommandLineParser::BStringArray& args)
   {
      clear();

      if (!processParams(args))
         return false;

      if (!findFiles())
         return false;

      bool status = processFiles();

      gConsoleOutput.printf("Files processed successfully: %i, skipped: %i, failed: %i, non-tactic files skipped: %i\n", mNumFilesProcessed, mNumFilesSkipped, mNumFailedFiles, mNumNonTacticFiles);

      if (mCmdLineParams.mFileStats)
      {
         const BFileStats& stats = mOverallFileStats;

         gConsoleOutput.printf("Overall file stats:\n");
                         
         gConsoleOutput.printf("    Num files: %u\n", stats.mNumFiles);
         gConsoleOutput.printf("    Total size: %u\n", stats.mTotalSize);
      }
            
      gConsoleOutput.printf("Total errors: %u, Total warnings: %u\n", BConsoleAppHelper::getTotalErrorMessages(), BConsoleAppHelper::getTotalWarningMessages());
            
      return status;    
   }
};


//=====================================================================================================================
//=====================================================================================================================
void GrannyLogCallback(
   granny_log_message_type Type,
   granny_log_message_origin Origin,
   char const *Error,
   void *UserData)
{
   Type;
   Origin;
   Error;
   UserData;
   if (Error)
   {
      gConsoleOutput.warning("(Granny) %s\n", Error);
   }
}

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================
int main(int argc, const char *argv[])
{
   XCoreCreate();
         
   BConsoleAppHelper::setup();
         
   BCommandLineParser::BStringArray args;
   
   if (!BConsoleAppHelper::init(args, argc, argv))
   {  
      BConsoleAppHelper::deinit();
      
      XCoreRelease();
      
      return 100;
   }
   
   gConsoleOutput.printf(TOOL_NAME " Compiled %s %s\n", __DATE__, __TIME__);
      
   if (!GrannyVersionsMatch)
   {
      gConsoleOutput.error("The Granny DLL currently loaded doesn't match the .h file used during compilation\n");
      
      BConsoleAppHelper::deinit();
      
      XCoreRelease();

      return 100;
   }

   granny_log_callback Callback;
   Callback.Function = GrannyLogCallback;
   Callback.UserData = 0;
   GrannySetLogCallback(&Callback);

   BTacticProcess tacticProcess;

   DWORD startTime = GetTickCount();

   const bool success = tacticProcess.process(args);

   DWORD endTime = GetTickCount();

   gConsoleOutput.printf("Total time: %f\n", (endTime - startTime) * .001f);

   BConsoleAppHelper::deinit();

   XCoreRelease();

   return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
