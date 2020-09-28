//==============================================================================
// gamepadmap.cpp
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "gamepadmap.h"
#include "inputcontrolenum.h"
#include "xmlreader.h"

//==============================================================================
// BGamepadMapItem::BGamepadMapItem
//==============================================================================
BGamepadMapItem::BGamepadMapItem() :
   mBaseControlType(-1),
   mControlIndex(-1),
   mMinVal(0),
   mMaxVal(0)
{
}

//==============================================================================
// BGamepadMapItem::setup
//==============================================================================
bool BGamepadMapItem::setup(BXMLNode node)
{
   long index;

   BSimString baseTypeName;
   if(node.getAttribValue("baseType", &baseTypeName))
   {
      index=lookupBaseControl(baseTypeName.getPtr());
      if(index!=-1)
         mBaseControlType=index;
   }

   BSimString povDirName;
   if(node.getAttribValue("povDir", &povDirName))
   {
      index=lookupPovDir(povDirName.getPtr());
      if(index!=-1)
         mPovDir=index;
   }

   node.getAttribValueAsLong("index", mControlIndex);
   node.getAttribValueAsLong("minVal", mMinVal);
   node.getAttribValueAsLong("maxVal", mMaxVal);

   return true;
}

//==============================================================================
// BGamepadMapItem::translate
//==============================================================================
float BGamepadMapItem::translate(XINPUT_STATE& state)
{
//-- FIXING PREFIX BUG ID 7778
   const SHORT* sVal=NULL;
//--
   switch(mBaseControlType)
   {
      case cBaseControlX: sVal=&(state.Gamepad.sThumbLX); break;
      case cBaseControlY: sVal=&(state.Gamepad.sThumbLY); break;
      case cBaseControlRX: sVal=&(state.Gamepad.sThumbRX); break;
      case cBaseControlRY: sVal=&(state.Gamepad.sThumbRY); break;
   }
   if(sVal)
   {
      if(mMaxVal<mMinVal)
      {
         if(*sVal<mMinVal)
            return fabs(((*sVal)-mMinVal)/(float)(mMinVal-mMaxVal));
      }
      else
      {
         if(*sVal>mMinVal)
            return fabs(((*sVal)-mMinVal)/(float)(mMaxVal-mMinVal));
      }
      return 0.0f;
   }

   switch(mBaseControlType)
   {
      case cBaseControlZ:
         return ((float)state.Gamepad.bLeftTrigger)/255.0f;

      case cBaseControlRZ:
         return ((float)state.Gamepad.bRightTrigger)/255.0f;

      case cBaseControlButton:
         if(mControlIndex!=-1)
         {
            if((state.Gamepad.wButtons & mControlIndex)!=0)
               return 1.0f;
         }
         return 0.0f;
   }

   return 0.0f;
}

#ifndef XBOX
//==============================================================================
// BGamepadMapItem::translate
//==============================================================================
float BGamepadMapItem::translate(DIJOYSTATE& state)
{
   LONG* val=NULL;

   switch(mBaseControlType)
   {
      case cBaseControlX: val=&(state.lX); break;
      case cBaseControlY: val=&(state.lY); break;
      case cBaseControlZ: val=&(state.lZ); break;
      case cBaseControlRX:val=&(state.lRx); break;
      case cBaseControlRY:val=&(state.lRy); break;
      case cBaseControlRZ:val=&(state.lRz); break;
      case cBaseControlSlider:
         if(mControlIndex>=0 && mControlIndex<=1)
            val=&(state.rglSlider[mControlIndex]);
         else
            return 0.0f;
   }

   if(val)
   {
      if(mMaxVal<mMinVal)
      {
         if(*val<mMinVal)
            return fabs(((*val)-mMinVal)/(float)(mMinVal-mMaxVal));
      }
      else
      {
         if(*val>mMinVal)
            return fabs(((*val)-mMinVal)/(float)(mMaxVal-mMinVal));
      }
      return 0.0f;
   }

   switch(mBaseControlType)
   {
      case cBaseControlPOV:
         if(mControlIndex>=0 && mControlIndex<=3)
         {
            DWORD pov=state.rgdwPOV[mControlIndex];
            switch(mPovDir)
            {
            case cPovDirUp    : return((pov==0     || pov==4500  || pov==31500) ? 1.0f : 0.0f);
            case cPovDirRight : return((pov==4500  || pov==9000  || pov==3500)  ? 1.0f : 0.0f);
            case cPovDirDown  : return((pov==13500 || pov==18000 || pov==22500) ? 1.0f : 0.0f);
            case cPovDirLeft  : return((pov==22500 || pov==27000 || pov==31500) ? 1.0f : 0.0f);
            }
         }
         break;

      case cBaseControlButton:
         if(mControlIndex!=-1)
         {
            if((state.rgbButtons[mControlIndex]&0x80)!=0)
               return 1.0f;
         }
         break;
   }

   return 0.0f;
}
#endif

//==============================================================================
// BGamepadMap::BGamepadMap
//==============================================================================
BGamepadMap::BGamepadMap() :
   mName()
{
}

//==============================================================================
// BGamepadMap::setup
//==============================================================================
bool BGamepadMap::setup(BXMLNode node)
{
   BSimString str;
   if(!node.getAttribValue("name", &str))
      return false;
   mName=str;

   for(long i=0; i<node.getNumberChildren(); i++)
   {
      BXMLNode controlNode(node.getChild(i));
      if(controlNode.getName()==B("Control"))
      {
         if(controlNode.getAttribValue("name", &str))
         {
            long control=lookupControl(str.getPtr());
            if(control!=-1)
               mItems[control].setup(controlNode);
         }
      }
   }
   return true;
}

//==============================================================================
// BGamepadMap::translate
//==============================================================================
float BGamepadMap::translate(long control, XINPUT_STATE& state)
{
   return mItems[control].translate(state);
}

#ifndef XBOX
//==============================================================================
// BGamepadMap::translate
//==============================================================================
float BGamepadMap::translate(long control, DIJOYSTATE& state)
{
   return mItems[control].translate(state);
}
#endif
