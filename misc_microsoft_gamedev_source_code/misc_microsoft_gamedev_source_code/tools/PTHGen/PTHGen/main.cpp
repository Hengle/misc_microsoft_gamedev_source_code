//============================================================================
//
//  File: main.cpp
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================

//XCORE
#include "xcore.h"
#include "xcorelib.h"

#include "file\win32FileStream.h"
#include "resource/resourceTag.h"
#include "resource/ecfFileData.h"
#include "resource\ecfUtils.h"

#include "stream\dynamicStream.h"
#include "xml\xmxDataBuilder.h"
#include "xml\xmxDataDumper.h"
#include "utils\consoleAppHelper.h"
#include "consoleOutput.h"

#include "utils\commandLineParser.h"
#include "utils\consoleAppHelper.h"
#include "stream\cfileStream.h"
#include "utils\endianSwitch.h"
#include "resource\ecfUtils.h"
#include "resource\resourceTag.h"



//#include <windows.h>
//#include <stdio.h>
#include <fstream>
#include <vector>


//using namespace std;

#include "PathEnginePC/interface/i_pathengine.h"



//general
#include <conio.h>


#define PROGRAM_TITLE "GENPTH" 
enum 
{
   cPTH_ECFFileID          = 0x00817817,
   cPTHHeaderID            = 0x34F00817,
   cPTHVersion             = 0x00000001,
   cPTHGroundMeshID        = 0x00000001,
   cPTHPrecomputedShapeID  = 0x00000002,
   cPTHAirMeshID           = 0x00000003,
};

//--------------------------------------------------
class BCmdLineParams
{
public:
   BString mNavMeshFile;
   BString mPthFile;
   //BCommandLineParser::BStringArray mExcludeStrings;
   BString mOutputPathString;
   BString mAppendString;
   //BString mRulesFilename;

   bool mOutSameDirFlag;
   bool mTimestampFlag;
   bool mDeepFlag;
   bool mRecreateFlag;
   bool mNoOverwrite;
   bool mIgnoreErrors;

   bool mSimulateFlag;
   bool mStatsFlag;

   bool mFileStats;
   //bool mConversionDetails;

   bool mDisableNumerics;
   bool mPermitUnicode;
   bool mForceUnicode;
   bool mLittleEndian;
   bool mDumpPackedData;
   bool mDisableTagChunk;
   bool mDeltaMode;

   bool mCheckOut;

   BCmdLineParams() :
   mOutSameDirFlag(false),
      mTimestampFlag(false),
      mDeepFlag(false),
      mRecreateFlag(false),
      mNoOverwrite(false),
      mIgnoreErrors(false),
      mSimulateFlag(false),
      mStatsFlag(false),
      mFileStats(false),
      //      mConversionDetails(false),
      mCheckOut(false),
      mDisableNumerics(false),
      mPermitUnicode(true),
      mForceUnicode(false),
      mLittleEndian(false),
      mDumpPackedData(false),
      mDisableTagChunk(false),
      mDeltaMode(false)
   {
   }

