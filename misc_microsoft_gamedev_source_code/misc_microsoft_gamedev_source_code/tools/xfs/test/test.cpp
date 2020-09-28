// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "xmlreader.h"

int _tmain(int argc, _TCHAR* argv[])
{
   argc;
   argv;

   gXFS.setup("sys442.ENS.Ensemble-Studios.com");

   long cDir=gFileManager.addBaseDirectory(BString("game:\\"));

   BXMLReader reader;
   bool retval=reader.loadFileSAX(cDir, "data\\proto.xml", NULL, false);
   if(!retval)
      trace("loadFileSAX error");
   //retval=reader.writeXML(cDir, BString("data\\temp.xml"));
   //if(!retval)
   //   trace("writeXML error");

   const DWORD cBufferSize=5120000;
   BYTE* buffer=new BYTE[cBufferSize];

   HANDLE hFile=CreateFile("game:\\splashx.bmp", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
   if(hFile!=INVALID_HANDLE_VALUE)
   {
      DWORD bytesRead=0;
      DWORD time=timeGetTime();
      BOOL retval=ReadFile(hFile, buffer, cBufferSize, &bytesRead, NULL);
      DWORD time2=timeGetTime();
      trace("time=%u", time2-time);
      if(retval)
      {
         FILE* file=fopen("game:\\temp.bmp", "wb");
         if(file)
         {
            fwrite(buffer, bytesRead, 1, file);
            fclose(file);
         }
      }
      else
         trace("read error");
      CloseHandle(hFile);
   }
   else
      trace("open error");

   delete[] buffer;

   gXFS.shutdown();
	return 0;
}

