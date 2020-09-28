

#include "OBJFileIO.h"
#include "math.h"




#define MAX_VERTS_PER_FACE   16
//#define NO_DUPLICATES


void NormalizePoint(NmRawPoint &point)
{
   float l = sqrt(point.x * point.x + point.y * point.y + point.z * point.z);

   point.x /= l;
   point.y /= l;
   point.z /= l;
}

OBJFile::OBJFile()
{
}

OBJFile::~OBJFile()
{
   int i;
   for(i = 0; i < (int)m_objects.size(); i++)
   {
      delete(m_objects[i]);
   }
}

void OBJFile::setName(const char *name)
{
   strncpy(m_name, name, 80);
}

void OBJFile::setNumPositions(int numPositions)
{
   m_vertexPositionArray.reserve(numPositions);
}

void OBJFile::setNumTextureCoords(int numTextureCoords)
{
   m_vertexTextureCoordArray.reserve(numTextureCoords);
}

void OBJFile::setNumNormals(int numNormals)
{
   m_vertexNormalArray.reserve(numNormals);
}

void OBJFile::setNumTriangles(unsigned int objectIndex, int numTriangles)
{
   if(objectIndex >= m_objects.size())
      return;

   OBJObject *pObject = m_objects[objectIndex];

   pObject->m_faces.reserve(numTriangles);
}


bool OBJFile::addObject(const char *name)
{
   OBJObject *pNewObject = new OBJObject();
   strncpy(pNewObject->m_name, name, 80);

   m_objects.push_back(pNewObject);

   return(true);
}


bool OBJFile::addTriangle(unsigned int objectIndex, NmRawPoint &pos1, NmRawPoint &pos2, NmRawPoint &pos3)
{
   if(objectIndex >= m_objects.size())
   {
      return false;
   }

   OBJObject *pObject = m_objects[objectIndex];

   // Create new face
   OBJFace *pNewFace = new OBJFace();

   pNewFace->m_vertexIndex[0] = addUniquePosition(pos1) + 1;
   pNewFace->m_vertexIndex[1] = addUniquePosition(pos2) + 1;
   pNewFace->m_vertexIndex[2] = addUniquePosition(pos3) + 1;

   pObject->m_faces.push_back(pNewFace);
   return(true);
}


bool OBJFile::addTriangle(unsigned int objectIndex, NmRawPoint &pos1, NmRawPoint &pos2, NmRawPoint &pos3,
                                                NmRawTexCoord &tcoord1, NmRawTexCoord &tcoord2, NmRawTexCoord &tcoord3)
{
   if(objectIndex >= m_objects.size())
   {
      return false;
   }

   OBJObject *pObject = m_objects[objectIndex];

   // Create new face
   OBJFace *pNewFace = new OBJFace();

   pNewFace->m_vertexIndex[0] = addUniquePosition(pos1) + 1;
   pNewFace->m_vertexIndex[1] = addUniquePosition(pos2) + 1;
   pNewFace->m_vertexIndex[2] = addUniquePosition(pos3) + 1;

   pNewFace->m_vertexTextureIndex[0] = addUniqueTextureCoord(tcoord1) + 1;
   pNewFace->m_vertexTextureIndex[1] = addUniqueTextureCoord(tcoord2) + 1;
   pNewFace->m_vertexTextureIndex[2] = addUniqueTextureCoord(tcoord3) + 1;

   pObject->m_faces.push_back(pNewFace);
   return(true);
}