   bool parse(BCommandLineParser::BStringArray& args)
   {
      const BCLParam clParams[] =
      {
         {"navmeshfile",           cCLParamTypeBStringPtr, &mNavMeshFile },
         {"pthfile",               cCLParamTypeBStringPtr, &mPthFile },
         //{"append",              cCLParamTypeBStringPtr, &mAppendString },
         //{"outsamedir",          cCLParamTypeFlag, &mOutSameDirFlag },
         //{"nooverwrite",         cCLParamTypeFlag, &mNoOverwrite },
         //{"timestamp",           cCLParamTypeFlag, &mTimestampFlag },
         //{"deep",                cCLParamTypeFlag, &mDeepFlag },
         //{"recreate",            cCLParamTypeFlag, &mRecreateFlag },
         //{"ignoreerrors",        cCLParamTypeFlag, &mIgnoreErrors },
         //{"simulate",            cCLParamTypeFlag, &mSimulateFlag },
         //{"stats",               cCLParamTypeFlag, &mStatsFlag },
         //{"filestats",           cCLParamTypeFlag, &mFileStats },
         ////{"details",             cCLParamTypeFlag, &mConversionDetails },
         //{"checkout",            cCLParamTypeFlag, &mCheckOut },
         //{"disableNumerics",     cCLParamTypeFlag, &mDisableNumerics },
         ////{"permitUnicode",       cCLParamTypeFlag, &mPermitUnicode },
         //{"forceUnicode",        cCLParamTypeFlag, &mForceUnicode },
         //{"littleEndian",        cCLParamTypeFlag, &mLittleEndian },
         ////{"rules",               cCLParamTypeBStringPtr, &mRulesFilename },
         //{"dump",                cCLParamTypeFlag, &mDumpPackedData },
         //{"exclude",             cCLParamTypeBStringArrayPtr, &mExcludeStrings },
         //{"noTags",              cCLParamTypeFlag, &mDisableTagChunk },
         //{"delta",               cCLParamTypeFlag, &mDeltaMode },
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
      gConsoleOutput.printf(" ");
      gConsoleOutput.printf("Usage: genPTH <options>\n");
      gConsoleOutput.printf("Options:\n");

      gConsoleOutput.printf(" -navmeshfile filename    Specify source navmesh file (*.NM)\n");
      gConsoleOutput.printf(" -pthfile path            Specify output PTH file (*.PTH)\n");
      //gConsoleOutput.printf(" -outfile filename Specify output filename\n");
      //gConsoleOutput.printf(" -append string    Append string to filename\n");
      //gConsoleOutput.printf(" -outsamedir       Write output files to source path\n");
      //gConsoleOutput.printf(" -timestamp        Compare timestamps and skip files that are not outdated\n");
      //gConsoleOutput.printf(" -deep             Recurse subdirectories\n");
      //gConsoleOutput.printf(" -recreate         Recreate directory structure\n");
      //gConsoleOutput.printf(" -simulate         Only print filenames of files to be processed\n");
      //gConsoleOutput.printf(" -nooverwrite      Don't overwrite existing files\n");
      //gConsoleOutput.printf(" -ignoreerrors     Don't stop on failed files\n");
      //gConsoleOutput.printf(" -stats            Display statistics\n");
      //gConsoleOutput.printf(" -details          Print conversion information to log file\n");
      //gConsoleOutput.printf(" -checkout         Use P4 to check out output file if read only\n");
      //gConsoleOutput.printf(" -disableNumeric   Disable XML attrib/element text field compression\n");
      //gConsoleOutput.printf(" -permitUnicode    Unicode attrib/element text fields\n");
      //gConsoleOutput.printf(" -forceUnicode     Force Unicode attrib/element text fields\n");
      //gConsoleOutput.printf(" -littleEndian     Pack output for little endian machines\n");
      //gConsoleOutput.printf(" -dump             Dump packed data to new XML file\n");
      //gConsoleOutput.printf(" -rules filename   Use XML rules file\n");
      //gConsoleOutput.printf(" -exclude substr   Exclude files that contain substr (multiple OK)\n");
      //gConsoleOutput.printf(" -noTags           Don't include asset tag chunk in output file\n");
      //gConsoleOutput.printf(" -delta            Only process changed files (uses XMB resource tags)\n");
      gConsoleOutput.printf(" ");
      gConsoleOutput.printf(" ");
      BConsoleAppHelper::printHelp();
   }
}; // class BCmdLineParams



BCmdLineParams mCmdLineParams;
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------


iPathEngine *gPathEngine = NULL;
HINSTANCE gPathEngineInstanceHandle = NULL;

typedef iPathEngine* (__stdcall* tGetInterfaceFunction)(iErrorHandler*);

class pathEngineErrorHandler : public iErrorHandler
{
public:
   ~pathEngineErrorHandler() {};

