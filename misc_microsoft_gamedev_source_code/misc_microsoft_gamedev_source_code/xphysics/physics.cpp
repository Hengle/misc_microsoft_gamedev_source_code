//==============================================================================
// physics.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================
 
// Includes
#include "common.h"
#include "physics.h"
#include "physicsworld.h"
//#include "math\quat.h"

//==============================================================================
// Static Definitions

BShapeIDRemapFunc BPhysics::sShapeIDRemapFunc = NULL;
BPhysicsInfoIDRemapFunc BPhysics::sPhysicsInfoIDRemapFunc = NULL;

//==============================================================================
// Defines

//=============================================================================
// Metric / English unit conversion
//=============================================================================
float convertKilogramsToPounds(float kg)
{
   return(kg * 2.20462262f);
}

float convertPoundsToKilograms(float lbs)
{
   return(lbs * 0.45359237f);
}

float convertInchesToMillimeters(float inches)
{
   return(inches * 25.4f);
}

float convertMillimetersToInches(float mm)
{
   return(mm * 0.0393700787f);
}

void getNormalizedQuaternionFromPossiblyNonOrthonormalRotation(hkRotation &, hkQuaternion &);


//==============================================================================
//==============================================================================
customStreamReader::customStreamReader(const char* fname): mbOK(true)
{
   BString filename(fname);

   bool ok=false;
   ok = mFile.openReadOnly(cDirProduction, filename);

   if(!ok)
      mbOK = false;
}

//==============================================================================
//==============================================================================
customStreamReader::customStreamReader(long dirID, const BCHAR_T* fname): mbOK(true)
{
   BString filename(fname);

   bool ok=false;
   ok = mFile.openReadOnly(dirID, filename);

   if(!ok)
      mbOK = false;
}

//==============================================================================
//==============================================================================
customStreamReader::~customStreamReader()
{
   mFile.close();
}

//==============================================================================
//==============================================================================
hkBool  customStreamReader::isOk () const
{
   if (mFile.getLastError() != BFILE_NO_ERROR)
      return (false);

   return (mbOK);
}

//==============================================================================
//==============================================================================
int  customStreamReader::read(void *buf, int nbytes)
{
   int bytesRead = int(mFile.readEx(buf, nbytes));
   return(bytesRead);
}

//==============================================================================
//==============================================================================
int  customStreamReader::skip (int nbytes)
{

   //-- just read into a local buffer
   static char buf[512];


   int bytesRemaining = nbytes;
   int numToRead = 0;
   int totalRead = 0;

   while (bytesRemaining > 0)
   {
      if (bytesRemaining > 512)
      {
         numToRead = 512;
      }
      else
      {
         numToRead = bytesRemaining;
      }

       int bytesRead = int(mFile.readEx(buf, numToRead));

       if (bytesRead <= 0)
       {
          if (numToRead > 0)
             mbOK = false;

          break;
       }

       totalRead += bytesRead;
       bytesRemaining -= bytesRead;
      
   }
  
   return(totalRead);
}

//==============================================================================
//==============================================================================
hkBool  customStreamReader::seekTellSupported () const
{
   return (true);
}

//==============================================================================
//==============================================================================
hkResult  customStreamReader::seek (int offset, SeekWhence whence)
{
   bool ok = false;
   switch(whence)
   {
   case STREAM_SET:
      ok = mFile.setOffset(offset, BFILE_OFFSET_BEGIN);
      break;

   case STREAM_CUR:
      ok = mFile.setOffset(offset, BFILE_OFFSET_CURRENT);
      break;

   case STREAM_END:
      ok = mFile.setOffset(offset, BFILE_OFFSET_END);
      break;
   }

   if(!ok)
      return(HK_FAILURE);
   return(HK_SUCCESS);
}

//==============================================================================
//==============================================================================
int customStreamReader::tell () const
{
   DWORD offset = 0;
   mFile.getOffset(offset);
   return(int(offset));
}

//==============================================================================
//==============================================================================
customStreamWriter::customStreamWriter(const char* fname) : mbOK(true)
{
 
   BSimString filename(fname);

   bool ok=false;
   ok = mFile.openWriteable(cDirProduction, filename, BFILE_OPEN_ENABLE_BUFFERING);
 
   if(!ok)
      mbOK = false;
}

