// skinning.inc

float4 gInstanceData[4]                   : register(INSTANCE_DATA_REG);
float4 gInstanceControl                   : register(INSTANCE_CONTROL_REG);

bool gOneBone                             : register(ONE_BONE_REG);
bool gFourBones                           : register(FOUR_BONES_REG);

static void RetrieveMatrix(float matrixIndex, out float3 rot0, out float3 rot1, out float3 rot2, out float3 tran)
{
   float4 fx, fy, fz, fw;
   
   asm
   {
      vfetch fx, matrixIndex, texcoord4, UseTextureCache=true
      vfetch fy, matrixIndex, texcoord5, UseTextureCache=true
      vfetch fz, matrixIndex, texcoord6, UseTextureCache=true
      vfetch fw, matrixIndex, texcoord7, UseTextureCache=true
   };
        
   rot0 = fx;
   rot1 = fy;
   rot2 = float3(fz.x, fz.y, fx.w);
   tran = fw;
}

static void CalcBindToWorldMatrix(float instanceBoneMatrices, int4 Indices, float4 Weights, out float3 rot0, out float3 rot1, out float3 rot2, out float3 tran)
{
   RetrieveMatrix(Indices[0] + instanceBoneMatrices, rot0, rot1, rot2, tran);
   
   if (!gOneBone)
   {
      float3 tempRot0;
      float3 tempRot1;
      float3 tempRot2;
      float3 tempTran;

      RetrieveMatrix(Indices[1] + instanceBoneMatrices, tempRot0, tempRot1, tempRot2, tempTran);   
      
      rot0 = rot0 * Weights[0] + tempRot0 * Weights[1];
      rot1 = rot1 * Weights[0] + tempRot1 * Weights[1];
      rot2 = rot2 * Weights[0] + tempRot2 * Weights[1];
      tran = tran * Weights[0] + tempTran * Weights[1];
      
      if (gFourBones)
      {
         RetrieveMatrix(Indices[2] + instanceBoneMatrices, tempRot0, tempRot1, tempRot2, tempTran);   
         
         rot0 += tempRot0 * Weights[2];
         rot1 += tempRot1 * Weights[2];
         rot2 += tempRot2 * Weights[2];
         tran += tempTran * Weights[2];
         
         RetrieveMatrix(Indices[3] + instanceBoneMatrices, tempRot0, tempRot1, tempRot2, tempTran);   
         
         rot0 += tempRot0 * Weights[3];
         rot1 += tempRot1 * Weights[3];
         rot2 += tempRot2 * Weights[3];
         tran += tempTran * Weights[3];
      }
   }
}

