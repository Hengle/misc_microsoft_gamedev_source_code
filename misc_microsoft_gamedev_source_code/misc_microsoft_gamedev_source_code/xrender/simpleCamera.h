//============================================================================
//
//  simpleCamera.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "math/generalMatrix.h"

class BSimpleCameraXInputPolicy
{
public:
   enum eButton
   {
      cButtonRotLeft,
      cButtonRotRight,
      cButtonRotDown,
      cButtonRotUp,
      cButtonRotCW,
      cButtonRotCCW,
      cButtonMoveForward,
      cButtonMoveBackward,
      cButtonMoveUp,
      cButtonMoveDown,
      cButtonMoveLeft,
      cButtonMoveRight,
      cButtonControl,
      cButtonReset,
      cButtonShift,
      
      cButtonMax,
   };

   BSimpleCameraXInputPolicy()
   {
      Utils::ClearObj(mState);
   }
   
   void update(void)
   {
      XInputGetState(0, &mState);
   }
   
   bool getButtonDown(eButton button)
   {
      switch (button)
      {
         case cButtonRotLeft:       return 0 != (mState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
         case cButtonRotRight:      return 0 != (mState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
         case cButtonRotDown:       return 0 != (mState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
         case cButtonRotUp:         return 0 != (mState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP);
         case cButtonRotCW:         return false;
         case cButtonRotCCW:        return false;
         case cButtonMoveForward:   return mState.Gamepad.sThumbRY < -16384;
         case cButtonMoveBackward:  return mState.Gamepad.sThumbRY > 16384;
         case cButtonMoveUp:        return mState.Gamepad.sThumbLY < -16384;
         case cButtonMoveDown:      return mState.Gamepad.sThumbLY > 16384;
         case cButtonMoveLeft:      return mState.Gamepad.sThumbRX < -16384;
         case cButtonMoveRight:     return mState.Gamepad.sThumbRX > 16384;
         case cButtonControl:       return mState.Gamepad.bRightTrigger > 128;
         case cButtonReset:         return 0 != (mState.Gamepad.wButtons & XINPUT_GAMEPAD_A);
         case cButtonShift:         return false;
      }
      
      return false;         
   }
   
   void getMotion(float& x, float& y)
   {
   }
      
private:
   XINPUT_STATE mState;
};

template <class InputPolicy> 
class BSimpleCamera
{
public:
   BSimpleCamera(InputPolicy& inputPolicy) : 
      mInputPolicy(inputPolicy),
      mControlDown(false)
   {
      mPos.setZero();
      mAng.setZero();
      mFrame.setIdentity();
      updateMatrices();
   }
   
   int blah;
   
   InputPolicy& getInputPolicy(void) const { return mInputPolicy; }

   BMatrix44 getWorldToView(void) const { return mFrame * mWorldView; }
   
   void tick(float deltaT, const BVec4& camOfs = BVec4(0.0f))
   {
      if ((mControlDown) && (!mInputPolicy.getButtonDown(InputPolicy::cButtonControl)))
         setFrame();
      else if (mInputPolicy.getButtonDown(InputPolicy::cButtonReset))
         reset();
      else
      {
         tickOfs(deltaT, camOfs);
         tickAng(deltaT);
      }

      mControlDown = mInputPolicy.getButtonDown(InputPolicy::cButtonControl);
      
      updateMatrices();
   }

   void reset(void)
   {  
      mFrame.setIdentity();
      mAng.setZero();
      mPos.setZero();
      updateMatrices();
   }
         
protected:
   BMatrix44 mFrame;
   BMatrix44 mWorldView;
   BVec4 mPos;
   BVec3 mAng;
   bool mControlDown;
   InputPolicy& mInputPolicy;

   void setFrame(void)
   {
      mFrame = mFrame * mWorldView;
      mFrame.orthonormalize();

      mPos.setZero();
      mAng.setZero();
      mWorldView.setIdentity();
   }

   void tickAng(float deltaT)
   {  
      if (mInputPolicy.getButtonDown(InputPolicy::cButtonRotLeft))    
         mAng[1] -= Math::fDegToRad(20.0f) * deltaT;
      if (mInputPolicy.getButtonDown(InputPolicy::cButtonRotRight))   
         mAng[1] += Math::fDegToRad(20.0f) * deltaT;

      if (mInputPolicy.getButtonDown(InputPolicy::cButtonRotDown))    
         mAng[0] += Math::fDegToRad(20.0f) * deltaT;
      if (mInputPolicy.getButtonDown(InputPolicy::cButtonRotUp))      
         mAng[0] -= Math::fDegToRad(20.0f) * deltaT;

      if (mInputPolicy.getButtonDown(InputPolicy::cButtonRotCW))      
         mAng[2] -= Math::fDegToRad(20.0f) * deltaT;
      if (mInputPolicy.getButtonDown(InputPolicy::cButtonRotCCW))     
         mAng[2] += Math::fDegToRad(20.0f) * deltaT;

      float x = 0.0f, y = 0.0f;
      mInputPolicy.getMotion(x, y);

      if (mInputPolicy.getButtonDown(InputPolicy::cButtonControl))
         mAng[2] -= x * Math::fDegToRad(.55f);// * deltaT;
      else
         mAng[1] -= x * Math::fDegToRad(.55f);// * deltaT;

      mAng[0] -= y * Math::fDegToRad(.55f);// * deltaT;
   }

   void tickOfs(float deltaT, const BVec4& camOfs)
   {
      BVec4 ofs(0.0f);

      if (mInputPolicy.getButtonDown(InputPolicy::cButtonMoveForward))
         ofs[2] += 5500.0f * deltaT;
      else if (mInputPolicy.getButtonDown(InputPolicy::cButtonMoveBackward))
         ofs[2] += -5500.0f * deltaT;

      if (mInputPolicy.getButtonDown(InputPolicy::cButtonMoveLeft)) 
         ofs[0] = -2500.0f * deltaT;
      if (mInputPolicy.getButtonDown(InputPolicy::cButtonMoveRight)) 
         ofs[0] = 2500.0f * deltaT;

      if (mInputPolicy.getButtonDown(InputPolicy::cButtonMoveDown)) 
         ofs[1] = -2500.0f * deltaT;
      if (mInputPolicy.getButtonDown(InputPolicy::cButtonMoveUp)) 
         ofs[1] = 2500.0f * deltaT;

      if (mInputPolicy.getButtonDown(InputPolicy::cButtonShift))
         ofs *= 1.0f / 4000.0f;
      else
         ofs *= 1.0f / 2000.0f;
      
      ofs += camOfs;

      //if (RightHanded)
         ofs[2] *= -1.0f;

      mPos += BMatrix44::transformVector(ofs, mWorldView.inverse());
   }

   void updateMatrices(void)
   {
      BMatrix44 x(BMatrix44::makeRotate(0, mAng[0]));
      BMatrix44 y(BMatrix44::makeRotate(1, mAng[1]));
      BMatrix44 z(BMatrix44::makeRotate(2, mAng[2]));
      BMatrix44 t(BMatrix44::makeTranslate((-mPos).toPoint()));

      mWorldView = t * y * x * z;
   }   
};
