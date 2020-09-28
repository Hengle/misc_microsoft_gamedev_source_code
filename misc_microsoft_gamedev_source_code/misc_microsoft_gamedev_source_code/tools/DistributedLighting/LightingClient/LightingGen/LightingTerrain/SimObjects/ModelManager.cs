
using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using System.Runtime.InteropServices;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using Xceed.FileSystem;
using Xceed.Compression;
using Xceed.Zip;

using Rendering;

namespace Sim
{

   class ModelContainer
   {
      public void destroy()
      {
         mVisualModel.destroy();
         mInstanceOrientations.Clear();

         if(mInstanceVB!=null)
         {
            mInstanceVB.Dispose();
            mInstanceVB = null;
         }
      }
      public string mGR2Name = "";
      public BRenderGrannyMesh mVisualModel;
      public List<Matrix> mInstanceOrientations = new List<Matrix>();
      public VertexBuffer mInstanceVB = null;
   };

   partial class ModelManager
   {
      static List<ModelContainer> mModels = new List<ModelContainer>();
      public static Vector3 mAABBMin = Vector3.Empty;
      public static Vector3 mAABBMax = Vector3.Empty;

      public static ShaderHandle mGPUShader = null;
      static EffectHandle mShaderWVPHandle = null;
      static EffectHandle mShaderViewHandle = null;
      static EffectHandle mShaderProjHandle = null;
      public static EffectHandle s_shaderAOPrevDepthTexHandle;
      

      public enum StreamSourceFrequency
      {
         IndexedObjectData = (1 << 30),
         InstanceData = (2 << 30)
      }

      public static void init()
      {
         loadShader();
      }
      static public void mainShaderParams(string filename)
      {
         mShaderWVPHandle = mGPUShader.getEffectParam("worldViewProj");  
         mShaderViewHandle = mGPUShader.getEffectParam("view");  
         mShaderProjHandle = mGPUShader.getEffectParam("proj");  
         s_shaderAOPrevDepthTexHandle = mGPUShader.getEffectParam("prevDepthTexture");
      }
      static void loadShader()
      {
        // mTerrainGPUShader = BRenderDevice.getShaderManager().getShader(AppDomain.CurrentDomain.BaseDirectory + "\\shaders\\gpuTerrainExport.fx", mainShaderParams);
         mGPUShader =  new ShaderHandle();
         mGPUShader.loadFromString(mShaderString, mainShaderParams);

         mainShaderParams(null);
      }

      public static void destroy()
      {
         for(int i=0;i<mModels.Count;i++)
            mModels[i].destroy();
         mModels.Clear();

         mAABBMin = Vector3.Empty;
         mAABBMax = Vector3.Empty;

          if (mGPUShader!=null)
         {
            BRenderDevice.getShaderManager().freeShader(mGPUShader.mFilename);
            mGPUShader.mShader.Dispose();
            mGPUShader.mShader = null;
            mGPUShader = null;
         }

      }


      public static bool loadModelFromDisk(string filename)
      {
         if (!File.Exists(filename))
            return false;

         ModelContainer model = new ModelContainer();
         model.mGR2Name = filename;         
         model.mVisualModel = GrannyBridge.LoadGR2FromFile(filename);
         
         mModels.Add(model);

         return true;
      }
      public static bool loadModelFromArchive(string filename, AbstractFolder folder)
      {
         AbstractFile file = folder.GetFile(filename);
         if (!file.Exists)
            return false;

         ModelContainer model = new ModelContainer();
         model.mGR2Name = filename;
         {
            //Since granny is unmanaged, we have to allocate a contiguous block of memory for this
            Stream stream = file.OpenRead();
            BinaryReader br = new BinaryReader(stream);

            byte[] fullFule = br.ReadBytes((int)file.Size);

            br.Close();
            stream.Close();

            IntPtr pMarshaledIndexMem = System.Runtime.InteropServices.Marshal.AllocHGlobal(fullFule.Length);
            System.Runtime.InteropServices.Marshal.Copy(fullFule, 0, pMarshaledIndexMem,fullFule.Length);

            model.mVisualModel = GrannyBridge.LoadGR2FromIntPtr(pMarshaledIndexMem, fullFule.Length);

            System.Runtime.InteropServices.Marshal.FreeHGlobal(pMarshaledIndexMem);
         }


         mModels.Add(model);


         return true;
      }
      public static void addInstance(string modelName, Matrix orientation)
      {
          for(int i=0;i<mModels.Count;i++)
          {
             if(mModels[i].mGR2Name == modelName)
             {
                mModels[i].mInstanceOrientations.Add(orientation);
                return;
             }
          }
      }