   eAction handle(const char *type, const char *description, const char *const* attributes)
   {
      char buffer[500];
      if(attributes)
         sprintf(buffer, "%s : %s : %s ", type, description,attributes[0]);
      else
         sprintf(buffer, "%s : %s ", type, description);
      gConsoleOutput.printf("----PATHENGINE ERROR----");
      gConsoleOutput.printf(buffer);
      return BREAK;
   }
};

iPathEngine* LoadPathEngine( iErrorHandler* handler)
{
   char buffer[500];
   DWORD errorValue;

   gPathEngineInstanceHandle = LoadLibraryA("pathengine.dll");
   if(!gPathEngineInstanceHandle)
   {
      errorValue = GetLastError();
      gConsoleOutput.printf("Error: failed calling LoadLibrary() for pathengine.dll. Ensure the file exists in the same dir");
      return 0;
   }
   FARPROC procAddress;
   SetLastError(0);
   procAddress = GetProcAddress(gPathEngineInstanceHandle, (LPCSTR)1);
   if(!procAddress)
   {
      errorValue = GetLastError();
       gConsoleOutput.printf("Error: Failed to obtain PathEngine entrypoint in pathengine.dll");
      return 0;
   }

   tGetInterfaceFunction getInterfaceFunction = (tGetInterfaceFunction)procAddress;
   return getInterfaceFunction(handler);
}

void closePathEngine()
{
   FreeLibrary(gPathEngineInstanceHandle);
   gPathEngineInstanceHandle=NULL;
   gPathEngine=NULL;
}


//-----------------------------------------
//-----------------------------------------
//-----------------------------------------
//-----------------------------------------
//-----------------------------------------
class vertex
{
public:
   int X;
   int Y;
};
class triangle
{
public:
   int faceIndex[3];

};

class MeshWrapper : public iFaceVertexMesh
{
public:
   vertex *mVerts;
   triangle *mTris;

   int mNumVerts;
   int mNumTris;
   int mNumInds;

   MeshWrapper():
   mVerts(0),
      mTris(0),
      mNumVerts(0),
      mNumTris(0)
   {
   }

