//------------------------------------------------------------------------------------
// File: ugfout.h
// Copyright (C)2003 Blank Cartidge, Inc.
//------------------------------------------------------------------------------------
#ifndef UGFEXP__H
#define UGFEXP__H
//------------------------------------------------------------------------------------
#if _DEBUG
   #define CATCH_EXCEPTIONS 0
#else
   #define CATCH_EXCEPTIONS 1
#endif

#include "common/core/core.h"
#include "common/geom/unigeom.h"
#include "common/scene/scene_data.h"
#include "common/utils/stream.h"
#include "common/utils/logfile.h"

#include "Max.h"
#include "resource.h"
#include "istdplug.h"
#include "stdmat.h"
#include "phyexp.h"
#include "shape.h"
#include "iskin.h"
#include "bipexp.h"
#include "max_utils.h"

//------------------------------------------------------------------------------------

extern ClassDesc* GetLGTOutDesc();
extern TCHAR *GetString(int id);
extern HINSTANCE hInstance;

extern gr::LogFile gLog;

//------------------------------------------------------------------------------------
// This is the main class for the exporter.

class UGFExporter
{
public:
   bool exportLights(gr::Stream& stream, Interface* ip, int startFrame, int endFrame, int frameInterval, bool onlySelected);
     
private:
            
   gr::SceneData::Base& createSceneDataBase(gr::SceneData::Base& base, INode* pNode, TimeValue time);
   gr::SceneData::Light createSceneDataLight(INode* pNode, TimeValue time);
   gr::SceneData::Object createSceneDataObject(INode* pNode, TimeValue time, bool firstFrame);
   gr::SceneData::Camera createSceneDataCamera(INode* pNode, TimeValue time, bool firstFrame);
   void findSceneObjectsEnumNodes(INode* pNode, INodePtrVec& nodes, TimeValue time, bool onlySelected);
   void findSceneObjects(INodePtrVec& nodes, Interface* ip, TimeValue time, bool onlySelected);
   void sampleSceneAtTime(gr::SceneData::Scene& scene, TimeValue time, INodePtrVec& nodes, bool firstFrame);
   void sampleScene(gr::SceneData::Scene& scene, TimeValueVec timeVec, INodePtrVec& nodes);

   bool sampleControllerTimes(INode* pNode, TimeValueVec& timeVec);
   bool enumNodeForKeyFrameObject(INode* pNode, TimeValueVec& timeVec, bool morph);
   bool findKeyFrameObject(TimeValueVec& timeVec, Interface* ip, bool morph);
   void createUniformKeyFrameTimes(
      TimeValueVec& timeVec, 
      int startFrame, int endFrame, int frameInterval);
};

//------------------------------------------------------------------------------------

class LGTOut : public SceneExport 
{
   friend BOOL CALLBACK LGTExportDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
   LGTOut();
   ~LGTOut();

   // SceneExport methods
   int    ExtCount();     // Number of extensions supported
   const TCHAR * Ext(int n);     // Extension #n (i.e. "LGT")
   const TCHAR * LongDesc();     // Long ASCII description (i.e. "Ascii Export")
   const TCHAR * ShortDesc();    // Short ASCII description (i.e. "Ascii")
   const TCHAR * AuthorName();    // ASCII Author name
   const TCHAR * CopyrightMessage();   // ASCII Copyright message
   const TCHAR * OtherMessage1();   // Other message #1
   const TCHAR * OtherMessage2();   // Other message #2
   unsigned int Version();     // Version number * 100 (i.e. v3.01 = 301)
   void  ShowAbout(HWND hWnd);  // Show DLL's "About..." box
   int      DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0); // Export file
   BOOL  SupportsOptions(int ext, DWORD options);

   // Interface to member variables
   TimeValue GetStartFrame()              { return nStartFrame; }
   TimeValue GetEndFrame()                { return nEndFrame; }
   void SetStartFrame(TimeValue val)      { nStartFrame = val; }
   void SetEndFrame(TimeValue val)        { nEndFrame = val; }
   void SetSampleInterval(int frames)     { mSampleInterval = frames; }
   int GetSampleInterval(void)            { return mSampleInterval; }

   Interface*  GetInterface()        { return ip; }
  
private:
   
   TimeValue   nStartFrame;
   TimeValue   nEndFrame;
   int mSampleInterval;

   Interface*  ip;
   FILE*    pStream;

   bool canceled;

   bool includeHidden;
   bool onlySelected;

   UGFExporter mUGFExporter;

   void ResetExportOptions(void);
};

//------------------------------------------------------------------------------------

#endif // __UGFEXP__H
