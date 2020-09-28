// File: gammaRamp.cpp
#include "xgameRender.h"
#include "gammaRamp.h"
#include "reloadManager.h"
#include "xmlReader.h"
#include "xmlWriter.h"
#include "consoleOutput.h"

BGammaRamp gGammaRamp;

enum
{
   cCommandSetGammaRamp = 0,
};

struct BGammaRampSettings
{
   BGammaRampSettings(float gamma, float contrast) : 
      mGamma(gamma), mContrast(contrast)
   {
   }      
   
   float mGamma;
   float mContrast;
};

BGammaRamp::BGammaRamp() :
   mGamma(1.0f),
   mContrast(1.0f),
   mDirID(-1)
{
}

BGammaRamp::~BGammaRamp()
{
}

void BGammaRamp::init(void)
{
   commandListenerInit();
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverInit();
#endif
   
   mGamma = 1.0f;
   mContrast = 1.0f;
   
   mDirID = -1;
   mFilename.empty();
}

void BGammaRamp::deinit(void)
{
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverDeinit();
#endif
   commandListenerDeinit();
}

bool BGammaRamp::load(long dirID, const char* pFilename)
{
#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.deregisterClient(getEventHandle());
#endif

   mDirID = dirID;
   mFilename.set(pFilename);

#ifdef ENABLE_RELOAD_MANAGER
   BSimString fullFilename;
   gFileManager.constructQualifiedPath(dirID, mFilename, fullFilename);

   BReloadManager::BPathArray paths;
   paths.pushBack(fullFilename);
   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, getEventHandle());
#endif

   BXMLReader xmlReader;
   if (!xmlReader.load(dirID, pFilename, XML_READER_IGNORE_BINARY))
   {
      gConsoleOutput.output(cMsgError, "BGammaRamp::load: Unable to load gamma ramp file %s\n", pFilename);
      return false;
   }
   
   BXMLNode rootNode(xmlReader.getRootNode());   
   
   bool success = true;
   
   float gamma = 1.0f;
   float contrast = 1.0f;
   
   BXMLNode gammaNode;
   if (rootNode.getChild("gamma", &gammaNode))
   {
      if (gammaNode.getTextAsFloat(gamma))
         gamma = Math::Clamp(gamma, .01f, 5.0f);
   }
   else
      success = false;
            
   BXMLNode contrastNode;
   if (rootNode.getChild("contrast", &contrastNode))
   {
      if (contrastNode.getTextAsFloat(contrast))
         contrast = Math::Clamp(contrast, .01f, 5.0f);
      else
         success = false;
   }
   else
      success = false;
   
   if (!success)
   {  
      gConsoleOutput.output(cMsgError, "BGammaRamp::load: Gamma ramp file %s is invalid\n", pFilename);
      return false;
   }
   
   trace("BGammaRamp::load: Loaded %s, Gamma: %f Contrast: %f\n", pFilename, gamma, contrast);
   
   set(gamma, contrast);
      
   return true;
}

bool BGammaRamp::save(long dirID, const char* pFilename)
{
   BXMLWriter writer;
   
   if (!writer.create(dirID, pFilename))
      return false;

   writer.startItem("gammaRamp");
   writer.addItem("gamma", mGamma);
   writer.addItem("contrast", mContrast);
   writer.endItem();
   
   writer.close();
   
   return true;
}

bool BGammaRamp::reload(void)
{
   if ((mDirID != -1) && (!mFilename.isEmpty()))
      return load(mDirID, mFilename);
   return false;      
}

void BGammaRamp::set(float gamma, float contrast)
{
   if ((gamma == mGamma)  && (contrast == mContrast))
      return;
      
   mGamma = Math::Clamp(gamma, .01f, 5.0f);
   mContrast = Math::Clamp(contrast, .01f, 5.0f);
   
   gRenderThread.submitCommand(mCommandHandle, cCommandSetGammaRamp, BGammaRampSettings(mGamma, mContrast));
}
#ifdef ENABLE_RELOAD_MANAGER
bool BGammaRamp::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cEventClassReloadNotify:
      {
         reload();
         break;
      }
   }
   
   return false;
}
#endif
void BGammaRamp::initDeviceData(void)
{
}

void BGammaRamp::frameBegin(void)
{
}

void BGammaRamp::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
      case cCommandSetGammaRamp:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BGammaRampSettings));
         const BGammaRampSettings* pSettings = reinterpret_cast<const BGammaRampSettings*>(pData);
         
         if (BD3D::mD3DPP.FrontBufferFormat == D3DFMT_LE_X2R10G10B10)
         {
            D3DPWLGAMMA ramp;
            //BD3D::mpDev->GetPWLGamma(&ramp);
            
            // rg [6/16/06] - FIXME: The final segment will be brighter than MS's default. Why do they do this?
            for (uint i = 0; i < 128; i++)
            {
               double start = i / 128.0f;
               double end = (i + 1) / 128.0f;
               
               start = Math::Clamp<double>(pSettings->mContrast * pow(start, (double)pSettings->mGamma), 0.0f, 1.0f);
               end = Math::Clamp<double>(pSettings->mContrast * pow(end, (double)pSettings->mGamma), 0.0f, 1.0f);
               
               start *= 65535.0f;
               end *= 65535.0f;
               
               uint istart = Math::Clamp(Math::FloatToIntRound((float)start), 0, 65535);            
               uint iend = Math::Clamp(Math::FloatToIntRound((float)end), 0, 65535);
               
               ramp.red[i].Base = (WORD)istart;
               ramp.red[i].Delta = (WORD)(iend - istart);
               
               ramp.green[i].Base = (WORD)istart;
               ramp.green[i].Delta = (WORD)(iend - istart);
               
               ramp.blue[i].Base = (WORD)istart;
               ramp.blue[i].Delta = (WORD)(iend - istart);
            }
            
            BD3D::mpDev->SetPWLGamma(D3DSGR_IMMEDIATE, &ramp);
         }
                  
         break;
      }
   }
}

void BGammaRamp::frameEnd(void)
{
}

void BGammaRamp::deinitDeviceData(void)
{
}
