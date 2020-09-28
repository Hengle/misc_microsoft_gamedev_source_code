using System;
using System.Drawing;
using System.Collections.Generic;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Text;
using System.IO;
using System.Collections;
 
using System.Text.RegularExpressions;

using EditorCore;
using Rendering;

//CLM : THIS FILE SERVES AS A REPLACEMENT FOR GRANNY.H in C++
namespace ModelSystem
{
   
   public class GrannyBridge
   {
      static public unsafe BRenderGrannyMesh LoadGR2(string filename)
      {
         BRenderGrannyMesh mesh = new BRenderGrannyMesh();
         //granny_file* grannyFile = null;
         granny_file_info *file_info = null;

         if (!File.Exists(filename))
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("GrannyFile {0} was not found", filename));
            return null;
         }

         granny_file* grannyFile = (granny_file*)GrannyReadEntireFile(filename);
         if (grannyFile != null)
         {
            // It's a file Granny can load (but might be just a raw bunch of bits).
            file_info = GrannyGetFileInfo((IntPtr)grannyFile);
            if (file_info != null)
            {
               //we need to transform the file here...
               mGR2Path = Path.GetDirectoryName(filename);



               //Vector3 Origin = new Vector3( 0, 0, 0 );
               //Vector3 RightVector = new Vector3(  1, 0, 0 );
               //Vector3 UpVector = new Vector3(  0, 1, 0 );
               //Vector3 BackVector = new Vector3(  0, 0, 1 );


               //// Tell Granny to construct the transform from the file's coordinate
               //// system to our coordinate system
               //Vector3 Affine3 = new Vector3();
               //float [] Linear3x3=new float[9];
               //float[] InverseLinear3x3 = new float[9];




               ////Oh noes, we don't need this no more.

               //fixed (float* lfp = Linear3x3)
               //{
               //   fixed (float* ilfp = InverseLinear3x3)
               //   {
               //      GrannyComputeBasisConversion(file_info, 1/*UnitsPerMeter*/, ref Origin, ref RightVector, ref UpVector, ref BackVector, ref Affine3, lfp, ilfp);


               //      // Tell Granny to transform the file into our coordinate system
               //      GrannyTransformFile(file_info, ref Affine3, lfp, ilfp,
               //         1e-5f, 1e-5f,
               //         GrannyRenormalizeNormals | GrannyReorderTriangleIndices);

               //   }
               //}

               ConvertCoordinateSystem(file_info, true, true);    
          
               //file has been transformed, load our meshes
               createMeshFromGR2(filename, ref mesh,file_info);

               //we're done. Destroy!
               GrannyFreeFile((IntPtr) grannyFile);
               grannyFile = null;

               return mesh;
            }
         }
         return null;
      }

      static public unsafe void ConvertCoordinateSystem(granny_file_info* pGrannyFileInfo, bool model, bool flipWinding)
      {
         //pGrannyFileInfo->ArtToolInfo->FromArtToolName 
         IntPtr strPtr = new IntPtr((byte*)pGrannyFileInfo->ArtToolInfo->FromArtToolName);
         string name = Marshal.PtrToStringAnsi(strPtr);
         //POST_GRANNY_EXPORT_TOOL_NAME "ESPostExport"
         if (name != "ESPostExport")
         {
            Vector3 affine3 = new Vector3();
            float[] linear3x3 = new float[9];
            float[] inverseLinear3x3 = new float[9];

            Vector3 forward = new Vector3( 0, 0, 1 );
            Vector3 right = new Vector3( -1, 0, 0 );
            Vector3 origin = new Vector3(0, 0, 0);
            Vector3 upVector = new Vector3(0, 1, 0);


            fixed (float* lfp = linear3x3)
            {
               fixed (float* ilfp = inverseLinear3x3)
               {
                  GrannyComputeBasisConversion(pGrannyFileInfo, (float)(pGrannyFileInfo->ArtToolInfo->UnitsPerMeter / 64.0),
                       ref origin, ref right, ref upVector, ref forward, ref affine3, lfp, ilfp);

                  if (model)
                  {
                     GrannyTransformFile(pGrannyFileInfo, ref affine3, lfp, ilfp, 1e-5f, 1e-5f, (flipWinding ? GrannyReorderTriangleIndices : 0) | GrannyRenormalizeNormals);
                  }
                  else
                  {
                     //not implemented
                  }
               }
            }
         }
      }

////============================================================================
//// BGrannyManager::convertCooridnateSystem
////============================================================================
//void BGrannyManager::convertCooridnateSystem(granny_file_info* pGrannyFileInfo, bool model, bool flipWinding)
//{
//   if(strcmp(pGrannyFileInfo->ArtToolInfo->FromArtToolName, POST_GRANNY_EXPORT_TOOL_NAME) != 0)
//   {
//      // Transform from the art tool's coordinate system to the game's coordinate system.
//      granny_real32 affine3[3];
//      granny_real32 linear3x3[9];
//      granny_real32 inverseLinear3x3[9];
//      BVector forward =  cZAxisVector;
//      BVector right   = -cXAxisVector;

//      GrannyComputeBasisConversion(pGrannyFileInfo, pGrannyFileInfo->ArtToolInfo->UnitsPerMeter / 64.0f, 
//         (float*)&cOriginVector, (float*)&right, (float*)&cYAxisVector, (float*)&forward,
//         affine3, linear3x3, inverseLinear3x3);

//      if(model)
//      {
//         GrannyTransformFile(pGrannyFileInfo, affine3, linear3x3, inverseLinear3x3, 1e-5f, 1e-5f, (flipWinding ? GrannyReorderTriangleIndices : 0) | GrannyRenormalizeNormals);
//      }
//      else
//      {
//         for(long i=0; i<pGrannyFileInfo->AnimationCount; i++)
//            GrannyTransformAnimation(pGrannyFileInfo->Animations[i], affine3, linear3x3, inverseLinear3x3, 1e-5f, 1e-5f, 0);