bool OBJFile::addTriangle(unsigned int objectIndex, NmRawPoint &pos1, NmRawPoint &pos2, NmRawPoint &pos3,
                                                   NmRawPoint &norm1, NmRawPoint &norm2, NmRawPoint &norm3)
{
   if(objectIndex >= m_objects.size())
   {
      return false;
   }

   OBJObject *pObject = m_objects[objectIndex];

   // Create new face
   OBJFace *pNewFace = new OBJFace();

   pNewFace->m_vertexIndex[0] = addUniquePosition(pos1) + 1;
   pNewFace->m_vertexIndex[1] = addUniquePosition(pos2) + 1;
   pNewFace->m_vertexIndex[2] = addUniquePosition(pos3) + 1;

   pNewFace->m_vertexNormalIndex[0] = addUniqueNormal(norm1) + 1;
   pNewFace->m_vertexNormalIndex[1] = addUniqueNormal(norm2) + 1;
   pNewFace->m_vertexNormalIndex[2] = addUniqueNormal(norm3) + 1;

   pObject->m_faces.push_back(pNewFace);
   return(true);
}

bool OBJFile::addTriangle(unsigned int objectIndex, NmRawPoint &pos1, NmRawPoint &pos2, NmRawPoint &pos3,
                 NmRawTexCoord &tcoord1, NmRawTexCoord &tcoord2, NmRawTexCoord &tcoord3,
                 NmRawPoint &norm1, NmRawPoint &norm2, NmRawPoint &norm3)
{
   if(objectIndex >= m_objects.size())
   {
      return false;
   }

   OBJObject *pObject = m_objects[objectIndex];

   // Create new face
   OBJFace *pNewFace = new OBJFace();

   pNewFace->m_vertexIndex[0] = addUniquePosition(pos1) + 1;
   pNewFace->m_vertexIndex[1] = addUniquePosition(pos2) + 1;
   pNewFace->m_vertexIndex[2] = addUniquePosition(pos3) + 1;

   pNewFace->m_vertexTextureIndex[0] = addUniqueTextureCoord(tcoord1) + 1;
   pNewFace->m_vertexTextureIndex[1] = addUniqueTextureCoord(tcoord2) + 1;
   pNewFace->m_vertexTextureIndex[2] = addUniqueTextureCoord(tcoord3) + 1;

   pNewFace->m_vertexNormalIndex[0] = addUniqueNormal(norm1) + 1;
   pNewFace->m_vertexNormalIndex[1] = addUniqueNormal(norm2) + 1;
   pNewFace->m_vertexNormalIndex[2] = addUniqueNormal(norm3) + 1;

   pObject->m_faces.push_back(pNewFace);
   return(true);
}

unsigned int OBJFile::addUniquePosition(NmRawPoint &pos)
{
#ifdef NO_DUPLICATES

   NmRawPoint *vertPos;
   int numVertexPositions = m_vertexPositionArray.size();
   for(int i = numVertexPositions - 1; i >= 0; i--)
   {
      vertPos = &m_vertexPositionArray[i];
      if((pos.x == vertPos->x) &&
         (pos.y == vertPos->y) &&
         (pos.z == vertPos->z))
         return(i);
   }

   // since we couldn't find it, add it
   m_vertexPositionArray.push_back(pos);
   m_vertexPositionArray.resize()
   return(numVertexPositions);

#else

   m_vertexPositionArray.push_back(pos);
   return(m_vertexPositionArray.size() - 1);

#endif
}

unsigned int OBJFile::addUniqueTextureCoord(NmRawTexCoord &coord)
{
#ifdef NO_DUPLICATES

   NmRawTexCoord *vertTexCoord;
   int numVertexTextureCoords = m_vertexTextureCoordArray.size();
   for(int i = numVertexTextureCoords - 1; i >= 0; i--)
   {
      vertTexCoord = &m_vertexTextureCoordArray[i];
      if((coord.u == vertTexCoord->u) &&
         (coord.v == vertTexCoord->v))
         return(i);
   }

   // since we couldn't find it, add it
   m_vertexTextureCoordArray.push_back(coord);
   return(numVertexTextureCoords);

#else

   m_vertexTextureCoordArray.push_back(coord);
   return(m_vertexTextureCoordArray.size() - 1);

#endif
}


