//------------------------------------------------------------------------------------------------------------------------
//
//  File: gr2ugx.cpp
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#include "xcore.h"
#include "xcoreLib.h"

//#include <d3d9.h>
//#include <d3dx9.h>

// xcore
#include "consoleOutput.h"
#include "file\win32FindFiles.h"
#include "file\win32FileUtils.h"
#include "utils\commandLineParser.h"
#include "utils\consoleAppHelper.h"
#include "granny\win32\granny.h"
#include "stream\cfileStream.h"
#include "stream\dynamicStream.h"
#include "utils\endianSwitch.h"
#include "resource\resourceTag.h"
#include "uaxDefs.h"

// xgeom
#include "grannyToUnigeom.h"
#include "ugxInstancer.h"
#include "staticGeomBuilder.h"

// local
#include "grannyDataEndianSwapper.h"

#include <conio.h>

#define TOOL_NAME "gr2ugx"
#define TEXTURE_EXTENSION ".ddx"

#define SPC_TEXTURE_PREFIX "spc_"      

const uint cAssetTagCreatorToolVersion = 1;

//-------------------------------------------------

class BCmdLineParams
{
public:
   BString mFileString;
   BString mOutputPathString;
   BString mOutputFilenameString;
   BString mAppendString;
   bool mOutSameDirFlag;
   bool mTimestampFlag;
   bool mDeepFlag;
   bool mRecreateFlag;
   bool mNoOverwrite;
   bool mIgnoreErrors;

   bool mSimulateFlag;
   bool mStatsFlag;

   bool mFileStats;
   bool mConversionDetails;
   
   bool mCheckOut;
   bool mDisableTagChunk;
   bool mDeltaMode;

   bool mSkipDepFile;
      
   BCmdLineParams() :
      mOutSameDirFlag(false),
      mTimestampFlag(false),
      mDeepFlag(false),
      mRecreateFlag(false),
      mFileString(""),
      mNoOverwrite(false),
      mIgnoreErrors(false),
      mSimulateFlag(false),
      mStatsFlag(false),
      mFileStats(false),
      mConversionDetails(false),
      mCheckOut(false),
      mDisableTagChunk(false),
      mDeltaMode(false),
      mSkipDepFile(false)
   {
   }

   bool parse(BCommandLineParser::BStringArray& args)
   {
      const BCLParam clParams[] =
      {
         {"file",          cCLParamTypeBStringPtr, &mFileString },
         {"outpath",       cCLParamTypeBStringPtr, &mOutputPathString },
         {"outfile",       cCLParamTypeBStringPtr, &mOutputFilenameString },
         {"append",        cCLParamTypeBStringPtr, &mAppendString },
         {"outsamedir",    cCLParamTypeFlag, &mOutSameDirFlag },
         {"nooverwrite",   cCLParamTypeFlag, &mNoOverwrite },
         {"timestamp",     cCLParamTypeFlag, &mTimestampFlag },
         {"deep",          cCLParamTypeFlag, &mDeepFlag },
         {"recreate",      cCLParamTypeFlag, &mRecreateFlag },
         {"ignoreerrors",  cCLParamTypeFlag, &mIgnoreErrors },
         {"simulate",      cCLParamTypeFlag, &mSimulateFlag },
         {"stats",         cCLParamTypeFlag, &mStatsFlag },
         {"filestats",     cCLParamTypeFlag, &mFileStats },
         {"details",       cCLParamTypeFlag, &mConversionDetails },
         {"checkout",      cCLParamTypeFlag, &mCheckOut },
         {"noTags",        cCLParamTypeFlag, &mDisableTagChunk },
         {"delta",         cCLParamTypeFlag, &mDeltaMode },
         {"skipdepfile",   cCLParamTypeFlag, &mSkipDepFile },
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
      gConsoleOutput.printf(" -outpath path     Specify output path\n");
      gConsoleOutput.printf(" -outfile filename Specify output filename\n");
      gConsoleOutput.printf(" -append string    Append string to filename\n");
      gConsoleOutput.printf(" -outsamedir       Write output files to source path\n");
      
      gConsoleOutput.printf(" -timestamp        Compare timestamps and skip files that are not outdated\n");
      gConsoleOutput.printf(" -deep             Recurse subdirectories\n");
      gConsoleOutput.printf(" -recreate         Recreate directory structure\n");
      
      gConsoleOutput.printf(" -simulate         Only print filenames of files to be processed\n");
      gConsoleOutput.printf(" -nooverwrite      Don't overwrite existing files\n");
      gConsoleOutput.printf(" -ignoreerrors     Don't stop on failed files\n");
      gConsoleOutput.printf(" -stats            Display statistics\n");
      gConsoleOutput.printf(" -details          Print UGX conversion information to log file\n");
      gConsoleOutput.printf(" -checkout         Use P4 to check out output file if read only\n");
      gConsoleOutput.printf(" -noTags           Don't include asset tag chunk in output file\n");
      gConsoleOutput.printf(" -delta            Only process changed files (requires asset tags)\n");
      gConsoleOutput.printf(" -skipdepfile      Don't create or update the dep file\n");
      
      BConsoleAppHelper::printHelp();
   }
}; // class BCmdLineParams

class BGR2UGXConv
{
   BCmdLineParams mCmdLineParams;

