//============================================================================
//
//  ParticleSystemParticle.h
//
//  Copyright (c) 2000 Ensemble Studios
//
//============================================================================
#pragma once

class BParticleEmitter;
class BParticleEmitterDefinition;
class BEmitterTextureSet;
class BParticleEffect;

//----------------------------------------------------------------------------
//  Class BParticleSystemParticle
//----------------------------------------------------------------------------
class BParticleSystemParticle
{
   public:
      enum eParticleState
      {
         eStateDead = 0,
         eStateActive,
         eStateLingering,
         eStateTotal
      };

      //-- Interface
      void  reset                (void);
      void  init                 (BParticleEmitter* RESTRICT pParent, int index, int currentTime, BMatrix emitterMatrix);      
      void  markDead             (void) { mState = eStateDead; };
      bool  isDead               (void) { return (mState == eStateDead); };
      bool  isLingering          (void) { return (mState == eStateLingering); }
      int   sizeOf               (void) {return sizeof(BParticleSystemParticle);}
      void  releaseEffect        (bool bKillImmediately);

      //-- System Data   
      XMVECTOR                   mPosition;
      XMVECTOR                   mVelocity;

      //-- Motion Data            
      XMHALF2                    mTumbleParams;  // x == angle    
                                                 // y == velocity
      int                        mBirthTime;
      int                        mDeathTime;
      float                      mFBirthTime;
      float                      mFOOLifeTime;

      
      XMCOLOR                    mTextureArrayZ; // r = Diffuse 1
                                                 // g = Diffuse 2
                                                 // b = Diffuse 3
                                                 // a = Intensity
      
      BParticleEffect*           mpEffect;
      WORD                       mVarience;   //-- Variance Index (used for random progressions)
      BYTE                       mState;
      
   private:
      //-- Private Init Functions      
      void  initPosition      (const BParticleEmitter* RESTRICT pParent, const BParticleEmitterDefinition* RESTRICT pData);
      void  initVelocity      (const BParticleEmitterDefinition* RESTRICT pData);
      void  initTumble        (void);
      void  initAppearance    (const BParticleEmitterDefinition* pData);      
      BYTE  initTextureArrayZ (const BEmitterTextureSet* pTextureSet);
      void  initFinish        (BParticleEmitter* RESTRICT pParent, const BParticleEmitterDefinition* RESTRICT pData, BMatrix emitterMatrix);
      void  initEffect        (BParticleEmitter* RESTRICT pParent, const BParticleEmitterDefinition* RESTRICT pData, BMatrix emitterMatrix);

      int   computeDeathTime  (const BParticleEmitterDefinition* RESTRICT pEmitterData) const;

      void  initTrail         (BParticleEmitter* RESTRICT pParent,  const BParticleEmitterDefinition* RESTRICT pData, BMatrix emitterMatrix);

      //-- Private Update Functions
      

      //-- Private Helper Functions
      void  chooseEmissionPoint(const BParticleEmitter* RESTRICT pParent, const BParticleEmitterDefinition* RESTRICT pData, XMVECTOR* pPos);
};

DEFINE_BUILT_IN_TYPE(BParticleSystemParticle);