unsigned int OBJFile::addUniqueNormal(NmRawPoint &norm)
{
#ifdef NO_DUPLICATES

   NmRawPoint *vertNorm;
   int numVertexNormals = m_vertexNormalArray.size();
   for(int i = numVertexNormals - 1; i >= 0; i--)
   {
      vertNorm = &m_vertexNormalArray[i];
      if((norm.x == vertNorm->x) &&
         (norm.y == vertNorm->y) &&
         (norm.z == vertNorm->z))
         return(i);
   }

   // since we couldn't find it, add it
   m_vertexNormalArray.push_back(norm);
   return(numVertexNormals);

#else

   m_vertexNormalArray.push_back(norm);
   return(m_vertexNormalArray.size() - 1);

#endif
}

bool OBJFile::read(FILE* stream)
{
   unsigned int   numVertexPositions = 0;
   unsigned int   numVertexTextureCoords = 0;
   unsigned int   numVertexNormals = 0;
   unsigned int	numObjects = 0;
   unsigned int	numFaces = 0;

   std::vector <int>   facesPerObjectArray;


   if(stream == NULL) 
   {
      return(false);
   }

   // First count number of elemets
   countElements(stream, numVertexPositions, numVertexTextureCoords, numVertexNormals, numObjects, numFaces, facesPerObjectArray);

   // Resize arrays
   m_vertexPositionArray.resize(numVertexPositions);
   m_vertexTextureCoordArray.resize(numVertexTextureCoords);
   m_vertexNormalArray.resize(numVertexNormals);

   // Allocate objects
   for(unsigned int i = 0; i < numObjects; i++)
   {
      OBJObject *obj = new OBJObject();
      m_objects.push_back(obj);

      for(int j = 0; j < facesPerObjectArray[i]; j++)
      {
         OBJFace *face = new OBJFace();
         obj->m_faces.push_back(face);
      }
   }

   // Read elements
   rewind(stream);
   readElements(stream);

   return(true);
}



bool OBJFile::write(FILE* stream)
{
   if(stream == NULL) 
   {
      return(false);
   }

   unsigned int i, j;

   fprintf(stream, "# Ensemble Studios' OBJ file I/O\n");
   fprintf(stream, "#\n");

   // Write out vertex positions first
   unsigned int numVertexPositions = m_vertexPositionArray.size();
   if(numVertexPositions)
   {
      for(unsigned int i = 0; i < numVertexPositions; i++)
      {
         NmRawPoint *vertex = &m_vertexPositionArray[i];
         fprintf(stream, "v %f %f %f\n", (vertex->x), (vertex->y), (vertex->z));
      }
      fprintf(stream, "# %d vertices\n\n\n", numVertexPositions);
   }

   // Write out vertex texture coords
   unsigned int numVertexTextureCoords = m_vertexTextureCoordArray.size();
   if(numVertexTextureCoords)
   {
      for(i = 0; i < numVertexTextureCoords; i++)
      {
         NmRawTexCoord *texCoord = &m_vertexTextureCoordArray[i];
         fprintf(stream, "vt %f %f\n", (texCoord->u), (texCoord->u));
      }
      fprintf(stream, "# %d texture vertices\n\n\n", numVertexTextureCoords);
   }

   // Write out vertex normals
   unsigned int numVertexNormals = m_vertexNormalArray.size();
   if(numVertexNormals)
   {
      for(i = 0; i < numVertexNormals; i++)
      {
         NmRawPoint *normal = &m_vertexNormalArray[i];
         fprintf(stream, "vn %f %f %f\n", (normal->x), (normal->y), (normal->z));
      }
      fprintf(stream, "# %d vertex normals\n\n\n", numVertexNormals);
   }


   // Write out objects
   unsigned int numObjects = m_objects.size();

   for(i = 0; i < numObjects; i++)
   {
      OBJObject* object = m_objects[i];

      // Write out object name
      fprintf(stream, "g %s\n", object->m_name);

      // Write out object faces
      unsigned int numFaces = object->m_faces.size();

      for(j = 0; j < numFaces; j++)
      {
         OBJFace* face = object->m_faces[j];

         if(face->m_vertexIndex[0] != -1)
         {
            if((face->m_vertexTextureIndex[0] != -1) &&
               (face->m_vertexNormalIndex[0] != -1))
            {
               fprintf(stream, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", face->m_vertexIndex[0], face->m_vertexTextureIndex[0], face->m_vertexNormalIndex[0],
                                                               face->m_vertexIndex[1], face->m_vertexTextureIndex[1], face->m_vertexNormalIndex[1],
                                                               face->m_vertexIndex[2], face->m_vertexTextureIndex[2], face->m_vertexNormalIndex[2] );
            }
            else if((face->m_vertexTextureIndex[0] == -1) &&
                     (face->m_vertexNormalIndex[0] != -1))
            {
               fprintf(stream, "f %d//%d %d//%d %d//%d\n", face->m_vertexIndex[0], face->m_vertexNormalIndex[0],
                                                            face->m_vertexIndex[1], face->m_vertexNormalIndex[1],
                                                            face->m_vertexIndex[2], face->m_vertexNormalIndex[2]);
            }
            else if((face->m_vertexTextureIndex[0] != -1) &&
                     (face->m_vertexNormalIndex[0] == -1))
            {
               fprintf(stream, "f %d/%d %d/%d %d/%d\n", face->m_vertexIndex[0], face->m_vertexTextureIndex[0],
                                                            face->m_vertexIndex[1], face->m_vertexTextureIndex[1],
                                                            face->m_vertexIndex[2], face->m_vertexTextureIndex[2]);
            }
            else if((face->m_vertexTextureIndex[0] == -1) &&
                     (face->m_vertexNormalIndex[0] == -1))
            {
               fprintf(stream, "f %d %d %d\n", face->m_vertexIndex[0],
                                                face->m_vertexIndex[1],
                                                face->m_vertexIndex[2]);
            }
         }
      }
      fprintf(stream, "# %d faces\n\n\n", numFaces);
   }
   return(true);
}