   uint mNumFilesProcessed;
   uint mNumFilesSkipped;
   uint mNumFailedFiles;
   uint mNumNonGR2Files;

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
   
   bool addResourceTagChunk(BECFFileBuilder& ecfFileBuilder, const char* pSrcFilename, const char* pDstFilename)
   {
      pDstFilename;
      
      if (mCmdLineParams.mDisableTagChunk)
         return true;
      
      BResourceTagBuilder resourceTagBuilder;

      resourceTagBuilder.setPlatformID(BResourceTagHeader::cPIDXbox);

      BString commandLine;
      commandLine.set("GR2UGX -file %s");
      
      resourceTagBuilder.setCreatorToolInfo(commandLine, cAssetTagCreatorToolVersion);

      resourceTagBuilder.setSourceFilename(pSrcFilename);
      if (!resourceTagBuilder.setSourceDigestAndTimeStamp(pSrcFilename))
      {
         gConsoleOutput.error("Failed computing source file digest: %s\n", pSrcFilename);
         return false;
      }

      if (!resourceTagBuilder.finalize())
      {
         gConsoleOutput.error("Failed computing resource tag for file: %s\n", pSrcFilename);
         return false;
      }

      const BByteArray& tagData = resourceTagBuilder.getFinalizedData();

      BECFChunkData& resourceChunk = ecfFileBuilder.addChunk((uint64)cResourceTagECFChunkID, tagData);
      resourceChunk.setResourceBitFlag(cECFChunkResFlagIsResourceTag, true);
      
      return true;
   }

