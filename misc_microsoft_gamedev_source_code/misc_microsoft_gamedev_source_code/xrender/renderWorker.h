// File: renderWorker.h
#pragma once

enum eRenderCommandClass
{
   cRCCControl,
   cRCCRender,

   cRCCMax
};

enum eRenderCommandType
{
   cRCTBeginScene,
   cRCTClear,
   cRCTEndScene,
   cRCTPresent,
   cRCTExit,

   cRCTRenderInstance,

   cRCTMax
};

class BTriangleRenderer;

class BRenderWorker
{
public:
   BRenderWorker();
   ~BRenderWorker();

   bool init(void);
   bool deinit(void);

   void submitCommand(DWORD commandClass, DWORD commandType, DWORD commandLen = 0, const void* pCommandData = NULL);
   
   void kickCommands(void);
   
   IDirect3DDevice9*       getDevice(void) const { return mpD3DDev; }
   const D3DPRESENT_PARAMETERS& getD3DPP(void) const { return mD3DPP; }
   
   void panic(const char* pMsg, ...);
      
private:
   IDirect3D9*             mpD3D; 
   IDirect3DDevice9*       mpD3DDev; 
   D3DPRESENT_PARAMETERS   mD3DPP; 
   HANDLE                  mThreadHandle;

   bool mTerminate;
      
   enum { NumSegments = 64, SegmentSize = 4096 };

   struct BSegment
   {
      uchar mData[SegmentSize];
   };

   typedef BCommandFIFO<BSegment, NumSegments> BCommandBuffer;

   BCommandBuffer mCmdBuf;
   uchar* mpCmdBufSegment;
   DWORD mCmdBufSegmentOfs;

   struct BCommandHeader
   {
      enum { cCommandHeaderMagic = 0xE014 };

      WORD mMagic;
      WORD mClass;
      WORD mType;
      WORD mLen;
   };
   
   BTriangleRenderer* mpTriangleRenderer;

   static void workerThreadFunc(void* pData);
   
   bool workerThread(void);

   bool initD3D(void);

   bool initData(void);
   bool deinitData(void);

   bool commandLoop(void);

   void processCommand(const BCommandHeader& header, const uchar* pData);
   void processControlCommand(const BCommandHeader& header, const uchar* pData);
   void processRenderCommand(const BCommandHeader& header, const uchar* pData);
};

extern BRenderWorker gRenderWorker;