bool OBJFile::readBinary(FILE* stream)
{
   if(stream == NULL) 
   {
      return(false);
   }


   // Read out vertex positions first
   unsigned int numVertexPositions;
   fread( &numVertexPositions, sizeof(int), 1, stream);

   if(numVertexPositions)
   {
      m_vertexPositionArray.resize(numVertexPositions);
      for(unsigned int i = 0; i < numVertexPositions; i++)
         fread( &(m_vertexPositionArray[i]), sizeof(NmRawPoint), 1, stream);
   }

   // Read out vertex texture coords
   unsigned int numVertexTextureCoords;
   fread( &numVertexTextureCoords, sizeof(int), 1, stream);

   if(numVertexTextureCoords)
   {
      m_vertexTextureCoordArray.resize(numVertexTextureCoords);
      for(unsigned int i = 0; i < numVertexTextureCoords; i++)
         fread( &(m_vertexTextureCoordArray[i]), sizeof(NmRawTexCoord), 1, stream);
   }

   // Read out vertex texture coords
   unsigned int numVertexNormals;
   fread( &numVertexNormals, sizeof(int), 1, stream);

   if(numVertexNormals)
   {
      m_vertexNormalArray.resize(numVertexNormals);
      for(unsigned int i = 0; i < numVertexNormals; i++)
         fread( &(m_vertexNormalArray[i]), sizeof(NmRawPoint), 1, stream);
   }


   // Read out objects
   unsigned int numObjects;
   fread( &numObjects, sizeof(int), 1, stream);


   // Allocate objects
   for(unsigned int i = 0; i < numObjects; i++)
   {
      OBJObject *obj = new OBJObject();

      // Read out object name
      fread( &obj->m_name, sizeof(char), 80, stream);

      // Read out object faces
      unsigned int numFaces;
      fread( &numFaces, sizeof(int), 1, stream);

      for(unsigned int j = 0; j < numFaces; j++)
      {

         OBJFace *face = new OBJFace();

         fread(&face->m_vertexIndex, sizeof(int), 3, stream);
         fread(&face->m_vertexTextureIndex, sizeof(int), 3, stream);		 
         fread(&face->m_vertexNormalIndex, sizeof(int), 3, stream);

         obj->m_faces.push_back(face);

      }
      m_objects.push_back(obj);
   }

   return(true);
}