   ~MeshWrapper()
   {
      if(mVerts)
      {
         delete []mVerts;
         mVerts=NULL;
      }
      if(mTris)
      {
         delete []mTris;
         mTris=NULL;
      }
   }
   long faces() const
   {
      return mNumTris;
   };
   long vertices() const
   {
      return mNumVerts;
   };
   long vertexIndex(long face, long vertexInFace) const{return mTris[face].faceIndex[vertexInFace];};
   long vertexX(long index) const{return mVerts[index].X;};
   long vertexY(long index) const{return mVerts[index].Y;};
   float vertexZ(long index) const{return 0;};
   long faceAttribute(long face, long attributeIndex) const{return 0;};

};

//-----------------------------------------
void loadNavMeshFromDiskToMesh(const char *filename, iMesh **groundMsh, iMesh **airMesh)
{
   const int * vertXZArray;
   int numVerts; 
   const int *indexesArray; 
   int numTris;

   FILE *f = fopen(filename,"rb");

   fread(&numVerts,sizeof(int),1,f);
   fread(&numTris,sizeof(int),1,f);

   //load our navMesh from disk
   MeshWrapper **mWArray = new MeshWrapper*[1];
   mWArray[0] = new MeshWrapper();

   MeshWrapper *mw = mWArray[0];
   mw->mNumVerts = numVerts;
   mw->mVerts = new vertex[numVerts];
   fread(mw->mVerts,sizeof(int)*2*numVerts,1,f);
   //memcpy(mw->mVerts,vertXZArray,sizeof(int)*2*numVerts);

   mw->mNumInds = numTris * 3;
   mw->mNumTris = numTris;
   mw->mTris = new triangle[mw->mNumTris];
   fread(mw->mTris,sizeof(triangle)*mw->mNumTris,1,f);
   //memcpy(mw->mTris,indexesArray,sizeof(triangle)*mw->mNumTris);

   fclose(f);


   iFaceVertexMesh **iF = (iFaceVertexMesh**)mWArray;

   *groundMsh = gPathEngine->buildMeshFromContent(iF, 1, NULL);


  

   /////////air mesh
   MeshWrapper **mAArray = new MeshWrapper*[1];
   mAArray[0] = new MeshWrapper();

   MeshWrapper *mwt = mAArray[0];

   mwt->mNumVerts = 4;
   mwt->mNumInds  = 6;
   mwt->mNumTris  = 2;
   mwt->mVerts = new vertex[mwt->mNumVerts];
   mwt->mTris = new triangle[mwt->mNumTris];

   int minX=5000000;
   int minZ=5000000;
   int maxX=-5000000;
   int maxZ=-5000000;
   for(int i=0;i<numVerts;i++)
   {
      if(mw->mVerts[i].X>maxX)maxX = mw->mVerts[i].X;
      if(mw->mVerts[i].Y>maxZ)maxZ = mw->mVerts[i].Y;
      if(mw->mVerts[i].X<minX)minX = mw->mVerts[i].X;
      if(mw->mVerts[i].Y<minZ)minZ = mw->mVerts[i].Y;
   }
   mwt->mVerts[0].X = minX;
   mwt->mVerts[0].Y = minZ;
   mwt->mVerts[1].X = minX;
   mwt->mVerts[1].Y = maxZ;
   mwt->mVerts[2].X = maxX;
   mwt->mVerts[2].Y = maxZ;
   mwt->mVerts[3].X = maxX;
   mwt->mVerts[3].Y = minZ;

   mwt->mTris[0].faceIndex[0] = 0;
   mwt->mTris[0].faceIndex[1] = 1;
   mwt->mTris[0].faceIndex[2] = 2;

   mwt->mTris[1].faceIndex[0] = 2;
   mwt->mTris[1].faceIndex[1] = 3;
   mwt->mTris[1].faceIndex[2] = 0;

   iFaceVertexMesh **iA = (iFaceVertexMesh**)mAArray;
   *airMesh = gPathEngine->buildMeshFromContent(iA, 1, NULL);

   delete [] mw->mTris;
   delete [] mw->mVerts;
   delete [] mWArray;

   delete [] mwt->mTris;
   delete [] mwt->mVerts;
   delete [] mAArray;
}
//-----------------------------------------
//------------------------------------
//-----------------------------------------
//-----------------------------------------
//-----------------------------------------
class cFileOutputStream : public iOutputStream
{
   std::ofstream os;

public:
   cFileOutputStream(const char* name) : os(name, std::ios_base::binary)
   {
   }

   void put(const char* data, unsigned long dataSize)
   {
      unsigned long i;
      for(i = 0; i < dataSize; i++)
      {
         os << data[i];
      }
   }
};
//-----------------------------------------
class memStreamHolder
{
public :
   memStreamHolder():
       mMemStream(0),
          mMemStreamSize(0)
       {
       }
       ~memStreamHolder()
       {
          destroy();
       }
       void destroy()
       {
          if(mMemStream!=NULL)
          {
             delete []mMemStream;
             mMemStream=NULL;
          }

       }

       byte *mMemStream;
       unsigned long mMemStreamSize;
};
class cMemoryStream : public iOutputStream
{
public:
   std::vector<byte> mStreamDat;
   
   cMemoryStream()
   {
   }
   ~cMemoryStream()
   {
      destroy();
   }
   void destroy()
   {
      mStreamDat.clear();  
   }
   void put(const char* data, unsigned long dataSize)
   {
      unsigned long i;
      for(i = 0; i < dataSize; i++)
      {
         mStreamDat.push_back(data[i]);
      }
   }

   int giveTotalSize()
   {
      return mStreamDat.size();
   }