   bool createGRXChunk(granny_file_info* pFileInfo, BECFFileBuilder& ecfBuilder)
   {
      pFileInfo->ArtToolInfo = NULL;
      //pFileInfo->ArtToolInfo->ExtendedData.Object = NULL;
      //pFileInfo->ArtToolInfo->ExtendedData.Type = NULL;

      pFileInfo->ExporterInfo = NULL;
      pFileInfo->FromFileName = TOOL_NAME;
      pFileInfo->TextureCount = 0;
      pFileInfo->Textures = NULL;
      pFileInfo->MaterialCount = 0;
      pFileInfo->Materials = 0;
      pFileInfo->VertexDataCount = 0;
      pFileInfo->VertexDataCount = NULL;
      pFileInfo->TriTopologyCount = 0;
      pFileInfo->TriTopologies = NULL;
      pFileInfo->TrackGroupCount = 0;
      pFileInfo->TrackGroups = NULL;
      pFileInfo->AnimationCount = 0;
      pFileInfo->Animations = NULL;
      pFileInfo->ExtendedData.Type = NULL;
      pFileInfo->ExtendedData.Object = NULL;

      // Look for track masks
      bool foundTrackMasks = false;
      for(granny_int32x Skel = 0; Skel < pFileInfo->SkeletonCount; ++Skel)
      {
         granny_skeleton* Skeleton = pFileInfo->Skeletons[Skel];

         for(granny_int32x Bone = 0; Bone < Skeleton->BoneCount; ++Bone)
         {
            if (Skeleton->Bones[Bone].ExtendedData.Type && Skeleton->Bones[Bone].ExtendedData.Object)
            {
               const BSimString trackMaskIdentifier("TrackMask");

               // Scan through all extended data
               int extendedDataIndex = 0;
               while (1)
               {
                  // Look for the last extended data field
                  if (Skeleton->Bones[Bone].ExtendedData.Type[extendedDataIndex].Type == GrannyEndMember)
                     break;

                  const BSimString extendedDataName(Skeleton->Bones[Bone].ExtendedData.Type[extendedDataIndex].Name);

                  // Look for a track mask
                  if (extendedDataName.contains(trackMaskIdentifier))
                  {
                     foundTrackMasks = true;
                     break;
                  }

                  extendedDataIndex++;
               }
            }

            if (foundTrackMasks)
               break;
         }

         if (foundTrackMasks)
            break;
      }   

      // Remove bone extended data if no track masks were found
      if (!foundTrackMasks)
      {
         for(granny_int32x Skel = 0; Skel < pFileInfo->SkeletonCount; ++Skel)
         {
            granny_skeleton* Skeleton = pFileInfo->Skeletons[Skel];

            for(granny_int32x Bone = 0; Bone < Skeleton->BoneCount; ++Bone)
            {
               Skeleton->Bones[Bone].ExtendedData.Type = 0;
               Skeleton->Bones[Bone].ExtendedData.Object = 0;
            }
         }   
      }

/*      
      // Remove bone extended data
      for(granny_int32x Skel = 0; Skel < pFileInfo->SkeletonCount; ++Skel)
      {
         granny_skeleton* Skeleton = pFileInfo->Skeletons[Skel];
         
         for(granny_int32x Bone = 0; Bone < Skeleton->BoneCount; ++Bone)
         {
            if (Skeleton->Bones[Bone].ExtendedData.Type && Skeleton->Bones[Bone].ExtendedData.Object)
            {
               const BSimString trackMaskIdentifier("TrackMask");

               int goodExtendedDataIndex = 0;
               int extendedDataIndex = 0;

               // Scan through all extended data and decided which we're going to keep
               while (1)
               {
                  // Look for the last extended data field
                  if (Skeleton->Bones[Bone].ExtendedData.Type[extendedDataIndex].Type == GrannyEndMember)
                  {
                     Skeleton->Bones[Bone].ExtendedData.Type[goodExtendedDataIndex] = Skeleton->Bones[Bone].ExtendedData.Type[extendedDataIndex];
                     Skeleton->Bones[Bone].ExtendedData.Object[goodExtendedDataIndex] = Skeleton->Bones[Bone].ExtendedData.Object[extendedDataIndex];
                     break;
                  }

                  const BSimString extendedDataName(Skeleton->Bones[Bone].ExtendedData.Type[extendedDataIndex].Name);
                  
                  // Look for a track mask
                  if (extendedDataName.contains(trackMaskIdentifier))
                  {
                     if (extendedDataIndex != goodExtendedDataIndex)
                     {
                        Skeleton->Bones[Bone].ExtendedData.Type[goodExtendedDataIndex] = Skeleton->Bones[Bone].ExtendedData.Type[extendedDataIndex];
                        Skeleton->Bones[Bone].ExtendedData.Object[goodExtendedDataIndex] = Skeleton->Bones[Bone].ExtendedData.Object[extendedDataIndex];
                     }
                     goodExtendedDataIndex++;
                  }

                  extendedDataIndex++;
               }
            }

            // SLB: Old code that threw away useful extended data
            //Skeleton->Bones[Bone].ExtendedData.Type = 0;
            //Skeleton->Bones[Bone].ExtendedData.Object = 0;
         }
      }   
*/      
      for (int i = 0; i < pFileInfo->MeshCount; i++)
      {
         granny_mesh* pMesh = pFileInfo->Meshes[i];
         
         pMesh->PrimaryVertexData = NULL;
         pMesh->MorphTargetCount = 0;
         pMesh->MorphTargets = NULL;
         pMesh->PrimaryTopology = NULL;
         pMesh->MaterialBindingCount = 0;
         pMesh->MaterialBindings = NULL;
         pMesh->ExtendedData.Type = 0;
         pMesh->ExtendedData.Object = 0;
         
         for (int j = 0; j < pMesh->BoneBindingCount; j++)
         {
            granny_bone_binding* pBinding = &pMesh->BoneBindings[j];
            
            pBinding->TriangleCount = 0;
            pBinding->TriangleIndices = NULL;
         }
      }
      
      BDEBUG_ASSERT((pFileInfo->ModelCount >= 1) && pFileInfo->Models);
      pFileInfo->ModelCount = 1;
                        
      granny_file_writer* pFileWriter = GrannyCreateMemoryFileWriter(TOOL_NAME, 0, 16*1024*1024);
      if (!pFileWriter)
         return false;
      
      granny_grn_file_magic_value magicValue;
      bool success = GrannyGetMagicValueForPlatform(32, true, &magicValue);
      success;
      BDEBUG_ASSERT(success);
         
      granny_file_builder* pBuilder = GrannyBeginFileInMemory(
         1,
         GrannyCurrentGRNStandardTag,
         &magicValue, //GrannyGRNFileMV_32Bit_BigEndian,
         65536);
      if (!pBuilder)         
      {
         GrannyDeleteFileWriter(pFileWriter);
         return false;
      }

      granny_file_data_tree_writer* pWriter = GrannyBeginFileDataTreeWriting(GrannyFileInfoType, pFileInfo, 0, 0);
      
      GrannySetFileSectionFormat(pBuilder, 0, GrannyNoCompression, 16);

      GrannyWriteDataTreeToFileBuilder(pWriter, pBuilder);
      GrannyEndFileDataTreeWriting(pWriter);
      
      if(!GrannyEndFileRawToWriter(pBuilder, pFileWriter))
      {
         GrannyDeleteFileWriter(pFileWriter);
         return false;
      }    
                  
      granny_uint8* pBuf = NULL;
      granny_int32x bufSize = 0;
      if (!GrannyStealMemoryWriterBuffer(pFileWriter, &pBuf, &bufSize))
      {
         GrannyDeleteFileWriter(pFileWriter);
         return false;
      }
      
      if ((!pBuf) || (!bufSize))
      {
         GrannyDeleteFileWriter(pFileWriter);
         return false;
      }
      
      BByteArray newBuf;
      newBuf.pushBack((BYTE*)pBuf, bufSize);
      
      BGrannyDataEndianSwapper endianSwapper(pBuf, newBuf.getPtr(), bufSize, GrannyFileInfoType);
      
      BECFChunkData& grxChunk = ecfBuilder.addChunk(BUGXGeom::cECFGRXChunkID, newBuf.getPtr(), bufSize);
      grxChunk.setAlignmentLog2(4);
                        
      GrannyFreeMemoryWriterBuffer(pBuf);
      GrannyDeleteFileWriter(pFileWriter);
            
      return true;
   }
   