//         for(long i=0; i<pGrannyFileInfo->SkeletonCount; i++)
//            GrannyTransformSkeleton(pGrannyFileInfo->Skeletons[i], affine3, linear3x3, inverseLinear3x3, 1e-5f, 1e-5f, 0);
//      }
//   }
//}



      static public unsafe string giveClientFilename(string inFN)
      {

         return mGR2Path + "\\" +Path.GetFileName(inFN);

      }
      static public unsafe char* stringToCharPtr(string str)
      {
         System.Text.ASCIIEncoding aenc = new System.Text.ASCIIEncoding();
         byte[] strB = aenc.GetBytes(str);
         
         fixed(byte *bt = strB)
            return (char*)bt;
      }
      static public unsafe bool getVertexTypeFromGranny(string filename, granny_data_type_definition* grannyVertType, 
         ref int vertMemSize, 
         ref granny_data_type_definition []grnDTD, 
         ref VertexDeclaration grnVD,
         ref VertexDeclaration d3dVD)
      {




         List<VertexTypes.eVertexDeclElement> decls = new List<VertexTypes.eVertexDeclElement>();
         List<granny_data_type_definition> grndcls = new List<granny_data_type_definition>();

         granny_data_type_definition* def = grannyVertType;

         while (def->memberType != granny_member_type.GrannyEndMember)
         {
            byte* str = (byte*)def->Name;

            IntPtr strPtr = new IntPtr((byte*)str);
            string name = Marshal.PtrToStringAnsi(strPtr);
            if (name.Equals("FD")) //COMPRESSED VERTEX FORMAT
            {
               CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Granny file {0} is Compressed. The current build of the editor does not support this. \n To view this model in the editor, please re-export uncompressed.", filename));
               return false;
            }

            if (def->memberType == granny_member_type.GrannyReal32Member)
            {
               granny_data_type_definition pdef;
               pdef.ArrayWidth = def->ArrayWidth;
               pdef.Extra0 = def->Extra0;
               pdef.Extra1 = def->Extra1;
               pdef.Extra2 = def->Extra2;
               pdef.memberType = def->memberType;
               pdef.Name = def->Name;
               pdef.ReferenceType = def->ReferenceType;
               pdef.TraversalID = def->TraversalID;

               //grndcls.Add(pdef);

               if (def->ArrayWidth == 4)
               {
                //  if (name.Equals("DiffuseColor0")) decls.Add(VertexTypes.eVertexDeclElement.cVDE_ColorDWORD);
               }
               else if (def->ArrayWidth == 3)
               {
                  if (name.Equals("Position")) { decls.Add(VertexTypes.eVertexDeclElement.cVDE_Position); grndcls.Add(pdef); }
                  else if (name.Equals("Normal")) { decls.Add(VertexTypes.eVertexDeclElement.cVDE_Normal); grndcls.Add(pdef); }
                  else if (name.Equals("Tangent")) { decls.Add(VertexTypes.eVertexDeclElement.cVDE_Tangent); grndcls.Add(pdef); }
                  else if (name.Equals("Binormal")) {  decls.Add(VertexTypes.eVertexDeclElement.cVDE_BiNormal); grndcls.Add(pdef); }
               }
               else if (def->ArrayWidth == 2)
               {
                  if (name.Equals("TextureCoordinates0")) { decls.Add(VertexTypes.eVertexDeclElement.cVDE_TexCoord2); grndcls.Add(pdef); }
                  else if (name.Equals("TextureCoordinates1")) { decls.Add(VertexTypes.eVertexDeclElement.cVDE_TexCoord2); grndcls.Add(pdef); }
                  else if (name.Equals("TextureCoordinates2")) { decls.Add(VertexTypes.eVertexDeclElement.cVDE_TexCoord2); grndcls.Add(pdef); }
                  else if (name.Equals("TextureCoordinates3")) { decls.Add(VertexTypes.eVertexDeclElement.cVDE_TexCoord2); grndcls.Add(pdef); }
               }
            }
           /* else if (def->memberType == granny_member_type.GrannyNormalUInt8Member)
            {

            //   CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Granny file {0} has an unsupported vertex structure", filename));
            //   return false;

               if (name.Equals("BoneWeights"))
               {
                  if (def->ArrayWidth == 4)
                     decls.Add(VertexTypes.eVertexDeclElement.cVDE_BlendWeight4);
               }
               else if (name.Equals("BoneIndices")) 
                  decls.Add(VertexTypes.eVertexDeclElement.cVDE_BlendIndicies);
            }*/

            def++;
         }

         //create our granny vertex 

         grnDTD = new granny_data_type_definition[grndcls.Count + 1];
         for (int k = 0; k < grndcls.Count; k++)
            grnDTD[k] = grndcls[k]; 
         grnDTD[decls.Count] = new granny_data_type_definition();
         grnDTD[decls.Count].memberType = granny_member_type.GrannyEndMember;


         //get our D3D Version of things
         short val = 0;
         grnVD = VertexTypes.genVertexDecl(decls, false, ref val,true);
         vertMemSize = (int)val;
         d3dVD = VertexTypes.genVertexDecl(decls, true, ref val);

          return true;
      }
      
      static public unsafe void swzzlGrnyVertsToD3DVerts(byte[] grnyVerts, VertexDeclaration grnyVD, int vertMemSize,int numVerts,
                                                     ref byte[] d3dVerts,  VertexDeclaration d3dVD )
      {
 
         //loop through all the verts and swizzle them to be FVF friendly for d3d
         VertexElement[] grnVE = grnyVD.GetDeclaration();
         VertexElement[] d3dVE = d3dVD.GetDeclaration();
        
         
                  for (int j = 0; j < grnVE.Length; j++)
                  {
                     VertexElement ve = grnVE[j];
                     for (int k = 0; k < d3dVE.Length; k++)
                     {
                        VertexElement dstVE = d3dVE[k];
                        if (dstVE.DeclarationUsage == ve.DeclarationUsage)
                        {
                           int count = sizeof(float) * 3;

                           if (ve.DeclarationType == DeclarationType.Float3) count = sizeof(float) * 3;
                           if (ve.DeclarationType == DeclarationType.Float2) count = sizeof(float) * 2;
                           if (ve.DeclarationType == DeclarationType.Float1) count = sizeof(float) * 1;
                           if (ve.DeclarationType == DeclarationType.Color) count = sizeof(int);
                           if (ve.DeclarationType == DeclarationType.Ubyte4N) count = sizeof(int);
                           
                         
                           for (int i = 0; i < numVerts; i++)
                           {
                              int srcInd = (i * vertMemSize) + ve.Offset;
                              int dstInd = (i * vertMemSize) + dstVE.Offset;

                              for (int q = 0; q < count; q++)
                              {
                                 d3dVerts[dstInd + q] = grnyVerts[srcInd + q];
                              }
                           }

                           k = d3dVE.Length + 1; ;
                        }
                     }
                  }

      }
      
      static private unsafe void genBBOX(byte[] verts, int vertStride, int numVerts, ref BBoundingBox bbox)
      {
         bbox.min.X = 10000;
         bbox.min.Y = 10000;
         bbox.min.Z = 10000;
         bbox.max.X = -10000;
         bbox.max.Y = -10000;
         bbox.max.Z = -10000;
         fixed (byte* ptr = verts)
         {
            int counter = 0;
            for(int i=0;i<numVerts;i++)
            {
               Vector3 *v = (Vector3*)(byte*)(ptr + counter);
               counter += vertStride;

               if (v->X < bbox.min.X) bbox.min.X = v->X;
               if (v->Y < bbox.min.Y) bbox.min.Y = v->Y;
               if (v->Z < bbox.min.Z) bbox.min.Z = v->Z;

               if (v->X > bbox.max.X) bbox.max.X = v->X;
               if (v->Y > bbox.max.Y) bbox.max.Y = v->Y;
               if (v->Z > bbox.max.Z) bbox.max.Z = v->Z;
            }
         }
      }


      static private unsafe void processMesh(string filename, ref BRenderGrannyMesh mesh, granny_mesh* grannyMesh)
      {
         BRenderPrimitive prim = new BRenderPrimitive();

         //indicies
         prim.mNumInds = GrannyGetMeshTriangleCount(grannyMesh) * 3;
         int[] indData = new int[prim.mNumInds];

         int indexSize = prim.mNumInds * sizeof(int);
         IntPtr pMarshaledIndexMem = System.Runtime.InteropServices.Marshal.AllocHGlobal(indexSize);

         GrannyCopyMeshIndices(grannyMesh, 4, (int*)pMarshaledIndexMem);
         System.Runtime.InteropServices.Marshal.Copy(pMarshaledIndexMem, indData, 0, prim.mNumInds);
         System.Runtime.InteropServices.Marshal.FreeHGlobal(pMarshaledIndexMem);


         //ib's
         prim.mIB = new IndexBuffer(typeof(int), prim.mNumInds, BRenderDevice.getDevice(), Usage.None, Pool.Managed);
         GraphicsStream stream = prim.mIB.Lock(0, 0, LockFlags.None);
         int* outInds = (int*)stream.InternalDataPointer;
         for (int q = 0; q < prim.mNumInds; q++)
            outInds[q] = indData[q];

         prim.mIB.Unlock();
         stream.Close();

         
         //verticies
         prim.mVertexSize = 0;


         granny_data_type_definition *grnyVertTypeDef = GrannyGetMeshVertexType(grannyMesh);
         granny_data_type_definition[] grnDTD = null;
         VertexDeclaration grnVD = null;
         if (!getVertexTypeFromGranny(filename, grnyVertTypeDef, ref prim.mVertexSize, ref grnDTD,ref grnVD, ref prim.mVDecl))
         {
            //already logged
            //CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Error loading {0} getVertexTypeFromGranny failed", filename));

            return;
         }

         prim.mNumVerts = GrannyGetMeshVertexCount(grannyMesh);

      {
         //CLM TALK TO ME IF YOU NEED THIS!!!!!
            //int size = prim.mNumVerts * prim.mVertexSize;
            //IntPtr pMarshaledMem = System.Runtime.InteropServices.Marshal.AllocHGlobal(size);
            //byte[] tGrnVerts = new byte[size];


            //fixed (granny_data_type_definition* grnPD = grnDTD)
            //   GrannyCopyMeshVertices(grannyMesh, grnPD/*grnyVertTypeDef*/, (void*)pMarshaledMem);
            //System.Runtime.InteropServices.Marshal.Copy(pMarshaledMem, tGrnVerts, 0, size);
            //System.Runtime.InteropServices.Marshal.FreeHGlobal(pMarshaledMem);

            //byte[] d3dVerts = new byte[size];

            ////swizzle the granny verts to be d3d friendly before copying them to the device
            //swzzlGrnyVertsToD3DVerts(tGrnVerts, grnVD, prim.mVertexSize,prim.mNumVerts,
            //                         ref d3dVerts,  prim.mVDecl );

         

            //prim.mVB = new VertexBuffer(BRenderDevice.getDevice(), (int)prim.mNumVerts * prim.mVertexSize, Usage.None, VertexFormats.None, Pool.Managed);
            //stream = prim.mVB.Lock(0, 0, LockFlags.None);
           
            //stream.Write(d3dVerts, 0, size);
            //prim.mVB.Unlock();
            //stream.Close();

            ////generate bounding box
            //genBBOX(d3dVerts, prim.mVertexSize, prim.mNumVerts, ref mesh.mBBox);

            //tGrnVerts = null;
            //grnVD.Dispose();
            //grnVD = null;
            
         }
         //SUB GROUPS
         int groupCount = GrannyGetMeshTriangleGroupCount(grannyMesh);
         granny_tri_material_group* GrannyMatGroups = GrannyGetMeshTriangleGroups(grannyMesh);


         //process our material groups for this mesh

     
            for (int k = 0; k < groupCount; k++)
            {
               BRenderMaterialGroup group = new BRenderMaterialGroup();


               group.mStartIndex = GrannyMatGroups[k].TriFirst * 3;
               group.mPrimCount = GrannyMatGroups[k].TriCount;

                if (grannyMesh->MaterialBindings!=null)
               {
                  granny_material* GrannyMaterial = grannyMesh->MaterialBindings[GrannyMatGroups[k].MaterialIndex].Material;
                  granny_texture* GrannyTexture = GrannyGetMaterialTextureByType(GrannyMaterial, granny_material_texture_type.GrannyDiffuseColorTexture);

                  IntPtr p1 = new IntPtr((void*)GrannyMaterial->Name);
                  string materialName = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(p1);

                  string albedoFN = null;

                  if (GrannyTexture != null)
                  {

                     IntPtr p = new IntPtr((void*)GrannyTexture->FromFileName);
                     albedoFN = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(p);
                  }
                  else
                  {

                     //Build some lists of texture information
                     Dictionary<string, string> allExtendedData = GetExtendedStringData(GrannyMaterial->ExtendedData);
                     Dictionary<string, string>.Enumerator it = allExtendedData.GetEnumerator();
                     while(it.MoveNext())
                     {
                        string possibleFileName = it.Current.Value;
                        ResourcePathInfo pathInfo = new ResourcePathInfo(possibleFileName);
                        if (pathInfo.IsFilePath)
                        {
                           if (!mesh.mAllFilenames.Contains(possibleFileName))
                              mesh.mAllFilenames.Add(possibleFileName);
                        }

                        //if (pathInfo.IsTexture)
                        //{
                        //   if (!mesh.mAllListedTextureFilenames.Contains(possibleFileName))
                        //      mesh.mAllTextureFilenames.Add(possibleFileName);
                        //}
                        //if (pathInfo.IsWorkRelativePath)
                        //{
                        //   if (!mesh.mAllListedRelativeTextureFilenames.Contains(pathInfo.RelativePathNoExt))
                        //      mesh.mAllListedRelativeTextureFilenames.Add(pathInfo.RelativePathNoExt);
                        //}
                        //else if (pathInfo.IsFilePath && pathInfo.IsRelativePath == true)
                        //{
                        //   if (!mesh.mAllListedOtherRelativeFilenames.Contains(pathInfo.RelativePath))
                        //      mesh.mAllListedOtherRelativeFilenames.Add(pathInfo.RelativePath);
                        //}
                        //else if (pathInfo.IsFilePath && pathInfo.IsRelativePath == false)
                        //{
                        //   if (!mesh.mAllListedNotRelativeFilenames.Contains(possibleFileName))
                        //      mesh.mAllListedNotRelativeFilenames.Add(possibleFileName);
                        //}
                        
                     }

                     //Find the fucking diffuse texture
                     it = allExtendedData.GetEnumerator();
                     string paramName = "";
                     while(it.MoveNext())
                     {
                        if (it.Current.Key.Contains("ESEffect") && it.Current.Value == "diffuse")
                        {
                           paramName = it.Current.Key;
                           break;
                        }

                     }

                     //Find child values of the diffuse parameter
                     paramName = paramName.Replace("Name","Value");
                     it = allExtendedData.GetEnumerator();
                     while (it.MoveNext())
                     {
                        if(it.Current.Key.Contains(paramName))
                        {
                           ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                           if((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                           {
                              //omg its a texture
                              albedoFN = pathInfo.Value;
                              break;
                           }

                        }
                     }

#if false                 
                     // Granny extended data type for render effect params.
                     granny_data_type_definition[] extendedDataDef = new granny_data_type_definition[3];
                     extendedDataDef[0] = new granny_data_type_definition();
                     extendedDataDef[1] = new granny_data_type_definition();
                     extendedDataDef[2] = new granny_data_type_definition();
                     extendedDataDef[0].memberType = granny_member_type.GrannyStringMember;
                     extendedDataDef[1].memberType = granny_member_type.GrannyStringMember;
                     extendedDataDef[2].memberType = granny_member_type.GrannyEndMember;

                     string name = @"ESEffectParamName1";
                     string value = @"ESEffectParamValue11";

                     System.Text.ASCIIEncoding aenc = new System.Text.ASCIIEncoding();
                     byte[] name0 = aenc.GetBytes(name);
                     byte[] name1 = aenc.GetBytes(value);


                     fixed (byte* nPtr = name0)
                     {
                        fixed (byte* vPtr = name1)
                        {

                           extendedDataDef[0].Name = (char*)nPtr;
                           extendedDataDef[1].Name = (char*)vPtr;

                           BRenderEffectParamExtendedData extendedData = new BRenderEffectParamExtendedData();


                           fixed (granny_data_type_definition* dd = extendedDataDef)
                              GrannyConvertSingleObject(GrannyMaterial->ExtendedData.Type, GrannyMaterial->ExtendedData.Object, dd, ref extendedData);

                           if ((extendedData.paramName == null) || (extendedData.paramValue == null))
                              break;

                           /*    uint j;
                              for (j = 0; j < cNumSupportedESEffectMaps; j++)
                                 if (stricmp(supportedESEffectMaps[j].pName, extendedData.paramName) == 0)
                                    break;

                              if (j == cNumSupportedESEffectMaps)   
                                 continue;
                           */
                           IntPtr p = new IntPtr((void*)extendedData.paramValue);
                           albedoFN = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(p);

                       
                        }
                     }
#endif
                  }
                   TextureHandle t=null;                
                   //if(albedoFN!=null)
                   if(albedoFN == null)
                      albedoFN = "";

                   {
                      //fixup our path filename so that we get the accurate folder for this client.
                      string texFilename = giveClientFilename(albedoFN);

                      //load the file so that we can display it
                      if (texFilename != "")
                      {
                         //CLM [05.10.06] RG changed our tools to all use TGA files now.
                         //try TGA form
                         string nPath = Path.ChangeExtension(texFilename, ".tga").ToLower();

                         
                         //if (mesh.mTextureCache.ContainsKey(nPath))
                         //{
                         //   t = mesh.mTextureCache[nPath];
                         //   continue;
                         //}
                         //else
                         {

                            if (File.Exists(nPath))
                            {
                               t = BRenderDevice.getTextureManager().getTexture(nPath,null,2) ;// TextureLoader.FromFile(BRenderDevice.getDevice(), nPath);
                               mesh.mTextureCache[nPath] = t;

                               mesh.mTextureFilenames.Add(texFilename.ToLower());
                            }
                            else
                            {
                               {
                                  t = BRenderDevice.getTextureManager().getTexture(CoreGlobals.getWorkPaths().mBlankTextureName);
                                  mesh.mTextureCache[CoreGlobals.getWorkPaths().mBlankTextureName] = t;
                               }                               
                            }
                         }

                         if (t != null)
                         {
                            group.mMaterial = new BRenderMaterial();
                            group.mMaterial.mTextures = new List<TextureHandle>();

                            group.mMaterial.mTextures.Add(t);
                         }
                         else
                         {
                            CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Error loading {0} \"{1}\" \n Was found, but did not load from disk properly..", filename, texFilename));
                         }
                      }
                      else
                      {
                         CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Error loading {0} texFilename != \"\" in {1}", materialName, filename));

                      }
                   }
                   //else
                   //{
                   //   CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("GrannyGetMaterialTextureByType(DiffuseColorTexture) failed for {0} in {1}.  Mapcount:{2}", materialName, Path.GetFileName(filename), GrannyMaterial->MapCount));
                   //}
                }
                else
               {

                  CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Granny reports that {0} does not have a material bound to it\n",filename));
               }



               //load your texture here.
               prim.mGroups.Add(group);
            }
         



         mesh.addRenderPrimitive(prim);
         
      
      }


      static unsafe public Dictionary<string,string> GetExtendedStringData(granny_variant extendedVariant)
      {

         Dictionary<string, string> hashtable = new Dictionary<string, string>();

         granny_data_type_definition[] extendedDataDef = new granny_data_type_definition[3];
         extendedDataDef[0] = new granny_data_type_definition();
         extendedDataDef[1] = new granny_data_type_definition();
         extendedDataDef[2] = new granny_data_type_definition();
         extendedDataDef[0].memberType = granny_member_type.GrannyStringMember;
         extendedDataDef[1].memberType = granny_member_type.GrannyStringMember;
         extendedDataDef[2].memberType = granny_member_type.GrannyEndMember;

         int i = 0;
         while (true)
         {
            if (extendedVariant.Type[i].memberType == granny_member_type.GrannyEndMember)
            {
               break;
            }
            if (extendedVariant.Type[i].memberType == granny_member_type.GrannyStringMember)
            {
               IntPtr p = new IntPtr((void*)extendedVariant.Type[i].Name);
               string typename = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(p);

               hashtable[typename] = "";

               string name = typename;
               string value = typename;

               System.Text.ASCIIEncoding aenc = new System.Text.ASCIIEncoding();
               byte[] name0 = aenc.GetBytes(name);
               byte[] name1 = aenc.GetBytes(value);

               fixed (byte* nPtr = name0)
               {
                  fixed (byte* vPtr = name1)
                  {

                     extendedDataDef[0].Name = (char*)nPtr;
                     extendedDataDef[1].Name = (char*)vPtr;

                     BRenderEffectParamExtendedData extendedData = new BRenderEffectParamExtendedData();

                     fixed (granny_data_type_definition* dd = extendedDataDef)
                        GrannyConvertSingleObject(extendedVariant.Type, extendedVariant.Object, dd, ref extendedData);

                     if ((extendedData.paramName == null) || (extendedData.paramValue == null))
                        break;

                     p = new IntPtr((void*)extendedData.paramValue);
                     string valstring = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(p);

                     hashtable[typename] = valstring;
                  }
               }
            }
            i++;

         }
         return hashtable;
      }

      static private unsafe void createMeshFromGR2(string filename, ref BRenderGrannyMesh mesh, granny_file_info *fileInfo)
      {

         mesh.mBBox.min.X = -1;
         mesh.mBBox.min.Y = -1;
         mesh.mBBox.min.Z = -1;
         mesh.mBBox.max.X = 1;
         mesh.mBBox.max.Y = 1;
         mesh.mBBox.max.Z = 1;

         int NumMeshes = fileInfo->ModelCount;
         for (int i = 0; i < NumMeshes; i++)
         {
            try
            {
               if (fileInfo->Models[i]->MeshBindingCount == 0)
               {
                  CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("No Mesh Binding for model# {0} out of {1} in {2}", i, NumMeshes, filename) );
               }
               else
               {
                  processMesh(filename, ref mesh, fileInfo->Models[i]->MeshBindings[0].Mesh);
               }
            }
            catch(System.Exception ex)
            {
               CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Error processing mesh #{0} out of {1} in {2}.  {3}",i,NumMeshes,filename,ex.ToString()   ) );
            }

         }
       
      }

       // VD: methods to get various gr2 file data for detail inspection
       //****************************************************************
      static public unsafe ArrayList getMaterialNames(granny_file_info* fileInfo)
      {
         ArrayList materials = new ArrayList();
         int num = fileInfo->MaterialCount;
         for (int matIndex = 0; matIndex < num; matIndex++ )
         {
            granny_material *mtl = fileInfo->Materials[matIndex];
            IntPtr p1 = new IntPtr((void*)mtl->Name);
            materials.Add(System.Runtime.InteropServices.Marshal.PtrToStringAnsi(p1));
         }   
         return materials;
      }
      static public unsafe ArrayList getBoneNames(granny_file_info* fileInfo)
      {
         ArrayList models = new ArrayList();
         ArrayList bones = new ArrayList();

         BRenderGrannyMesh mesh = new BRenderGrannyMesh();

         int modelCount = fileInfo->ModelCount;

         for (int modelIndex = 0; modelIndex < modelCount; ++modelIndex)
         {
            // This is the modelIndex'th model in the file
            granny_model *model = fileInfo->Models[modelIndex];

            // This is the name of the model, which is usually generated from its root bone
            IntPtr p1 = new IntPtr((void*)model->Name);
            models.Add(System.Runtime.InteropServices.Marshal.PtrToStringAnsi(p1));

            // This is the skeleton of the model, which contains the the bones
            // that control the model's structure
            //IntPtr pSkeleton = new IntPtr((void*)model->Skeleton);
            granny_skeleton* skeleton = model->Skeleton;
            {
               // This is the number of bones in the skeleton
               int boneCount = skeleton->BoneCount;

               for (int boneIndex = 0; boneIndex < boneCount; ++boneIndex)
               {
                  // This is the boneIndex'th bone in the skeleton
                  granny_bone* bone = &skeleton->Bones[boneIndex];
                  
                  IntPtr boneName = new IntPtr((void*)bone->Name);
                  bones.Add(System.Runtime.InteropServices.Marshal.PtrToStringAnsi(boneName));
               }
            }
         }
         return bones;
      }
      static public unsafe ArrayList getMeshNames(granny_file_info* fileInfo)
      {
         ArrayList fileMeshes = new ArrayList();
         for(int meshIndex = 0; meshIndex < fileInfo->MeshCount; ++meshIndex)
         {
            granny_mesh *gMesh = fileInfo->Meshes[meshIndex];
            
            IntPtr p1 = new IntPtr((void*)gMesh->Name);
            fileMeshes.Add(System.Runtime.InteropServices.Marshal.PtrToStringAnsi(p1));
         }
         return fileMeshes;
      }
      static public unsafe int getTotalVerts(granny_file_info *fileInfo)
      {
         int totalVerts = 0;

         for (int meshIndex = 0; meshIndex < fileInfo->MeshCount; ++meshIndex)
         {
            granny_mesh *gMesh = fileInfo->Meshes[meshIndex];
            totalVerts += gMesh->PrimaryVertexData->VertexCount;
         }
         return totalVerts;
      }
      static public unsafe int getTotalFaces(granny_file_info *fileInfo)
      {
        int totalFaces = 0;
        for (int meshIndex = 0; meshIndex < fileInfo->MeshCount; ++meshIndex)
        {
            granny_mesh *gMesh = fileInfo->Meshes[meshIndex];
            IntPtr gTop = gMesh->PrimaryTopology;
        }
        return 0;
      }
      
      static public unsafe int getMeshVerts(granny_file_info* fileInfo, int meshIndex)
      {
         granny_mesh* gMesh = fileInfo->Meshes[meshIndex];
         return gMesh->PrimaryVertexData->VertexCount;
      }
      static public unsafe ArrayList getMeshMaterials(granny_file_info* fileInfo, int meshIndex)
      {
         ArrayList meshMaterials = new ArrayList();

         granny_mesh* gMesh = fileInfo->Meshes[meshIndex];
         int materialBindingCount = gMesh->MaterialBindingCount;

         for(int materialBindingIndex = 0; materialBindingIndex < materialBindingCount; ++materialBindingIndex)
         {
             granny_material_binding materialBinding = new granny_material_binding();
            materialBinding = gMesh->MaterialBindings[materialBindingIndex];
            if(materialBinding.Material != null)
                meshMaterials.Add(System.Runtime.InteropServices.Marshal.PtrToStringAnsi(new IntPtr((byte*)materialBinding.Material->Name)));
         }
         return meshMaterials;
      }
       static public unsafe ArrayList getMeshTextures(granny_mesh *grMesh)
       {
           ArrayList textureNames = new ArrayList();
            int mTextureCount = grMesh->MaterialBindingCount;
            TextureStateIndices[] mTextureIndices = new TextureStateIndices[mTextureCount];

            int textureIndex;
            for (textureIndex = 0; textureIndex < mTextureCount; ++textureIndex)
            {
               mTextureIndices[textureIndex] = new TextureStateIndices();


               granny_material* gran_material = grMesh->MaterialBindings[textureIndex].Material;
               granny_texture* gran_texture = GrannyGetMaterialTextureByType(gran_material, granny_material_texture_type.GrannyDiffuseColorTexture);

               string diffuseTextureName = null;

               if (gran_texture != null)
               {
                  IntPtr p = new IntPtr((void*)gran_texture->FromFileName);
                  diffuseTextureName = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(p);
               }
               else
               {
                  //Build some lists of texture information
                  Dictionary<string, string> allExtendedData = new Dictionary<string, string>();

                  if (gran_material != null)
                  {

                     allExtendedData = GetExtendedStringData(gran_material->ExtendedData);
                     Dictionary<string, string>.Enumerator it = allExtendedData.GetEnumerator();

                     string paramName = "";
                     while (it.MoveNext())
                     {
                        if (it.Current.Key.Contains("ESEffect"))// && it.Current.Value == "diffuse")
                        {
                            switch (it.Current.Value)
                            {
                                case "diffuse":
                                    {
                                        paramName = it.Current.Key;
                                        paramName = paramName.Replace("Name", "Value");
                                        while (it.MoveNext())
                                        {
                                            if (it.Current.Key.Contains(paramName))
                                            {
                                                ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                                                if ((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                                                {
                                                    textureNames.Add(pathInfo.Value);
                                                    break;
                                                }

                                            }
                                        }
                                        break;
                                    }
                                case "Normal":
                                    {
                                        paramName = it.Current.Key;
                                        paramName = paramName.Replace("Name", "Value");
                                        while (it.MoveNext())
                                        {
                                            if (it.Current.Key.Contains(paramName))
                                            {
                                                ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                                                if ((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                                                {
                                                    textureNames.Add(pathInfo.Value);
                                                    break;
                                                }

                                            }
                                        }
                                        break;
                                    }
                                case "Gloss":
                                    {
                                        paramName = it.Current.Key;
                                        paramName = paramName.Replace("Name", "Value");
                                        while (it.MoveNext())
                                        {
                                            if (it.Current.Key.Contains(paramName))
                                            {
                                                ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                                                if ((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                                                {
                                                    textureNames.Add(pathInfo.Value);
                                                    break;
                                                }

                                            }
                                        }
                                        break;
                                    }
                                case "Displacement":
                                    {
                                        paramName = it.Current.Key;
                                        paramName = paramName.Replace("Name", "Value");
                                        while (it.MoveNext())
                                        {
                                            if (it.Current.Key.Contains(paramName))
                                            {
                                                ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                                                if ((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                                                {
                                                    textureNames.Add(pathInfo.Value);
                                                    break;
                                                }

                                            }
                                        }
                                        break;
                                    }
                                case "Opacity":
                                    {
                                        paramName = it.Current.Key;
                                        paramName = paramName.Replace("Name", "Value");
                                        while (it.MoveNext())
                                        {
                                            if (it.Current.Key.Contains(paramName))
                                            {
                                                ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                                                if ((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                                                {
                                                    textureNames.Add(pathInfo.Value);
                                                    break;
                                                }

                                            }
                                        }
                                        break;
                                    }
                                case "pixelxformColor":
                                    {
                                        paramName = it.Current.Key;
                                        paramName = paramName.Replace("Name", "Value");
                                        while (it.MoveNext())
                                        {
                                            if (it.Current.Key.Contains(paramName))
                                            {
                                                ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                                                if ((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                                                {
                                                    textureNames.Add(pathInfo.Value);
                                                    break;
                                                }

                                            }
                                        }
                                        break;
                                    }
                                case "emPixelxformColor":
                                    {
                                        paramName = it.Current.Key;
                                        paramName = paramName.Replace("Name", "Value");
                                        while (it.MoveNext())
                                        {
                                            if (it.Current.Key.Contains(paramName))
                                            {
                                                ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                                                if ((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                                                {
                                                    textureNames.Add(pathInfo.Value);
                                                    break;
                                                }

                                            }
                                        }
                                        break;
                                    }
                                case "Emissive":
                                    {
                                        paramName = it.Current.Key;
                                        paramName = paramName.Replace("Name", "Value");
                                        while (it.MoveNext())
                                        {
                                            if (it.Current.Key.Contains(paramName))
                                            {
                                                ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                                                if ((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                                                {
                                                    textureNames.Add(pathInfo.Value);
                                                    break;
                                                }

                                            }
                                        }
                                        break;
                                    }
                                case "AO":
                                    {
                                        paramName = it.Current.Key;
                                        paramName = paramName.Replace("Name", "Value");
                                        while (it.MoveNext())
                                        {
                                            if (it.Current.Key.Contains(paramName))
                                            {
                                                ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                                                if ((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                                                {
                                                    textureNames.Add(pathInfo.Value);
                                                    break;
                                                }

                                            }
                                        }
                                        break;
                                    }
                                case "Enviornment":
                                    {
                                        paramName = it.Current.Key;
                                        paramName = paramName.Replace("Name", "Value");
                                        while (it.MoveNext())
                                        {
                                            if (it.Current.Key.Contains(paramName))
                                            {
                                                ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                                                if ((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                                                {
                                                    textureNames.Add(pathInfo.Value);
                                                    break;
                                                }

                                            }
                                        }
                                        break;
                                    }
                                case "ReflectionMask":
                                    {
                                        paramName = it.Current.Key;
                                        paramName = paramName.Replace("Name", "Value");
                                        while (it.MoveNext())
                                        {
                                            if (it.Current.Key.Contains(paramName))
                                            {
                                                ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                                                if ((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                                                {
                                                    textureNames.Add(pathInfo.Value);
                                                    break;
                                                }

                                            }
                                        }
                                        break;
                                    }
                            }
                        }
                     }


                     //Find child values of the diffuse parameter
                     paramName = paramName.Replace("Name", "Value");
                     it = allExtendedData.GetEnumerator();
                     while (it.MoveNext())
                     {
                        if (it.Current.Key.Contains(paramName))
                        {
                           ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                           if ((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                           {
                              diffuseTextureName = pathInfo.Value;
                              break;
                           }

                        }
                     }
                  }
                  else
                  {
                     allExtendedData.Clear();
                  }
               }
            }
            return (textureNames);
         }

      static public string mGR2Path;
      

      #region struct mimmics

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct BRenderEffectParamExtendedData
         {
            public char* paramName;
            public char* paramValue;
         };


         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_bone_binding
         {
            public char* BoneName;
            public Vector3 OBBMin;
            public Vector3 OBBMax;
            public int TriangleCount;
            public int* TriangleIndices;
         };


         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_mesh
         {
             public char * Name;
             public granny_vertex_data * PrimaryVertexData;
             public int MorphTargetCount;
             public IntPtr /*granny_morph_target **/ MorphTargets;
             public IntPtr /*granny_tri_topology **/ PrimaryTopology;
             public int MaterialBindingCount;
             public granny_material_binding * MaterialBindings;
             public int BoneBindingCount;
             public granny_bone_binding * BoneBindings;
             public granny_variant ExtendedData;
         };

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_tri_topology
         {
            public int GroupCount;
            public granny_tri_material_group * Groups;
            public int IndexCount;
            public IntPtr Indices;
            public int Index16Count;
            public UInt16 * Indices16;
            public int VertecToVertexCount;
            public IntPtr VertexToVertexMap;
            public int VertexToTriangleCount;
            public IntPtr VertexToTriangleMap;
            public int SideToNeighborCount;
            public IntPtr SideToNeightborMap;
            public int BonesForTriangleCount;
            public IntPtr BonesForTriangle;
            public int TriangleToBoneCount;
            public IntPtr TriangleToBoneIndices;
            public int TriAnnotationSetCount;
            public granny_tri_annotation_set * TriAnnotationSets;            
         }

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_tri_annotation_set
         {
            public char * Name;
            public granny_data_type_definition * TriAnnotationType;
            public int TriAnnotationCount;
            public uint * TriAnnotations;
            public int IndicesMapFromTriToAnnotation;
            public int TriAnnotationIndexCount;
            public IntPtr TriAnnotationIndices;
         }

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_vertex_data
         {
            public IntPtr /*granny_data_type_definition **/ VertexType;
            public int VertexCount;
            public IntPtr Vertices;
            public int VertexComponentNameCount;
            public char** VertexComponentNames;
            public int VertexAnnotationSetCount;
            public IntPtr /*granny_vertex_annotation_set **/ VertexAnnotationSets;
         };



         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_file
         {
            public bool IsByteReversed;
            public IntPtr /*granny_grn_file_header**/ Header;
            public IntPtr /*granny_grn_file_magic_value**/ SourceMagicValue;
            public int SectionCount;
            public void** Sections;
            public bool* Marshalled;
            public bool* IsUserMemory;
            public void* ConversionBuffer;
         };

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_file_info
         {
            public granny_art_tool_info* ArtToolInfo;
            public IntPtr /*granny_exporter_info**/ ExporterInfo;
            public char* FromFileName;
            public int TextureCount;
            public IntPtr /*granny_texture***/ Textures;
            public int MaterialCount;
            public granny_material** Materials;
            public int SkeletonCount;
            public granny_skeleton** Skeletons;
            public int VertexDataCount;
            public granny_vertex_data** VertexDatas;
            public int TriTopologyCount;
            public IntPtr /*granny_tri_topology***/ TriTopologies;
            public int MeshCount;
            public granny_mesh** Meshes;
            public int ModelCount;
            public granny_model** Models;
            public int TrackGroupCount;
            public granny_track_group** TrackGroups;
            public int AnimationCount;
            public granny_animation** Animations;
            public granny_variant ExtendedData;
         };

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_track_group
         {
            public char* Name;
            public int VectorTrackCount;
            public IntPtr /*granny_vector_track* */ VectorTracks;
            public int TransformTrackCount;
            public IntPtr /*granny_transform_track* */ TransformTracks;
            public int TransformLODErrorCount;
            public float* TransformLODErrors;
            public int TextTrackCount;
            public IntPtr /*granny_text_track* */ TextTracks;
            public granny_transform InitialPlacement;
            public int Flags;
            public float* LoopTranslation;
            public IntPtr /*granny_periodic_loop* */ PeriodicLoop;
            public IntPtr /*granny_transform_track* */ RootMotion;
            public granny_variant ExtendedData;
         };


         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_variant
         {
            public granny_data_type_definition* Type;
            public char* Object;
         };



         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_transform
         {
             public int Flags;
             public Vector3 Position;
             public Vector4 Orientation;
             public Vector3 ScaleShear0;
             public Vector3 ScaleShear1;
             public Vector3 ScaleShear2;
         };

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_model
         {
            public char* Name;
            public granny_skeleton* Skeleton;
            public granny_transform InitialPlacement;
            public int MeshBindingCount;
            public  granny_model_mesh_binding* MeshBindings;
         }

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_model_mesh_binding
         {
            public granny_mesh* Mesh;
         };

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_model_instance
         { };

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_animation
         {
             public char * Name;
             public float Duration;
             public float TimeStep;
             public float Oversampling;
             public int TrackGroupCount;
             public granny_track_group** TrackGroups;
         };


         [StructLayout(LayoutKind.Sequential)]
         public struct granny_pnt332_vertex
         {
            Vector3 Position;
            Vector3 Normal;
            Vector2 UV;         
         };

         [StructLayout(LayoutKind.Sequential)]
         public struct granny_tri_material_group
         {
            public int MaterialIndex;
            public int TriFirst;
            public int TriCount;
         };

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_material
         {
            public char* Name;
            public int MapCount;
            public IntPtr /*granny_material_map* */ Maps;
            public IntPtr /*granny_texture* */ Texture;
            public granny_variant ExtendedData;
         };

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_material_binding
         {
            public granny_material* Material;
         };

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_texture
         {
            public char *FromFileName;
            public int TextureType;
            public int Width;
            public int Height;
            public int Encoding;
            public int SubFormat;
            public IntPtr/*granny_pixel_layout*/ Layout;
            public int ImageCount;
            public IntPtr/*granny_texture_image**/ Images;
            public granny_variant ExtendedData;
         };

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct  granny_data_type_definition
         {
             public granny_member_type memberType;
             public char *Name;
             public granny_data_type_definition * ReferenceType;
             public int ArrayWidth;
             public int Extra0;
             public int Extra1;
             public int Extra2;
             public uint TraversalID;
         };

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_skeleton
         {
            public char* Name;
            public int BoneCount;
            public granny_bone* Bones;
            public int LODType;
         };

         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_bone
         {
            public byte* Name;
            public int ParentIndex;
            public granny_transform LocalTransform;
            public Matrix /*granny_matrix_4x4*/ InverseWorld4x4;
            public float LODError;
            public granny_variant ExtendedData;
         };

 



         [StructLayout(LayoutKind.Sequential)]
         public unsafe struct granny_art_tool_info
         {
            public char* FromArtToolName;
            public int ArtToolMajorRevision;
            public int ArtToolMinorRevision;
            public float UnitsPerMeter;
            public Vector3 Origin;
            public Vector3 RightVector;
            public Vector3 UpVector;
            public Vector3 BackVector;
            public granny_variant ExtendedData;
         };


      [StructLayout(LayoutKind.Sequential)]
      public unsafe struct granny_control { };

      [StructLayout(LayoutKind.Sequential)]
      public unsafe struct granny_model_control_binding { };

      [StructLayout(LayoutKind.Sequential)]
      public unsafe struct granny_world_pose { };

      [StructLayout(LayoutKind.Sequential)]
      public unsafe struct granny_mesh_binding { };

      [StructLayout(LayoutKind.Sequential)]
      public unsafe struct granny_mesh_deformer { };

      [StructLayout(LayoutKind.Sequential)]
      public unsafe struct granny_local_pose { };



      #endregion


      #region enums

         public enum granny_member_type
               {
                   GrannyEndMember,
                   GrannyInlineMember,
                   GrannyReferenceMember,
                   GrannyReferenceToArrayMember,
                   GrannyArrayOfReferencesMember,
                   GrannyVariantReferenceMember,
                   GrannySwitchableTypeMember,
                   GrannyReferenceToVariantArrayMember,
                   GrannyStringMember,
                   GrannyTransformMember,
                   GrannyReal32Member,
                   GrannyInt8Member,
                   GrannyUInt8Member,
                   GrannyBinormalInt8Member,
                   GrannyNormalUInt8Member,
                   GrannyInt16Member,
                   GrannyUInt16Member,
                   GrannyBinormalInt16Member,
                   GrannyNormalUInt16Member,
                   GrannyInt32Member,
                   GrannyUInt32Member,
                   GrannyOnePastLastMemberType,
                   GrannyBool32Member = GrannyInt32Member,
               };
            public enum granny_material_texture_type
            {
                GrannyUnknownTextureType,
                GrannyAmbientColorTexture,
                GrannyDiffuseColorTexture,
                GrannySpecularColorTexture,
                GrannySelfIlluminationTexture,
                GrannyOpacityTexture,
                GrannyBumpHeightTexture,
                GrannyReflectionTexture,
                GrannyRefractionTexture,
                GrannyDisplacementTexture,
                GrannyOnePastLastMaterialTextureType
            } ;

            public enum granny_standard_section_index
            {
               GrannyStandardMainSection = 0,
               GrannyStandardRigidVertexSection = 1,
               GrannyStandardRigidIndexSection = 2,
               GrannyStandardDeformableVertexSection = 3,
               GrannyStandardDeformableIndexSection = 4,
               GrannyStandardTextureSection = 5,
               GrannyStandardDiscardableSection = 6,
               GrannyStandardUnloadedSection = 7,
               GrannyStandardSectionCount
            } ;

            public enum granny_deformation_type
            {
               GrannyDeformPosition = 1,
               GrannyDeformPositionNormal,
               GrannyDeformPositionNormalTangent,
               GrannyDeformPositionNormalTangentBinormal,
            };

            public enum granny_deformer_tail_flags
            {
               GrannyDontAllowUncopiedTail,
               GrannyAllowUncopiedTail,
            };

 


      #endregion

      #region DLL Defines
      const int GrannyRenormalizeNormals = 0x1;
      const int GrannyReorderTriangleIndices = 0x2;

      public enum GrannyControlUserData
      {
	      AnimIndex = 0,
	      IsBlendControl,
	      EasingOut,
	      NOTUSED2
      };

      [DllImport("Granny2.dll")]
      public static extern unsafe IntPtr /*granny_file**/ GrannyReadEntireFile(string filename);
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_file_info* GrannyGetFileInfo(IntPtr /*granny_file **/File);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeFile(IntPtr /*granny_file**/ File);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeModelInstance(granny_model_instance *ModelInstance);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeMeshBinding(IntPtr granny_mesh_binding);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeMeshDeformer(IntPtr granny_mesh_deformer);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeWorldPose(granny_world_pose *WorldPose);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeLocalPose(IntPtr granny_local_pose);
      [DllImport("Granny2.dll")]
      public static extern unsafe int GrannyGetMeshTriangleCount(granny_mesh *mesh);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyCopyMeshIndices(granny_mesh *mesh, int bytesPerIndex, void *dstIndexData);
      [DllImport("Granny2.dll")]
      public static extern unsafe int GrannyGetMeshVertexCount(granny_mesh *mesh);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyCopyMeshVertices(granny_mesh* mesh, granny_data_type_definition *vertType, void* dstVertexData);
      [DllImport("Granny2.dll")]
      public static extern unsafe int GrannyGetMeshTriangleGroupCount(granny_mesh* mesh);
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_data_type_definition *GrannyGetMeshVertexType(granny_mesh* mesh);
      [DllImport("Granny2.dll")]
      public static extern unsafe bool GrannyComputeBasisConversion( granny_file_info * FileInfo,
                                                  float DesiredUnitsPerMeter,
                                                  ref Vector3 DesiredOrigin3,
                                                  ref Vector3 DesiredRight3,
                                                  ref Vector3 DesiredUp3,
                                                  ref Vector3 DesiredBack3,
                                                  ref Vector3 ResultAffine3,
                                                  float * ResultLinear3x3,
                                                  float * ResultInverseLinear3x3);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyTransformFile(granny_file_info * FileInfo,
                                                   ref Vector3 Affine3,
                                                   float * Linear3x3, 
                                                   float * InverseLinear3x3, 
                                                   float AffineTolerance,
                                                   float LinearTolerance,
                                                   int Flags);
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_tri_material_group* GrannyGetMeshTriangleGroups(granny_mesh *mesh);
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_texture* GrannyGetMaterialTextureByType(granny_material* material, 
                                                                                 granny_material_texture_type channel);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyConvertSingleObject(granny_data_type_definition* SourceType, 
                                                   void* SourceObject, 
                                                   granny_data_type_definition* DestType, 
                                                   ref BRenderEffectParamExtendedData DestObject);

      [DllImport("Granny2.dll")]
      public static extern unsafe int GrannyGetTypeTableCount(granny_data_type_definition ** TypeTable);

//      [DllImport("Granny2.dll")]
//      public static extern unsafe granny_data_type_definition ** GrannyGetTypeTableFor(granny_data_type_definition * MemberType);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyBuildSkeletonRelativeTransforms(int SourceTransformStride,
                                                                            granny_transform* SourceTransforms,
                                                                            int SourceParentStride,
                                                                            int* SourceParents,
                                                                            int Count,
                                                                            int ResultStride,
                                                                            granny_transform * Results);
      
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_model_instance * GrannyInstantiateModel(granny_model * Model);

      [DllImport("Granny2.dll")]
      public static extern unsafe granny_control * GrannyPlayControlledAnimation(   float StartTime,
                                                                                    granny_animation * Animation,
                                                                                    granny_model_instance * Model);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannySetControlLoopCount(granny_control* Control,
                                                                   int LoopCount);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeControlOnceUnused(granny_control* Control);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeControl(granny_control* Control);


      [DllImport("Granny2.dll")]
      public static extern unsafe granny_model_control_binding* GrannyModelControlsBegin(granny_model_instance * Model);
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_model_control_binding* GrannyModelControlsEnd(granny_model_instance * Model);
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_model_control_binding* GrannyModelControlsNext(granny_model_control_binding * Binding);
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_control* GrannyGetControlFromBinding(granny_model_control_binding * Binding);
      [DllImport("Granny2.dll")]
      public static extern unsafe float GrannyEaseControlOut(granny_control* Control, float Duration);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyCompleteControlAt(granny_control * Control, float AtSeconds);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeFileSection(granny_file* File, int SectionIndex);
      
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_world_pose* GrannyNewWorldPose(int BoneCount);

      [DllImport("Granny2.dll")]
      public static extern unsafe granny_mesh_binding* GrannyNewMeshBinding(granny_mesh* Mesh, 
                                                                  granny_skeleton* FromSkeleton, 
                                                                  granny_skeleton* ToSkeleton);


      [DllImport("Granny2.dll")]
      public static extern unsafe int GrannyGetMeshIndexCount(granny_mesh * Mesh);

      [DllImport("Granny2.dll")]
      public static extern unsafe bool GrannyMeshIsRigid(granny_mesh * Mesh);

      [DllImport("Granny2.dll")]
      public static extern unsafe granny_mesh_deformer* GrannyNewMeshDeformer(granny_data_type_definition* InputVertexLayout, 
                                                                  granny_data_type_definition* OutputVertexLayout,
                                                                  granny_deformation_type DeformationType,
                                                                  granny_deformer_tail_flags TailFlag);

      [DllImport("Granny2.dll")]
      public static extern unsafe int * GrannyGetMeshBindingToBoneIndices(granny_mesh_binding * Binding);

      [DllImport("Granny2.dll")]
      public static extern unsafe Matrix* GrannyGetWorldPoseComposite4x4Array(granny_world_pose* WorldPose);

      [DllImport("Granny2.dll")]
      public static extern unsafe granny_skeleton * GrannyGetSourceSkeleton(granny_model_instance * Model);
 
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannySetModelClock(granny_model_instance * ModelInstance, float NewClock);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyUpdateModelMatrix(granny_model_instance* ModelInstance, 
                                                                  float SecondsElapsed, 
                                                                  float* ModelMatrix4x4, 
                                                                  float* DestMatrix4x4, 
                                                                  bool Inverse);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannySampleModelAnimations(granny_model_instance * ModelInstance, 
                                                                  int FirstBone, 
                                                                  int BoneCount, 
                                                                  granny_local_pose * Result);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyBuildWorldPose(granny_skeleton * Skeleton,
                                                                  int FirstBone,
                                                                  int BoneCount,
                                                                  granny_local_pose * LocalPose, 
                                                                  float * Offset4x4, 
                                                                  granny_world_pose * Result);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeCompletedModelControls(granny_model_instance * ModelInstance);

      [DllImport("Granny2.dll")]
      public static extern unsafe granny_local_pose* GrannyNewLocalPose(int BoneCount);

      [DllImport("Granny2.dll")]
      public static extern unsafe float* GrannyGetWorldPose4x4(granny_world_pose* WorldPose, int BoneIndex);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyDeformVertices(granny_mesh_deformer * Deformer,
                                                             int * MatrixIndices,
                                                             float * MatrixBuffer4x4,
                                                             int VertexCount,
                                                             void * SourceVertices,
                                                             void * DestVertices);


      [DllImport("Granny2.dll")]
      public static extern unsafe void * GrannyGetMeshVertices(granny_mesh * Mesh);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyGetWorldMatrixFromLocalPose(granny_skeleton * Skeleton, 
                                                            int BoneIndex, 
                                                            granny_local_pose * LocalPose,
                                                            float * Offset4x4, 
                                                            float * Result4x4,
                                                            int * SparseBoneArray,
                                                            int * SparseBoneArrayReverse);

      [DllImport("Granny2.dll")]
      public static extern unsafe bool GrannyFindBoneByName(granny_skeleton * Skeleton, 
                                                            char * BoneName, 
                                                            int * BoneIndex);

      
      [DllImport("Granny2.dll")]
      public static extern unsafe void** GrannyGetControlUserDataArray(granny_control* Control);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyBuildCompositeTransform4x4(granny_transform * transform, float * matrix4x4);


      #endregion

   }

   public unsafe class GrannyTexture
   {
      public string mName;
      public TextureHandle mTexture;

      ~GrannyTexture()
      {
         deinit();
      }

      public void deinit()
      {
         if (mTexture != null)
         {
            BRenderDevice.getTextureManager().freeTexture(mName);
            BRenderDevice.getTextureManager().removeWatchedTexture(mName);
            mTexture.destroy();
            mTexture = null;
         }
      }

      public void reload()
      {
         deinit();

         mTexture = BRenderDevice.getTextureManager().getTexture(mName, null, 2);
      }
   }

   public class TextureStateIndices
   {
      public int diffuse = -1;
      public int xform = -1;
      public int opacity = -1;
   }


   public unsafe class GrannyMesh : GrannyBridge
   {
      // Granny's mesh information for this mesh
      public granny_mesh* mGrannyMesh = null;

      // The binding between this mesh and the model its deformed with
      public granny_mesh_binding* mGrannyBinding = null;

      // The deformer that I'm using to deform the mesh
      public granny_mesh_deformer* mGrannyDeformer = null;

      // All meshes will have their indices submitted to DirectX, but
      // only rigid meshes will have their vertices submitted.  Thus,
      // IndexBuffer is always an index buffer, but VertexBuffer will
      // be 0 for deformable meshes.
      public IndexBuffer mIndexBuffer = null;
      public VertexBuffer mVertexBuffer = null;

      public VertexDeclaration mVDecl = null;
      public int mVertexSize = 0;

      // The textures used by this mesh
      public int mTextureCount;
      public TextureStateIndices[] mTextureIndices;
      public int mDDXTextureMemorySize = 0;


      ~GrannyMesh()
      {
         deinit();
      }

      public bool init(granny_model *grModel, granny_mesh *grMesh, string mName)
      {
         try
         {

            mGrannyMesh = grMesh;

            /* First I create the Granny binding for this mesh.  The binding
            is basically a table that says what bone slots of the mesh go
            with which bones of the model.  It's used during rendering to
            pull the correct matrices from the skeleton to use for
            deformation (or just to load the correct single transform in
            the case of rigid meshes). */
            granny_skeleton* grSkeleton = grModel->Skeleton;
            mGrannyBinding = GrannyNewMeshBinding(grMesh, grSkeleton, grSkeleton);

            /* Next, I create a D3D index buffer for the indices of this mesh... */
            int IndexCount = GrannyGetMeshIndexCount(grMesh);

            if (IndexCount > 0)
            {
               mIndexBuffer = new IndexBuffer(typeof(short), IndexCount, BRenderDevice.getDevice(), Usage.None, Pool.Managed);

               /* ... and I copy them in.  The GrannyCopyMeshIndices routine
               can do arbitrary bytes-per-index conversion, so the 2 just says
               "make sure it's 2-byte (16-bit) indices".  If it was stored
               in the file as 16-bit indices, it's a block copy, but if it wasn't,
               it does index-by-index conversion. */
               GraphicsStream stream = mIndexBuffer.Lock(0, 0, LockFlags.None);
               int* outInds = (int*)stream.InternalDataPointer;

               GrannyCopyMeshIndices(grMesh, 2, outInds);

               mIndexBuffer.Unlock();
            }

            /* Now I process the vertex data. */
            int VertexCount = GrannyGetMeshVertexCount(grMesh);

            granny_data_type_definition* grnyVertTypeDef = GrannyGetMeshVertexType(grMesh);
            granny_data_type_definition[] grnDTD = null;
            VertexDeclaration d3dVD = null;

            string filename = "";
            if (!getVertexTypeFromGranny(filename, grnyVertTypeDef, ref mVertexSize, ref grnDTD, ref mVDecl, ref d3dVD))
               return false;

            // Release not needed stuff
            d3dVD.Dispose();
            d3dVD = null;

            int VertexBufferSize = VertexCount * mVertexSize;

            if (GrannyMeshIsRigid(grMesh))
            {
               /* When the mesh is rigid (meaning that the entire mesh
               is controlled by a single bone), I can submit all the vertex
               data to D3D for card residency. */
               if (VertexBufferSize > 0)
               {
                  /*
                  IntPtr pMarshaledMem = System.Runtime.InteropServices.Marshal.AllocHGlobal(VertexBufferSize);
                  byte[] tGrnVerts = new byte[VertexBufferSize];

                  fixed (granny_data_type_definition* grnPD = grnDTD)
                     GrannyCopyMeshVertices(grMesh, grnPD, (void*)pMarshaledMem);
                  System.Runtime.InteropServices.Marshal.Copy(pMarshaledMem, tGrnVerts, 0, VertexBufferSize);
                  System.Runtime.InteropServices.Marshal.FreeHGlobal(pMarshaledMem);

                  byte[] d3dVerts = new byte[VertexBufferSize];

                  //swizzle the granny verts to be d3d friendly before copying them to the device
                  swzzlGrnyVertsToD3DVerts(tGrnVerts, grnVD, mVertexSize, VertexCount,
                                           ref d3dVerts, mVDecl);



                  mVertexBuffer = new VertexBuffer(BRenderDevice.getDevice(), VertexBufferSize, Usage.WriteOnly, VertexFormats.None, Pool.Managed);
                  GraphicsStream stream = mVertexBuffer.Lock(0, 0, LockFlags.None);

                  stream.Write(d3dVerts, 0, VertexBufferSize);
                  mVertexBuffer.Unlock();
                  stream.Close();

                  ////generate bounding box
                  //genBBOX(d3dVerts, prim.mVertexSize, prim.mNumVerts, ref mesh.mBBox);

                  tGrnVerts = null;
                  */

                  mVertexBuffer = new VertexBuffer(BRenderDevice.getDevice(), VertexBufferSize, Usage.None, VertexFormats.None, Pool.Managed);
                  GraphicsStream stream = mVertexBuffer.Lock(0, 0, LockFlags.None);
                  int* outInds = (int*)stream.InternalDataPointer;

                  fixed (granny_data_type_definition* grnPD = grnDTD)
                     GrannyCopyMeshVertices(grMesh, grnPD, outInds);

                  mVertexBuffer.Unlock();
               }

               // Since the mesh is rigid, I won't be needing a deformer.
               mGrannyDeformer = null;
            }
            else
            {
               // Check if too deformable many verts
               if (VertexBufferSize > GrannyManager2.MaxMutableVertexBufferSize)
               {
                  CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Too many deformable verts in mesh \"{0}\" within model \"{1}\".  This mesh won't be rendered.", Marshal.PtrToStringAnsi(new IntPtr((void*)grMesh->Name)), mName));
                  return (false);
               }
   
               /* ... and then I create a Granny deformer for this mesh.  I
               ask for deformations for the position and normal in this
               case, since that's all I've got, but I could also ask for
               deformations of the tangent space if I was doing bump
               mapping. */
               fixed (granny_data_type_definition* grnPD = grnDTD)
                  mGrannyDeformer = GrannyNewMeshDeformer(grnyVertTypeDef, grnPD, granny_deformation_type.GrannyDeformPositionNormal, granny_deformer_tail_flags.GrannyDontAllowUncopiedTail);
               if (mGrannyDeformer == null)
               {
                  CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Granny didn't find a matching deformer for the vertex format used by mesh \"{0}\" within model \"{1}\".  This mesh won't be rendered.", Marshal.PtrToStringAnsi(new IntPtr((void*)grMesh->Name)), mName));
                  return (false);
               }
            }

            /* Now that all the mesh data is submitted, I spin through the
            textures referenced by this mesh and look up their in-memory
            versions that I've created with CreateTexture. */
            mTextureCount = grMesh->MaterialBindingCount;
            mTextureIndices = new TextureStateIndices[mTextureCount];

            int textureIndex;
            for (textureIndex = 0; textureIndex < mTextureCount; ++textureIndex)
            {
               mTextureIndices[textureIndex] = new TextureStateIndices();


               granny_material* gran_material = grMesh->MaterialBindings[textureIndex].Material;
               granny_texture* gran_texture = GrannyGetMaterialTextureByType(gran_material, granny_material_texture_type.GrannyDiffuseColorTexture);

               string diffuseTextureName = null;
               string opacityTextureName = null;

               if (gran_texture != null)
               {
                  IntPtr p = new IntPtr((void*)gran_texture->FromFileName);
                  diffuseTextureName = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(p);
               }
               else
               {
                  //Build some lists of texture information
                  Dictionary<string, string> allExtendedData = new Dictionary<string, string>();

                  if (gran_material != null)
                  {

                     allExtendedData = GetExtendedStringData(gran_material->ExtendedData);
                     Dictionary<string, string>.Enumerator it = allExtendedData.GetEnumerator();

                     string difParamName = "";
                     string opParamName = "";

                     bool foundDif = false;
                     bool foundOp = false;

                     while (it.MoveNext())
                     {
                        if (it.Current.Key.Contains("ESEffect"))
                        {
                           if (it.Current.Value == "diffuse")
                           {
                              difParamName = it.Current.Key;

                              foundDif = true;
                              if (foundDif && foundOp)
                                 break;
                           }
                           else if (it.Current.Value == "Opacity")
                           {
                              opParamName = it.Current.Key;

                              foundOp = true;
                              if (foundDif && foundOp)
                                 break;
                           }
                        }
                     }

                     foundDif = false;
                     foundOp = false;

                     //Find child values of the diffuse parameter
                     difParamName = difParamName.Replace("Name", "Value");
                     opParamName = opParamName.Replace("Name", "Value");
                     it = allExtendedData.GetEnumerator();
                     while (it.MoveNext())
                     {
                        if (it.Current.Key.Contains(difParamName))
                        {
                           ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                           if ((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                           {
                              diffuseTextureName = pathInfo.Value;
                           }

                           foundDif = true;
                           if (foundDif && foundOp)
                              break;
                        }
                        else if (it.Current.Key.Contains(opParamName))
                        {
                           ResourcePathInfo pathInfo = new ResourcePathInfo(it.Current.Value);
                           if ((pathInfo.IsFilePath) && (pathInfo.IsTexture))
                           {
                              opacityTextureName = pathInfo.Value;
                           }

                           foundOp = true;
                           if (foundDif && foundOp)
                              break;
                        }
                     }
                  }
                  else
                  {
                     allExtendedData.Clear();
                  }
               }

               if (!String.IsNullOrEmpty(diffuseTextureName))
               {
                  // look for diffuse texture
                  //CLM this is causing exceptions, strip out any prior paths..
                  string tstStr = "work\\art";
                  if (diffuseTextureName.Contains(tstStr))
                     diffuseTextureName = diffuseTextureName.Substring(diffuseTextureName.LastIndexOf(tstStr) + tstStr.Length) ;

                  string dfTextureAbsoluteName = CoreGlobals.getWorkPaths().mGameArtDirectory + diffuseTextureName;
                  if (-1 == GrannyManager2.findTexture(dfTextureAbsoluteName))
                  {
                     //give us an estimate of our DDX Memory footprint for textures
                     string rootName = dfTextureAbsoluteName.Substring(0, dfTextureAbsoluteName.LastIndexOf("_"));
                     mDDXTextureMemorySize += DDXBridge.give360TextureMemFootprint(rootName + "_df.ddx");
                     mDDXTextureMemorySize += DDXBridge.give360TextureMemFootprint(rootName + "_nm.ddx");
                     mDDXTextureMemorySize += DDXBridge.give360TextureMemFootprint(rootName + "_ao.ddx");
                     mDDXTextureMemorySize += DDXBridge.give360TextureMemFootprint(rootName + "_sp.ddx");
                     mDDXTextureMemorySize += DDXBridge.give360TextureMemFootprint(rootName + "_em.ddx");
                     mDDXTextureMemorySize += DDXBridge.give360TextureMemFootprint(rootName + "_rm.ddx");
                     mDDXTextureMemorySize += DDXBridge.give360TextureMemFootprint(rootName + "_xf.ddx");
                  }
                  mTextureIndices[textureIndex].diffuse = GrannyManager2.getOrCreateTexture(dfTextureAbsoluteName);

                  // look for xform (player color) texture
                  string xformTextureAbsoluteName = dfTextureAbsoluteName.Substring(0, dfTextureAbsoluteName.Length - 6);
                  xformTextureAbsoluteName = xformTextureAbsoluteName + "xf.tga";
                  mTextureIndices[textureIndex].xform = GrannyManager2.getOrCreateTexture(xformTextureAbsoluteName);
               }

               if (!String.IsNullOrEmpty(opacityTextureName))
               {
                  // look for opacity texture
                  //CLM this is causing exceptions, strip out any prior paths..
                  string tstStr = "work\\art";
                  if (opacityTextureName.Contains(tstStr))
                     opacityTextureName = opacityTextureName.Substring(opacityTextureName.LastIndexOf(tstStr) + tstStr.Length);

                  string opTextureAbsoluteName = CoreGlobals.getWorkPaths().mGameArtDirectory + opacityTextureName;
                  if (-1 == GrannyManager2.findTexture(opTextureAbsoluteName))
                  {
                     //give us an estimate of our DDX Memory footprint for textures
                     string rootName = opTextureAbsoluteName.Substring(0, opTextureAbsoluteName.LastIndexOf("_"));
                     mDDXTextureMemorySize += DDXBridge.give360TextureMemFootprint(rootName + "_op.ddx");
                  }
                  mTextureIndices[textureIndex].opacity = GrannyManager2.getOrCreateTexture(opTextureAbsoluteName);
               }

            }


            return (true);
         }
         catch(System.Exception ex)
         {
            return false;
         }
      }

      public void deinit()
      {
         if(mIndexBuffer != null)
         {
            mIndexBuffer.Dispose();
            mIndexBuffer = null;
         }

         if (mVertexBuffer != null)
         {
            mVertexBuffer.Dispose();
            mVertexBuffer = null;
         }
      }


      public unsafe bool testRayIntersection(Vector3 r0, Vector3 rD, Matrix worldMat, Matrix* CompositeBuffer)
      {
         if (mGrannyMesh == null) return true;

         try
         {


            VertexBuffer curVertexBuffer = null;
            GraphicsStream VBStream = null;
            GraphicsStream IBStream = null;

            bool hit = false;


            if (mIndexBuffer != null)
            {
               IBStream = mIndexBuffer.Lock(0, 0, LockFlags.ReadOnly);

               if (mVertexBuffer != null)
               {
                  curVertexBuffer = mVertexBuffer;
                  VBStream = curVertexBuffer.Lock(0, 0, LockFlags.ReadOnly);
               }
               else if (mGrannyDeformer != null)
               {
                  // It's a deformable mesh, so I grab the next available
                  // AGP vertex buffer.
                  curVertexBuffer = GrannyManager2.MutableVertexBufferRing[GrannyManager2.MutableVertexBufferIndex++ % GrannyManager2.MaxMutableVertexBufferCount];

                  // Now I tell Granny to deform the vertices of the mesh
                  // with the current world pose of the model, and dump
                  // the results into the AGP vertex buffer.
                  VBStream = curVertexBuffer.Lock(0, 0, LockFlags.None);
                  {
                     byte* DestVertices = (byte*)VBStream.InternalDataPointer;

                     int* ToBoneIndices = GrannyGetMeshBindingToBoneIndices(mGrannyBinding);
                     int VertexCount = GrannyGetMeshVertexCount(mGrannyMesh);

                     GrannyDeformVertices(mGrannyDeformer,
                                  ToBoneIndices, (float*)CompositeBuffer,
                                  VertexCount,
                                  GrannyGetMeshVertices(mGrannyMesh),
                                  DestVertices);
                  }
               }


               short* inds = (short*)IBStream.InternalDataPointer;
               float* verts = (float*)VBStream.InternalDataPointer;

               int vSize = mVertexSize / sizeof(float);
               int numTris = GrannyGetMeshIndexCount(mGrannyMesh) / 3; //mPrimitives[k].mNumInds / 3;



               Vector3[] kverts = new Vector3[3];
               int vIndex = 0;
               Vector3 iPoint = Vector3.Empty;

               int c = 0;
               for (int i = 0; i < numTris; i++)
               {
                  kverts[0] = Vector3.Empty;
                  vIndex = inds[c++];
                  kverts[0].X = verts[vIndex * vSize + 0];
                  kverts[0].Y = verts[vIndex * vSize + 1];
                  kverts[0].Z = verts[vIndex * vSize + 2];

                  kverts[1] = Vector3.Empty;
                  vIndex = inds[c++];
                  kverts[1].X = verts[vIndex * vSize + 0];
                  kverts[1].Y = verts[vIndex * vSize + 1];
                  kverts[1].Z = verts[vIndex * vSize + 2];

                  kverts[2] = Vector3.Empty;
                  vIndex = inds[c++];
                  kverts[2].X = verts[vIndex * vSize + 0];
                  kverts[2].Y = verts[vIndex * vSize + 1];
                  kverts[2].Z = verts[vIndex * vSize + 2];

                  //transform the verts by the world matrix
                  for (int q = 0; q < 3; q++)
                  {
                     Vector4 p = Vector3.Transform(kverts[q], worldMat);
                     kverts[q].X = p.X;
                     kverts[q].Y = p.Y;
                     kverts[q].Z = p.Z;
                  }

                  if (BMathLib.raySegmentIntersectionTriangle(kverts, ref r0, ref rD, false, ref iPoint))
                  {
                     hit = true;
                     break;
                  }
               }

               curVertexBuffer.Unlock();
               mIndexBuffer.Unlock();
            }

            return hit;
         }


         catch (System.Exception ex)
         {
            return false;
         }
      }

      public unsafe void burnIntoObjFile(OBJFile obj, int objectId, Matrix worldMat, Matrix* CompositeBuffer)
      {
         if (mGrannyMesh == null) 
            return;

         try
         {
            VertexBuffer curVertexBuffer = null;
            GraphicsStream VBStream = null;
            GraphicsStream IBStream = null;

            float magicScale = 64;

            if (mIndexBuffer != null)
            {
               IBStream = mIndexBuffer.Lock(0, 0, LockFlags.ReadOnly);

               if (mVertexBuffer != null)
               {
                  curVertexBuffer = mVertexBuffer;
                  VBStream = curVertexBuffer.Lock(0, 0, LockFlags.ReadOnly);
               }
               else if (mGrannyDeformer != null)
               {
                  // It's a deformable mesh, so I grab the next available
                  // AGP vertex buffer.
                  curVertexBuffer = GrannyManager2.MutableVertexBufferRing[GrannyManager2.MutableVertexBufferIndex++ % GrannyManager2.MaxMutableVertexBufferCount];

                  // Now I tell Granny to deform the vertices of the mesh
                  // with the current world pose of the model, and dump
                  // the results into the AGP vertex buffer.
                  VBStream = curVertexBuffer.Lock(0, 0, LockFlags.None);
                  {
                     byte* DestVertices = (byte*)VBStream.InternalDataPointer;

                     int* ToBoneIndices = GrannyGetMeshBindingToBoneIndices(mGrannyBinding);
                     int VertexCount = GrannyGetMeshVertexCount(mGrannyMesh);

                     GrannyDeformVertices(mGrannyDeformer,
                                  ToBoneIndices, (float*)CompositeBuffer,
                                  VertexCount,
                                  GrannyGetMeshVertices(mGrannyMesh),
                                  DestVertices);
                  }
               }


               short* inds = (short*)IBStream.InternalDataPointer;
               float* verts = (float*)VBStream.InternalDataPointer;

               int vSize = mVertexSize / sizeof(float);
               int numTris = GrannyGetMeshIndexCount(mGrannyMesh) / 3; //mPrimitives[k].mNumInds / 3;



               Vector3[] kverts = new Vector3[3];
               int vIndex = 0;
               Vector3 iPoint = Vector3.Empty;

               int c = 0;
               for (int i = 0; i < numTris; i++)
               {
                  kverts[0] = Vector3.Empty;
                  vIndex = inds[c++];
                  kverts[0].X = verts[vIndex * vSize + 0];
                  kverts[0].Y = verts[vIndex * vSize + 1];
                  kverts[0].Z = verts[vIndex * vSize + 2];

                  kverts[1] = Vector3.Empty;
                  vIndex = inds[c++];
                  kverts[1].X = verts[vIndex * vSize + 0];
                  kverts[1].Y = verts[vIndex * vSize + 1];
                  kverts[1].Z = verts[vIndex * vSize + 2];

                  kverts[2] = Vector3.Empty;
                  vIndex = inds[c++];
                  kverts[2].X = verts[vIndex * vSize + 0];
                  kverts[2].Y = verts[vIndex * vSize + 1];
                  kverts[2].Z = verts[vIndex * vSize + 2];

                  //transform the verts by the world matrix
                  for (int q = 0; q < 3; q++)
                  {
                     Vector4 p = Vector3.Transform(kverts[q], worldMat);
                     kverts[q].X = p.X;
                     kverts[q].Y = p.Y;
                     kverts[q].Z = p.Z;

                     kverts[q].X *= -magicScale;
                     kverts[q].Y *= magicScale;
                     kverts[q].Z *= -magicScale;
                  }

                  obj.addTriangle(objectId, kverts[0], kverts[2], kverts[1]);
               }

               curVertexBuffer.Unlock();
               mIndexBuffer.Unlock();
            }
         }


         catch (System.Exception ex)
         {
            return;
         }
      }
   };

   public unsafe class GrannyModel : GrannyBridge
   {
      public string mName;
      public granny_file* mGrannyFile = null;

      // The meshes that comprise this model
      public List<GrannyMesh> mMeshes = new List<GrannyMesh>();


      ~GrannyModel()
      {
         deinit();
      }

      /*
      public bool init(string name, granny_file* grFile)
      {
         mName = name;
         mGrannyFile = grFile;

         granny_model* grModel = GrannyGetFileInfo((IntPtr)mGrannyFile)->Models[0];
         if (grModel == null)
            return false;


         // Now, I loop through all the meshes in the model and process
         //them.
         int meshCount = grModel->MeshBindingCount;

         int meshIndex;
         for (meshIndex = 0; meshIndex < meshCount; ++meshIndex)
         {
            GrannyMesh mesh = new GrannyMesh();

            mesh.init(grModel, grModel->MeshBindings[meshIndex].Mesh);

            mMeshes.Add(mesh);
         }

         return true;
      }
*/


      public unsafe bool init(string filename)
      {
         deinit();

         // load
         granny_file* grannyFile = GrannyManager2.loadGrannyFile(filename);

         if (grannyFile != null)
         {
            // Check to make sure that there is at least one model in this file, since
            // granny files can have any number of models.
            granny_file_info* grannyFileInfo = GrannyGetFileInfo((IntPtr)grannyFile);
            if (grannyFileInfo->ModelCount < 1)
            {
               GrannyFreeFile((IntPtr)grannyFile);
               return false;
            }

            granny_model* grannyModel = grannyFileInfo->Models[0];
            if (grannyModel == null)
            {
               GrannyFreeFile((IntPtr)grannyFile);
               return false;
            }

            mGrannyFile = grannyFile;
            mName = filename;

            // Loop through all the meshes in the model and process them.
            int meshCount = grannyModel->MeshBindingCount;

            int meshIndex;
            for (meshIndex = 0; meshIndex < meshCount; ++meshIndex)
            {
               GrannyMesh mesh = new GrannyMesh();

               mesh.init(grannyModel, grannyModel->MeshBindings[meshIndex].Mesh, mName);

               mMeshes.Add(mesh);
            }


            // TODO:  Free data copied into vertex and index buffers from granny file.
            /*
            GrannyFreeFileSection(grannyFile, (int)granny_standard_section_index.GrannyStandardRigidVertexSection);
            GrannyFreeFileSection(grannyFile, (int)granny_standard_section_index.GrannyStandardRigidIndexSection);
            GrannyFreeFileSection(grannyFile, (int)granny_standard_section_index.GrannyStandardDeformableIndexSection);
            GrannyFreeFileSection(grannyFile, (int)granny_standard_section_index.GrannyStandardTextureSection);
            */

            return true;
         }
         return false;
      }


      public void deinit()
      {
         mName = null;

         if (mGrannyFile != null)
         {
            GrannyFreeFile((IntPtr)mGrannyFile);
            mGrannyFile = null;
         }
         for (int i = 0; i < mMeshes.Count; i++)
            mMeshes[i].deinit();
         mMeshes.Clear();
      }

      public granny_file_info* getGrannyFileInfo()
      {
         return (GrannyGetFileInfo((IntPtr)mGrannyFile));
      }

      public int getBoneHandle(string boneName)
      {
         if ((mGrannyFile == null) || (String.IsNullOrEmpty(boneName)))
            return -1;

         granny_file_info* grannyFileInfo = GrannyGetFileInfo((IntPtr) mGrannyFile);

         int skelCount = grannyFileInfo->SkeletonCount;
         if(skelCount <= 0)
         {
            return -1;
         }

         // Only check first skeleton
         granny_skeleton* pSkeleton = grannyFileInfo->Skeletons[0];
         long boneCount = pSkeleton->BoneCount;
         for (int b = 0; b < boneCount; b++)
         {
            string curBoneName = Marshal.PtrToStringAnsi(new IntPtr((byte*)pSkeleton->Bones[b].Name));

            if (string.Compare(boneName, curBoneName, true) == 0)
            {
               return b;
            }
         }

         return(-1);
      }


      public bool reload()
      {
         string name = mName;

         // unload first
         deinit();

         // now reload
         return (init(name));
      }

   }
   
   public unsafe class GrannyAnimation : GrannyBridge
   {
      public string mName;
      public granny_file* mGrannyFile;


      ~GrannyAnimation()
      {
         deinit();
      }

      public bool init(string filename)
      {
         deinit();

         granny_file* grannyFile = GrannyManager2.loadGrannyFile(filename);

         if (grannyFile != null)
         {
            // Check to make sure that there is at least one animation in this file, since
            // granny files can have any number of animations.
            granny_file_info* grannyFileInfo = GrannyGetFileInfo((IntPtr)grannyFile);
            if (grannyFileInfo->AnimationCount < 1)
            {
               GrannyFreeFile((IntPtr)grannyFile);
               return false;
            }

            mName = filename;
            mGrannyFile = grannyFile;

            return true;
         }

         return false;
      }


      public void deinit()
      {
         mName = null;

         if (mGrannyFile != null)
         {
            GrannyFreeFile((IntPtr)mGrannyFile);
            mGrannyFile = null;
         }
      }

      public void applyInitialPlacement(ref Matrix worldMatrix)
      {
         granny_file_info* fileInfo = getGrannyFileInfo();

         if ((fileInfo != null) && (fileInfo->TrackGroupCount > 0))
         {
            fixed (Matrix* fixedMatrix = &worldMatrix)
            {
               GrannyBuildCompositeTransform4x4(&fileInfo->TrackGroups[0]->InitialPlacement, (float*)fixedMatrix);
            }
         }
         else
         {
            worldMatrix = Matrix.Identity;
         }
      }

      public granny_file_info* getGrannyFileInfo()
      {
         return(GrannyGetFileInfo((IntPtr) mGrannyFile));
      }

      public float getDuration()
      {
         granny_file_info* animFileInfo = getGrannyFileInfo();
         if (animFileInfo->Animations == null)
            return 0.0f;

         granny_animation* anim = animFileInfo->Animations[0];
         return (anim->Duration);
      }


      public bool reload()
      {
         string name = mName;

         // unload first
         deinit();

         // now reload
         return (init(name));
      }
   }
   
   public unsafe class GrannyInstance : GrannyBridge
   {
      public int mModelIndex = -1;
      public granny_model_instance* mGrannyModelInstance = null;

      protected granny_world_pose*  mWorldPose = null;      // The current world-space state of the model at sampled time
      protected float               mSampledTime = -1.0f;    // Time mWorldPose was computed

      protected float               mAnimClock = 0.0f;

      public BBoundingBox           mStaticBoundingBox = new BBoundingBox();


      public enum eVisualEditorRenderMode
      {
         cRenderFull = 0,
         cRenderFullWireframe,
         cRenderFlat,
         cRenderFlatWireframe,
         cRenderFullOverlay,
         cRenderDepthPeelAO,
         cRenderDepth,
         cRenderHeightForSimrep,
      };

      private static eVisualEditorRenderMode mRenderMode = eVisualEditorRenderMode.cRenderFull;


      ~GrannyInstance()
      {
         mModelIndex = -1;
         GrannyFreeModelInstance(mGrannyModelInstance);
         GrannyFreeWorldPose(mWorldPose);
      }


      public bool init(int modelId)
      {
         // Clear first
         deinit();

         GrannyModel model = GrannyManager2.getModel(modelId);
         if (model == null)
            return false;

         granny_file_info *fileInfo = model.getGrannyFileInfo();
         if(fileInfo == null)
            return false;

         granny_model *gmodel = fileInfo->Models[0];
         if(model == null)
            return false;

         int boneCount = gmodel->Skeleton->BoneCount;

         mModelIndex = modelId;
         mGrannyModelInstance = GrannyInstantiateModel(gmodel);
         mWorldPose = GrannyNewWorldPose(boneCount);

         computeStaticBoundingBox();

         return(true);
      }


      public void deinit()
      {
	      // Free instance.
         if (mGrannyModelInstance != null)
	      {
            stopAnimations();

            GrannyFreeModelInstance(mGrannyModelInstance);
            mGrannyModelInstance = null;
	      }
      	
         mAnimClock = 0.0f;
         
         mModelIndex = -1;
      }


      public static void setRenderMode(eVisualEditorRenderMode mode)
      {
         mRenderMode = mode;
      }
      
      public static eVisualEditorRenderMode getRenderMode()
      {
         return mRenderMode;
      }

      public void computeStaticBoundingBox()
      {
         mStaticBoundingBox.empty();

         GrannyModel model = GrannyManager2.getModel(mModelIndex);
         if (model == null)
            return;

         int BoneCount = GrannyGetSourceSkeleton(mGrannyModelInstance)->BoneCount;
         GrannySampleModelAnimations(mGrannyModelInstance, 0, BoneCount,
                                    GrannyManager2.SharedLocalPose);

         GrannyBuildWorldPose(GrannyGetSourceSkeleton(mGrannyModelInstance),
                  0, BoneCount, GrannyManager2.SharedLocalPose, null, mWorldPose);

         granny_world_pose* pWorldPose = mWorldPose;


         BOrientedBoundingBox orientedBox = new BOrientedBoundingBox();
         BBoundingBox aaBox = new BBoundingBox();

         int meshIndex;
         for (meshIndex = 0; meshIndex < model.mMeshes.Count; ++meshIndex)
         {
            GrannyMesh mesh = model.mMeshes[meshIndex];

            granny_mesh* pMesh = mesh.mGrannyMesh;
            granny_mesh_binding* pMeshBinding = mesh.mGrannyBinding;

            if (pMeshBinding == null)
               continue;

            for (long j = 0; j < pMesh->BoneBindingCount; j++)
            {
               int boneIndex = GrannyGetMeshBindingToBoneIndices(pMeshBinding)[j];

               float* transform = GrannyGetWorldPose4x4(pWorldPose, boneIndex);

               Matrix xform;
               xform.M11 = transform[0]; xform.M12 = transform[1]; xform.M13 = transform[2]; xform.M14 = transform[3];
               xform.M21 = transform[4]; xform.M22 = transform[5]; xform.M23 = transform[6]; xform.M24 = transform[7];
               xform.M31 = transform[8]; xform.M32 = transform[9]; xform.M33 = transform[10]; xform.M34 = transform[11];
               xform.M41 = transform[12]; xform.M42 = transform[13]; xform.M43 = transform[14]; xform.M44 = transform[15];

               granny_bone_binding boneBinding = pMesh->BoneBindings[j];

               Vector3 pOBBMin = boneBinding.OBBMin;
               Vector3 pOBBMax = boneBinding.OBBMax;

               orientedBox.construct(pOBBMin, pOBBMax, xform);
               orientedBox.computeAABB(ref aaBox);

               mStaticBoundingBox.addBox(aaBox);
            }
         }
      }
      
      public void computeDynamicBoundingBox(ref BBoundingBox box)
      {
         box.empty();

         GrannyModel model = GrannyManager2.getModel(mModelIndex);
         if (model == null)
            return;


         int BoneCount = GrannyGetSourceSkeleton(mGrannyModelInstance)->BoneCount;
         GrannySampleModelAnimations(mGrannyModelInstance, 0, BoneCount,
                                    GrannyManager2.SharedLocalPose);

         GrannyBuildWorldPose(GrannyGetSourceSkeleton(mGrannyModelInstance),
                  0, BoneCount, GrannyManager2.SharedLocalPose, null, mWorldPose);

         granny_world_pose* pWorldPose = mWorldPose;


         BOrientedBoundingBox orientedBox = new BOrientedBoundingBox();
         BBoundingBox aaBox = new BBoundingBox();
         
         int meshIndex;
         for (meshIndex = 0; meshIndex < model.mMeshes.Count; ++meshIndex)
         {
            GrannyMesh mesh = model.mMeshes[meshIndex];

            granny_mesh* pMesh = mesh.mGrannyMesh;
            granny_mesh_binding* pMeshBinding = mesh.mGrannyBinding;

            if(pMeshBinding == null)
               continue;

            for(long j = 0; j < pMesh->BoneBindingCount; j++)
            {
               int boneIndex = GrannyGetMeshBindingToBoneIndices(pMeshBinding)[j];

               float* transform = GrannyGetWorldPose4x4(pWorldPose, boneIndex);

               Matrix xform;
               xform.M11 = transform[0]; xform.M12 = transform[1]; xform.M13 = transform[2]; xform.M14 = transform[3];
               xform.M21 = transform[4]; xform.M22 = transform[5]; xform.M23 = transform[6]; xform.M24 = transform[7];
               xform.M31 = transform[8]; xform.M32 = transform[9]; xform.M33 = transform[10]; xform.M34 = transform[11];
               xform.M41 = transform[12]; xform.M42 = transform[13]; xform.M43 = transform[14]; xform.M44 = transform[15];

               granny_bone_binding boneBinding = pMesh->BoneBindings[j];

               Vector3 pOBBMin = boneBinding.OBBMin;
               Vector3 pOBBMax = boneBinding.OBBMax;

               orientedBox.construct(pOBBMin, pOBBMax, xform);
               orientedBox.computeAABB(ref aaBox);

               box.addBox(aaBox);
            }
         }
      }

      public int getBoneHandle(string boneName)
      {
         GrannyModel model = GrannyManager2.getModel(mModelIndex);
         if (model == null)
            return -1;

         return (model.getBoneHandle(boneName));
      }

      #region ANIMATION_METHONDS

      public bool playAnimation(int animId)
      {
         GrannyAnimation animation = GrannyManager2.getAnimation(animId);

         if (animation == null)
            return false;


         // Stop currently playing anims
         stopAnimations();


         /* GrannyPlayControlledAnimation is the
            simplest way to start an animation playing
            on a model instance.  There are more
            complicated calls that allow for greater
            control over how the animation is played,
            but the purposes of this sample, this is
            all I need. */
         granny_file_info* animFileInfo = animation.getGrannyFileInfo();
         if (animFileInfo->Animations == null)
            return false;

         granny_control* control = GrannyPlayControlledAnimation(
                                   0.0f, animation.getGrannyFileInfo()->Animations[0],
                                   mGrannyModelInstance);
         if (control != null)
         {
            /* I want to play this animation ad infinitum,
               so I set the loop count to the magic value of
               0, which means forever.  Any other loop value
               would play the animation for that many loops
               and then clamp to the final frame. */
            GrannySetControlLoopCount(control, 1);

            /* Since I don't plan to make any further
               adjustments to this animation, I can just
               throw away the control handle completely.
               However, so I don't leak memory, I have to
               tell Granny that, once the model(s) that
               this control affects are freed (which I
               will do during shutdown), free the control
               too.  Normally Granny won't ever free
               something you've created unless you tell
               her too, so this call is basically giving
               her permission. */
            GrannyFreeControlOnceUnused(control);

            // Add user data
            void** ppUserData = GrannyGetControlUserDataArray(control);
            if (ppUserData != null)
            {
               ppUserData[(int)GrannyControlUserData.AnimIndex] = (void*)animId;
               ppUserData[(int)GrannyControlUserData.IsBlendControl] = (void*)0;
               ppUserData[(int)GrannyControlUserData.EasingOut] = (void*)0;
               ppUserData[(int)GrannyControlUserData.NOTUSED2] = (void*)0;
            }
         }

         return true;
      }

      public void stopAnimations()
      {
         if (mGrannyModelInstance == null)
            return;

         // Iterate over controls
         for (granny_model_control_binding* pBinding = GrannyModelControlsBegin(mGrannyModelInstance);
                  pBinding != GrannyModelControlsEnd(mGrannyModelInstance);
                  pBinding = GrannyModelControlsNext(pBinding))
         {
            // Get the control.
            granny_control* pControl = GrannyGetControlFromBinding(pBinding);

            // Free it
            GrannyFreeControl(pControl);
         }
      }

      public void stopAnimations(float stopOverSeconds)
      {
         if (mGrannyModelInstance == null)
            return;

         // Iterate over controls
         for (granny_model_control_binding* pBinding = GrannyModelControlsBegin(mGrannyModelInstance);
                  pBinding != GrannyModelControlsEnd(mGrannyModelInstance);
                  pBinding = GrannyModelControlsNext(pBinding))
         {
            // Get the control.
            granny_control* pControl = GrannyGetControlFromBinding(pBinding);


            // Ease it out.
            float easeTime = GrannyEaseControlOut(pControl, stopOverSeconds);

            // Tell it to go away at it's stopping time.
            GrannyCompleteControlAt(pControl, easeTime);
         }
      }

      public void getAnimationList(List<int> animList)
      {         
         // Iterate over controls
         for (granny_model_control_binding* pBinding = GrannyModelControlsBegin(mGrannyModelInstance);
                  pBinding != GrannyModelControlsEnd(mGrannyModelInstance);
                  pBinding = GrannyModelControlsNext(pBinding))
         {
            // Get the control.
            granny_control* pControl = GrannyGetControlFromBinding(pBinding);

            void** ppUserData = GrannyGetControlUserDataArray(pControl);
            animList.Add((int)ppUserData[(int)GrannyControlUserData.AnimIndex]);
         }
      }

      public void rebindAnimation(int animId)
      {
         if (mGrannyModelInstance == null)
            return;

         // Iterate over controls
         for (granny_model_control_binding* pBinding = GrannyModelControlsBegin(mGrannyModelInstance);
            pBinding != GrannyModelControlsEnd(mGrannyModelInstance);
            pBinding = GrannyModelControlsNext(pBinding))
         {
            granny_control* pControl = GrannyGetControlFromBinding(pBinding);

            void** ppUserData = GrannyGetControlUserDataArray(pControl);
            if (ppUserData[(int)GrannyControlUserData.AnimIndex] == (void*)animId)
            {
               // Free control
               GrannyFreeControl(pControl);

               // play new animation
               playAnimation(animId);
            }
         }
      }
      #endregion

      #region RENDERING_METHODS

      protected void sampleAnimations()
      {
         // Returned if we have already sampled this frame
         if (mSampledTime == mAnimClock)
            return;

         // Sample
         int BoneCount = GrannyGetSourceSkeleton(mGrannyModelInstance)->BoneCount;

         GrannySampleModelAnimations(mGrannyModelInstance, 0, BoneCount,
                                    GrannyManager2.SharedLocalPose);

         GrannyBuildWorldPose(GrannyGetSourceSkeleton(mGrannyModelInstance),
                  0, BoneCount, GrannyManager2.SharedLocalPose, null, mWorldPose);

         mSampledTime = mAnimClock;
      }


      public bool render()
      {
         return(render(new Vector3(), false));
      }


      public bool render(Vector3 playerColor, bool bUsePlayerColor)
      {
         if(GrannyManager2.s_modelGPUShader.Disposed == true)
         {
            GrannyManager2.s_modelGPUShader = GrannyManager2.s_modelGPUShaderHandle.mShader;
         }

         GrannyModel model = GrannyManager2.getModel(mModelIndex);
         if (model == null)
            return false;

         sampleAnimations();

         
         /* Before I do any rendering, I set the vertex shader to coincide
         with the vertex format I'm using.  You could do this once for
         the entire app, since I never render anything else, but I
         figured it'd me more instructive to put it here where I
         actually do the rendering. */
         //BRenderDevice.getDevice().VertexShader = null;
         //BRenderDevice.getDevice().PixelShader = null;

          

         if (mRenderMode == eVisualEditorRenderMode.cRenderFullWireframe || mRenderMode == eVisualEditorRenderMode.cRenderFlatWireframe)
            BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.WireFrame);
         else if (mRenderMode == eVisualEditorRenderMode.cRenderFull || 
                  mRenderMode == eVisualEditorRenderMode.cRenderFlat || 
                  mRenderMode == eVisualEditorRenderMode.cRenderFullOverlay ||
                  mRenderMode == eVisualEditorRenderMode.cRenderDepthPeelAO)
            BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.Solid);


         int passId = 0;
         if (mRenderMode == eVisualEditorRenderMode.cRenderFull || mRenderMode == eVisualEditorRenderMode.cRenderFullWireframe || mRenderMode == eVisualEditorRenderMode.cRenderFullOverlay)
            passId = 0;
         else if (mRenderMode == eVisualEditorRenderMode.cRenderFlat || mRenderMode == eVisualEditorRenderMode.cRenderFlatWireframe)
            passId = 1;
         else if (mRenderMode == eVisualEditorRenderMode.cRenderDepthPeelAO)
            passId = 3;
         else if (mRenderMode == eVisualEditorRenderMode.cRenderDepth)
            passId = 4;
         else if (mRenderMode == eVisualEditorRenderMode.cRenderHeightForSimrep)
            passId = 5;



         int passCount = GrannyManager2.s_modelGPUShader.Begin(FX.DoNotSaveState);



         Matrix worldMat = BRenderDevice.getDevice().GetTransform(TransformType.World);


         /* Since I'm going to need it constantly, I dereference the composite
         transform buffer for the model's current world-space pose.  This
         buffer holds the transforms that move vertices from the position
         in which they were modeled to their current position in world space. */
         Matrix* CompositeBuffer = GrannyGetWorldPoseComposite4x4Array(mWorldPose);

         /* Rendering the meshes is a simple matter of loading the appropriate
         vertex and index buffers, and then rendering by material group. */
         int meshIndex;
         int meshCount = model.mMeshes.Count;
         for (meshIndex = 0; meshIndex < meshCount; ++meshIndex)
         {
            GrannyMesh mesh = model.mMeshes[meshIndex];

            BRenderDevice.getDevice().VertexDeclaration = mesh.mVDecl;

            // First I load the mesh's index buffer.
            if (mesh.mIndexBuffer != null)
            {
               BRenderDevice.getDevice().Indices = mesh.mIndexBuffer;

               // Then, I dereference the index table that maps mesh bone indices
               // to bone indices in the model.
               int *ToBoneIndices = GrannyGetMeshBindingToBoneIndices(mesh.mGrannyBinding);

               // Next I load the mesh's vertex buffer, or deform
               // into a temporary buffer and load that, depending on
               // whether the mesh is rigid or not.
               int VertexCount = GrannyGetMeshVertexCount(mesh.mGrannyMesh);
               if(mesh.mVertexBuffer != null)
               {
                  // It's a rigid mesh, so I load the mesh's static vertex
                  // buffer.
                  BRenderDevice.getDevice().SetStreamSource(0, mesh.mVertexBuffer, 0, mesh.mVertexSize);

                  // Now I look up the transform for this mesh, and load it.
                  Matrix* xform = &CompositeBuffer[ToBoneIndices[0]];
                  Matrix compositeMatrix = Matrix.Multiply(*xform, worldMat);

                  BRenderDevice.getDevice().SetTransform(TransformType.World, compositeMatrix);
               }
               else if(mesh.mGrannyDeformer != null)
               {
                  // It's a deformable mesh, so I grab the next available
                  // AGP vertex buffer.
                  VertexBuffer curVB = GrannyManager2.MutableVertexBufferRing[GrannyManager2.MutableVertexBufferIndex++ % GrannyManager2.MaxMutableVertexBufferCount];



/*
                  IntPtr pMarshaledMem = System.Runtime.InteropServices.Marshal.AllocHGlobal(VertexBufferSize);
                  byte[] tGrnVerts = new byte[VertexBufferSize];

                  fixed (granny_data_type_definition* grnPD = grnDTD)
                     GrannyCopyMeshVertices(grMesh, grnPD, (void*)pMarshaledMem);
                  System.Runtime.InteropServices.Marshal.Copy(pMarshaledMem, tGrnVerts, 0, VertexBufferSize);
                  System.Runtime.InteropServices.Marshal.FreeHGlobal(pMarshaledMem);

                  byte[] d3dVerts = new byte[VertexBufferSize];

                  //swizzle the granny verts to be d3d friendly before copying them to the device
                  swzzlGrnyVertsToD3DVerts(tGrnVerts, grnVD, mVertexSize, VertexCount,
                                           ref d3dVerts, mVDecl);



                  mVertexBuffer = new VertexBuffer(BRenderDevice.getDevice(), VertexBufferSize, Usage.WriteOnly, VertexFormats.None, Pool.Managed);
                  GraphicsStream stream = mVertexBuffer.Lock(0, 0, LockFlags.None);

                  stream.Write(d3dVerts, 0, VertexBufferSize);
                  mVertexBuffer.Unlock();
                  stream.Close();

                  ////generate bounding box
                  //genBBOX(d3dVerts, prim.mVertexSize, prim.mNumVerts, ref mesh.mBBox);

                  tGrnVerts = null;
*/


                  // Now I tell Granny to deform the vertices of the mesh
                  // with the current world pose of the model, and dump
                  // the results into the AGP vertex buffer.
                  using (GraphicsStream stream = curVB.Lock(0, 0, LockFlags.None))
                  {
                     byte* DestVertices = (byte*)stream.InternalDataPointer;

                     GrannyDeformVertices(mesh.mGrannyDeformer,
                                  ToBoneIndices, (float*)CompositeBuffer,
                                  VertexCount,
                                  GrannyGetMeshVertices(mesh.mGrannyMesh),
                                  DestVertices);

                     curVB.Unlock();
                  }
                  /*
                  curVB.Lock(0, 0, &DestVertices, 0);
                  GrannyDeformVertices(Mesh.GrannyDeformer, 
                               ToBoneIndices, (float *)CompositeBuffer,
                               VertexCount,
                               GrannyGetMeshVertices(Mesh.GrannyMesh),
                               DestVertices);
                  curVB.Unlock();
                  */

                  // Then I load that vertex buffer. 
                  BRenderDevice.getDevice().SetStreamSource(0, curVB, 0, mesh.mVertexSize);//sizeof(granny_pnt332_vertex));
               }
               else
               {
                  // If we got here, we had a bad mesh (the CreateMesh call
                  // should've popped up an error message announcing this
                  // earlier), so we skip it.
                  continue;
               }

               /* Now both the indices and vertices are loaded, so I can
               render.  I grab the material groups and spin over them,
               changing to the appropriate texture and rendering each batch.
               A more savvy rendering loop might have instead built a
               sorted list of material groups to minimize texture changes,
               etc., but this is the most basic way to render. */
               int groupCount = GrannyGetMeshTriangleGroupCount(mesh.mGrannyMesh);
               granny_tri_material_group *triMatGroup = GrannyGetMeshTriangleGroups(mesh.mGrannyMesh);
               while (groupCount-- != 0)
               {
                  bool applyAlphaTest = false;

                  if (triMatGroup->MaterialIndex < mesh.mTextureCount &&
                     mRenderMode != eVisualEditorRenderMode.cRenderDepthPeelAO &&
                     mRenderMode != eVisualEditorRenderMode.cRenderDepth)
                  {
                     GrannyTexture textureStage1 = GrannyManager2.getTexture(mesh.mTextureIndices[triMatGroup->MaterialIndex].diffuse);
                     GrannyTexture textureStage2 = GrannyManager2.getTexture(mesh.mTextureIndices[triMatGroup->MaterialIndex].xform);
                     GrannyTexture textureStage3 = GrannyManager2.getTexture(mesh.mTextureIndices[triMatGroup->MaterialIndex].opacity);

                     GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderUseAlbedoHandle, false);
                     GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderUsePlayerColorHandle, false);
                     GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderUseOpacityHandle, false);


                     if (textureStage1 != null)
                     {
                        GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderUseAlbedoHandle, true);
                        GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderAlbedoTexHandle, textureStage1.mTexture.mTexture);
                     }

                     if (bUsePlayerColor && (textureStage2 != null))
                     {
                        GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderUsePlayerColorHandle, true);
                        GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderXformTexHandle, textureStage2.mTexture.mTexture);
                        GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderPlayerColorHandle, new Vector4(playerColor.X, playerColor.Y, playerColor.Z, 1.0f));
                     }

                     if (textureStage3 != null)
                     {
                        GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderUseOpacityHandle, true);
                        GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderOpacityTexHandle, textureStage3.mTexture.mTexture);

                        applyAlphaTest = true;
                     }


                     /*
                     if (textureStage1 != null)
                     {
                        if (bUsePlayerColor && (textureStage2 != null))
                        {
                           BRenderDevice.getDevice().SetTexture(0, textureStage1.mTexture);
                           BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
                           BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TextureColor);
                           BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
                           BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TextureColor);

                           BRenderDevice.getDevice().SetTexture(1, textureStage2.mTexture);
                           BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, (int)BRenderDevice.D3DCOLOR_COLORVALUE(playerColor.X, playerColor.Y, playerColor.Z, 1));
                           BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.TextureCoordinateIndex, 0);
                           BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.ColorOperation, (int)TextureOperation.Lerp);
                           BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.ColorArgument0, (int)TextureArgument.TextureColor);
                           BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.ColorArgument1, (int)TextureArgument.Current);
                           BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.ColorArgument2, (int)TextureArgument.TFactor);
                           BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
                           BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.AlphaArgument1, (int)TextureArgument.Current);

                           if (textureStage3 != null)
                           {
                              applyAlphaTest = true;
                              BRenderDevice.getDevice().SetTexture(2, textureStage3.mTexture);
                              BRenderDevice.getDevice().SetTextureStageState(2, TextureStageStates.TextureCoordinateIndex, 0);
                              BRenderDevice.getDevice().SetTextureStageState(2, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
                              BRenderDevice.getDevice().SetTextureStageState(2, TextureStageStates.ColorArgument1, (int)TextureArgument.Current);
                              BRenderDevice.getDevice().SetTextureStageState(2, TextureStageStates.ColorArgument2, (int)TextureArgument.Diffuse);
                              BRenderDevice.getDevice().SetTextureStageState(2, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
                              BRenderDevice.getDevice().SetTextureStageState(2, TextureStageStates.AlphaArgument1, (int)TextureArgument.Current);
                              BRenderDevice.getDevice().SetTextureStageState(2, TextureStageStates.AlphaArgument2, (int)TextureArgument.TextureColor);

                              BRenderDevice.getDevice().SetTextureStageState(3, TextureStageStates.ColorOperation, (int)TextureOperation.Disable);
                           }
                           else
                           {
                              BRenderDevice.getDevice().SetTextureStageState(2, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
                              BRenderDevice.getDevice().SetTextureStageState(2, TextureStageStates.ColorArgument1, (int)TextureArgument.Current);
                              BRenderDevice.getDevice().SetTextureStageState(2, TextureStageStates.ColorArgument2, (int)TextureArgument.Diffuse);
                              BRenderDevice.getDevice().SetTextureStageState(2, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
                              BRenderDevice.getDevice().SetTextureStageState(2, TextureStageStates.AlphaArgument1, (int)TextureArgument.Current);

                              BRenderDevice.getDevice().SetTextureStageState(3, TextureStageStates.ColorOperation, (int)TextureOperation.Disable);
                           }
                        }
                        else
                        {
                           BRenderDevice.getDevice().SetTexture(0, textureStage1.mTexture);
                           BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
                           BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
                           BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
                           BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
                           BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TextureColor);

                           if (textureStage3 != null)
                           {
                              applyAlphaTest = true;
                              BRenderDevice.getDevice().SetTexture(1, textureStage3.mTexture);
                              BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.TextureCoordinateIndex, 0);
                              BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
                              BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.ColorArgument1, (int)TextureArgument.Current);
                              BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
                              BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
                              BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.AlphaArgument1, (int)TextureArgument.Current);
                              BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.AlphaArgument2, (int)TextureArgument.TextureColor);

                              BRenderDevice.getDevice().SetTextureStageState(2, TextureStageStates.ColorOperation, (int)TextureOperation.Disable);
                           }
                           else
                           {
                              BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.ColorOperation, (int)TextureOperation.Disable);
                           }
                        }
                     }
                     else
                     {
                        BRenderDevice.getDevice().SetTexture(0, null);
                        BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Disable);
                     }
                     */
                  }

                  //VP Matrix
                  Matrix g_matWorld;
                  Matrix g_matView;
                  Matrix g_matProj;

                  g_matWorld = BRenderDevice.getDevice().Transform.World;
                  g_matView = BRenderDevice.getDevice().Transform.View;
                  g_matProj = BRenderDevice.getDevice().Transform.Projection;
                  Matrix worldViewProjection = g_matWorld * g_matView * g_matProj;
                  Matrix viewProjection =  g_matView * g_matProj;

                  GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderWHandle, g_matWorld);
                  GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderWVPHandle, worldViewProjection);
                  GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderVPHandle, viewProjection);
                  

                  Vector3 eyePos = CoreGlobals.getEditorMain().mITerrainShared.getCameraPos();
                  GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderWorldCameraPosHandle, new Vector4(eyePos.X, eyePos.Y, eyePos.Z, 1.0f));


                  if (applyAlphaTest && 
                     mRenderMode != eVisualEditorRenderMode.cRenderDepthPeelAO &&
                     mRenderMode != eVisualEditorRenderMode.cRenderDepth)
                  {
                     BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaTestEnable, true);
                     BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaFunction, (int)Compare.Greater);
                     BRenderDevice.getDevice().SetRenderState(RenderStates.ReferenceAlpha, (int)0x80);
                  }



                  GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderWHandle, g_matWorld);
                  GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderWVPHandle, worldViewProjection);
                  GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderVPHandle, g_matView * g_matProj);


                  // Apply pass 1
                  GrannyManager2.s_modelGPUShader.BeginPass(passId);
                  BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 
                                          0,
                                          0, 
                                          VertexCount,
                                          3 * triMatGroup->TriFirst,
                                          triMatGroup->TriCount);
                  GrannyManager2.s_modelGPUShader.EndPass();

                  // Apply pass 2
                  if (mRenderMode == eVisualEditorRenderMode.cRenderFullOverlay)
                  {
                     // Set state
                     BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.WireFrame);
                     BRenderDevice.getDevice().SetRenderState(RenderStates.DepthBias, (float)-0.00005f);

                     BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
                     BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, true);


                     GrannyManager2.s_modelGPUShader.BeginPass(2);
                     BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList,
                                             0,
                                             0,
                                             VertexCount,
                                             3 * triMatGroup->TriFirst,
                                             triMatGroup->TriCount);
                     GrannyManager2.s_modelGPUShader.EndPass();


                     // Restore state
                     BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.Solid);
                     BRenderDevice.getDevice().SetRenderState(RenderStates.DepthBias, (float)0.0f);

                     BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
                     BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, false);
                  }



                  if (applyAlphaTest)
                  {
                     BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaTestEnable, false);
                  }

                  ++triMatGroup;
               }
            }


            // Restore world transform
            BRenderDevice.getDevice().SetTransform(TransformType.World, worldMat);
         }



         GrannyManager2.s_modelGPUShader.End();


         // Restore state
         BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.Solid);

         return true;
      }


      public bool renderSkeleton()
      {
         GrannyModel model = GrannyManager2.getModel(mModelIndex);
         if (model == null)
            return false;

         sampleAnimations();

         granny_skeleton* skeleton = GrannyGetSourceSkeleton(mGrannyModelInstance);
         granny_bone* bones = skeleton->Bones;
         int boneCount = skeleton->BoneCount;
         int boneIndex;
 
         Vector3 pointsColor = new Vector3(1.0f, 0.0f, 0.0f);
         Vector3 linesColor = new Vector3(1.0f, 0.5f, 0.0f);


         // Update Vertex and Index buffers
         //

         // update verts
         SkeletonVert[] skeletonVerts = new SkeletonVert[boneCount];
 
         for (boneIndex = 0; boneIndex < boneCount; boneIndex++)
         {
            float* transform = GrannyGetWorldPose4x4(mWorldPose, boneIndex);
            skeletonVerts[boneIndex].xyz = new Vector3(transform[12], transform[13], transform[14]);
         }

         using (GraphicsStream stream = GrannyManager2.SkeletonVB.Lock(0, 0, LockFlags.None))
         {
            stream.Write(skeletonVerts);
            GrannyManager2.SkeletonVB.Unlock();
         }

         // update indices
         int[] lineIndices = new int[boneCount * 2];

         for (boneIndex = 0; boneIndex < boneCount; boneIndex++)
         {
            int thisIndex = boneIndex;
            int parentIndex = bones[boneIndex].ParentIndex;

            if (parentIndex == -1)
               parentIndex = thisIndex;

            lineIndices[boneIndex * 2] = thisIndex;
            lineIndices[(boneIndex * 2) + 1] = parentIndex;
         }

         using (GraphicsStream stream = GrannyManager2.SkeletonIB.Lock(0, 0, LockFlags.None))
         {
            stream.Write(lineIndices);
            GrannyManager2.SkeletonIB.Unlock();
         }


         // Set common state 
         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;


         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TFactor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TFactor);

         BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, true);

         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.Always);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferWriteEnable, false);


         // Draw Points
         //
         float colorFade = 0.75f;
         float alphaFade = 0.75f;
         BRenderDevice.getDevice().SetRenderState(RenderStates.PointSize, 4.0f);
         BRenderDevice.getDevice().VertexFormat = VertexFormats.Position;
         BRenderDevice.getDevice().Indices = null;
         BRenderDevice.getDevice().SetStreamSource(0, GrannyManager2.SkeletonVB, 0);

            // - (first pass - non-ocluded)
            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.LessEqual);
            BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, (int)BRenderDevice.D3DCOLOR_COLORVALUE(pointsColor.X, pointsColor.Y, pointsColor.Z, 1));

            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.PointList, 0, boneCount);

            // - (first pass - non-ocluded)
            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.Greater);
            BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, (int)BRenderDevice.D3DCOLOR_COLORVALUE(pointsColor.X * colorFade, pointsColor.Y * colorFade, pointsColor.Z * colorFade, alphaFade));

            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.PointList, 0, boneCount);

         // Draw Lines 
         //
         BRenderDevice.getDevice().VertexFormat = VertexFormats.Position;
         BRenderDevice.getDevice().Indices = GrannyManager2.SkeletonIB;
         BRenderDevice.getDevice().SetStreamSource(0, GrannyManager2.SkeletonVB, 0);

            // - (first pass - non-ocluded)
            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.LessEqual);
            BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, (int)BRenderDevice.D3DCOLOR_COLORVALUE(linesColor.X, linesColor.Y, linesColor.Z, 1));

            BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.LineList, 0, 0, boneCount, 0, boneCount);

            // - (second pass - ocluded)
            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.Greater);
            BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, (int)BRenderDevice.D3DCOLOR_COLORVALUE(linesColor.X * colorFade, linesColor.Y * colorFade, linesColor.Z * colorFade, alphaFade));

            BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.LineList, 0, 0, boneCount, 0, boneCount);


         // Restore state
         BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.LessEqual);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferWriteEnable, true);

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);

         return true;
      }

      public bool renderBoneAxis()
      {
         GrannyModel model = GrannyManager2.getModel(mModelIndex);
         if (model == null)
            return false;

         sampleAnimations();

         Matrix worldMat = BRenderDevice.getDevice().GetTransform(TransformType.World);


         granny_skeleton* skeleton = GrannyGetSourceSkeleton(mGrannyModelInstance);
         int boneCount = skeleton->BoneCount;
         int boneIndex;

         // Set common state 
         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);

         BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, true);

         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.Always);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferWriteEnable, false);

         BRenderDevice.getDevice().VertexFormat = VertexFormats.Position | VertexFormats.Diffuse;
         BRenderDevice.getDevice().SetStreamSource(0, GrannyManager2.AxisVB, 0);

         // Draw Bone Axis
         for (boneIndex = 0; boneIndex < boneCount; boneIndex++)
         {
            float* transform = GrannyGetWorldPose4x4(mWorldPose, boneIndex);

            Matrix xform;
            xform.M11 = transform[0];  xform.M12 = transform[1];  xform.M13 = transform[2];  xform.M14 = transform[3];
            xform.M21 = transform[4];  xform.M22 = transform[5];  xform.M23 = transform[6];  xform.M24 = transform[7];
            xform.M31 = transform[8];  xform.M32 = transform[9];  xform.M33 = transform[10]; xform.M34 = transform[11];
            xform.M41 = transform[12]; xform.M42 = transform[13]; xform.M43 = transform[14]; xform.M44 = transform[15];

            xform.Multiply(worldMat);

            BRenderDevice.getDevice().SetTransform(TransformType.World, xform);

            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.LineList, 0, 3);
         }

         // Restore state
         BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.LessEqual);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferWriteEnable, true);

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);


         // Restore world transform
         BRenderDevice.getDevice().SetTransform(TransformType.World, worldMat);

         return true;
      }

      public bool renderBoneNames()
      {
         GrannyModel model = GrannyManager2.getModel(mModelIndex);
         if (model == null)
            return false;

         sampleAnimations();

         Matrix projMat = BRenderDevice.getDevice().GetTransform(TransformType.Projection);
         Matrix viewMat = BRenderDevice.getDevice().GetTransform(TransformType.View);
         Matrix worldMat = BRenderDevice.getDevice().GetTransform(TransformType.World);

         Viewport vport;
         vport = BRenderDevice.getDevice().Viewport;

         granny_skeleton* skeleton = GrannyGetSourceSkeleton(mGrannyModelInstance);
         granny_bone* bones = skeleton->Bones;
         int boneCount = skeleton->BoneCount;
         int boneIndex;

         // Draw Bone Names
         for (boneIndex = 0; boneIndex < boneCount; boneIndex++)
         {
            granny_bone* curBone = &bones[boneIndex];

            string name = Marshal.PtrToStringAnsi(new IntPtr((byte*)curBone->Name));

            float* transform = GrannyGetWorldPose4x4(mWorldPose, boneIndex);
            Vector3 pos3D = new Vector3(transform[12], transform[13], transform[14]);
            Vector3 pos2D;

            pos2D = Vector3.Project(pos3D, vport, projMat, viewMat, worldMat);
            pos2D.X += 5.0f;
            pos2D.Y -= 15.0f;

            GrannyManager2.s_font.DrawText(null, name, new System.Drawing.Rectangle((int)pos2D.X, (int)pos2D.Y, 0, 0),
                                          DrawTextFormat.NoClip, System.Drawing.Color.Red);
         }

         return true;
      }

      public bool renderMeshBoxes()
      {
         GrannyModel model = GrannyManager2.getModel(mModelIndex);
         if (model == null)
            return false;

         sampleAnimations();

         // Set common state 
         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;


         BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TFactor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TFactor);

         BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, Color.Green.ToArgb());

         int meshIndex;
         int meshCount = model.mMeshes.Count;
         for (meshIndex = 0; meshIndex < meshCount; ++meshIndex)
         {
            GrannyMesh mesh = model.mMeshes[meshIndex];

            granny_mesh* pMesh = mesh.mGrannyMesh;
            granny_mesh_binding* pMeshBinding = mesh.mGrannyBinding;

            if (pMeshBinding == null)
               continue;

            for (long j = 0; j < pMesh->BoneBindingCount; j++)
            {
               int boneIndex = GrannyGetMeshBindingToBoneIndices(pMeshBinding)[j];
               granny_bone_binding boneBinding = pMesh->BoneBindings[j];

               float* transform = GrannyGetWorldPose4x4(mWorldPose, boneIndex);

               Matrix xform;
               xform.M11 = transform[0]; xform.M12 = transform[1]; xform.M13 = transform[2]; xform.M14 = transform[3];
               xform.M21 = transform[4]; xform.M22 = transform[5]; xform.M23 = transform[6]; xform.M24 = transform[7];
               xform.M31 = transform[8]; xform.M32 = transform[9]; xform.M33 = transform[10]; xform.M34 = transform[11];
               xform.M41 = transform[12]; xform.M42 = transform[13]; xform.M43 = transform[14]; xform.M44 = transform[15];

               BOrientedBoundingBox orientedBox = new BOrientedBoundingBox();
               orientedBox.construct(boneBinding.OBBMin, boneBinding.OBBMax, xform);

               BRenderDebugOOBB box = new BRenderDebugOOBB(orientedBox.mCenter, orientedBox.mExtents, orientedBox.mOrientation, Color.Green.ToArgb(), false);

               box.render();
            }
         }


         // Restore state
         BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, false);

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);

         return true;
      }

      public bool renderStatsInfo()
      {
         GrannyModel model = GrannyManager2.getModel(mModelIndex);
         if (model == null)
            return false;

         int startVerticalPosition = GrannyManager2.s_statsInfoVerticalPosition;

         granny_skeleton* skeleton = GrannyGetSourceSkeleton(mGrannyModelInstance);
         granny_bone* bones = skeleton->Bones;
         int boneCount = skeleton->BoneCount;

         granny_file_info* info = model.getGrannyFileInfo();
         int vertCount = getTotalVerts(info);
         
         GrannyManager2.getTotalFaces(info);
         
         ArrayList meshes = getMeshNames(info);
         ArrayList materials = getMaterialNames(info);

         string text = "";
         text += "base GR2: \nmeshes: \nvertices: \nbones: \nmaterials: \n";

         GrannyManager2.s_font.DrawText(null, text, 10, startVerticalPosition, System.Drawing.Color.Orange);
         text = "";
         text += String.Concat(model.mName, "\n", meshes.Count, "\n", vertCount, "\n", boneCount);
         for(int i = 0; i < materials.Count; ++i)
         {
            text += String.Concat("\n", materials[i]);
         }
         GrannyManager2.s_font.DrawText(null, text, 80, startVerticalPosition, System.Drawing.Color.LightGreen);


         // Count number of lines
         int numLines = 1;

         foreach(char character in text)
         {
            if (character.Equals('\n'))
               numLines++;
         }

         // Add one more for a space line
         numLines++;

         GrannyManager2.incrementStatsInfoVerticalPosition(numLines * 16);
         return true;
      }