//==============================================================================
//==============================================================================
customStreamWriter::customStreamWriter(long dirID, const BCHAR_T* fname): mbOK(true)
{

   BSimString filename(fname);

   bool ok=false;
   ok = mFile.openWriteable(dirID, filename, BFILE_OPEN_ENABLE_BUFFERING);

   if(!ok)
      mbOK = false;
}

//==============================================================================
//==============================================================================
customStreamWriter::~customStreamWriter()
{
   mFile.close();
}

//==============================================================================
//==============================================================================
hkBool  customStreamWriter::isOk () const
{
   if (mFile.getLastError() != BFILE_NO_ERROR)
      return (false);

   return (mbOK);

}

//==============================================================================
//==============================================================================
int  customStreamWriter::write (const void *buf, int nbytes)
{
   bool ok = mFile.write(buf, nbytes);
   if (ok)
      return (nbytes);
   
   return (0);
}

//==============================================================================
//==============================================================================
void  customStreamWriter::flush ()
{
   //noop for now
}

//==============================================================================
//==============================================================================
hkBool  customStreamWriter::seekTellSupported () const
{
   return (true);
}

//==============================================================================
//==============================================================================
hkResult  customStreamWriter::seek (int offset, SeekWhence whence)
{
   bool ok = false;
   switch(whence)
   {
   case STREAM_SET:
      ok = mFile.setOffset(offset, BFILE_OFFSET_BEGIN);
      break;

   case STREAM_CUR:
      ok = mFile.setOffset(offset, BFILE_OFFSET_CURRENT);
      break;

   case STREAM_END:
      ok = mFile.setOffset(offset, BFILE_OFFSET_END);
      break;
   }

   if(!ok)
      return(HK_FAILURE);
   return(HK_SUCCESS);
}
//==============================================================================
//==============================================================================
int  customStreamWriter::tell () const
{
   DWORD offset = 0;
   mFile.getOffset(offset);
   return(int(offset));
}






//==============================================================================
// BPhysics::BPhysics
//==============================================================================
BPhysics::BPhysics(void) :
   mpRenderInterface(NULL)
{
} // BPhysics::BPhysics

//==============================================================================
// BPhysics::~BPhysics
//==============================================================================
BPhysics::~BPhysics(void)
{
   shutdown();
} // BPhysics::~BPhysics


//==============================================================================
// BPhysics::setup
//==============================================================================
bool BPhysics::setup(void)
{
   return(true);
} 

//==============================================================================
// BPhysics::shutdown
//==============================================================================
bool BPhysics::shutdown(void)
{
   if (mpRenderInterface)
   {
      delete mpRenderInterface;
      mpRenderInterface = NULL;
   }
   return(true);
}


//==============================================================================
// BPhysics::convertModelSpacePoint
//==============================================================================
void BPhysics::convertModelSpacePoint(const hkVector4& in, BVector &out)
{
   #ifdef HANDEDNESS_FLIP
      out.set(in(2), in(1), in(0));
   #else
      out.set(in(0), in(1), in(2));
   #endif
}

//==============================================================================
// BPhysics::convertModelSpacePoint
//==============================================================================
void BPhysics::convertModelSpacePoint(const BVector& in, hkVector4 &out)
{
   #ifdef HANDEDNESS_FLIP
      out.set(in.z, in.y, in.x, 0.0f);
   #else
      out.set(in.x, in.y, in.z, 0.0f);
   #endif
}

