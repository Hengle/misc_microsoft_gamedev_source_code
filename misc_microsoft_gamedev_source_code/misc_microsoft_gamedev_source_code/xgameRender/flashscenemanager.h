//============================================================================
//
// flashscenemanager.h
//
//============================================================================

#pragma once


class BFlashSceneManager : public BRenderCommandListener, public BEventReceiver
{
   public:
      BFlashSceneManager();
     ~BFlashSceneManager();

   private:
      virtual void initDeviceData(void);
      virtual void frameBegin(void);
      virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
      virtual void frameEnd(void);
      virtual void deinitDeviceData(void);
      virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};