#endregion



      public void setClock(float seconds)
      {
	      // Check if we have an instance.
         if (mGrannyModelInstance == null)
		      return;

	      // Cache time
	      mAnimClock = seconds;

	      // Set time.
         GrannySetModelClock(mGrannyModelInstance, mAnimClock);

	      // Kill any unneeded controls.
         GrannyFreeCompletedModelControls(mGrannyModelInstance);
      }

      public void updateMotion(float delta, ref Matrix worldMatrix)
      {
         // Check if we have an instance.
         if (mGrannyModelInstance == null)
            return;

         fixed (Matrix* fixedMatrix = &worldMatrix)
         {
            GrannyUpdateModelMatrix(mGrannyModelInstance, delta, (float*)fixedMatrix, (float*)fixedMatrix, false);
         }
      }

      public float getClock()
      {
         return (mAnimClock);
      }

      public void update(float elapsedTime)
      {
         setClock(mAnimClock+elapsedTime);
      }


      public bool getBone(string boneName, ref Matrix pMatrix)
      {
         GrannyModel model = GrannyManager2.getModel(mModelIndex);
         if (model == null)
            return false;

         return getBone(model.getBoneHandle(boneName), ref pMatrix);
      }

      public bool getBone(int boneIndex, ref Matrix pMatrix)
      {
         if (boneIndex == -1)
            return false;

         Matrix resultMatrix = Matrix.Identity;
         Matrix offsetMatrix = Matrix.Identity;
         granny_skeleton* pSkeleton = GrannyGetSourceSkeleton(mGrannyModelInstance);

         if(boneIndex > pSkeleton->BoneCount)
         {
            return false;
         }
         GrannySampleModelAnimations(mGrannyModelInstance, 0, boneIndex + 1, GrannyManager2.SharedLocalPose);
         GrannyGetWorldMatrixFromLocalPose(pSkeleton, boneIndex, GrannyManager2.SharedLocalPose, (float*)&offsetMatrix, (float*)&resultMatrix, null, null);

         pMatrix = resultMatrix;
         return true;
      }


      public void getBoneNames(ref List<string> bones)
      {
         granny_skeleton* pSkeleton = GrannyGetSourceSkeleton(mGrannyModelInstance);

         // This is the number of bones in the skeleton
         int boneCount = pSkeleton->BoneCount;

         for (int boneIndex = 0; boneIndex < boneCount; ++boneIndex)
         {
            // This is the boneIndex'th bone in the skeleton
            granny_bone* bone = &pSkeleton->Bones[boneIndex];

            IntPtr boneName = new IntPtr((void*)bone->Name);
            bones.Add(System.Runtime.InteropServices.Marshal.PtrToStringAnsi(boneName));
         }
      }


      public unsafe bool testRayIntersection(Vector3 r0, Vector3 rD, Matrix worldMat)
      {
         GrannyModel model = GrannyManager2.getModel(mModelIndex);
         if (model == null)
            return false;

         sampleAnimations();

         Matrix* CompositeBuffer = GrannyGetWorldPoseComposite4x4Array(mWorldPose);

         int meshIndex;
         int meshCount = model.mMeshes.Count;
         for (meshIndex = 0; meshIndex < meshCount; ++meshIndex)
         {
            GrannyMesh mesh = model.mMeshes[meshIndex];

            // Early out (test bounding box intersection)
            bool bbCollision = false;

            // TODO:  (fix this) should test against BB first
            bbCollision = true;
            /*
            granny_mesh* pMesh = mesh.mGrannyMesh;
            granny_mesh_binding* pMeshBinding = mesh.mGrannyBinding;

            for (long j = 0; j < pMesh->BoneBindingCount; j++)
            {
               int boneIndex = GrannyGetMeshBindingToBoneIndices(pMeshBinding)[j];
               granny_bone_binding boneBinding = pMesh->BoneBindings[j];

               float* transform = GrannyGetWorldPose4x4(mWorldPose, boneIndex);

               Matrix xform;
               xform.M11 = transform[0]; xform.M12 = transform[1]; xform.M13 = transform[2]; xform.M14 = transform[3];
               xform.M21 = transform[4]; xform.M22 = transform[5]; xform.M23 = transform[6]; xform.M24 = transform[7];
               xform.M31 = transform[8]; xform.M32 = transform[9]; xform.M33 = transform[10]; xform.M34 = transform[11];
               xform.M41 = transform[12]; xform.M42 = transform[13]; xform.M43 = transform[14]; xform.M44 = transform[15];

               BOrientedBoundingBox orientedBox = new BOrientedBoundingBox();
               orientedBox.construct(boneBinding.OBBMin, boneBinding.OBBMax, xform);

               float tVal = 0.0f;
               if (BMathLib.rayOOBBIntersect(orientedBox.mCenter, orientedBox.mExtents, orientedBox.mOrientation, r0, rD, ref tVal))
               {
                  bbCollision = true;
                  break;
               }
            }
            */

            if (!bbCollision)
               return (false);

            // Test against mesh triangles
            if (mesh.testRayIntersection(r0, rD, worldMat, CompositeBuffer))
            {
               return (true);
            }
         }

         return false;
      }

      public unsafe void burnIntoObjFile(OBJFile obj, int objectId, Matrix worldMat)
      {
         GrannyModel model = GrannyManager2.getModel(mModelIndex);
         if (model == null)
            return;

         sampleAnimations();

         Matrix* CompositeBuffer = GrannyGetWorldPoseComposite4x4Array(mWorldPose);

         int meshIndex;
         int meshCount = model.mMeshes.Count;
         for (meshIndex = 0; meshIndex < meshCount; ++meshIndex)
         {
            GrannyMesh mesh = model.mMeshes[meshIndex];

            mesh.burnIntoObjFile(obj, objectId, worldMat, CompositeBuffer);
         }
      }
   }

   public struct SkeletonVert
   {
      public Vector3 xyz;
   }

   public struct AxisVert
   {
      public Vector3 xyz;
      public int     color;
   }

   public unsafe class GrannyManager2 : GrannyBridge
   {
      static List<GrannyModel> mModels = new List<GrannyModel>();
      static List<GrannyAnimation> mAnimations = new List<GrannyAnimation>();
      static List<GrannyInstance> mInstances = new List<GrannyInstance>();
      static List<GrannyTexture> mTextures = new List<GrannyTexture>();


      public static bool isInitialized = false;

      // An animation blending buffer that I use for all models, since
      // it's contents don't need to be preserved
      public static granny_local_pose* SharedLocalPose = null;

      // A vertex blending buffer that I use for all meshes, since
      // it's contents also don't need to be preserved

      public static int MaxMutableVertexBufferSize = 640000;
      public static int MaxMutableVertexBufferCount = 4;
      public static int MutableVertexBufferIndex;
      public static VertexBuffer[] MutableVertexBufferRing;


      public static VertexBuffer SkeletonVB = null;
      public static IndexBuffer SkeletonIB = null;
      public static Microsoft.DirectX.Direct3D.Font s_font = null;

      public static VertexBuffer AxisVB = null;


      private static FileSystemWatcher s_fileWatcher = null;
      private static List<int> s_modelIndexesToReload = new List<int>();
      private static List<int> s_animationIndexesToReload = new List<int>();
      private static List<int> s_textureIndexesToReload = new List<int>();

      public static int s_statsInfoVerticalPosition = 10;



      public static Microsoft.DirectX.Direct3D.Effect s_modelGPUShader = null;
      public static EffectHandle s_shaderWHandle;
      public static EffectHandle s_shaderVPHandle;
      public static EffectHandle s_shaderWVPHandle;

      public static EffectHandle s_shaderAlbedoTexHandle;
      public static EffectHandle s_shaderXformTexHandle;
      public static EffectHandle s_shaderOpacityTexHandle;
      public static EffectHandle s_shaderPlayerColorHandle;

      public static EffectHandle s_shaderUseAlbedoHandle;
      public static EffectHandle s_shaderUsePlayerColorHandle;
      public static EffectHandle s_shaderUseOpacityHandle;

      public static EffectHandle s_shaderWorldCameraPosHandle;

      public static EffectHandle s_shaderPlanarFogEnabledHandle;
      public static EffectHandle s_shaderRadialFogColorHandle;
      public static EffectHandle s_shaderPlanarFogColorHandle;
      public static EffectHandle s_shaderFogParamsHandle;

      public static EffectHandle s_shaderAOPrevDepthTexHandle;

      static public unsafe void init()
      {
         if (isInitialized)
            return;

         int maxBoneCount = 128;

         SharedLocalPose = GrannyNewLocalPose(maxBoneCount);

         MutableVertexBufferRing = new VertexBuffer[MaxMutableVertexBufferCount];
         {
            for (int BufferIndex = 0; BufferIndex < MaxMutableVertexBufferCount; ++BufferIndex)
            {
               /* Note that I pass D3DUSAGE_DYNAMIC here - that's
                  important, because we're constantly going to be
                  writing into these buffers, so we want D3D to put
                  them in AGP memory where they can be picked up
                  directly by the video card. */
               //BRenderDevice.getDevice()->CreateVertexBuffer(MaxMutableVertexBufferSize, D3DUSAGE_DYNAMIC, GlobalVertexFormat, D3DPOOL_DEFAULT,
               //               &GlobalScene.MutableVertexBufferRing[BufferIndex]);

               MutableVertexBufferRing[BufferIndex] = new VertexBuffer(BRenderDevice.getDevice(), MaxMutableVertexBufferSize, Usage.None, VertexFormats.None, Pool.Managed);
            }
         }


         // Release first
         if (SkeletonVB != null)
         {
            SkeletonVB.Dispose();
            SkeletonVB = null;
         }
         SkeletonVB = new VertexBuffer(typeof(SkeletonVert), maxBoneCount, BRenderDevice.getDevice(), Usage.WriteOnly, VertexFormats.Position, Pool.Default);

         if (SkeletonIB != null)
         {
            SkeletonIB.Dispose();
            SkeletonIB = null;
         }
         SkeletonIB = new IndexBuffer(typeof(int), maxBoneCount * 2, BRenderDevice.getDevice(), Usage.WriteOnly, Pool.Default);


         if (AxisVB != null)
         {
            AxisVB.Dispose();
            AxisVB = null;
         }
         AxisVB = new VertexBuffer(typeof(AxisVert), 6, BRenderDevice.getDevice(), Usage.WriteOnly, VertexFormats.Position | VertexFormats.Diffuse, Pool.Managed);


         AxisVert[] axisVerts = new AxisVert[6];

         float axisLength = 0.2f;

         axisVerts[0].xyz = new Vector3(0.0f, 0.0f, 0.0f);
         axisVerts[0].color = (int)BRenderDevice.D3DCOLOR_COLORVALUE(1, 0, 0, 1);
         axisVerts[1].xyz = new Vector3(axisLength, 0.0f, 0.0f);
         axisVerts[1].color = (int)BRenderDevice.D3DCOLOR_COLORVALUE(1, 0, 0, 1);
         axisVerts[2].xyz = new Vector3(0.0f, 0.0f, 0.0f);
         axisVerts[2].color = (int)BRenderDevice.D3DCOLOR_COLORVALUE(0, 1, 0, 1);
         axisVerts[3].xyz = new Vector3(0.0f, axisLength, 0.0f);
         axisVerts[3].color = (int)BRenderDevice.D3DCOLOR_COLORVALUE(0, 1, 0, 1);
         axisVerts[4].xyz = new Vector3(0.0f, 0.0f, 0.0f);
         axisVerts[4].color = (int)BRenderDevice.D3DCOLOR_COLORVALUE(0, 0, 1, 1);
         axisVerts[5].xyz = new Vector3(0.0f, 0.0f, axisLength);
         axisVerts[5].color = (int)BRenderDevice.D3DCOLOR_COLORVALUE(0, 0, 1, 1);

         using (GraphicsStream stream = AxisVB.Lock(0, 0, LockFlags.None))
         {
            stream.Write(axisVerts);
            AxisVB.Unlock();
         }


         string fontName = "Arial";
         s_font = new Microsoft.DirectX.Direct3D.Font(BRenderDevice.getDevice(), 16, 0, FontWeight.Regular,
             1, false, CharacterSet.Default, Precision.Default, FontQuality.Default,
             PitchAndFamily.DefaultPitch | PitchAndFamily.FamilyDoNotCare, fontName);



         s_fileWatcher = new FileSystemWatcher();
         s_fileWatcher.Path = EditorCore.CoreGlobals.getWorkPaths().mGameArtDirectory;
         s_fileWatcher.Filter = "*.*";
         //s_fileWatcher.NotifyFilter = NotifyFilters.LastWrite;
         s_fileWatcher.NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.CreationTime | NotifyFilters.Size;

         s_fileWatcher.Changed += new FileSystemEventHandler(FileWatcher_Changed);
         s_fileWatcher.IncludeSubdirectories = true;
         s_fileWatcher.EnableRaisingEvents = true;



         // Load Our Shader
         string errors = "";
         if (s_modelGPUShader == null || s_modelGPUShader.Disposed == true)
         {
            try
            {
               s_modelGPUShaderHandle = BRenderDevice.getShaderManager().getShader(CoreGlobals.getWorkPaths().mEditorShaderDirectory + "\\gpuModel.fx", GrannyManager2.mainShaderParams);
               s_modelGPUShader = s_modelGPUShaderHandle.mShader;
               GrannyManager2.mainShaderParams(null);

               if (s_modelGPUShader == null)
               {
                  MessageBox.Show("Shader did not load:\n " + errors);
               }
            }
            catch (System.Exception ex)
            {
               MessageBox.Show("Shader did not load");
               throw (ex);
            }
         }

         isInitialized = true;
      }
      static public void destroy()
      {
         unload();
         if(MutableVertexBufferRing!=null)
         {
            for (int i = 0; i < MaxMutableVertexBufferCount; i++)
            {
               if (MutableVertexBufferRing[i] != null)
               {
                  MutableVertexBufferRing[i].Dispose();
                  MutableVertexBufferRing[i] = null;
               }
            } 
            MutableVertexBufferRing = null;
         }

         if (SkeletonVB != null)
         {
            SkeletonVB.Dispose();
            SkeletonVB = null;
         }
         if (SkeletonIB != null)
         {
            SkeletonIB.Dispose();
            SkeletonIB = null;
         }
         if (AxisVB != null)
         {
            AxisVB.Dispose();
            AxisVB = null;
         }
         if (s_modelGPUShader !=null)
         {
            s_modelGPUShader.Dispose();
            s_modelGPUShader = null;
         }
         visual.destoyStatics();
      }

      static public void unload()   //called after a map is unloaded
      {
         for (int i = 0; i < mTextures.Count; i++)
            mTextures[i].deinit();
         mTextures.Clear();

         for (int i = 0; i < mAnimations.Count; i++)
            mAnimations[i].deinit();
         mAnimations.Clear();

         for (int i = 0; i < mInstances.Count; i++)
            mInstances[i].deinit();
         mInstances.Clear();

         for (int i = 0; i < mModels.Count; i++)
            mModels[i].deinit();
         mModels.Clear();

         GC.Collect();
         GC.WaitForPendingFinalizers();
      }
      static public ShaderHandle s_modelGPUShaderHandle = null;
      static public void mainShaderParams(string filename)
      {

         s_shaderWHandle = s_modelGPUShaderHandle.getEffectParam( "world");
         s_shaderVPHandle = s_modelGPUShaderHandle.getEffectParam("ViewProj");
         s_shaderWVPHandle = s_modelGPUShaderHandle.getEffectParam( "worldViewProj");

         s_shaderAlbedoTexHandle = s_modelGPUShaderHandle.getEffectParam("albedoTexture");
         s_shaderXformTexHandle = s_modelGPUShaderHandle.getEffectParam("xformTexture");
         s_shaderOpacityTexHandle = s_modelGPUShaderHandle.getEffectParam("opacityTexture");

         s_shaderPlayerColorHandle = s_modelGPUShaderHandle.getEffectParam("g_playerColor");

         s_shaderUseAlbedoHandle = s_modelGPUShaderHandle.getEffectParam("g_bUseAlbedo");
         s_shaderUsePlayerColorHandle = s_modelGPUShaderHandle.getEffectParam("g_bUsePlayerColor");
         s_shaderUseOpacityHandle = s_modelGPUShaderHandle.getEffectParam("g_bUseOpacity");

         s_shaderWorldCameraPosHandle = s_modelGPUShaderHandle.getEffectParam("gWorldCameraPos");

         s_shaderPlanarFogEnabledHandle = s_modelGPUShaderHandle.getEffectParam("gPlanarFogEnabled");
         s_shaderRadialFogColorHandle = s_modelGPUShaderHandle.getEffectParam("gRadialFogColor");
         s_shaderPlanarFogColorHandle = s_modelGPUShaderHandle.getEffectParam("gPlanarFogColor");
         s_shaderFogParamsHandle = s_modelGPUShaderHandle.getEffectParam("gFogParams");

         s_shaderAOPrevDepthTexHandle = s_modelGPUShaderHandle.getEffectParam("prevDepthTexture");

      }
      static public unsafe granny_file* loadGrannyFile(string filename)
      {
         if (!File.Exists(filename))
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("GrannyFile {0} was not found", filename));
            return null;
         }