//==============================================================================
// BPhysics::convertModelSpaceRotation
//==============================================================================
void BPhysics::convertModelSpaceRotation(const hkTransform &in,  BPhysicsMatrix &out)
{
   //-- OK, this is some whacky code that runs slow, but 
   //-- it's only used for debug info right now.
   //-- this could all be done with the right matrix multiply (after the quat conversion)
   //-- but I am too tired to figure this out right now... WMJ
   //-- WMJ [9/27/2004]

   //-- get the quaternion for this rotation
   hkQuaternion quat;
   quat.set(in.getRotation());

   BPhysicsQuat *pQuat = (BPhysicsQuat*) &quat;
   out.makeRotationQuat(*pQuat);

   #ifdef HANDEDNESS_FLIP
      //-- switch to right handed system
      out._13 = -out._13;
      out._23 = -out._23;
      out._33 = -out._33;

      static BVector forward, right, up;
      out.getForward(forward);
      out.getRight(right);
      out.getUp(up);
      
      static BVector newForward, newRight, newUp;
      newForward.set(-right.z, right.y, right.x);
      newUp.set(-up.z, up.y, up.x);
      newRight.set(-forward.z, forward.y, forward.x);

      out.setForward(newForward);
      out.setUp(newUp);
      out.setRight(newRight);
   #endif
}


//==============================================================================
// BPhysics::convertPoint
//==============================================================================
void BPhysics::convertPoint(const hkVector4& in, BVector &out) 
{
   #ifdef HANDEDNESS_FLIP
      out.set(in(0), in(1), -in(2));
   #else
      out.set(in(0), in(1), in(2));
   #endif
}

//==============================================================================
void BPhysics::convertPoint(const BVector& in, hkVector4 &out) 
{
   #ifdef HANDEDNESS_FLIP
      out.set(in.x, in.y, -in.z, 0.0f);
   #else
      out.set(in.x, in.y, in.z, 0.0f);
      if (!out.isOk3())
      {
         BASSERT(0);
         out.setAll(0.0f);
      }
   #endif
}

//==============================================================================
// BPhysics::convertDimension
//==============================================================================
/*
void BPhysics::convertDimension(const hkVector4& in, BVector &out) 
{
   out.set(in(0), in(1), in(2));
}

//==============================================================================
void BPhysics::convertDimension(const BVector& in, hkVector4 &out) 
{
   out.set(in.x, in.y, in.z);
}
*/


//==============================================================================
// BPhysics::convertRotation
//==============================================================================
void BPhysics::convertRotation(const hkRotation &in, BPhysicsMatrix &out) 
{

   //-- get the quaternion for this rotation
   hkQuaternion quat;
   quat.set(in);

   //-- pass to the other conversion function
   convertRotation(quat, out);

}

//==============================================================================
void BPhysics::convertRotation(const hkQuaternion &in, BPhysicsMatrix &out) 
{
   #ifdef HANDEDNESS_FLIP
      BPhysicsQuat *pQuat = (BPhysicsQuat*) &in;
      out.makeRotationQuat(*pQuat);

      //-- switch to right handed system
    
      out._13 = -out._13;
      out._23 = -out._23;
      out._33 = -out._33;

      //-- transform rows
      BVector forward;
      BVector right;

      out.getForward(forward);
      out.getRight(right);
      out.setForward(right);
      out.setRight(forward);
   #else
      BPhysicsQuat *pQuat = (BPhysicsQuat*) &in;
      out.makeRotationQuat(*pQuat);
   #endif
}
/*
//==============================================================================
void BPhysics::convertRotation(const hkQuaternion &in, BQuat &out) 
{
   out.set(in(0), in(1), in(2), in(3)); 
}
*/
/*
//==============================================================================
void BPhysics::convertRotation(const BQuat &quat, hkQuaternion &hkQuat) 
{
   hkQuat = *((hkQuaternion*) (&quat));
}
*/

//==============================================================================
void BPhysics::convertRotation(const BPhysicsMatrix &in, hkRotation &out) 
{
   #ifdef HANDEDNESS_FLIP
      //-- copy the matrix into a hkRotation
      hkVector4 hkForward, hkRight, hkUp;
      hkForward.set(in._31, in._32, -in._33);
      hkForward.normalize3();
      hkRight.set(in._11, in._12, -in._13);
      hkRight.normalize3();
      hkUp.set(in._21, in._22, -in._23);
      hkUp.normalize3();

      out.setCols(hkForward, hkUp, hkRight);
   #else
      //-- copy the matrix into a hkRotation
      hkVector4 hkForward, hkRight, hkUp;
      hkForward.set(in._31, in._32, in._33);
      hkForward.normalize3();
      hkRight.set(in._11, in._12, in._13);
      hkRight.normalize3();
      hkUp.set(in._21, in._22, in._23);
      hkUp.normalize3();

      out.setCols(hkRight, hkUp, hkForward);

      //CLM[12.06.07] hkRot can become QNAN here if IN is all 0s
      if(!out.isOk())
      {
         BASSERT(0);
         out.setIdentity();
      }

   #endif
}

