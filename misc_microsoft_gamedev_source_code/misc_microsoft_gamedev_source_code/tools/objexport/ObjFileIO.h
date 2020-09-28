

#ifndef __OBJFILEIO__
#define __OBJFILEIO__

#define _CRT_SECURE_NO_DEPRECATE

#include <vector>




typedef union
{
   struct { float x, y, z; };
   struct { float v[3]; };
} NmRawPoint;

typedef union
{
   struct { double x, y, z; };
   struct { double v[3]; };
} NmRawPointD;

typedef union
{
   struct { float u, v; };
   struct { float uv[2]; };
} NmRawTexCoord;

typedef struct
{
   NmRawPoint vert[3];
   NmRawPoint norm[3];
   NmRawTexCoord texCoord[3];
} NmRawTriangle;


class OBJFace
{
	public:

      OBJFace()
      {
         for(int i = 0; i < 3; i++)
         {
            m_vertexIndex[i] = -1;
            m_vertexTextureIndex[i] = -1;
            m_vertexNormalIndex[i] = -1;
         }
      }

		// Indices to vertex data
		int	m_vertexIndex[3];
		int   m_vertexTextureIndex[3];
		int   m_vertexNormalIndex[3];
};

class OBJObject
{
	public:

      ~OBJObject()
      {
         int i;
         for(i = 0; i < (int)m_faces.size(); i++)
         {
            delete(m_faces[i]);
         }
      }

		char		m_name[80];
      std::vector <OBJFace*>   m_faces;
};


class OBJFile
{
	public:

		char			m_name[80];

		// Groups
      std::vector <OBJObject*>   m_objects;

		// Vertex data
      std::vector <NmRawPoint>     m_vertexPositionArray;
      std::vector <NmRawTexCoord>  m_vertexTextureCoordArray;
      std::vector <NmRawPoint>     m_vertexNormalArray;


      OBJFile();
      ~OBJFile();


      // Methods
      void setName(const char *name);
      void setNumPositions(int numPositions);
      void setNumTextureCoords(int numTextureCoords);
      void setNumNormals(int numNormals);
      void setNumTriangles(unsigned int objectIndex, int numTriangles);

      bool addObject(const char *name);
      bool addTriangle(unsigned int objectIndex, NmRawPoint &pos1, NmRawPoint &pos2, NmRawPoint &pos3);
      bool addTriangle(unsigned int objectIndex, NmRawPoint &pos1, NmRawPoint &pos2, NmRawPoint &pos3,
                                       NmRawTexCoord &tcoord1, NmRawTexCoord &tcoord2, NmRawTexCoord &tcoord3);
      bool addTriangle(unsigned int objectIndex, NmRawPoint &pos1, NmRawPoint &pos2, NmRawPoint &pos3,
                                       NmRawPoint &norm1, NmRawPoint &norm2, NmRawPoint &norm3);
      bool addTriangle(unsigned int objectIndex, NmRawPoint &pos1, NmRawPoint &pos2, NmRawPoint &pos3,
                                       NmRawTexCoord &tcoord1, NmRawTexCoord &tcoord2, NmRawTexCoord &tcoord3,
                                       NmRawPoint &norm1, NmRawPoint &norm2, NmRawPoint &norm3);

      // I/O
      bool read(FILE* fp);
      bool write(FILE* fp);

      bool readBinary(FILE* fp);
      bool writeBinary(FILE* fp);


	private:

		void countElements(FILE* fp, 
                      unsigned int &numVertex, unsigned int &numVertexTextures, unsigned int &numVertexNormals,
                      unsigned int &numObjects, unsigned int &numFaces, std::vector <int> &facesPerObjectArray) const;
		void readElements(FILE* fp);

      unsigned int addUniquePosition(NmRawPoint &pos);
      unsigned int addUniqueTextureCoord(NmRawTexCoord &coord);
      unsigned int addUniqueNormal(NmRawPoint &norm);

};




extern bool ObjReadTriangles (FILE* fp, int* numTris, NmRawTriangle** tris);
extern bool ObjReadTrianglesBinary (FILE* fp, int* numTris, NmRawTriangle** tris);

extern bool ObjReadTriangles (FILE* fp, int* lowResNumTris, NmRawTriangle** lowResTris,
                                       int* highResNumTris, NmRawTriangle** highResTris);




#endif