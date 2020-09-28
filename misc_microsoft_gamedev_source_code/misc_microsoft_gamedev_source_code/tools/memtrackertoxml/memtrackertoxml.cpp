// File: memStats.cpp
#include "xcore.h"
#include "xcoreLib.h"
#include "consoleOutput.h"
#include "containers\hashMap.h"
#include "utils\consoleAppHelper.h"
#include "stream\cfileStream.h"
#include "stream\byteStream.h"
#include "xdb\xdb.h"

#include "memory\allocationLoggerPackets.h"

#define cXMLStringBufferSize 200


const long cMaxCallstackEntries = 32;
class BCallstackInfo
{
   public:
      BCallstackInfo() : mCallstackCount(0), mAllocatedSizes(0) {}
      ~BCallstackInfo() {delete []mAllocatedSizes;}
      
      bool                       load(BCFileStream &stream, long sample)
                                 {
                                    // Get size for this stack.
                                    mAllocatedSizes[sample] = 0;
                                    stream.readObjBigEndian(mAllocatedSizes[sample]);

                                    // Get count of callstack entries.
                                    mCallstackCount = 0;
                                    stream.readObjBigEndian(mCallstackCount);
                                    
                                    // Sanity.
                                    if(mCallstackCount <= 0 || mCallstackCount > cMaxCallstackEntries)
                                    {
                                       BFAIL("bogus stack count");
                                       return(false);
                                    }
                                    
                                    // Load the callstacks.
                                    for(long i=0; i<mCallstackCount; i++)
                                    {
                                       // Read address.
                                       mCallstack[i] = 0;
                                       stream.readObjBigEndian(mCallstack[i]);
                                    }
                                    
                                    return(true);
                                 };
                                 
      bool                       callstacksMatch(const BCallstackInfo &other)
                                 {
                                    // If sizes don't match, we fail immediately.
                                    if(mCallstackCount != other.mCallstackCount)
                                       return(false);
                                       
                                    // Walk the callstacks.
                                    for(long i=0; i<mCallstackCount; i++)
                                    {
                                       // If the address don't match, we've failed.
                                       if(mCallstack[i] != other.mCallstack[i])
                                          return(false);
                                    }
                                    
                                    // If we get here, there is a match.
                                    return(true);
                                 };                                 
   
      DWORD                      mCallstack[cMaxCallstackEntries];
      long                       mCallstackCount;
      
      long                       *mAllocatedSizes;
};


//-------------------------------------------------

class BMemTrackerToXML
{
public:
   BMemTrackerToXML() :
      mXDBStream((const void*)NULL, 0),
      mCallstacks(NULL),
      mNumCallstacks(0),
      mNextNewCallstackIndex(0)
   {
   }
   
   ~BMemTrackerToXML()
   {
      delete []mCallstacks;
   }

   bool process(BCommandLineParser::BStringArray& args)
   {
      if (!parseArgs(args))
         return false;
   
      if (!openOutputFile())
         return false;         
         
      if (!openXDBFile())
         return false;

      if (!parseInputFiles())
         return false;
            
      writeXML();
      closeOutputFile();
      closeXDBFile();

      return true;
   }

private:
   BString mXDBFilename;
   BByteArray mXDBFileData;
   BByteStream mXDBStream;
   BXDBFileReader mXDBFileReader;

   BString mOutputFilename;   
   BCFileStream mOutputStream;

   BDynamicArray<BString> mInputFilenames;

