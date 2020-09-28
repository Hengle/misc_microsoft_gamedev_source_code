using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using Microsoft.DirectX;

namespace EditorCore
{
   public class OBJObject
   {
      public OBJObject()
      {}

      public OBJObject(string name)
      {
         m_name = name;
      }

      public string m_name = "";
      public List<OBJFace> m_faces = new List<OBJFace>();
   }

   public class OBJFace
   {
      public OBJFace()
      {
         m_vertexIndex = new int[3];
         m_normalIndex = new int[3];
         for (int i = 0; i < 3; i++)
         {
            m_vertexIndex[i] = -1;
            m_normalIndex[i] = -1;
         }
      }
      public int[] m_vertexIndex;
      public int[] m_normalIndex;
   }

   public class OBJFile
   {

      public string m_name;

      // Groups
      public List<OBJObject> m_objects = new List<OBJObject>();

      // Vertex data
      public List<Vector3> m_vertexPositionArray = new List<Vector3>();

      // Normal data
      public List<Vector3> m_vertexNormalArray = new List<Vector3>();

      // Methods
      public int addObject(string name)
      {
         OBJObject newObject = new OBJObject(name);
         m_objects.Add(newObject);

         return (m_objects.Count - 1);
      }
      public bool addVertexList(int objectIndex, List<Vector3> verts, List<int> indices)
      {
         if (objectIndex >= m_objects.Count)
         {
            return false;
         }
         m_vertexPositionArray = verts;

         OBJObject pObject = m_objects[objectIndex];
         for(int i = 0; i < indices.Count; i+=3)
         {
            OBJFace newFace = new OBJFace();
            newFace.m_vertexIndex[0] = indices[i + 0]+1;
            newFace.m_vertexIndex[1] = indices[i + 1]+1;
            newFace.m_vertexIndex[2] = indices[i + 2]+1;

            pObject.m_faces.Add(newFace);
         }
         return (true);
      }
      public bool addVertexList(int objectIndex, List<Vector3> verts, List<Vector3> norms, List<int> indices)
      {
         if (objectIndex >= m_objects.Count)
         {
            return false;
         }

         m_vertexPositionArray = verts;
         m_vertexNormalArray = norms;

          OBJObject pObject = m_objects[objectIndex];
         for(int i = 0; i < indices.Count; i+=3)
         {
            OBJFace newFace = new OBJFace();
            newFace.m_vertexIndex[0] = indices[i]+1;
            newFace.m_vertexIndex[1] = indices[i+1]+1;
            newFace.m_vertexIndex[2] = indices[i+2]+1;

            newFace.m_normalIndex[0] = indices[i]+1;
            newFace.m_normalIndex[1] = indices[i+1]+1;
            newFace.m_normalIndex[2] = indices[i+2]+1;

            pObject.m_faces.Add(newFace);
         }
         return (true);
      }
      public bool addTriangle(int objectIndex, Vector3 pos1, Vector3 pos2, Vector3 pos3)
      {
         if (objectIndex >= m_objects.Count)
         {
            return false;
         }
         OBJObject pObject = m_objects[objectIndex];

         // Create new face
         OBJFace newFace = new OBJFace();

         newFace.m_vertexIndex[0] = addUniquePosition(pos1) + 1;
         newFace.m_vertexIndex[1] = addUniquePosition(pos2) + 1;
         newFace.m_vertexIndex[2] = addUniquePosition(pos3) + 1;

         pObject.m_faces.Add(newFace);
         return (true);
      }
      public bool addTriangle(int objectIndex, Vector3 pos1, Vector3 norm1, Vector3 pos2, Vector3 norm2, Vector3 pos3, Vector3 norm3)
      {
         if (objectIndex >= m_objects.Count)
         {
            return false;
         }
         OBJObject pObject = m_objects[objectIndex];

         // Create new face
         OBJFace newFace = new OBJFace();

         newFace.m_vertexIndex[0] = addUniquePosition(pos1) + 1;
         newFace.m_vertexIndex[1] = addUniquePosition(pos2) + 1;
         newFace.m_vertexIndex[2] = addUniquePosition(pos3) + 1;

         newFace.m_normalIndex[0] = addUniqueNormal(norm1) + 1;
         newFace.m_normalIndex[1] = addUniqueNormal(norm2) + 1;
         newFace.m_normalIndex[2] = addUniqueNormal(norm3) + 1;

         pObject.m_faces.Add(newFace);
         return (true);
      }
      private int addUniquePosition(Vector3 pos)
      {
         int numVertexPositions = m_vertexPositionArray.Count;

         /*
         // SAT:  Commenting this out since it takes forever, and it isn't really needed.
         //       (so we have duplicate verts - big deal).

         for (int i = 1; i < numVertexPositions; i++)
         {
          Vector3 vertPos = m_vertexPositionArray[i];
          if((pos.X == vertPos.X) &&
             (pos.Y == vertPos.Y) &&
             (pos.Z == vertPos.Z))
              return (i);
         }
         */

         //since we couldn't find it, add it
         m_vertexPositionArray.Add(pos);
         return (numVertexPositions);
      }
      private int addUniqueNormal(Vector3 norm)
      {
         int numVertexNormals = m_vertexNormalArray.Count;
         m_vertexNormalArray.Add(norm);
         return (numVertexNormals);
      }
      private void countElements(string file, ref int numVertex, ref int numObjects, ref int numFaces, ref List<int> facesPerObjectArray)
      {

         numVertex = 0;
         numObjects = 0;

         string[] input = File.ReadAllLines(file);
         for (int i = 0; i < input.Length; i++)
         {
            string line = input[i];
            if (line.StartsWith("v"))
            {
               numVertex++;
            }
            else if (line.StartsWith("g"))
            {
               numObjects++;
               facesPerObjectArray.Add(0);
            }
            else if (line.StartsWith("f"))
            {
               // count num verts in face
               int numVerts = 0;
               string[] split = line.Split(new Char[] { ' ' });
               for (int j = 1; j < split.Length; j++)
                  numVerts++;
               numFaces += numVerts - 2;
               int lastIndex = (int)(facesPerObjectArray.Count) - 1;
               facesPerObjectArray[lastIndex] += numVerts - 2;
            }
         }
      }
      public struct vertexInfo
      {
         public int vp;
         public int vt;
         public int vn;
      };
      private void readElements(string file)
      {
         int countObject = 0;


         OBJObject pCurObject = new OBJObject();

         string[] input = File.ReadAllLines(file);
         for (int i = 0; i < input.Length; i++)
         {
            string[] split = input[i].Split(new Char[] { ' ' });
            switch (split[0])
            {
               case "v":  // Plain vertex
                  //get and increment the current vertex
                  Vector3 vertex = new Vector3();
                  // initialize the vertex values (important if not all values are in file)
                  vertex.X = vertex.Y = vertex.Z = 0.0f;
                  //fill the values with the data in the line
                  vertex.X = System.Convert.ToSingle(split[1]);
                  vertex.Y = System.Convert.ToSingle(split[2]);
                  vertex.Z = System.Convert.ToSingle(split[3]);
                  m_vertexPositionArray.Add(vertex);
                  break;
               case "vt": // Texture vertex
                  break;
               case "vn": // vertex normal
                  break;
               case "g":  // new group
                  if (input[i].Length > 2)
                  {
                     pCurObject = m_objects[countObject++];
                     string[] sub = input[i].Split(new Char[] { ' ' });
                     m_name = sub[1];
                  }
                  break;
               case "s":  // new smoothing group
                  break;
               case "f":  // new face
                  List<vertexInfo> verts = new List<vertexInfo>();
                  vertexInfo vert = new vertexInfo();
                  vert.vp = vert.vt = vert.vn = -1;
                  if (split.Length < 4)
                     break;
                  for (int j = 1; j < split.Length; j++)
                  {
                     string[] v = split[j].Split(new Char[] { '/' });
                     if (v.Length > 1)
                     {
                        vert.vp = System.Convert.ToInt16(v[0]);
                        vert.vt = System.Convert.ToInt16(v[1]);
                        vert.vn = System.Convert.ToInt16(v[2]);
                        verts.Add(vert);
                     }
                     else
                     {
                        vert.vp = System.Convert.ToInt16(split[1]);
                        verts.Add(vert);
                        vert.vp = System.Convert.ToInt16(split[2]);
                        verts.Add(vert);
                        vert.vp = System.Convert.ToInt16(split[3]);
                        verts.Add(vert);
                     }
                  }
                  OBJFace newFace = new OBJFace();
                  pCurObject.m_faces.Add(newFace);

                  newFace.m_vertexIndex[0] = verts[0].vp;
                  newFace.m_vertexIndex[1] = verts[1].vp;
                  newFace.m_vertexIndex[2] = verts[2].vp;
                  break;
            }
         }
      }
      public bool write(FileStream stream)
      {
         StreamWriter sw = new StreamWriter(stream);
         if (stream == null)
         {
            return (false);
         }

         int i, j;

         // Write out vertex position first
         sw.Write("# Ensemble Studios' .Net OBJ file I/O\n");
         sw.Write("\n");

         // Write out vertex positions firt
         int numVertexPositions = m_vertexPositionArray.Count;
         if (numVertexPositions > 0)
         {
            for (i = 0; i < numVertexPositions; i++)
            {
               Vector3 vertex = m_vertexPositionArray[i];
               sw.Write("v {0} {1} {2}\n", vertex.X, vertex.Z, vertex.Y);
            }
            sw.Write("# {0} vertices\n\n\n", numVertexPositions);
         }

         // Write out vertex normals
         int numVertexNormals = m_vertexNormalArray.Count;
         if (numVertexNormals > 0)
         {
            for (i = 0; i < numVertexNormals; i++)
            {
               Vector3 normal = m_vertexNormalArray[i];
               sw.Write("vn {0} {1} {2}\n", normal.X, normal.Z, normal.Y);
            }
            sw.Write("# {0} vertex normals\n\n\n", numVertexNormals);
         }

         // Write out objects
         int numObjects = m_objects.Count;

         for (i = 0; i < numObjects; i++)
         {
            OBJObject pObject = m_objects[i];
            // Write out the object name
            sw.Write("g {0}\n", pObject.m_name);

            // Write out object faces
            int numFaces = pObject.m_faces.Count;

            for (j = 0; j < numFaces; j++)
            {
               OBJFace face = pObject.m_faces[j];

               if (face.m_vertexIndex[0] != -1)
               {
                  sw.Write("f {0} {1} {2}\n", face.m_vertexIndex[0], face.m_vertexIndex[1], face.m_vertexIndex[2]);
               }
            }
            sw.Write("# {0} faces\n\n\n", numFaces);
         }
         sw.Close();
         return (true);
      }
      public bool read(string file)
      {
         int numVertexPositions = 0;
         int numObjects = 0;
         int numFaces = 0;

         List<int> facesPerObjectArray = new List<int>();

         if (file == null)
         {
            return (false);
         }
         // First count number of elements
         countElements(file, ref numVertexPositions, ref numObjects, ref numFaces, ref facesPerObjectArray);

         // Allocate objects
         for (int i = 0; i < numObjects; i++)
         {
            OBJObject obj = new OBJObject();
            m_objects.Add(obj);
            /*
                            for( int j = 0; j < facesPerObjectArray[i]; j++)
                            {
                                OBJFace face = new OBJFace();
                                obj.m_faces.Add(face);
                            }
            */
         }
         readElements(file);
         return (true);
      }
   }
}