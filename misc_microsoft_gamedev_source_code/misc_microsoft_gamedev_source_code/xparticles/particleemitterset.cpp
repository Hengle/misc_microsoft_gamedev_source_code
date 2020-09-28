//============================================================================
// particleemitterset.h
//  Copyright (c) 2000-2006 Ensemble Studios
//============================================================================
#include "xparticlescommon.h"
#include "particleemitterset.h"
#include "boundingBox.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BParticleEffect::BParticleEffect():
   mDataIndex(-1),
   mPriority(0)
{
#ifndef BUILD_FINAL
   // variables only to be used for non final builds 
   mTransform = XMMatrixIdentity();
   mSecondaryTransform = XMMatrixIdentity();
   mTintColor = 0xFFFFFFFF;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BParticleEffect::~BParticleEffect()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEffect::updateWorldMatrix(BMatrix worldMatrix, const BMatrix* RESTRICT pLocalMatrix)                                        
{
   ASSERT_RENDER_THREAD
   
   //-- strip the scale out of the passed in world matrix -- we never want any scaling
   BMatrix unscaledWorldMatrix;
   BVector right;
   BVector up;
   BVector forward;
   BVector translation;

   worldMatrix.getRight(right);
   worldMatrix.getUp(up);
   worldMatrix.getForward(forward);
   worldMatrix.getTranslation(translation);

   right.safeNormalize();
   up.safeNormalize();
   forward.safeNormalize();

   unscaledWorldMatrix.makeOrient(forward, up, right);
   unscaledWorldMatrix.setTranslation(translation);
   
   BMatrix matrix;   
   if (pLocalMatrix)
   {

      //-- strip the scale out of the local matrix -- we never want any scaling
      BMatrix localMatrix;
      BVector localRight;
      BVector localUp;
      BVector localForward;
      BVector localTranslation;

      pLocalMatrix->getRight(localRight);
      pLocalMatrix->getUp(localUp);
      pLocalMatrix->getForward(localForward);
      pLocalMatrix->getTranslation(localTranslation);

      localRight.safeNormalize();
      localUp.safeNormalize();
      localForward.safeNormalize();

      localMatrix.makeOrient(localForward, localUp, localRight);
      localMatrix.setTranslation(localTranslation);
                  
      matrix.mult(localMatrix, unscaledWorldMatrix);      
   }
   else 
   {      
      matrix = unscaledWorldMatrix;
   }
   
   for (uint i = 0; i < mEmitters.getSize(); ++i)
   {
      if (mEmitters[i] == NULL)
         continue;

      mEmitters[i]->setTransform(matrix);
   }

#ifndef BUILD_FINAL
   mTransform = matrix;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEffect::setSecondaryTransform(BMatrix transform)
{
   ASSERT_RENDER_THREAD
   for (uint i = 0; i < mEmitters.getSize(); ++i)
   {
      if (mEmitters[i] == NULL)
         continue;

      mEmitters[i]->setSecondaryTransform(transform);
   }

#ifndef BUILD_FINAL
   mSecondaryTransform = transform;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEffect::setFlag(int flag, bool bState)
{
   ASSERT_RENDER_THREAD

   uint count = mEmitters.getSize();
   for (uint i = 0; i < count; ++i)
   {
      if (mEmitters[i] == NULL)
         continue;

      mEmitters[i]->setFlag(flag, bState);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEffect::setTintColor(DWORD color)
{
   ASSERT_RENDER_THREAD

   uint count = mEmitters.getSize();
   for (uint i = 0; i < count; ++i)
   {
      if (mEmitters[i] == NULL)
         continue;

      mEmitters[i]->setTintColor(color);
   }
#ifndef BUILD_FINAL
   mTintColor = color;
#endif
}