      public static unsafe void calcModelInstanceBuffers()
      {
         //for (int i = 0; i < mModels.Count; i++)
         //{
         //   int sizeOfBufferInBytes = mModels[i].mInstanceOrientations.Count * 64;// sizeof(Matrix);
         //   mModels[i].mInstanceVB = new VertexBuffer(BRenderDevice.getDevice(), sizeOfBufferInBytes, Usage.WriteOnly, VertexFormats.Texture4 | VertexFormats.Texture5
         //                                                                                                            | VertexFormats.Texture6 | VertexFormats.Texture7, Pool.Managed);
         //   GraphicsStream stream = mModels[i].mInstanceVB.Lock(0, 0, LockFlags.None);

         //   Matrix* matPtr = (Matrix*)stream.InternalDataPointer;

         //   for (int j = 0; j < mModels[i].mInstanceOrientations.Count;j++ )
         //      matPtr[j] = mModels[i].mInstanceOrientations[j];
            
         //   stream.Close();
            
         //   mModels[i].mInstanceVB.Unlock();
         //}

      }
      public static void renderModels(Texture prevDepthBuffer)
      {
         if (mModels.Count == 0)
            return;

         Matrix g_matView = BRenderDevice.getDevice().Transform.View;
         Matrix g_matProj = BRenderDevice.getDevice().Transform.Projection;
         
         mGPUShader.mShader.SetValue(s_shaderAOPrevDepthTexHandle,prevDepthBuffer);

         mGPUShader.mShader.Begin(0);
         mGPUShader.mShader.BeginPass(0);

         for (int i = 0; i < mModels.Count; i++)
         {
            //BRenderDevice.getDevice().SetStreamSourceFrequency(0, (int)StreamSourceFrequency.IndexedObjectData | mModels[i].mInstanceOrientations.Count);

            //BRenderDevice.getDevice().SetStreamSourceFrequency(1, (int)StreamSourceFrequency.InstanceData | 1);
            //BRenderDevice.getDevice().SetStreamSource(1, mModels[i].mInstanceVB, 0, 64);//System.Runtime.InteropServices.Marshal.SizeOf(Matrix));	

            for (int j = 0; j < mModels[i].mInstanceOrientations.Count; j++)
            {
               Matrix worldViewProjection = mModels[i].mInstanceOrientations[j] * g_matView * g_matProj;

               mGPUShader.mShader.SetValue(mShaderWVPHandle, worldViewProjection);

               mGPUShader.mShader.CommitChanges();

               mModels[i].mVisualModel.render();
            }
         }

         mGPUShader.mShader.EndPass();
         mGPUShader.mShader.End();

         BRenderDevice.getDevice().SetStreamSourceFrequency(0, 1);
         BRenderDevice.getDevice().SetStreamSourceFrequency(1, 1);
      }
   };


   partial class ModelManager
   {

      const string mShaderString =
         "float4x4 view			: View;	\n" +
         "float4x4 proj			: Projection;	\n" +
         "float4x4 worldViewProj	: WorldViewProjection;\n" +

"struct VS_OUTPUT_DEPTH{    float4 hposition	: POSITION;        float4 rPos			: TEXCOORD0;}; \n" +

"VS_OUTPUT_DEPTH myvsDepthPeel(   float4 vPosition : POSITION,float3 vNormal : NORMAL,float2 vUV : TEXCOORD0)\n" +
"{\n" +
"    VS_OUTPUT_DEPTH OUT;\n" +
      "	float4 tPos = mul( vPosition, worldViewProj );\n" +
      "	OUT.hposition = tPos;\n" +
      "	OUT.rPos = tPos;\n" +
      "	return OUT;\n" +
"}\n" +


"texture prevDepthTexture;\n" +
"sampler prevDepthSampler = sampler_state\n" +
"{\n" +
"    Texture   = (prevDepthTexture);\n" +
"    MipFilter = Point;\n" +
"    MinFilter = Point;\n" +
"    MagFilter = Point;\n" +
"    AddressU = WRAP;\n" +
"    AddressV = WRAP;\n" +
"    AddressW = CLAMP;\n" +
"};\n" +
"float gWindowWidth = 256;\n" +
"float4 mypsDepthPeel( VS_OUTPUT_DEPTH IN ) : COLOR\n" +
"{\n" +
"	float4 rasterPt = IN.rPos / IN.rPos.w;\n" +
"	rasterPt.xy = 0.5f * rasterPt.xy  + float2(0.5f,0.5f);\n" +
"	rasterPt.y = 1-rasterPt.y;\n" +
"    float myDepth =  IN.rPos.z / IN.rPos.w;\n" +
"    float prevDepth = tex2D(prevDepthSampler, rasterPt.xy);\n" +


    //clip me if i'm farther than previous depth
         //CLM this was 0.07 first, moving it to 0.01 made the entire thing darker 
         //althoug this may add extra passes to the depth peeling, causing more overhead (it was moved to 0.07 beacuse there were depth fragments that "SHOULD" not cause an extra pass, causing an extra pass..)
"    float zBufPrecision = 0.01f; \n" +
"    clip(prevDepth - (myDepth+zBufPrecision));\n" +
 "   return myDepth;\n" +
"}\n" +

"float4 mypsDepthOnly( VS_OUTPUT_DEPTH IN ) : COLOR\n" +
"{\n" +
"	float myDepth =  IN.rPos.z / IN.rPos.w;\n" +

 "   return myDepth;\n" +
"}\n" +


"technique Technique0 {pass DepthPeelForAOGen{VertexShader = compile vs_3_0 myvsDepthPeel();PixelShader  = compile ps_3_0 mypsDepthPeel();}  }\n";



   };
}