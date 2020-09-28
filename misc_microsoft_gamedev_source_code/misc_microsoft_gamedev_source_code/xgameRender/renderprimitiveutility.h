//==============================================================================
// renderprmitiveutility.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================
#pragma once 

#include "effect.h"
#include "renderThread.h"
#include "renderCommand.h"
#include "debugprimitives.h"

//==============================================================================
//==============================================================================


//==============================================================================
//==============================================================================
struct BRPUUpdateData
{
   int   mCount;
   void* mpData;
};

//==============================================================================
// BRenderPrimitiveUtility
//==============================================================================
class BRenderPrimitiveUtility : public BRenderCommandListenerInterface, BEventReceiver
{
   public:      
               BRenderPrimitiveUtility();
      virtual ~BRenderPrimitiveUtility();

      enum eBRUCommand
      {
         eBRUUpdateLine = 0,
         eBRUUpdateCircle,
         eBRUUpdateArrow, 
         eBRUUpdateBox,
         eBRUUpdateSphere,         
         eBRUUpdateText,
         eBRUUpdateThickLine,
         eBRUUpdateThickCircle,          
         eBRUCommandTotal
      };

      enum eRenderCommand
      {
         eRenderThickLine = 0,
         eRenderThickCircle,
         eRenderCommandTotal
      };

      enum eRenderCommandTechniquePass
      {
         eRenderPassAlphablend=0,
         eRenderPassAdditive,
         eRenderPassSubtractive,
         eRenderPassTotal
      };

      enum 
      {
         eEventClassReloadEffects = cEventClassFirstUser
      };
      
      void init(void);
      void deInit(void);

      void update(eBRUCommand type, int count, void* pData);
      void render();
      void workerRender(void);

      //-- BEvent Receiver interface
      bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
                 
   private:

      struct BThickLineVertex
      {  
         XMFLOAT4 mPos;      // position [xyz] thickness [w]
         XMCOLOR  mColor;    // Color         
      };

      //-- WORKER THREAD

      static void setWorldTransform(BMatrix worldMatrix, BVector scaleVector);
      static void setWorldTransform(BMatrix worldMatrix);
      
      void drawBoxesInternal(const BRPUUpdateData* pData);      
      void drawCirclesInternal(const BRPUUpdateData* pData);
      void drawThickCirclesInternal(const BRPUUpdateData* pData);
      void drawArrowsInternal(const BRPUUpdateData* pData);
      void drawSpheresInternal(const BRPUUpdateData* pData);      
      void drawLinesInternal(const BRPUUpdateData* pData);
      void drawTextInternal(const BRPUUpdateData* pData);
      void drawThickLinesInternal(const BRPUUpdateData* pData);

      // -- MAIN/WORKER
      void loadEffect();

      //-- MAIN
      void reloadInit();
      void reloadDeinit();

      //-- WORKER
      void initEffect(const BEvent& event);
      
      //-- WORKER THREAD
      //-- Only access this data on the worker thread!
      void initBoxBuffers();
      void initCircleBuffers();
      void initThickCircleBuffers();
      void initSphereBuffers();      
      void initArrowBuffers();

      //-- vertex decl initialization
      void initVertexDeclarations(void);

      void computeSpherePoint(float angle1, float angle2, BVector& v);
      void releaseBuffers();      
      
      enum { cNumUpdateDataContexts = 2 };
      BRPUUpdateData mUpdateData[cNumUpdateDataContexts][eBRUCommandTotal];
      
      BCommandListenerHandle  mCommandListenerHandle;
      IDirect3DVertexBuffer9* mpBoxVB;
      IDirect3DIndexBuffer9*  mpBoxIB;

      IDirect3DVertexBuffer9* mpArrowVB;
      IDirect3DIndexBuffer9*  mpArrowIB;

      IDirect3DVertexBuffer9* mpCircleVB;      
      IDirect3DVertexBuffer9* mpSphereVB;
      IDirect3DVertexBuffer9* mpThickCircleVB;      


      IDirect3DVertexDeclaration9* mpTickLineVertexDecl;
      BFXLEffect           mEffects       [eRenderCommandTotal];
      BFXLEffectTechnique  mTechnique     [eRenderCommandTotal];
      BFXLEffectParam      mColorParam    [eRenderCommandTotal];
      BFXLEffectParam      mScaleParam    [eRenderCommandTotal];
      BFXLEffectParam      mThicknessParam[eRenderCommandTotal];

            
      //-- Listener interface
      // init will be called from the worker thread after the D3D device is initialized.
      virtual void initDeviceData(void);

      // Called from worker thread.
      virtual void frameBegin(void);

      // Called from the worker thread to process commands.
      virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);

      // Called from worker thread.
      virtual void frameEnd(void);

      // deinit will be called from the worker thread before the RCL is freed, but always before the D3D device is release.
      virtual void deinitDeviceData(void);
};

//==============================================================================
// Globals
//==============================================================================
extern BRenderPrimitiveUtility gRenderPrimitiveUtility;
