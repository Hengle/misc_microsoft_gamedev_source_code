
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
float4 fetchColor1(int index)
{
   float4 color;
   asm
   {
      vfetch color, index, color1;
   };
   return color;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord0(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord0;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord1(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord1;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord2(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord2;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord3(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord3;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord4(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord4;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord5(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord5;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord6(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord6;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord7(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord7;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord8(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord8;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord9(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord9;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord10(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord10;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord11(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord11;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord12(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord12;
   };
   return texCoord;
}