
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using System.Runtime.InteropServices;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using Rendering;
using EditorCore;

namespace Sim
{
   public class GrannyBridge
   {

      static public unsafe BRenderGrannyMesh LoadGR2FromFile(string filename)
      {
         if (!File.Exists(filename))
         {
            //  CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("GrannyFile {0} was not found", filename));
            return null;
         }

         IntPtr ptr = GrannyReadEntireFile(filename);
         BRenderGrannyMesh mesh = LoadGR2Internal((granny_file*)ptr);
         GrannyFreeFile(ptr);

         return mesh;
      }

      static public unsafe BRenderGrannyMesh LoadGR2FromIntPtr(IntPtr ptr, int size)
      {
         granny_file* grannyFile = (granny_file*)GrannyReadEntireFileFromMemory(size, ptr.ToPointer());

         return LoadGR2Internal(grannyFile);
      }

      static unsafe BRenderGrannyMesh LoadGR2Internal(granny_file* grannyFile)
      {
         try
         {
            if (grannyFile != null)
            {
               // It's a file Granny can load (but might be just a raw bunch of bits).
               granny_file_info* file_info = GrannyGetFileInfo((IntPtr)grannyFile);
               if (file_info != null)
               {
                  ConvertCoordinateSystem(file_info, true, true);

                  //file has been transformed, load our meshes
                  BRenderGrannyMesh mesh = new BRenderGrannyMesh();
                  createMeshFromGR2(ref mesh, file_info);

                  return mesh;
               }
            }
         }
         catch (Exception e)
         {
            Console.WriteLine("Error loading granny file");
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

            Vector3 forward = new Vector3(0, 0, 1);
            Vector3 right = new Vector3(-1, 0, 0);
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


      static public unsafe string giveClientFilename(string inFN)
      {

         return mGR2Path + "\\" + Path.GetFileName(inFN);

      }
      static public unsafe char* stringToCharPtr(string str)
      {
         System.Text.ASCIIEncoding aenc = new System.Text.ASCIIEncoding();
         byte[] strB = aenc.GetBytes(str);

         fixed (byte* bt = strB)
            return (char*)bt;
      }
      static public unsafe bool getVertexTypeFromGranny( granny_data_type_definition* grannyVertType,
         ref int vertMemSize,
         ref granny_data_type_definition[] grnDTD,
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
              // CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Granny file {0} is Compressed. The current build of the editor does not support this. \n To view this model in the editor, please re-export uncompressed.", filename));
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
                  else if (name.Equals("Binormal")) { decls.Add(VertexTypes.eVertexDeclElement.cVDE_BiNormal); grndcls.Add(pdef); }
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
         grnVD = VertexTypes.genVertexDecl(decls, false, ref val);
         vertMemSize = (int)val;
         d3dVD = VertexTypes.genVertexDecl(decls, true, ref val);

         return true;
      }

      static public unsafe void swzzlGrnyVertsToD3DVerts(byte[] grnyVerts, VertexDeclaration grnyVD, int vertMemSize, int numVerts,
                                                     ref byte[] d3dVerts, VertexDeclaration d3dVD)
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

      static private unsafe void processMesh(ref BRenderGrannyMesh mesh, granny_mesh* grannyMesh)
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


         granny_data_type_definition* grnyVertTypeDef = GrannyGetMeshVertexType(grannyMesh);
         granny_data_type_definition[] grnDTD = null;
         VertexDeclaration grnVD = null;
         if (!getVertexTypeFromGranny( grnyVertTypeDef, ref prim.mVertexSize, ref grnDTD, ref grnVD, ref prim.mVDecl))
         {
            //already logged
            //CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Error loading {0} getVertexTypeFromGranny failed", filename));

            return;
         }

         prim.mNumVerts = GrannyGetMeshVertexCount(grannyMesh);

         {

            int size = prim.mNumVerts * prim.mVertexSize;
            IntPtr pMarshaledMem = System.Runtime.InteropServices.Marshal.AllocHGlobal(size);
            byte[] tGrnVerts = new byte[size];


            fixed (granny_data_type_definition* grnPD = grnDTD)
               GrannyCopyMeshVertices(grannyMesh, grnPD/*grnyVertTypeDef*/, (void*)pMarshaledMem);
            System.Runtime.InteropServices.Marshal.Copy(pMarshaledMem, tGrnVerts, 0, size);
            System.Runtime.InteropServices.Marshal.FreeHGlobal(pMarshaledMem);

            byte[] d3dVerts = new byte[size];

            //swizzle the granny verts to be d3d friendly before copying them to the device
            swzzlGrnyVertsToD3DVerts(tGrnVerts, grnVD, prim.mVertexSize, prim.mNumVerts,
                                     ref d3dVerts, prim.mVDecl);



            prim.mVB = new VertexBuffer(BRenderDevice.getDevice(), (int)prim.mNumVerts * prim.mVertexSize, Usage.None, VertexFormats.None, Pool.Managed);
            stream = prim.mVB.Lock(0, 0, LockFlags.None);

            stream.Write(d3dVerts, 0, size);
            prim.mVB.Unlock();
            stream.Close();


            tGrnVerts = null;
            grnVD.Dispose();
            grnVD = null;

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

            //load your texture here.
            prim.mGroups.Add(group);
         }


         mesh.addRenderPrimitive(prim);

      }


      static unsafe public Dictionary<string, string> GetExtendedStringData(granny_variant extendedVariant)
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

      static private unsafe void createMeshFromGR2(ref BRenderGrannyMesh mesh, granny_file_info* fileInfo)
      {

         int NumMeshes = fileInfo->ModelCount;
         for (int i = 0; i < NumMeshes; i++)
         {
            try
            {
               if (fileInfo->Models[i]->MeshBindingCount == 0)
               {
                //  CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("No Mesh Binding for model# {0} out of {1} in {2}", i, NumMeshes, filename));
               }
               else
               {
                  processMesh(ref mesh, fileInfo->Models[i]->MeshBindings[0].Mesh);
               }
            }
            catch (System.Exception ex)
            {
             //  CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Error processing mesh #{0} out of {1} in {2}.  {3}", i, NumMeshes, filename, ex.ToString()));
            }

         }

      }

      static public unsafe int getTotalVerts(granny_file_info* fileInfo)
      {
         int totalVerts = 0;

         for (int meshIndex = 0; meshIndex < fileInfo->MeshCount; ++meshIndex)
         {
            granny_mesh* gMesh = fileInfo->Meshes[meshIndex];
            totalVerts += gMesh->PrimaryVertexData->VertexCount;
         }
         return totalVerts;
      }
      static public unsafe int getMeshVerts(granny_file_info* fileInfo, int meshIndex)
      {
         granny_mesh* gMesh = fileInfo->Meshes[meshIndex];
         return gMesh->PrimaryVertexData->VertexCount;
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
         public char* Name;
         public granny_vertex_data* PrimaryVertexData;
         public int MorphTargetCount;
         public IntPtr /*granny_morph_target **/ MorphTargets;
         public IntPtr /*granny_tri_topology **/ PrimaryTopology;
         public int MaterialBindingCount;
         public granny_material_binding* MaterialBindings;
         public int BoneBindingCount;
         public granny_bone_binding* BoneBindings;
         public granny_variant ExtendedData;
      };

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
         public granny_model_mesh_binding* MeshBindings;
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
         public char* Name;
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
         public char* FromFileName;
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
      public unsafe struct granny_data_type_definition
      {
         public granny_member_type memberType;
         public char* Name;
         public granny_data_type_definition* ReferenceType;
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
      public static extern unsafe IntPtr /*granny_file**/ GrannyReadEntireFileFromMemory(int len, void* Memory);
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_file_info* GrannyGetFileInfo(IntPtr /*granny_file **/File);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeFile(IntPtr /*granny_file**/ File);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeModelInstance(granny_model_instance* ModelInstance);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeMeshBinding(IntPtr granny_mesh_binding);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeMeshDeformer(IntPtr granny_mesh_deformer);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeWorldPose(granny_world_pose* WorldPose);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeLocalPose(IntPtr granny_local_pose);
      [DllImport("Granny2.dll")]
      public static extern unsafe int GrannyGetMeshTriangleCount(granny_mesh* mesh);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyCopyMeshIndices(granny_mesh* mesh, int bytesPerIndex, void* dstIndexData);
      [DllImport("Granny2.dll")]
      public static extern unsafe int GrannyGetMeshVertexCount(granny_mesh* mesh);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyCopyMeshVertices(granny_mesh* mesh, granny_data_type_definition* vertType, void* dstVertexData);
      [DllImport("Granny2.dll")]
      public static extern unsafe int GrannyGetMeshTriangleGroupCount(granny_mesh* mesh);
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_data_type_definition* GrannyGetMeshVertexType(granny_mesh* mesh);
      [DllImport("Granny2.dll")]
      public static extern unsafe bool GrannyComputeBasisConversion(granny_file_info* FileInfo,
                                                  float DesiredUnitsPerMeter,
                                                  ref Vector3 DesiredOrigin3,
                                                  ref Vector3 DesiredRight3,
                                                  ref Vector3 DesiredUp3,
                                                  ref Vector3 DesiredBack3,
                                                  ref Vector3 ResultAffine3,
                                                  float* ResultLinear3x3,
                                                  float* ResultInverseLinear3x3);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyTransformFile(granny_file_info* FileInfo,
                                                   ref Vector3 Affine3,
                                                   float* Linear3x3,
                                                   float* InverseLinear3x3,
                                                   float AffineTolerance,
                                                   float LinearTolerance,
                                                   int Flags);
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_tri_material_group* GrannyGetMeshTriangleGroups(granny_mesh* mesh);
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_texture* GrannyGetMaterialTextureByType(granny_material* material,
                                                                                 granny_material_texture_type channel);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyConvertSingleObject(granny_data_type_definition* SourceType,
                                                   void* SourceObject,
                                                   granny_data_type_definition* DestType,
                                                   ref BRenderEffectParamExtendedData DestObject);

      [DllImport("Granny2.dll")]
      public static extern unsafe int GrannyGetTypeTableCount(granny_data_type_definition** TypeTable);

      //      [DllImport("Granny2.dll")]
      //      public static extern unsafe granny_data_type_definition ** GrannyGetTypeTableFor(granny_data_type_definition * MemberType);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyBuildSkeletonRelativeTransforms(int SourceTransformStride,
                                                                            granny_transform* SourceTransforms,
                                                                            int SourceParentStride,
                                                                            int* SourceParents,
                                                                            int Count,
                                                                            int ResultStride,
                                                                            granny_transform* Results);

      [DllImport("Granny2.dll")]
      public static extern unsafe granny_model_instance* GrannyInstantiateModel(granny_model* Model);

      [DllImport("Granny2.dll")]
      public static extern unsafe granny_control* GrannyPlayControlledAnimation(float StartTime,
                                                                                    granny_animation* Animation,
                                                                                    granny_model_instance* Model);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannySetControlLoopCount(granny_control* Control,
                                                                   int LoopCount);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeControlOnceUnused(granny_control* Control);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeControl(granny_control* Control);


      [DllImport("Granny2.dll")]
      public static extern unsafe granny_model_control_binding* GrannyModelControlsBegin(granny_model_instance* Model);
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_model_control_binding* GrannyModelControlsEnd(granny_model_instance* Model);
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_model_control_binding* GrannyModelControlsNext(granny_model_control_binding* Binding);
      [DllImport("Granny2.dll")]
      public static extern unsafe granny_control* GrannyGetControlFromBinding(granny_model_control_binding* Binding);
      [DllImport("Granny2.dll")]
      public static extern unsafe float GrannyEaseControlOut(granny_control* Control, float Duration);
      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyCompleteControlAt(granny_control* Control, float AtSeconds);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeFileSection(granny_file* File, int SectionIndex);

      [DllImport("Granny2.dll")]
      public static extern unsafe granny_world_pose* GrannyNewWorldPose(int BoneCount);

      [DllImport("Granny2.dll")]
      public static extern unsafe granny_mesh_binding* GrannyNewMeshBinding(granny_mesh* Mesh,
                                                                  granny_skeleton* FromSkeleton,
                                                                  granny_skeleton* ToSkeleton);


      [DllImport("Granny2.dll")]
      public static extern unsafe int GrannyGetMeshIndexCount(granny_mesh* Mesh);

      [DllImport("Granny2.dll")]
      public static extern unsafe bool GrannyMeshIsRigid(granny_mesh* Mesh);

      [DllImport("Granny2.dll")]
      public static extern unsafe granny_mesh_deformer* GrannyNewMeshDeformer(granny_data_type_definition* InputVertexLayout,
                                                                  granny_data_type_definition* OutputVertexLayout,
                                                                  granny_deformation_type DeformationType,
                                                                  granny_deformer_tail_flags TailFlag);

      [DllImport("Granny2.dll")]
      public static extern unsafe int* GrannyGetMeshBindingToBoneIndices(granny_mesh_binding* Binding);

      [DllImport("Granny2.dll")]
      public static extern unsafe Matrix* GrannyGetWorldPoseComposite4x4Array(granny_world_pose* WorldPose);

      [DllImport("Granny2.dll")]
      public static extern unsafe granny_skeleton* GrannyGetSourceSkeleton(granny_model_instance* Model);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannySetModelClock(granny_model_instance* ModelInstance, float NewClock);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyUpdateModelMatrix(granny_model_instance* ModelInstance,
                                                                  float SecondsElapsed,
                                                                  float* ModelMatrix4x4,
                                                                  float* DestMatrix4x4,
                                                                  bool Inverse);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannySampleModelAnimations(granny_model_instance* ModelInstance,
                                                                  int FirstBone,
                                                                  int BoneCount,
                                                                  granny_local_pose* Result);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyBuildWorldPose(granny_skeleton* Skeleton,
                                                                  int FirstBone,
                                                                  int BoneCount,
                                                                  granny_local_pose* LocalPose,
                                                                  float* Offset4x4,
                                                                  granny_world_pose* Result);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyFreeCompletedModelControls(granny_model_instance* ModelInstance);

      [DllImport("Granny2.dll")]
      public static extern unsafe granny_local_pose* GrannyNewLocalPose(int BoneCount);

      [DllImport("Granny2.dll")]
      public static extern unsafe float* GrannyGetWorldPose4x4(granny_world_pose* WorldPose, int BoneIndex);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyDeformVertices(granny_mesh_deformer* Deformer,
                                                             int* MatrixIndices,
                                                             float* MatrixBuffer4x4,
                                                             int VertexCount,
                                                             void* SourceVertices,
                                                             void* DestVertices);


      [DllImport("Granny2.dll")]
      public static extern unsafe void* GrannyGetMeshVertices(granny_mesh* Mesh);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyGetWorldMatrixFromLocalPose(granny_skeleton* Skeleton,
                                                            int BoneIndex,
                                                            granny_local_pose* LocalPose,
                                                            float* Offset4x4,
                                                            float* Result4x4,
                                                            int* SparseBoneArray,
                                                            int* SparseBoneArrayReverse);

      [DllImport("Granny2.dll")]
      public static extern unsafe bool GrannyFindBoneByName(granny_skeleton* Skeleton,
                                                            char* BoneName,
                                                            int* BoneIndex);


      [DllImport("Granny2.dll")]
      public static extern unsafe void** GrannyGetControlUserDataArray(granny_control* Control);

      [DllImport("Granny2.dll")]
      public static extern unsafe void GrannyBuildCompositeTransform4x4(granny_transform* transform, float* matrix4x4);


      #endregion

   }
}