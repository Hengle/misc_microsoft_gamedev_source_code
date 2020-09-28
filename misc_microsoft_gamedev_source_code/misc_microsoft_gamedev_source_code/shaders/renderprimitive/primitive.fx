#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"

const float  cCornerMultiplier[4] = {1,-1,-1, 1};
float4x4 gScaleMatrix;
float4   gColor= float4(1,1,1,1);
float    gThickness= 1.0f;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchPosition(int index)
{
   float4 pos;
   asm
   {
      vfetch pos, index, position0
   };
   return pos;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchColor0(int index)
{
   float4 color;
   asm
   {
      vfetch color, index, color0;
   };
   return color;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int getCornerIndex(int vertIndex)
{
   int iDiv = (vertIndex+0.5f) / 4; // add 0.5f to avoid rounding errors
   int iMod = vertIndex - (iDiv * 4);
   return iMod;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float3 ComputeCornerPos(float3 pos, float3 forward, float3 up, float scale, int index)
{
   int cornerIndex = getCornerIndex(index);
   float cornerMultiplier = cCornerMultiplier[cornerIndex];
   float3 rightV = cross(up, forward);

   rightV *= cornerMultiplier;
   rightV *= scale;
   
   float3 finalPos = pos + rightV;
   return finalPos;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PrimitiveVS(   in  int Index          : INDEX
                   ,out float4 OutPosition : POSITION
                   ,out float4 OutColor    : COLOR0
                   )
{
   int vertexIndex = (floor((Index+0.5f)/2));
   //int remainderDivBy2 = Index - (vertexIndex * 2);
   int vertexIndexOdd = vertexIndex % 2;

   float4 InPosition = fetchPosition(vertexIndex);
#ifdef USE_COLOR
   float4 InColor    = fetchColor0(vertexIndex);
#endif

   float3 forward = float3(0,0,1);
   
   //-- if its the first trail poly we use the next vertex position to figure out our 
   //-- forward vector
   if (vertexIndexOdd <= 0)
   {
      //-- fetch the last position for a trail
      float4 nextPosition = fetchPosition(vertexIndex+1);
      forward = normalize(nextPosition.xyz - InPosition.xyz);
   } 
   else //-- we had a previous position use that to compute our forward vector
   {
      float4 InLastPosition = fetchPosition(max(0, vertexIndex-1));
      forward = normalize(InPosition.xyz - InLastPosition.xyz);
   }
   
   float scale = InPosition.w;
#ifdef USE_THICKNESS_OVERRIDE
   scale = gThickness;
#endif 
   InPosition.w = 1.0f;
   OutPosition.xyz = ComputeCornerPos(InPosition, forward, float3(0,1,0), scale, Index);
   OutPosition.w = 1.0f;

#ifdef USE_SCALEMATRIX
   OutPosition = mul(OutPosition, gScaleMatrix);
#endif
   OutPosition = mul(OutPosition, gWorldToProj);

   OutColor    = gColor;
#ifdef USE_COLOR
   OutColor   *= InColor;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PrimitivePS(  in  float4 InColor0    : COLOR0
                  ,out float4 OutColor    : COLOR0                 
                  )
{
   OutColor = InColor0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
technique Default
{
   pass AlphaBlend
   {
      VertexShader      = compile vs_3_0 PrimitiveVS();
      PixelShader       = compile ps_3_0 PrimitivePS();

      //-- RS States      
      ZWriteEnable      = FALSE;
      AlphaBlendEnable  = TRUE;
      AlphaTestEnable   = TRUE;
      SrcBlend          = SRCALPHA;
      DestBlend         = INVSRCALPHA;
      BlendOp           = ADD;
      AlphaRef          = 0;
      CullMode          = NONE;
   }

   pass Additive
   {
      VertexShader      = compile vs_3_0 PrimitiveVS(); 
      PixelShader       = compile ps_3_0 PrimitivePS();

      ZWriteEnable      = FALSE;
      AlphaBlendEnable  = TRUE;
      AlphaTestEnable   = TRUE;
      SrcBlend          = ONE;
      DestBlend         = ONE;
      BlendOp           = ADD;
      AlphaRef          = 0;
      CullMode          = NONE;
   }

   pass Subtractive
   {
      VertexShader = compile vs_3_0 PrimitiveVS();
      PixelShader  = compile ps_3_0 PrimitivePS();

      ZWriteEnable      = FALSE;
      AlphaBlendEnable  = TRUE;
      AlphaTestEnable   = TRUE;
      SrcBlend          = SRCALPHA;
      DestBlend         = ONE;
      BlendOp           = REVSUBTRACT;
      AlphaRef          = 0;
      AlphaFunc         = GREATEREQUAL;
      CullMode          = NONE;
   }    
}