   BCallstackInfo *mCallstacks;
   long mNumCallstacks;
   long mNextNewCallstackIndex;
   
   
   bool parseInputFiles()
   {
      // First snoop the files to find the one with the largest unique callstack count.
      // This SHOULD be the last one, but we'll be thorough.
      // Also, this assumes that each larger log is a superset of any previous ones, which is true with the
      // current tracker since no callstack entries are ever removed, even if they have 0 allocated.
      BCFileStream stream;
      mNumCallstacks = 0;
      for(long i=0; i<mInputFilenames.getNumber(); i++)
      {
         // Open it.
         bool ok = stream.open(mInputFilenames[i]);
         if(!ok)
         {
            gConsoleOutput.printf("Could not open input file %S.\n", mInputFilenames[i].getPtr());
            return(false);
         }

         // Get xex checksum.
         DWORD xexChecksum = 0;
         stream.readObjBigEndian(xexChecksum);

         // Does it match the xdb checksum?
         if(xexChecksum != mXDBFileReader.getCheckSum())
         {
            // Generate a warning.
            gConsoleOutput.printf("XEX checksum in %S does not match!  You might be using the wrong xdb.\n", mInputFilenames[i].getPtr());
         }

         // Read number of callstacks in file.
         long numCallstacks = 0;
         stream.readObjBigEndian(numCallstacks);
         
         // Save if bigger than max so far.
         if(numCallstacks > mNumCallstacks)
            mNumCallstacks = numCallstacks;
            
         // Done with the file, for now.
         stream.close();
      }
      
      // Allocate space for the unique callstacks.
      mCallstacks = new BCallstackInfo[mNumCallstacks];
      
      // Allocate space for samples and zero them out.
      for(long i=0; i<mNumCallstacks; i++)
      {
         mCallstacks[i].mAllocatedSizes = new long[mInputFilenames.getNumber()];
         memset(mCallstacks[i].mAllocatedSizes, 0, mInputFilenames.getNumber()*sizeof(long));
      }
      
      // Now process the files for real.
      for(long fileIndex=0; fileIndex<mInputFilenames.getNumber(); fileIndex++)
      {
         // Open it.
         bool ok = stream.open(mInputFilenames[fileIndex]);
         if(!ok)
         {
            gConsoleOutput.printf("Could not open input file %S.\n", mInputFilenames[fileIndex].getPtr());
            return(false);
         }

         // Get xex checksum & ignore it since we've already warned about mismatches.
         DWORD xexChecksum = 0;
         stream.readObjBigEndian(xexChecksum);

         // Read number of callstacks in file.
         long numCallstacks = 0;
         stream.readObjBigEndian(numCallstacks);
         gConsoleOutput.printf("%s has %d unique callstacks.\n", mInputFilenames[fileIndex].getPtr(), numCallstacks);

         // Temp for loading.
         BCallstackInfo info;
         info.mAllocatedSizes = new long[1];
         
         // Iterate the callstacks.
         for(long i=0; i<numCallstacks; i++)
         {
            if(i>0 && i%1000 == 0)
               gConsoleOutput.printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b   %d processed.", i);
            
            bool ok = info.load(stream, 0);
            if(!ok)
            {
               gConsoleOutput.printf("   \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bFailed.                         \n");
               gConsoleOutput.printf("   Alloc log appears to be corrupted.\n");
               return(false);
            }
            
            // Find it in the list.
            long callstackIndex = findCallstackIndex(info);
            if(callstackIndex < 0)
            {
               // If there's no match, add it to the next free slot.
               callstackIndex = mNextNewCallstackIndex;
               
               // Safety check.
               if(mNextNewCallstackIndex >= mNumCallstacks)
               {
                  BFAIL("too many callstacks somehow");
                  return(false);
               }
               
               // Fill in info
               mCallstacks[callstackIndex].mCallstackCount = info.mCallstackCount;
               memcpy(mCallstacks[callstackIndex].mCallstack, info.mCallstack, info.mCallstackCount*sizeof(DWORD));
               
               // Slot is now used.
               mNextNewCallstackIndex++;
            }
            
            // Save off the size from this sample.
            mCallstacks[callstackIndex].mAllocatedSizes[fileIndex] = info.mAllocatedSizes[0];
         }
            
         // Done with the file.
         stream.close();

         gConsoleOutput.printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b   Done.                         \n\n");
      }
      
      return(true);
   }

   void writeXML(void)
   {
      gConsoleOutput.printf("Writing %s.\n", mOutputFilename.getPtr());
      
      mOutputStream.printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
      mOutputStream.printf("<MemStats>\n");
      
      BASSERT(mNextNewCallstackIndex == mNumCallstacks);
      
      // Iterate the callstacks.
      for(long i=0; i<mNextNewCallstackIndex; i++)
      {
         if(i>0 && i%1000 == 0)
            gConsoleOutput.printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b   %d processed.", i);

         // Get max sample for this callstack.
         long maxAllocated = 0;
         for(long fileIndex=0; fileIndex<mInputFilenames.getNumber(); fileIndex++)
         {
            if(mCallstacks[i].mAllocatedSizes[fileIndex] > maxAllocated)
               maxAllocated = mCallstacks[i].mAllocatedSizes[fileIndex];
         }
         
         // If max alloc size is positive, we'll log it out (otherwise we skip)
         if(maxAllocated > 0)
         {
            // Opening entry with size and lowest level address.  Done here because it needs the address from the first stack entry.
            mOutputStream.printf("   <alloc size='%d' address='%u'>\n", maxAllocated, mCallstacks[i].mCallstack[0]);
            
            // Write the sample sizes.
            for(long fileIndex=0; fileIndex<mInputFilenames.getNumber(); fileIndex++)
            {
               mOutputStream.printf("      <sizeSample sample='%d'>%d</sizeSample>\n", fileIndex, mCallstacks[i].mAllocatedSizes[fileIndex]);
            }
            
            for(long j=0; j<mCallstacks[i].mCallstackCount; j++)
            {
               // Resolve to symbols.
               BXDBFileReader::BLookupInfo lookupInfo;
               bool found = mXDBFileReader.lookup(mCallstacks[i].mCallstack[j], lookupInfo);

               // Stack entry
               if(found)
               {
                  static WCHAR fileName[cXMLStringBufferSize];
                  static WCHAR symbolName[cXMLStringBufferSize];
                  static BStringTemplate<WCHAR> buffer;

                  buffer.set(lookupInfo.mFilename);
                  strwEscapeForXML(buffer.getPtr(), fileName, cXMLStringBufferSize);

                  buffer.set(lookupInfo.mSymbol);
                  strwEscapeForXML(buffer.getPtr(), symbolName, cXMLStringBufferSize);

                  mOutputStream.printf("      <stack file='%S' line='%d'>%S(0x%x)</stack>\n", fileName, lookupInfo.mLine, symbolName, mCallstacks[i].mCallstack[j]);
               }
               else
               {
                  mOutputStream.printf("      <stack file='unknown' line='0'>0x%x</stack>\n", mCallstacks[i].mCallstack[j]);
               }
            }

            // End of this allocation.                  
            mOutputStream.printf("   </alloc>\n");
         }
      }
      gConsoleOutput.printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b   Done.                         \n");

      mOutputStream.printf("</MemStats>\n");

   }

   
   bool openXDBFile(void)
   {
      gConsoleOutput.printf("Reading XDB file: %s\n", mXDBFilename.getPtr());
      
      BCFileStream xdbFileStream;
      if (!xdbFileStream.open(mXDBFilename))
      {
         gConsoleOutput.error("Unable to open XDB file: %s\n", mXDBFilename.getPtr());
         return false;
      }
      
      if ((xdbFileStream.size() == 0) || (xdbFileStream.size() >= 1024*1024*1024))
      {
         gConsoleOutput.error("XDB file is invalid: %s\n", mXDBFilename.getPtr());
         return false;
      }
      
      mXDBFileData.resize((uint)xdbFileStream.size());
      if (xdbFileStream.readBytes(mXDBFileData.getPtr(), mXDBFileData.getSizeInBytes()) != mXDBFileData.getSizeInBytes())
      {
         gConsoleOutput.error("Can't read XDB file: %s\n", mXDBFilename.getPtr()); 
         return false;
      }
         
      mXDBStream.set(mXDBFileData.getPtr(), mXDBFileData.getSizeInBytes());               
      
      if (!mXDBFileReader.open(&mXDBStream))
      {
         gConsoleOutput.error("Unable to parse XDB file: %s\n", mXDBFilename.getPtr());
         return false;
      }
      
      gConsoleOutput.printf("XDB XEX Checksum: 0x%08X\n", mXDBFileReader.getCheckSum());
      
      return true;
   }
   