/*
         FileStream stream = File.Open(filename, FileMode.Open, FileAccess.Read);
         if(stream == null)
         {
            return null;
         }
         else
         {
            stream.Close();
         }
 */       
         granny_file* grannyFile = (granny_file*)GrannyReadEntireFile(filename);
         if (grannyFile != null)
         {
            // It's a file Granny can load (but might be just a raw bunch of bits).
            granny_file_info* file_info = GrannyGetFileInfo((IntPtr)grannyFile);
            if (file_info != null)
            {
               ConvertCoordinateSystem(file_info, true, true);

               return (grannyFile);
            }
         }
         return (null);
      }


      static public unsafe GrannyModel getModel(int modelId)
      {
         if ((modelId < 0) || (modelId >= mModels.Count))
            return null;

         return (mModels[modelId]);
      }

      static public unsafe int findModel(string filename)
      {
         // Find first
         for (int i = 0; i < mModels.Count; i++)
         {
            GrannyModel curModel = mModels[i];
            if (curModel.mName.CompareTo(filename) == 0)
               return (i);
         }


         return (-1);
      }

      static public unsafe int getOrCreateModel(string filename)
      {
         // Look for existing.
         int index = findModel(filename);

         // None existing, must create.
         if (index < 0)
         {
            GrannyModel model = new GrannyModel();
            if (model.init(filename))
            {
               // Add to global list
               mModels.Add(model);

               return (mModels.Count - 1);
            }
            /*

            granny_file* grannyFile = loadGrannyFile(filename);

            if (grannyFile != null)
            {
               // Check to make sure that there is at least one model in this file, since
               // granny files can have any number of models.
               granny_file_info* grannyFileInfo = GrannyGetFileInfo((IntPtr)grannyFile);
               if (grannyFileInfo->ModelCount < 1)
               {
                  return (-1);
               }

               // Create new animation entry
               mGR2Path = Path.GetDirectoryName(filename);

               GrannyModel model = new GrannyModel();
               model.init(filename, grannyFile);

               // Add to global list
               mModels.Add(model);

               return (mModels.Count - 1);
            }
             */
         }
         return (index);
      }


      static public unsafe GrannyAnimation getAnimation(int animId)
      {
         if ((animId < 0) || (animId >= mAnimations.Count))
            return null;

         return (mAnimations[animId]);
      }

      static public unsafe int findAnimation(string filename)
      {
         // Find first
         for (int i = 0; i < mAnimations.Count; i++)
         {
            GrannyAnimation curAnimation = mAnimations[i];
            if (curAnimation.mName.CompareTo(filename) == 0)
               return (i);
         }

         return (-1);
      }

      static public unsafe int getOrCreateAnimation(string filename)
      {
         // Look for existing.
         int index = findAnimation(filename);

         // None existing, must create.
         if (index < 0)
         {
            granny_file* grannyFile = loadGrannyFile(filename);

            if (grannyFile != null)
            {
               // Check to make sure that there is at least one animation in this file, since
               // granny files can have any number of animations.
               granny_file_info* grannyFileInfo = GrannyGetFileInfo((IntPtr)grannyFile);
               if (grannyFileInfo->AnimationCount < 1)
               {
                  return (-1);
               }

               // Create new animation entry
               GrannyAnimation animation = new GrannyAnimation();

               animation.mName = filename;
               animation.mGrannyFile = grannyFile;

               // Add to global list
               mAnimations.Add(animation);


               // TODO:  Free data copied into vertex and index buffers from granny file.
               /*
               GrannyFreeFileSection(grannyFile, (int)granny_standard_section_index.GrannyStandardRigidVertexSection);
               GrannyFreeFileSection(grannyFile, (int)granny_standard_section_index.GrannyStandardRigidIndexSection);
               GrannyFreeFileSection(grannyFile, (int)granny_standard_section_index.GrannyStandardDeformableIndexSection);
               GrannyFreeFileSection(grannyFile, (int)granny_standard_section_index.GrannyStandardTextureSection);
               */

               return (mAnimations.Count - 1);
            }
         }

         return (index);
      }

      static public unsafe GrannyTexture getTexture(int textureId)
      {
         if ((textureId < 0) || (textureId >= mTextures.Count))
            return null;

         return (mTextures[textureId]);
      }

      static public unsafe int findTexture(string filename)
      {
         // Find first
         for (int i = 0; i < mTextures.Count; i++)
         {
            GrannyTexture curTexture = mTextures[i];
            if (curTexture.mName.CompareTo(filename) == 0)
               return (i);
         }

         return (-1);
      }

      static public unsafe int getOrCreateTexture(string filename)
      {
         // Look for existing.
         int index = findTexture(filename);

         // None existing, must create.
         if (index < 0)
         {
            string actualTexName = filename;

            // fixup our path filename so that we get the accurate folder for this client.
            //string actualTexName = giveClientFilename(filename);
            //string actualTexName = CoreGlobals.getWorkPaths().mGameArtDirectory + filename;


            //load the file so that we can display it
            if (actualTexName != "")
            {
               //CLM [05.10.06] RG changed our tools to all use TGA files now.
               //try TGA form
               string nPath = Path.ChangeExtension(actualTexName, ".tga").ToLower();
               TextureHandle t = null;

               if (File.Exists(nPath))
               {
                  t = BRenderDevice.getTextureManager().getTexture(nPath, null, 2);
               }

               if (t != null)
               {
                  GrannyTexture texture = new GrannyTexture();

                  texture.mName = filename;
                  texture.mTexture = t;

                  // Add to global list
                  mTextures.Add(texture);

                  return (mTextures.Count - 1);
               }
               else
               {
                  return (-1);
               }
            }
            else
            {
               CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Error loading texFilename = {0}", filename));
               return (-1);
            }
         }

         return (index);
      }



      static public unsafe GrannyInstance createInstance(int modelId)
      {
         GrannyInstance instance = new GrannyInstance();

         instance.init(modelId);

         mInstances.Add(instance);

         return (instance);
      }


      static public unsafe void releaseInstance(GrannyInstance instance)
      {
         if (instance == null)
            return;

         mInstances.Remove(instance);
      }

      static public unsafe void rebindModel(int modelIndex)
      {
         for (int i = 0; i < mInstances.Count; i++)
         {
            GrannyInstance instance = mInstances[i];
            if (instance == null)
               continue;

            if (instance.mModelIndex == modelIndex)
            {
               // get time an animations
               List<int> animList = new List<int>();
               float clockTime = instance.getClock();
               instance.getAnimationList(animList);

               instance.init(modelIndex);

               // restore time an animations
               instance.setClock(clockTime);
               for (int j = 0; j < animList.Count; j++)
               {
                  instance.playAnimation(animList[i]);
               }
            }
         }
      }

      static public unsafe void rebindAnimation(int animIndex)
      {
         for(int i=0; i<mInstances.Count; i++)
         {
            GrannyInstance instance = mInstances[i];
            if (instance == null)
               continue;

            instance.rebindAnimation(animIndex);
         }
      }

      static public unsafe void update(float elapseTime)
      {
         int numInstances = mInstances.Count;

         for (int i = 0; i < numInstances; i++)
         {
            mInstances[i].update(elapseTime);
         }
      }





      //------------------------------------------------------------------
      static uint framecount = 0;
      static uint framecountWaitFor = uint.MaxValue;

      static private void FileWatcher_Changed(object sender, FileSystemEventArgs e)
      {
         string ext = Path.GetExtension(e.Name);

         if (string.Compare(ext, ".tga", true) == 0)
         {
            int index = findTexture(e.FullPath);//e.Name);

            //was the file that changed a texture in our list?
            if (index == -1)
               return;

            if (!s_textureIndexesToReload.Contains(index))
            {
               framecountWaitFor = framecount + 2;
               s_textureIndexesToReload.Add(index);
            }
         }
         else if (string.Compare(ext, ".gr2", true) == 0)
         {
            int index;

            if ((index = findModel(e.FullPath)) != -1)
            {
               if (!s_modelIndexesToReload.Contains(index))
               {
                  framecountWaitFor = framecount + 2;
                  s_modelIndexesToReload.Add(index);
               }
            }
            else if ((index = findAnimation(e.FullPath)) != -1)
            {
               if(!s_animationIndexesToReload.Contains(index))
               {
                  framecountWaitFor = framecount + 2;
                  s_animationIndexesToReload.Add(index);
               }
            }
         }
      }

      static bool mbDisableReload = false;
      static public void reloadChangedResources()
      {
         if (mbDisableReload == true)
            return;
         try
         {

            // hack to get pass the granny not being able to open a file that
            // was just touched.
            if (framecount++ != framecountWaitFor)
               return;

            // Reload needed textures
            for (int i = 0; i < s_textureIndexesToReload.Count; i++)
            {
               mTextures[s_textureIndexesToReload[i]].reload();
            }
            s_textureIndexesToReload.Clear();

            // Reload needed models
            for (int i = 0; i < s_modelIndexesToReload.Count; i++)
            {
               bool reloadSuccess = mModels[s_modelIndexesToReload[i]].reload();

               if (!reloadSuccess)
               {
                  CoreGlobals.getErrorManager().OnSimpleWarning("Unable to reload file");
               }

               // Reinit instances that were using this model
               rebindModel(s_modelIndexesToReload[i]);
            }
            s_modelIndexesToReload.Clear();

            // Reload needed animations
            for (int i = 0; i < s_animationIndexesToReload.Count; i++)
            {
               bool reloadSuccess = mAnimations[s_animationIndexesToReload[i]].reload();

               if (!reloadSuccess)
               {
                  CoreGlobals.getErrorManager().OnSimpleWarning("Unable to reload file");
               }

               // Fix bindings if the animation is playing
               rebindAnimation(s_animationIndexesToReload[i]);
            }
            s_animationIndexesToReload.Clear();
         }
         catch(System.Exception ex)
         {
            if(MessageBox.Show("Error in granny reloader. Disable it?","Error", MessageBoxButtons.YesNo) == DialogResult.Yes)
            {
               mbDisableReload = true;
               
            }
            else
            {
               framecountWaitFor += 30;
            }
         }
      }

      static public void resetStatsInfoVerticalPosition()
      {
         s_statsInfoVerticalPosition = 10;
      }
      static public void incrementStatsInfoVerticalPosition(int amount)
      {
         s_statsInfoVerticalPosition += amount;
      }

      static public void setFogParams(bool bFogEnabled,
                           System.Drawing.Color radialFogColor, float radialFogIntensity, float radialFogDensity, float radialFogStart,
                           System.Drawing.Color planarFogColor, float planarFogIntensity, float planarFogDensity, float planarFogStart)
      {
         if (bFogEnabled)
         {
            radialFogDensity = radialFogDensity / 1000.0f;
            Vector4 radialFogColorVec = new Vector4((radialFogColor.R / 255.0f) * radialFogIntensity,
                                                   (radialFogColor.G / 255.0f) * radialFogIntensity,
                                                   (radialFogColor.B / 255.0f) * radialFogIntensity,
                                                   1.0f);

            planarFogDensity = planarFogDensity / 1000.0f;
            Vector4 planarFogColorVec = new Vector4((planarFogColor.R / 255.0f) * planarFogIntensity,
                                                   (planarFogColor.G / 255.0f) * planarFogIntensity,
                                                   (planarFogColor.B / 255.0f) * planarFogIntensity,
                                                   1.0f);

            bool planarFogEnable = (planarFogDensity > 0.0f) ? true : false;

            // Apply fog params
            GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderPlanarFogEnabledHandle, planarFogEnable);
            GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderRadialFogColorHandle, radialFogColorVec);
            GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderPlanarFogColorHandle, planarFogColorVec);
            GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderFogParamsHandle, new Vector4(radialFogDensity * radialFogDensity,
                                                                        radialFogStart * radialFogStart,
                                                                        planarFogDensity * planarFogDensity,
                                                                        planarFogStart));
         }
         else
         {
            // Apply fog params
            GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderPlanarFogEnabledHandle, false);
            GrannyManager2.s_modelGPUShader.SetValue(GrannyManager2.s_shaderFogParamsHandle, new Vector4(0.0f,
                                                                     float.MaxValue,
                                                                     0.0f,
                                                                     float.MinValue));
         }
      }

      static public int giveTotal360TextureMemoryEstimate()
      {
         int mem = 0;
         for (int i = 0; i < mModels.Count; i++)
         {
            for (int j = 0; j < mModels[i].mMeshes.Count; j++)
            {
               if (mModels[i].mMeshes[j] !=null)
                  mem += mModels[i].mMeshes[j].mDDXTextureMemorySize;
            }
         }
         return mem;
      }
   }
}