   typedef BHashMap<BString, bool> BDepFileHashMap;
   
   void addDepFilename(const BString& name, bool isOptional, BDepFileHashMap& uniqueTextures)
   {
      uniqueTextures.insert(name, isOptional);
      
      int multiframeStringIndex = name.findLeft(UGX_MATERIAL_MULTIFRAME_SUBSTRING);
      if (multiframeStringIndex >= 0)
      {
         for (uint i = 2; i <= Unigeom::BMaterial::cMaxMultiframeTextures; i++)
         {
            BString filename(name);

            const uint frameIndex = i;
            filename.setChar(multiframeStringIndex + UGX_MATERIAL_MULTIFRAME_INDEX_OFS, (frameIndex / 10) + '0');            
            filename.setChar(multiframeStringIndex + UGX_MATERIAL_MULTIFRAME_INDEX_OFS + 1, (frameIndex % 10) + '0');    

            uniqueTextures.insert(filename, true);
         }
      }
   }      
   
   bool createDepFile(const char* pDstFilename, const Unigeom::Geom& geom)
   {
      BDepFileHashMap uniqueTextures;
      
      for (int matIndex = 0; matIndex < geom.numMaterials(); matIndex++)
      {
         const Unigeom::BMaterial& material = geom.material(matIndex);
         
         for (int mapType = 0; mapType < Unigeom::BMaterial::cNumMapTypes; mapType++)
         {  
            for (int mapIndex = 0; mapIndex < material.getNumMaps((Unigeom::BMaterial::eMapType)mapType); mapIndex++)
            {
               const Unigeom::BMap& map = material.getMap((Unigeom::BMaterial::eMapType)mapType, mapIndex);
                              
               BString name(map.getName().getPtr());
               if (name.isEmpty())
                  continue;
                  
               name.standardizePath();
               
               if (name.getChar(0) == '\\')
                  name = "art" + name;
               else
                  name = "art\\" + name;
               
               BString extension;
               if (!strPathGetExtension(name, extension))
                  strPathAddExtension(name, TEXTURE_EXTENSION);
               
               addDepFilename(name, false, uniqueTextures);               
               
               if ((Unigeom::BMaterial::cDiffuse == mapType) || (Unigeom::BMaterial::cNormal == mapType) || (Unigeom::BMaterial::cGloss == mapType))
               {
                  BString path;
                  BString filename;
                  strPathGetDirectory(name, path, true);
                  strPathGetFilename(name, filename);

                  BString spcFilename;
                  spcFilename.format("%s%s%s", path.getPtr(), SPC_TEXTURE_PREFIX, filename.getPtr());
                  
                  addDepFilename(spcFilename, true, uniqueTextures);
               }
            }
         }
      }
      
      BString depFilename(pDstFilename);
      strPathRemoveExtension(depFilename);
      strPathAddExtension(depFilename, ".dep");
      
      if (!BConsoleAppHelper::checkOutputFileAttribs(depFilename, mCmdLineParams.mCheckOut))
         return false;
         
      BCFileStream depFile;
      if (!depFile.open(depFilename, cSFWritable))
      {
         gConsoleOutput.error("Unable to open file: %s\n", depFilename.getPtr());
         return false;  
      }
      
      depFile.printf("<?xml version=\"1.0\"?>\n");
      depFile.printf("<UGXDependencies>\n");
      depFile.printf("  <FileDependencies>\n");
      
      for (BDepFileHashMap::const_iterator it = uniqueTextures.begin(); it != uniqueTextures.end(); ++it)
      {
         const BString& name = it->first;
         const bool isOptional = it->second;
         
         depFile.printf("    <File isOptional=\"%s\">%s</File>\n", isOptional ? "true" : "false", name.getPtr());
      }
      
      depFile.printf("  </FileDependencies>\n");
      depFile.printf("</UGXDependencies>\n");
   
      if (!depFile.close())
      {
         gConsoleOutput.error("Unable to write to file: %s\n", depFilename.getPtr());
         return false;  
      }
      
      return true;
   }
   