bool OBJFile::writeBinary(FILE* stream)
{
   if(stream == NULL) 
   {
      return(false);
   }


   // Write out vertex positions first
   unsigned int numVertexPositions = m_vertexPositionArray.size();
   fwrite( &numVertexPositions, sizeof(int), 1, stream);

   for(unsigned int i = 0; i < numVertexPositions; i++)
      fwrite( &(m_vertexPositionArray[i]), sizeof(NmRawPoint), 1, stream);


   // Write out vertex texture coords
   unsigned int numVertexTextureCoords = m_vertexTextureCoordArray.size();
   fwrite( &numVertexTextureCoords, sizeof(int), 1, stream);

   for(unsigned int i = 0; i < numVertexTextureCoords; i++)
      fwrite( &(m_vertexTextureCoordArray[i]), sizeof(NmRawTexCoord), 1, stream);


   // Write out vertex normals
   unsigned int numVertexNormals = m_vertexNormalArray.size();
   fwrite( &numVertexNormals, sizeof(int), 1, stream);

   for(unsigned int i = 0; i < numVertexNormals; i++)
      fwrite( &(m_vertexNormalArray[i]), sizeof(NmRawPoint), 1, stream);



   // Write out objects
   unsigned int numObjects = m_objects.size();
   fwrite( &numObjects, sizeof(int), 1, stream);

   for(unsigned int i = 0; i < numObjects; i++)
   {
      OBJObject* object = m_objects[i];

      // Write out object name
      fwrite( &object->m_name, sizeof(char), 80, stream);

      // Write out object faces
      unsigned int numFaces = object->m_faces.size();
      fwrite( &numFaces, sizeof(int), 1, stream);

      for(unsigned int j = 0; j < numFaces; j++)
      {
         OBJFace* face = object->m_faces[j];


         fwrite(&face->m_vertexIndex, sizeof(int), 3, stream);
         fwrite(&face->m_vertexTextureIndex, sizeof(int), 3, stream);		 
         fwrite(&face->m_vertexNormalIndex, sizeof(int), 3, stream);
      }
   }

   return(true);
}



void OBJFile::countElements(FILE* fp, 
							  unsigned int &numVertex, unsigned int &numVertexTextures, unsigned int &numVertexNormals,
							  unsigned int &numObjects, unsigned int &numFaces, std::vector <int> &facesPerObjectArray) const
{
	char line[256];

	numVertex = 0;
	numVertexTextures = 0;   
	numVertexNormals = 0;
	numObjects = 0;

	while(fgets(line, 256, fp) != NULL)
	{
		// the first letter of every line tells us what to do with the rest 
		// of the line
		switch (line[0])
		{
		case 'v': // new vertex of some type
			switch (line[1]) // switch on second char to determine which kind
			{
			case ' ': // plain vertex
				numVertex++;
				break;
			case 't': // texture vertex
				numVertexTextures++;
				break;
			case 'n': // vertex normal
				numVertexNormals++;
				break;
			}
			break;

		case 'g': // new groups
			if (strlen(line) > 2) 
			{
				numObjects++;
				facesPerObjectArray.push_back(0);
			}

			break;

		case 's': // a new smoothing group
			break;

		case 'f': // a new face

         // count verts in face
         int numVerts = 0;

         char *token = strtok(line," ");
         while ((token = strtok(NULL," ")) != NULL)
         {
            numVerts++;
            if(numVerts >= MAX_VERTS_PER_FACE)
               break;
         }

         numFaces += numVerts - 2;
         int lastIndex = int(facesPerObjectArray.size()) - 1;
         facesPerObjectArray[lastIndex] += numVerts - 2;
			break;
		}
	}
}