   void copyTo(byte* ptrToAllocatedArray)
   {
      //NOTE we assume that the array has already been allocated to mRunningSize
      memcpy(ptrToAllocatedArray,(byte*)&mStreamDat[0], mStreamDat.size());
   }

};
//-----------------------------------------
//-----------------------------------------
//-----------------------------------------
//-----------------------------------------
//-----------------------------------------

//-----------------------------------------
//-----------------------------------------
class shapeFileWrapper
{
public:
   shapeFileWrapper():
      mShapeCollisionData(0),
         mShapePathingData(0)
      {
      }
      ~shapeFileWrapper()
      {
         destroy();
      }
      void destroy()
      {
         if(mShapeCollisionData!=NULL)
         {
            delete []mShapeCollisionData;
            mShapeCollisionData=NULL;
         }
         if(mShapePathingData!=NULL)
         {
            delete []mShapePathingData;
            mShapePathingData=NULL;
         }
      }
      int mShapeType;
      int mShapeIndex;
      int mShapeSize;

      int mNumShapeVerts;
      long *mShapeVertsData;

      unsigned long mShapePathingDataSize;
      unsigned long mShapeCollisionDataSize;

      byte *mShapePathingData;
      byte *mShapeCollisionData;

      byte* toMemChunk(int &memSize)
      {
         int shapeVertMem = mNumShapeVerts*2*sizeof(long);
         memSize = (sizeof(int)*4) + (sizeof(long)*2) + mShapePathingDataSize + mShapeCollisionDataSize + shapeVertMem;
         byte *dta = new byte[memSize];
         byte *dtaC = dta;

         memcpy(dtaC,&mShapeType,sizeof(int)); dtaC += sizeof(int);
         memcpy(dtaC,&mShapeIndex,sizeof(int)); dtaC += sizeof(int);
         memcpy(dtaC,&mShapeSize,sizeof(int)); dtaC += sizeof(int);

         memcpy(dtaC,&mNumShapeVerts,sizeof(int)); dtaC += sizeof(int);
         memcpy(dtaC,mShapeVertsData,shapeVertMem); dtaC += (shapeVertMem);

         memcpy(dtaC,&mShapePathingDataSize,sizeof(long)); dtaC += sizeof(long);
         memcpy(dtaC,&mShapeCollisionDataSize,sizeof(long)); dtaC += sizeof(long);

         memcpy(dtaC,mShapePathingData,mShapePathingDataSize );dtaC += mShapePathingDataSize;
         memcpy(dtaC,mShapeCollisionData,mShapeCollisionDataSize );dtaC += mShapeCollisionDataSize;

         return dta;
      }

};
std::vector<shapeFileWrapper*> mShapes;
//-----------------------------------------
void addShape(iMesh* groundMsh,iShape *shape,int shapeType, int shapeIndex, int shapeWidth,int numShapeVerts, long *shapeVertDat,bool doPathProcess)
{


   shapeFileWrapper *sfw = new shapeFileWrapper();
   sfw->mShapeIndex = shapeIndex;
   sfw->mShapeType = shapeType;
   sfw->mShapeSize = shapeWidth;

   sfw->mNumShapeVerts = numShapeVerts;
   sfw->mShapeVertsData = new long[numShapeVerts*2];
   memcpy(sfw->mShapeVertsData, shapeVertDat,numShapeVerts * 2 * sizeof(long));
   EndianSwitchDWords( (DWORD*) sfw->mShapeVertsData,numShapeVerts * 2 );
   

   //CLM !! Path engine requires collision process to be run first!
   cMemoryStream *memStream1 = new cMemoryStream();
   groundMsh->generateCollisionPreprocessFor(shape,0);
   groundMsh->saveCollisionPreprocessFor(shape, memStream1 );
   sfw->mShapeCollisionDataSize = memStream1->giveTotalSize();
   sfw->mShapeCollisionData = new byte[sfw->mShapeCollisionDataSize];
   memStream1->copyTo(sfw->mShapeCollisionData);
   delete memStream1;


   if(doPathProcess)
   {
      cMemoryStream *memStream0 = new cMemoryStream();
      groundMsh->generatePathfindPreprocessFor(shape,0);
      groundMsh->savePathfindPreprocessFor(shape, memStream0 );
      sfw->mShapePathingDataSize = memStream0->giveTotalSize();
      sfw->mShapePathingData = new byte[sfw->mShapePathingDataSize];
      memStream0->copyTo(sfw->mShapePathingData);
      delete memStream0;
   }
   else
   {
      sfw->mShapePathingDataSize=0;
      sfw->mShapePathingData=0;
   }
   

   mShapes.push_back(sfw);


}
//-----------------------------------------
enum 
{
   cPTHBuildingShapeID = 0x0000AAAA,
   cPTHUnitShapeID = 0x0000BBBB,
   cPTHAirShapeID = 0x0000CCCC,
};
void generateBuildingShapes(iMesh* groundMsh)
{
   int numBuildingShapes =6;
   int sizes[] = {50,150,500,800,1370,4000};

   for(int i=0;i<numBuildingShapes;i++)
   {
      long szeArray[8]=
      {
         -sizes[i], sizes[i],
         sizes[i], sizes[i],
         sizes[i], -sizes[i],
         -sizes[i], -sizes[i]
      };

      if(gPathEngine->shapeIsValid(4,szeArray))
      {
         iShape *shp = gPathEngine->newShape(4, szeArray);
         addShape(groundMsh,shp,cPTHBuildingShapeID,i,sizes[i], 4,szeArray,false);
         delete shp;
      }
   }
}

void generateUnitShapes(iMesh* groundMsh)
{
   int numUnitShapes =5;
   int sizes[] = {50,150,250,320,500};

   for(int i=0;i<numUnitShapes;i++)
   {
      int width =sizes[i];

      // No shape of this radius exists yet - create and share it.
      float d = (float) width / 4.828f;
      long offset1 = (long) (1.414f * d);
      long offset2 = (long) (3.414f * d);
      // Make an octagon (as regular as integer precision permits)
      long szeArray[]=
      {
         offset1, offset2,
         offset2, offset1,
         offset2, -offset1,
         offset1, -offset2,
         -offset1, -offset2,
         -offset2, -offset1,
         -offset2, offset1,
         -offset1, offset2
      };


      iShape *shp = gPathEngine->newShape(8, szeArray);
      addShape(groundMsh,shp,cPTHUnitShapeID,i,sizes[i],8,szeArray,true);
      delete shp;
   }
}
void generateAirShapes(iMesh* groundMsh)
{
   int numAirShapes =6;
   int sizes[]={50,120,200,470,580, 7000};

   for(int i=0;i<numAirShapes;i++)
   {
      int width =sizes[i];

      // No shape of this radius exists yet - create and share it.
      float d = (float) width / 4.828f;
      long offset1 = (long) (1.414f * d);
      long offset2 = (long) (3.414f * d);
      // Make an octagon (as regular as integer precision permits)
      long szeArray[]=
      {
         offset1, offset2,
         offset2, offset1,
         offset2, -offset1,
         offset1, -offset2,
         -offset1, -offset2,
         -offset2, -offset1,
         -offset2, offset1,
         -offset1, offset2
      };

      iShape *shp = gPathEngine->newShape(8, szeArray);
      addShape(groundMsh,shp,cPTHAirShapeID,i,sizes[i],8,szeArray,true);
      delete shp;

   }
}

void generateShapes(iMesh* groundMsh,iMesh* airMsh)
{
   mShapes.clear();
   generateBuildingShapes(groundMsh);
   generateUnitShapes(groundMsh);
   generateAirShapes(airMsh);
}

//-----------------------------------------

void clearECFBuilder(BECFFileBuilder &ecfBuilder)
{
   for(int i=0;i<ecfBuilder.getNumChunks();i++)
   {
      ecfBuilder.removeChunkByIndex(i);
      i--;
   }
}
void writePathEngineToDisk(const char* filename,cMemoryStream *groundMeshData,cMemoryStream *airMeshData)
{  
   BECFFileBuilder ecfBuilder;
   ecfBuilder.setID(cPTH_ECFFileID);

   int version =cPTHVersion;
   ecfBuilder.addChunk(cPTHHeaderID,(BYTE*)&version,sizeof(int));

   //our ground chunk
   int totalMainChunkSize = groundMeshData->giveTotalSize();
   byte *mainChunkDat = new byte[totalMainChunkSize];
   groundMeshData->copyTo(mainChunkDat);
   ecfBuilder.addChunk(cPTHGroundMeshID, mainChunkDat, totalMainChunkSize);

   //our air chunk
   int totalAirChunkSize = airMeshData->giveTotalSize();
   byte *airChunkDat = new byte[totalAirChunkSize];
   airMeshData->copyTo(airChunkDat);
   ecfBuilder.addChunk(cPTHAirMeshID, airChunkDat, totalAirChunkSize);


   //write each of our pre-processed shapes
   for(int i=0;i<mShapes.size();i++)
   {
      int sizeToWrite =0;
      byte *dat =  mShapes[i]->toMemChunk(sizeToWrite);
      ecfBuilder.addChunk(cPTHPrecomputedShapeID, dat, sizeToWrite);
//      delete []dat;
   }

   BWin32FileStream fileStream;

   if (!fileStream.open(filename,cSFWritable))
   {
      clearECFBuilder(ecfBuilder);
      return;
   }

   ecfBuilder.writeToStream(fileStream);
   clearECFBuilder(ecfBuilder);

}
//-----------------------------------------

//-----------------------------------------
bool terrainNavMeshToPathEngineFile()
{
   iMesh* groundMsh=NULL;
   iMesh* airMsh=NULL;
   loadNavMeshFromDiskToMesh(mCmdLineParams.mNavMeshFile,&groundMsh,&airMsh);

   if(!groundMsh || !airMsh)
   {
      gConsoleOutput.error("\nThere was an error converting the navmesh file to an iMesh type.\n");
      return false;
   }
   


   //save our ground mesh to memory
   cMemoryStream *groundMeshStream = new cMemoryStream();
   groundMsh->saveGround("tok",true,groundMeshStream);

   cMemoryStream *airMeshStream = new cMemoryStream();
   airMsh->saveGround("tok",true,airMeshStream);


   //generate our pre-process shapes
   generateShapes(groundMsh,airMsh);


   //finally, write us to disk
   writePathEngineToDisk(mCmdLineParams.mPthFile,groundMeshStream,airMeshStream);


  

   delete groundMeshStream;
   delete airMeshStream;
   groundMsh=NULL;
   airMsh=NULL;
   for(int i=0;i<mShapes.size();i++)
   {
      mShapes[i]->destroy();
   }
   mShapes.clear();

   return true;

}
//--------------------------------------------------
bool genPTHFile()
{
   pathEngineErrorHandler errorHandler;
   gPathEngine = LoadPathEngine(&errorHandler);
   if(!gPathEngine)  return false;


   bool success =terrainNavMeshToPathEngineFile();

   gPathEngine->deleteAllObjects();
    closePathEngine();

    return success;
}
//--------------------------------------------------

bool processParams(BCommandLineParser::BStringArray& args)
{
   if (args.getSize() < 2)
   {
      mCmdLineParams.printHelp();
      return false;
   }

   if (!mCmdLineParams.parse(args))
      return false;

   if (mCmdLineParams.mNavMeshFile=="")
   {
      gConsoleOutput.error("No files specified to process!\n");
      return false;  
   }         

   return true;
}



//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------
int main(int argC, const char** argV)
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

   if (!BConsoleAppHelper::getQuiet())
      gConsoleOutput.printf(PROGRAM_TITLE " Compiled %s %s, Creating PTH Data Version 0x%08X\n", __DATE__, __TIME__, cPTHHeaderID);


   bool success = true;
   if(processParams(args))
      success = genPTHFile();

   BConsoleAppHelper::deinit();

   XCoreRelease();


   if(!success)
      BConsoleAppHelper::pause();
   

   return success ? EXIT_SUCCESS : EXIT_FAILURE;  
}