   BConvertFileStatus convertMeshFile(const char* pSrcFilename, const char* pDstFilename, granny_file_info* pFileInfo)
   {
      if ((!pFileInfo->ModelCount) || (!pFileInfo->MeshCount))
      {
         gConsoleOutput.printf("Skipping Granny file with no models or meshes: \"%s\"\n", pSrcFilename);
         return cCFSSkipped;
      }
      
      convertCoordinateSystem(pFileInfo, true);

      BFILETextDispatcher textDispatcher;
      if (mCmdLineParams.mConversionDetails)
         textDispatcher.open(BConsoleAppHelper::getLogFile());
      
      BGrannyToUnigeom grannyToUnigeom(pFileInfo, true, true, &textDispatcher);
      if (!grannyToUnigeom.getSuccess())
      {
         gConsoleOutput.error("Granny to UGF conversion failed processing file: \"%s\"\n%s\n", pSrcFilename, grannyToUnigeom.getErrorDesc().getPtr());
         return cCFSFailed;
      }
      
      gConsoleOutput.printf("Total triangles: %u\n", grannyToUnigeom.getGeom().numTris());
      gConsoleOutput.printf("Total unique materials: %u\n", grannyToUnigeom.getGeom().numMaterials());
      gConsoleOutput.printf("Total bones: %u\n", grannyToUnigeom.getGeom().numBones());
      
      const Unigeom::Geom& inputGeom = grannyToUnigeom.getGeom();

      if (!mCmdLineParams.mSkipDepFile)
      {
         if (!createDepFile(pDstFilename, inputGeom))
            return cCFSFailed;
      }
      
      const Unigeom::Geom* pSrcGeom = &inputGeom;
      typedef std::auto_ptr<BStaticGeomBuilder> BStaticGeomBuilderAutoPtr;
      BStaticGeomBuilderAutoPtr pStaticGeomBuilder(NULL);
      
      bool supportInstancing = true;
      if (inputGeom.getModelType() == Unigeom::Geom::cMTLarge)
      {
         supportInstancing = false;
         
         const bool fast = false;

         gConsoleOutput.printf("Auto-splitting large model\n");
         
         pStaticGeomBuilder = BStaticGeomBuilderAutoPtr(new BStaticGeomBuilder(fast, inputGeom));
         if (!pStaticGeomBuilder->getSucceeded())
         {
            gConsoleOutput.error("BStaticGeomBuilder failed splitting large model!\n%s\n", pStaticGeomBuilder->getError().getPtr());
            return cCFSFailed;
         }
         
         pSrcGeom = &pStaticGeomBuilder->getChunkedGeom();
      }
      
      // Hardcoded to always create big endian UGX files.
      const bool bigEndian = true;
      const bool optimizeTexCoords = false;
      const bool diffuseVertexColors = false;
      BUGXGeomInstancer ugxGeomInstancer(bigEndian, *pSrcGeom, &textDispatcher, diffuseVertexColors, optimizeTexCoords, supportInstancing);

      if (!ugxGeomInstancer.getSuccess())
      {
         gConsoleOutput.error("UGF to UGX data converter failed processing file: \"%s\"\n%s\n", pSrcFilename, ugxGeomInstancer.getErrorDesc().getPtr());
         return cCFSFailed;
      }
      
      if (pStaticGeomBuilder.get())
      {
         ugxGeomInstancer.getUGXCachedData().header().setLargeGeomBoneIndex(pStaticGeomBuilder->getRigidBoneIndex());
      }
      
      //if (mCmdLineParams.mConversionDetails)
      //   ugxGeomInstancer.logData(textDispatcher);
                        
      BECFFileBuilder ecfBuilder;
      
      if (!createGRXChunk(pFileInfo, ecfBuilder))
      {
         gConsoleOutput.error("Unable to create GRX chunk: \"%s\"\n", pDstFilename);
         return cCFSFailed;
      }
      
      if (!ugxGeomInstancer.packData(ecfBuilder))
      {
         gConsoleOutput.error("UGX data builder failed processing file: \"%s\"\n", pSrcFilename);
         return cCFSFailed;
      }
      
      if (pStaticGeomBuilder.get())
      {
         const BAABBTree& tree = pStaticGeomBuilder->getAABBTree();
         
         BDynamicStream stream;
         stream << tree;
         
         ecfBuilder.addChunk((uint64)BUGXGeom::cECFTreeChunkID, (const BYTE*)stream.ptr(), (uint)stream.size());
      }
      
      if (!addResourceTagChunk(ecfBuilder, pSrcFilename, pDstFilename))
         return cCFSFailed;
                  
      const bool succeeded = BWin32FileUtils::writeFileData(pDstFilename, ecfBuilder, NULL, true);
      if (succeeded)
         gConsoleOutput.printf("Successfully wrote file: \"%s\"\n", pDstFilename);
      else
         gConsoleOutput.error("Failed writing to file: \"%s\"\n", pDstFilename);
      
      return succeeded ? cCFSSucceeded : cCFSFailed;
   }
   