void OBJFile::readElements(FILE* fp)
{
	char line[256];
	char dummy[10];

	unsigned int countVertex = 0;
	unsigned int countVertexTextureCoord = 0;   
	unsigned int countVertexNormal = 0;
	unsigned int countObject = 0;
	unsigned int countFace = 0;

	OBJObject* pCurObject = NULL;

	while(fgets(line, 256, fp) != NULL)
	{
		// the first letter of every line tells us what to do with the rest 
		// of the line
		switch (line[0])
		{
		case 'v': // new vertex of some type
			switch (line[1]) // switch on second char to determine which kind
			{
         case ' ': // plain vertex
            {
            // get and increment the current vertex
            NmRawPoint& vertex = m_vertexPositionArray[countVertex++]; 
            // initialize the vertex values (important if not all values are in file)
            vertex.x = vertex.y = vertex.z = 0.0f;
            // fill the values with the data in the line
            sscanf(line, "%s %f %f %f", dummy, &(vertex.x), &(vertex.y), &(vertex.z));
            break;
            }
         case 't': // texture vertex
            {
            // get and increment the current texture vertex
            NmRawTexCoord& vt = m_vertexTextureCoordArray[countVertexTextureCoord++];
            // initialize the texture vertex values
            vt.u = vt.v = 0.0f;
            // fill the values with the data in the line
            sscanf(line, "%s %f %f", dummy, &(vt.u), &(vt.v));
            break;
            }
         case 'n': // vertex normal
            {
            // get and increment the current vertex normal
            NmRawPoint& vn = m_vertexNormalArray[countVertexNormal++];
            // initialize the vertex normal values
            vn.x = vn.y = vn.z = 0.0f;
            // fill the values with the data in the line
            sscanf(line, "%s %f %f %f", dummy, &(vn.x), &(vn.y), &(vn.z));

            // Normalize
            NormalizePoint(vn);
            break;
            }
         }
         break;

		case 'g': // new groups
			if (strlen(line) > 2) 
			{
				pCurObject = m_objects[countObject++];

				char *token = strtok(line," ");
				token = strtok(NULL," \n");
				strcpy(pCurObject->m_name,token);

				// reset face count
				countFace = 0;
			}
			break;

		case 's': // a new smoothing group
			break;

		case 'f': // a new face
         {
			   char *token = strtok(line," ");
			   int i;
            int numVerts = 0;


            struct vertexInfo
            {
               int vp;
               int vt;
               int vn;
            };

            vertexInfo verts[MAX_VERTS_PER_FACE];

            while ((token = strtok(NULL," ")) != NULL)
            {
               verts[numVerts].vp = verts[numVerts].vt = verts[numVerts].vn = -1;

               char tokenCopy[256];
               char *tokenPart = tokenCopy;
               strcpy(tokenCopy, token);


               // get position
               sscanf(tokenPart, "%d", &verts[numVerts].vp);

               // get texture coordinates
               tokenPart = strchr(tokenPart, '/');
               if(tokenPart != NULL)
               {
                  tokenPart += 1;
                  sscanf(tokenPart, "%d", &verts[numVerts].vt);

                  // get normal
                  tokenPart = strchr(tokenPart, '/');
                  if(tokenPart != NULL)
                  {
                     tokenPart += 1;
                     sscanf(tokenPart, "%d", &verts[numVerts].vn);
                  }
               }

               numVerts++;
               if(numVerts >= MAX_VERTS_PER_FACE)
                  break;
            }

            if(numVerts < 3)
               break;

            for(i = 2; i < numVerts; i++)
            {
               OBJFace* pFace = pCurObject->m_faces[countFace++];

               pFace->m_vertexIndex[0] = verts[0].vp;
               pFace->m_vertexTextureIndex[0] = verts[0].vt;
               pFace->m_vertexNormalIndex[0] = verts[0].vn;

               pFace->m_vertexIndex[1] = verts[i - 1].vp;
               pFace->m_vertexTextureIndex[1] = verts[i - 1].vt;
               pFace->m_vertexNormalIndex[1] = verts[i - 1].vn;

               pFace->m_vertexIndex[2] = verts[i].vp;
               pFace->m_vertexTextureIndex[2] = verts[i].vt;
               pFace->m_vertexNormalIndex[2] = verts[i].vn;
            }
         }

   		break;
		}
	}
}



