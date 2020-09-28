//============================================================================
// particlevertextypes.h
// Ensemble Studios (C) 2006
//============================================================================
#pragma once
struct BPVertexPT
{  
   XMFLOAT4 mPos;      // position and tumble angle stored in one vector    
   XMHALF4  mScale;    //-- scale [x,y], [z] = birth time, [w] random value
   XMHALF4  mVelocity; //-- velocity of the particle
   XMHALF2  mLife;     //-- x = life alpha; y = intensity;  zw = UNUSED   
   XMCOLOR  mColor;
   XMCOLOR  mTextureZ; //-- z lookup values r=diffuse1 g=diffuse2 b=diffuse3 a=intensity
   static IDirect3DVertexDeclaration9* msVertexDecl;
};

//-- we can't compress the trail positions because we get too much numerical error in the shaders when we do that 
//-- which makes the trails look terrible
struct BPTrailVertexPT
{  
   XMFLOAT4 mPos;      // position and tumble angle stored in one vector
   XMHALF4  mScale;    //-- scale [x,y], texture array index[z]
   XMHALF4  mUp;       // up vector
   XMHALF2  mLife;     //-- x = life alpha yzw - unused for now   
   XMCOLOR  mColor;
   XMCOLOR  mTextureZ; //-- z lookup values r=diffuse1 g=diffuse2 b=diffuse3 a=intensity
   static IDirect3DVertexDeclaration9* msVertexDecl;
};

struct BPBeamVertexPT
{  
   XMFLOAT4 mPos;      // position and tumble angle stored in one vector
   XMHALF4  mScale;    //-- scale [x,y], texture array index[z]
   XMHALF4  mUp;       // up vector
   XMHALF4  mLife;     //-- x = life alpha y = intensity value , z = distance alpha  w = unused
   XMCOLOR  mColor;
   XMCOLOR  mTextureZ; //-- z lookup values r=diffuse1 g=diffuse2 b=diffuse3 a=intensity
   static IDirect3DVertexDeclaration9* msVertexDecl;
};

void initParticleVertexDeclarations();
void deInitParticleVertexDeclarations();