//==============================================================================
bool BPhysics::convertStringToHKVector(const char* szString, hkVector4 &vec)
{
   if (!szString)
      return (false);

   float x,y,z,w;
   w = 0;

   //-- we may not get a w... that's ok
   if(sscanf_s(szString, "%f %f %f %f", &x, &y, &z, &w) < 3)
   {
      if (sscanf_s(szString, "(%f %f %f %f)", &x, &y, &z, &w) < 3)
         return (false);
   }
   
   vec.set(x,y,z,w);
   return true;
}

//==============================================================================
bool BPhysics::convertStringToHKTransform(const char* szString, hkTransform &t)
{
   if (!szString)
      return (false);

   hkVector4 hkv1, hkv2, hkv3, hkv4;
   
   float x1,x2,x3,x4;
   float y1,y2,y3,y4;
   float z1,z2,z3,z4;
   //float w1,w2,w3,w4;

 
   //-- we may not get a w... that's ok
   if(sscanf_s(szString, "(%f %f %f)(%f %f %f)(%f %f %f)(%f %f %f)", 
      &x1, &y1, &z1, 
      &x2, &y2, &z2, 
      &x3, &y3, &z3, 
      &x4, &y4, &z4) 
      < 12)
   {
      return (false);
   }

   hkv1.set(x1,x2,x3,x4);
   hkv2.set(y1,y2,y3,y4);
   hkv3.set(z1,z2,z3,z4);
   hkv4.set(0.0f,0.0f,0.0f,1.0f);

   t.setRows4(hkv1, hkv2, hkv3, hkv4);

#if 1
   // Make sure the rotation is orthonormal.
   hkQuaternion quat;
   hkRotation rotation = t.getRotation();
   getNormalizedQuaternionFromPossiblyNonOrthonormalRotation(rotation, quat);
   t.setRotation(quat);
#endif
  
   return true;
}



//==============================================================================
// BPhysics::createWorld
//==============================================================================
BPhysicsWorld *BPhysics::createWorld(void)
{
   BPhysicsWorld *world = new BPhysicsWorld;
   return(world);
}


//==============================================================================
// BPhysics::releaseWorld
//==============================================================================
void BPhysics::releaseWorld(BPhysicsWorld *world)
{
   delete world;
}




void getNormalizedQuaternionFromPossiblyNonOrthonormalRotation(hkRotation &r, hkQuaternion &q)
{
	hkReal trace = r(0,0) + r(1,1) + r(2,2);

	if( trace > 0 )
	{
		hkReal s = hkMath::sqrt( trace + 1.0f );
		hkReal t = 0.5f / s;
		q.m_vec(0) = (r(2,1)-r(1,2)) * t;
		q.m_vec(1) = (r(0,2)-r(2,0)) * t;
		q.m_vec(2) = (r(1,0)-r(0,1)) * t;
		q.m_vec(3) = 0.5f*s;
	}
	else
	{
		const int next[] = {1,2,0};
		int i=0;

		if(r(1,1) > r(0,0)) i=1;
		if(r(2,2) > r(i,i)) i=2;

		int j = next[i];
		int k = next[j];

		hkReal s = hkMath::sqrt(r(i,i) - (r(j,j)+r(k,k)) + 1.0f);
		hkReal t = 0.5f / s;

		q.m_vec(i) = 0.5f * s;
		q.m_vec(3) = (r(k,j)-r(j,k)) * t;
		q.m_vec(j) = (r(j,i)+r(i,j)) * t;
		q.m_vec(k) = (r(k,i)+r(i,k)) * t;
	}

	q.normalize();

}

//==============================================================================
// eof: physics.cpp
//==============================================================================