void fillTrisWithObject(OBJFile *pObjFile, OBJObject *pObjObject, int* pNumTriangles, NmRawTriangle** ppTriangleArray)
{
   int i, j;
   int numFaces = (int) pObjObject->m_faces.size();

   // Fill out triangle array
   for(i = 0; i < numFaces; i++)
   {
      OBJFace* pObjFace = pObjObject->m_faces[i];
      NmRawTriangle* triangle = &(*ppTriangleArray)[*pNumTriangles + i];

      bool hasNormals = false;

      int vp, vt, vn;
      for(j = 0; j < 3; j++)
      {
         vp = pObjFace->m_vertexIndex[j];
         vt = pObjFace->m_vertexTextureIndex[j];
         vn = pObjFace->m_vertexNormalIndex[j];

         if(vp != -1)
         {
            NmRawPoint point = pObjFile->m_vertexPositionArray[vp-1];

            // Convert coordinate to granny space
            // (Must do this in two steps or else we get differences in Debug vrs. Release builds,
            // so this is why values are computed into temporary float variables and then assigned).
            float x_granny = point.x * -0.01f;
            float y_granny = point.z * 0.01f;
            float z_granny = point.y * -0.01f;
            
            triangle->vert[j].x = x_granny;
            triangle->vert[j].y = y_granny;
            triangle->vert[j].z = z_granny;
         }
         if(vt != -1)
            triangle->texCoord[j] = pObjFile->m_vertexTextureCoordArray[vt-1];
         if(vn != -1)
         {
            hasNormals = true;
            NmRawPoint normal = pObjFile->m_vertexNormalArray[vn-1];

            triangle->norm[j].x = -normal.x;
            triangle->norm[j].y = normal.z;
            triangle->norm[j].z = -normal.y;
         }
      }


      if(!hasNormals)
      {
         // Calculate face normal
         /*
         BOptVec4 p1, p2, p3;
         p1.set(triangle->vert[0].x, triangle->vert[0].y, triangle->vert[0].z, 1.0f);
         p2.set(triangle->vert[1].x, triangle->vert[1].y, triangle->vert[1].z, 1.0f);
         p3.set(triangle->vert[2].x, triangle->vert[2].y, triangle->vert[2].z, 1.0f);
         BOptVec4 vec1 = p2 - p1;
         BOptVec4 vec2 = p3 - p1;

         BOptVec4 norm = vec2.cross(vec1);
         norm.normalize3();

         triangle->norm[0].x = triangle->norm[1].x = triangle->norm[2].x = norm.x;
         triangle->norm[0].y = triangle->norm[1].y = triangle->norm[2].y = norm.y;
         triangle->norm[0].z = triangle->norm[1].z = triangle->norm[2].z = norm.z;
         */
      }
   }
   


   *pNumTriangles += numFaces;
}