   BConvertFileStatus convertAnimFile(const char* pSrcFilename, const char* pDstFilename, granny_file_info* pFileInfo)
   {
      convertCoordinateSystem(pFileInfo, true);
      
      pFileInfo->ArtToolInfo = NULL;
      //pFileInfo->ArtToolInfo->ExtendedData.Object = NULL;
      //pFileInfo->ArtToolInfo->ExtendedData.Type = NULL;
      
      pFileInfo->ExporterInfo = NULL;
      pFileInfo->FromFileName = TOOL_NAME;
      pFileInfo->TextureCount = 0;
      pFileInfo->Textures = NULL;
      pFileInfo->MaterialCount = 0;
      pFileInfo->Materials = 0;
      pFileInfo->VertexDataCount = 0;
      pFileInfo->VertexDataCount = NULL;
      pFileInfo->TriTopologyCount = 0;
      pFileInfo->TriTopologies = NULL;
      pFileInfo->ExtendedData.Type = NULL;
      pFileInfo->ExtendedData.Object = NULL;

      // Remove bone extended data
      for(granny_int32x Skel = 0; Skel < pFileInfo->SkeletonCount; ++Skel)
      {
         granny_skeleton* Skeleton = pFileInfo->Skeletons[Skel];

         for(granny_int32x Bone = 0; Bone < Skeleton->BoneCount; ++Bone)
         {
            Skeleton->Bones[Bone].ExtendedData.Type = 0;
            Skeleton->Bones[Bone].ExtendedData.Object = 0;
         }
      }   

      for (int i = 0; i < pFileInfo->MeshCount; i++)
      {
         granny_mesh* pMesh = pFileInfo->Meshes[i];

         pMesh->PrimaryVertexData = NULL;
         pMesh->MorphTargetCount = 0;
         pMesh->MorphTargets = NULL;
         pMesh->PrimaryTopology = NULL;
         pMesh->MaterialBindingCount = 0;
         pMesh->MaterialBindings = NULL;
         pMesh->ExtendedData.Type = 0;
         pMesh->ExtendedData.Object = 0;

         for (int j = 0; j < pMesh->BoneBindingCount; j++)
         {
            granny_bone_binding* pBinding = &pMesh->BoneBindings[j];

            pBinding->TriangleCount = 0;
            pBinding->TriangleIndices = NULL;
         }
      }

      if (pFileInfo->ModelCount > 0)
         pFileInfo->ModelCount = 1;

      granny_file_writer* pFileWriter = GrannyCreateMemoryFileWriter(TOOL_NAME, 0, 16*1024*1024);
      if (!pFileWriter)
      {
         gConsoleOutput.error("GrannyCreateMemoryFileWriter failed: \"%s\"\n", pSrcFilename);
         return cCFSFailed;
      }

      granny_grn_file_magic_value magicValue;
      bool success = GrannyGetMagicValueForPlatform(32, true, &magicValue);
      success;
      BDEBUG_ASSERT(success);

      granny_file_builder* pBuilder = GrannyBeginFileInMemory(
         1,
         GrannyCurrentGRNStandardTag,
         &magicValue, //GrannyGRNFileMV_32Bit_BigEndian,
         65536);
      if (!pBuilder)         
      {
         gConsoleOutput.error("GrannyBeginFileInMemory failed: \"%s\"\n", pSrcFilename);
         GrannyDeleteFileWriter(pFileWriter);
         return cCFSFailed;
      }

      granny_file_data_tree_writer* pWriter = GrannyBeginFileDataTreeWriting(GrannyFileInfoType, pFileInfo, 0, 0);

      GrannySetFileSectionFormat(pBuilder, 0, GrannyNoCompression, 16);

      GrannyWriteDataTreeToFileBuilder(pWriter, pBuilder);
      GrannyEndFileDataTreeWriting(pWriter);

      if(!GrannyEndFileRawToWriter(pBuilder, pFileWriter))
      {
         gConsoleOutput.error("GrannyEndFileRawToWriter failed: \"%s\"\n", pSrcFilename);
         GrannyDeleteFileWriter(pFileWriter);
         return cCFSFailed;
      }    

      granny_uint8* pBuf = NULL;
      granny_int32x bufSize = 0;
      if (!GrannyStealMemoryWriterBuffer(pFileWriter, &pBuf, &bufSize))
      {
         gConsoleOutput.error("GrannyStealMemoryWriterBuffer failed: \"%s\"\n", pSrcFilename);
         GrannyDeleteFileWriter(pFileWriter);
         return cCFSFailed;
      }

      if ((!pBuf) || (!bufSize))
      {
         gConsoleOutput.error("GrannyStealMemoryWriterBuffer failed: \"%s\"\n", pSrcFilename);
         GrannyDeleteFileWriter(pFileWriter);
         return cCFSFailed;
      }

      BByteArray newBuf;
      newBuf.pushBack((BYTE*)pBuf, bufSize);

      BGrannyDataEndianSwapper endianSwapper(pBuf, newBuf.getPtr(), bufSize, GrannyFileInfoType);

      GrannyFreeMemoryWriterBuffer(pBuf);
      GrannyDeleteFileWriter(pFileWriter);      
      
      BString uaxFilename(pDstFilename);
      strPathRemoveExtension(uaxFilename);
      strPathAddExtension(uaxFilename, UAX_ANIM_EXTENSION);
      
      if (!BConsoleAppHelper::checkOutputFileAttribs(uaxFilename, mCmdLineParams.mCheckOut))
         return cCFSFailed;
         
      BECFFileBuilder ecfBuilder;
      ecfBuilder.setID((uint)BUAX::cECFFileID);
      
      BECFChunkData& uaxChunk = ecfBuilder.addChunk( (uint64)BUAX::cECFUAXChunkID, newBuf );
      uaxChunk.setAlignmentLog2(4);
      
      if (!addResourceTagChunk(ecfBuilder, pSrcFilename, uaxFilename))
         return cCFSFailed;
         
      if (!BWin32FileUtils::writeFileData(uaxFilename, ecfBuilder))
      {
         gConsoleOutput.printf("Failed writing file: \"%s\"\n", uaxFilename.getPtr());
         return cCFSFailed;
      }
      
      gConsoleOutput.printf("Successfully wrote file: \"%s\"\n", uaxFilename.getPtr());
      return cCFSSucceeded;
   }
      