   void closeXDBFile(void)
   {
      mXDBFileReader.close();
                 
      mXDBStream.set((void*)NULL, 0);
      
      mXDBFileData.resize(0);
   }
   
   long findCallstackIndex(const BCallstackInfo &info)
   {
      // Sanity.
      if(!mCallstacks)
         return(false);
         
      // Dumb linear search.
      for(long i=0; i<mNextNewCallstackIndex; i++)
      {
         // See if they match.
         if(mCallstacks[i].callstacksMatch(info))
         {
            // We found a match.
            return(i);
         }
      }
      
      // If we got here, nothing found.
      return(-1);
   }

   bool openOutputFile(void)
   {
      gConsoleOutput.printf("Opening output file: %s\n", mOutputFilename.getPtr());
      
      if (!mOutputStream.open(mOutputFilename, cSFWritable | cSFSeekable))
      {
         gConsoleOutput.error("Unable to open output file: %s\n", mOutputFilename.getPtr());
         return false;
      }
      
      return true;
   }
   
   void closeOutputFile(void)
   {
      mOutputStream.close();
   }
   
   void printHelp(void)
   {
      gConsoleOutput.printf("Usage: memtrackertoxml database.xdb log.bin destination.xml\n");
      BConsoleAppHelper::printHelp();
   }
   
   bool parseArgs(BCommandLineParser::BStringArray& args)
   {
      // jce [11/18/2008] -- useless param to avoid remunging this code to handle no params
      const BCLParam params[] = 
      {
         { "xdb", cCLParamTypeBStringPtr, &mXDBFilename},
         { "output", cCLParamTypeBStringPtr, &mOutputFilename},
      };
      const uint cNumCommandLineParams = sizeof(params)/sizeof(params[0]);
      
      BCommandLineParser parser(params, cNumCommandLineParams);
      const bool success = parser.parse(args, true, false);
      if (!success)
      {
         gConsoleOutput.error("%s\n", parser.getErrorString());
         return false;
      }
      
      const BDynamicArray<uint>& unparsedParams = parser.getUnparsedParams();
   
      for (uint j = 0; j < unparsedParams.getSize(); j++)
      {
         const uint i = unparsedParams[j];
         
         if (args[i].isEmpty())
            continue;

         mInputFilenames.add(BString(args[i]));
      }
      
      if (mXDBFilename.isEmpty() || mOutputFilename.isEmpty() || mInputFilenames.getNumber() < 1)
      {
         printHelp();
         return false;
      }
      
      return true;
   }
      
};

//-------------------------------------------------

int main(int argc, const char *argv[])
{
   BCommandLineParser::BStringArray args;
   if (!BConsoleAppHelper::init(args, argc, argv))
      return EXIT_FAILURE;
   
   gConsoleOutput.printf("memtrackertoxml Compiled %s %s\n", __DATE__, __TIME__);
   
   BMemTrackerToXML memTrackerToXML;
  
   if (!memTrackerToXML.process(args))
   {
      BConsoleAppHelper::deinit();
      return EXIT_FAILURE;
   }

   
   
   BConsoleAppHelper::deinit();
   return EXIT_SUCCESS;
}