////////////////////////////////////////////////////////////////////
// Read a block of triangles from the file into the array passed
////////////////////////////////////////////////////////////////////
bool
ObjReadTrianglesHelper (FILE* fp, bool isBinary, int* numTris, NmRawTriangle** tris)
{
   OBJFile *pObjFile = new OBJFile();

   // read file
   if(isBinary)
      pObjFile->readBinary(fp);
   else
      pObjFile->read(fp);


   // Count triangle faces
   int totalNumFaces = 0;
   for(int i = 0; i < (int)pObjFile->m_objects.size(); i++)
   {
      OBJObject *pObjObject = pObjFile->m_objects[i];
      totalNumFaces += pObjObject->m_faces.size();
   }


   // Clear tri count
   *numTris = 0;

   // Allocate triangle array
   *tris = new NmRawTriangle [totalNumFaces]; 
   memset(*tris, 0, totalNumFaces * sizeof(NmRawTriangle));


   // Pass first group 
   for(int i = 0; i < (int)pObjFile->m_objects.size(); i++)
   {
      OBJObject *pObjObject = pObjFile->m_objects[i];
      fillTrisWithObject(pObjFile, pObjObject, numTris, tris);
   }


   // delete objFile
   delete(pObjFile);

   return true;
}


////////////////////////////////////////////////////////////////////
// Read a block of triangles from the file into the array passed
////////////////////////////////////////////////////////////////////
bool
ObjReadTriangles (FILE* fp, int* numTris, NmRawTriangle** tris)
{
   return(ObjReadTrianglesHelper(fp, false, numTris, tris));
}

bool
ObjReadTrianglesBinary (FILE* fp, int* numTris, NmRawTriangle** tris)
{
   return(ObjReadTrianglesHelper(fp, true, numTris, tris));
}


////////////////////////////////////////////////////////////////////
// Read a block of triangles for the low res and high res models from
// the same file.  Look for group names with the strings "_low" and
// "_high".
////////////////////////////////////////////////////////////////////
bool
ObjReadTriangles (FILE* fp, int* lowResNumTris, NmRawTriangle** lowResTris,
                             int* highResNumTris, NmRawTriangle** highResTris)
{
   char name[80];
   OBJFile *pObjFile = new OBJFile();

   // read file
   pObjFile->read(fp);

   // Look for high/low polygon groups
   OBJObject *pObjObjectLowRes = NULL;
   OBJObject *pObjObjectHighRes = NULL;

   int i;
   int numGroups = (int) pObjFile->m_objects.size();

   for(i = 0; i < numGroups; i++)
   {
      OBJObject *pObjObject = pObjFile->m_objects[i];

      strcpy(name, pObjObject->m_name);
      _strlwr(name);
      char *foundSub;

      if((((foundSub = strstr(name, "_low")) != NULL) && (strlen(foundSub) == 4)) ||
         (((foundSub = strstr(name, "_lo")) != NULL) && (strlen(foundSub) == 3)) ||
         (((foundSub = strstr(name, "_l")) != NULL) && (strlen(foundSub) == 2)))
      {
         pObjObjectLowRes = pObjObject;
         continue;
      }

      if((((foundSub = strstr(name, "_high")) != NULL) && (strlen(foundSub) == 5)) ||
         (((foundSub = strstr(name, "_hi")) != NULL) && (strlen(foundSub) == 3)) ||
         (((foundSub = strstr(name, "_h")) != NULL) && (strlen(foundSub) == 2)))
      {
         pObjObjectHighRes = pObjObject;
         continue;
      }
   }

   // Return error if we have not found both, the low and high resolution, nodes in the file.
   if((pObjObjectLowRes == NULL) || (pObjObjectLowRes == NULL))
      return false;


   int numFaces = pObjObjectLowRes->m_faces.size();
   *lowResNumTris = 0;
   *lowResTris = new NmRawTriangle [numFaces]; 
   memset(*lowResTris, 0, numFaces * sizeof(NmRawTriangle));

   fillTrisWithObject(pObjFile, pObjObjectLowRes, lowResNumTris, lowResTris);


   numFaces = pObjObjectHighRes->m_faces.size();
   *highResNumTris = 0;
   *highResTris = new NmRawTriangle [numFaces]; 
   memset(*highResTris, 0, numFaces * sizeof(NmRawTriangle));

   fillTrisWithObject(pObjFile, pObjObjectHighRes, highResNumTris, highResTris);


   // delete objFile
   delete(pObjFile);

   return true;
}