   BConvertFileStatus convertFile(const char* pSrcFilename, const char* pDstFilename)
   {
      if (!BConsoleAppHelper::checkOutputFileAttribs(pDstFilename, mCmdLineParams.mCheckOut))
         return cCFSFailed;
         
      if (!GrannyFileCRCIsValid(pSrcFilename))
      {  
         gConsoleOutput.error("File CRC check failed: \"%s\"\n", pSrcFilename);
         return cCFSFailed;
      }
                  
      granny_file* pGrannyFile = GrannyReadEntireFile(pSrcFilename);
      if(!pGrannyFile)
      {
         gConsoleOutput.error("Unable to read file \"%s\"\n", pSrcFilename);
         return cCFSFailed;
      }

      granny_file_info* pFileInfo = GrannyGetFileInfo(pGrannyFile);
      if(!pFileInfo)
      {
         gConsoleOutput.error("Unable to read file \"%s\"\n", pSrcFilename);
                  
         GrannyFreeFile(pGrannyFile);
         return cCFSFailed;
      }
      
      BConvertFileStatus status;
      if (pFileInfo->AnimationCount)
      {
         gConsoleOutput.printf("Processing as an animation file.\n");
         status = convertAnimFile(pSrcFilename, pDstFilename, pFileInfo);
      }
      else
      {
         gConsoleOutput.printf("Processing as a mesh file.\n");
         status = convertMeshFile(pSrcFilename, pDstFilename, pFileInfo);
      }
      
      GrannyFreeFile(pGrannyFile);
      
      return status;
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
         
      if (mCmdLineParams.mFileString.isEmpty())
      {
         gConsoleOutput.error("No files specified to process!\n");
         return false;  
      }         
            
      return true;
   }

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

