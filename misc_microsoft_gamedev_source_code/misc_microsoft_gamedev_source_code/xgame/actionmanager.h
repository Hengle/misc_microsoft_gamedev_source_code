//==============================================================================
// actionmanager.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once 

#include "Action.h"
#include "SimTypes.h"

__declspec(selectany) extern const DWORD cActionIndexMask=0x00FFFFFF;
__declspec(selectany) extern const DWORD cActionTypeMask=0xFF000000;
#define ACTIONINDEXFROMID(n) ((n) & cActionIndexMask) 
#define ACTIONTYPEFROMID(n) (((n) & cActionTypeMask) >> 24) 
#define CREATEACTIONID(type, index) (( (type << 24) & cActionTypeMask) | (index))



//==============================================================================
//==============================================================================
class BActionManager
{
   public:

      BActionManager();
      virtual ~BActionManager();
      
      bool                    init();
      void                    reset();
      BAction*                createAction(BActionType type);   
      void                    releaseAction(BAction* pAction);

      int                     getNumberActionTypes() const { return mActionTypes.numTags(); }
      BActionType             getActionType(const char* pName);
      const char*             getActionName(BActionType type);
      uint                    getActionIndex(const BAction* pAction) const;

      GFDECLAREVERSION();
      bool preSave(BStream* pStream, int saveType) const;
      bool preLoad(BStream* pStream, int saveType);

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      bool savePtr(BStream* pStream, const BAction* pAction);
      bool loadPtr(BStream* pStream, BAction** ppAction);

   protected:
      BStringTable<BActionType>  mActionTypes;
      uint                    mRefCount;
      uint                    mActionIndex;
};

extern BActionManager gActionManager;

#define GFWRITEACTIONPTR(stream,varname) { if (!gActionManager.savePtr(stream, varname)) return false; }
#define GFREADACTIONPTR(stream,varname) { if (!gActionManager.loadPtr(stream, &(varname))) return false; }