   bool processFiles(void)
   {
      if (!mSourceFiles.size())
      {
         gConsoleOutput.error("No files found to process.\n");
         return false;
      }

      if ((!mCmdLineParams.mOutputFilenameString.isEmpty()) && (mSourceFiles.size() > 1))
      {
         gConsoleOutput.warning("Warning: More than one file found -- ignoring output filename %s\n", mCmdLineParams.mOutputFilenameString.getPtr());
         gConsoleOutput.printf("\n");
      }

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
         
         if ((_stricmp(srcExt, ".gr2") != 0) && (_stricmp(srcExt, "gr2") != 0))
         {
            mNumNonGR2Files++;
            continue;
         }

         BString dstFullFilename;

         if ((!mCmdLineParams.mOutputFilenameString.isEmpty()) && (mSourceFiles.size() == 1))
         {
            dstFullFilename = mCmdLineParams.mOutputFilenameString;
            
         }
         else
         {
            dstFullFilename.set((mCmdLineParams.mOutSameDirFlag ? srcPath : mCmdLineParams.mOutputPathString));

            strPathAddBackSlash(dstFullFilename, false);

            if ((!mCmdLineParams.mOutSameDirFlag) && (mCmdLineParams.mRecreateFlag))
            {
               dstFullFilename += srcRelPathname;       

               strPathAddBackSlash(dstFullFilename, false);  
            }

            if ((!mCmdLineParams.mSimulateFlag) && (!mCmdLineParams.mOutSameDirFlag) && (mCmdLineParams.mRecreateFlag))
               strPathCreateFullPath(dstFullFilename);

            dstFullFilename += srcFName;
            dstFullFilename += B(".");
            dstFullFilename += UGX_MODEL_EXTENSION;
         }

         WIN32_FILE_ATTRIBUTE_DATA dstFileAttributes;
         const BOOL dstFileExists = GetFileAttributesEx(dstFullFilename, GetFileExInfoStandard, &dstFileAttributes);

         if (dstFileExists)
         {
            if (mCmdLineParams.mNoOverwrite)
            {
               gConsoleOutput.printf("Skipping already existing file: %s\n", dstFullFilename.getPtr());
               mNumFilesSkipped++;
               continue;
            }

            if (mCmdLineParams.mTimestampFlag)
            {
               WIN32_FILE_ATTRIBUTE_DATA srcFileAttributes;
               const BOOL srcFileExists = GetFileAttributesEx(srcFullFilename, GetFileExInfoStandard, &srcFileAttributes);

               if (srcFileExists)
               {
                  LONG timeComp = CompareFileTime(&srcFileAttributes.ftLastWriteTime, &dstFileAttributes.ftLastWriteTime);
                  if (timeComp <= 0)
                  {
                     gConsoleOutput.printf("Skipping up to date file: %s\n", dstFullFilename.getPtr());
                     mNumFilesSkipped++;
                     continue;
                  }
               }                  
            }
            
            if (mCmdLineParams.mDeltaMode)
            {
               if (BResourceTagUtils::fileIsUnchanged(srcFullFilename, dstFullFilename, &cAssetTagCreatorToolVersion))
               {
                  gConsoleOutput.printf("Skipping up to date file: %s\n", dstFullFilename.getPtr());
                  mNumFilesSkipped++;
                  continue;
               }
            }
         }
         else
         {
            // This sucks, but we don't know if the GR2 is an anim or mesh file yet.
            
            if (mCmdLineParams.mDeltaMode)
            {
               BString uaxFilename(dstFullFilename);
               strPathRemoveExtension(uaxFilename);
               strPathAddExtension(uaxFilename, UAX_ANIM_EXTENSION);
               const BOOL dstFileExists = GetFileAttributesEx(uaxFilename, GetFileExInfoStandard, &dstFileAttributes);
               if (dstFileExists)
               {
                  if (BResourceTagUtils::fileIsUnchanged(srcFullFilename, uaxFilename, &cAssetTagCreatorToolVersion))
                  {
                     gConsoleOutput.printf("Skipping up to date file: %s\n", uaxFilename.getPtr());
                     mNumFilesSkipped++;
                     continue;
                  }
               }               
            }               
         }

         gConsoleOutput.printf("Processing file %u of %u: %s\n", 1 + fileIndex, mSourceFiles.size(), srcFullFilename.getPtr());                                                      
         BConvertFileStatus success = convertFile(srcFullFilename, dstFullFilename);

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

      return true;
   }

   void clear(void)
   {
      mSourceFiles.clear();

      mNumFilesProcessed = 0;
      mNumFilesSkipped = 0;
      mNumFailedFiles = 0;
      mNumNonGR2Files = 0;

      Utils::ClearObj(mOverallFileStats);
   }
  
public:
   BGR2UGXConv() 
   {
      clear();
   }
      
   bool process(BCommandLineParser::BStringArray& args)
   {
      clear();

      if (!processParams(args))
         return false;

      if (!findFiles())
         return false;

      bool status = processFiles();

      gConsoleOutput.printf("Files processed successfully: %i, skipped: %i, failed: %i, non-GR2 files skipped: %i\n", mNumFilesProcessed, mNumFilesSkipped, mNumFailedFiles, mNumNonGR2Files);

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

//----------------------------------------------------------------

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

   BGR2UGXConv gr2ugxConv;

   DWORD startTime = GetTickCount();

   const bool success = gr2ugxConv.process(args);

   DWORD endTime = GetTickCount();

   gConsoleOutput.printf("Total time: %f\n", (endTime - startTime) * .001f);

   BConsoleAppHelper::deinit();

   XCoreRelease();

   return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
