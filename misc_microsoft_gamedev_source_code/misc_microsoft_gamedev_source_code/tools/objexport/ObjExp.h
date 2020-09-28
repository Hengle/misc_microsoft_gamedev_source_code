//************************************************************************** 
//* Asciiexp.h	- Ascii File Exporter
//* 
//* By Christer Janson
//* Kinetix Development
//*
//* January 20, 1997 CCJ Initial coding
//*
//* Class definition 
//*
//* Copyright (c) 1997, All Rights Reserved. 
//***************************************************************************

#ifndef __OBJEXP__H
#define __OBJEXP__H


#define _CRT_SECURE_NO_DEPRECATE


#include "Max.h"
#include "resource.h"
#include "istdplug.h"
#include "stdmat.h"
#include "decomp.h"
#include "shape.h"
#include "interpik.h"


#include "ObjFileIO.h"

extern ClassDesc* GetObjExpDesc();
extern TCHAR *GetString(int id);
extern HINSTANCE hInstance;

#define VERSION			200			// Version number * 100
//#define FLOAT_OUTPUT	_T("%4.4f")	// Float precision for output

class MtlKeeper {
public:
	BOOL	AddMtl(Mtl* mtl);
	int		GetMtlID(Mtl* mtl);
	int		Count();
	Mtl*	GetMtl(int id);

	Tab<Mtl*> mtlTab;
};

// This is the main class for the exporter.

class OBJExp : public SceneExport {
public:
	OBJExp();
	~OBJExp();

	// SceneExport methods
	int    ExtCount();     // Number of extensions supported 
	const TCHAR * Ext(int n);     // Extension #n (i.e. "ASC")
	const TCHAR * LongDesc();     // Long ASCII description (i.e. "Ascii Export") 
	const TCHAR * ShortDesc();    // Short ASCII description (i.e. "Ascii")
	const TCHAR * AuthorName();    // ASCII Author name
	const TCHAR * CopyrightMessage();   // ASCII Copyright message 
	const TCHAR * OtherMessage1();   // Other message #1
	const TCHAR * OtherMessage2();   // Other message #2
	unsigned int Version();     // Version number * 100 (i.e. v3.01 = 301) 
	void	ShowAbout(HWND hWnd);  // Show DLL's "About..." box
	int		DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0); // Export	file
	BOOL	SupportsOptions(int ext, DWORD options);

	// Other methods

	// Node enumeration
	BOOL	nodeEnum(INode* node, int indentLevel);
	void	PreProcess(INode* node, int& nodeCount);
   void  PreProcessCount(INode* node, int& positionCount, int& texCoordCount, int& normalCount);

	// High level export
	void	ExportGlobalInfo();
	void	ExportMaterialList();
	void	ExportGeomObject(INode* node, int indentLevel); 
	void	ExportLightObject(INode* node, int indentLevel); 
	void	ExportCameraObject(INode* node, int indentLevel); 
	void	ExportShapeObject(INode* node, int indentLevel); 
	void	ExportHelperObject(INode* node, int indentLevel);

	// Mid level export
	void	ExportMesh(INode* node, TimeValue t, int indentLevel); 
	void	ExportAnimKeys(INode* node, int indentLevel);
	void	ExportMaterial(INode* node, int indentLevel); 
	void	ExportAnimMesh(INode* node, int indentLevel); 
	void	ExportIKJoints(INode* node, int indentLevel);
	void	ExportNodeTM(INode* node, int indentLevel);
	void	ExportNodeHeader(INode* node, TCHAR* type, int indentLevel);
	void	ExportCameraSettings(CameraState* cs, CameraObject* cam, TimeValue t, int indentLevel);
	void	ExportLightSettings(LightState* ls, GenLight* light, TimeValue t, int indentLevel);

	// Low level export
	void	DumpPoly(PolyLine* line, Matrix3 tm, int indentLevel);
	void	DumpMatrix3(Matrix3* m, int indentLevel);
	void	DumpMaterial(Mtl* mtl, int mtlID, int subNo, int indentLevel);
	void	DumpTexture(Texmap* tex, Class_ID cid, int subNo, float amt, int	indentLevel);
	void	DumpJointParams(JointParams* joint, int indentLevel);
	void	DumpUVGen(StdUVGen* uvGen, int indentLevel); 
	void	DumpPosSample(INode* node, int indentLevel); 
	void	DumpRotSample(INode* node, int indentLevel); 
	void	DumpScaleSample(INode* node, int indentLevel); 
	void	DumpPoint3Keys(Control* cont, int indentLevel);
	void	DumpFloatKeys(Control* cont, int indentLevel);
	void	DumpPosKeys(Control* cont, int indentLevel);
	void	DumpRotKeys(Control* cont, int indentLevel);
	void	DumpScaleKeys(Control* cont, int indentLevel);

	// Misc methods
	TCHAR*	GetMapID(Class_ID cid, int subNo);
	Point3	GetVertexNormal(Mesh* mesh, int faceNo, RVertex* rv);
	BOOL	CheckForAndExportFaceMap(Mtl* mtl, Mesh* mesh, int indentLevel); 
	void	make_face_uv(Face *f, Point3 *tv);
	BOOL	TMNegParity(Matrix3 &m);
	TSTR	GetIndent(int indentLevel);
	TCHAR*	FixupName(TCHAR* name);
	void	CommaScan(TCHAR* buf);
	BOOL	CheckForAnimation(INode* node, BOOL& pos, BOOL& rot, BOOL& scale);
	TriObject*	GetTriObjectFromNode(INode *node, TimeValue t, int &deleteIt);
	BOOL	IsKnownController(Control* cont);

	// A collection of overloaded value to string converters.
	TSTR	Format(int value);
	TSTR	Format(float value);
	TSTR	Format(Color value);
	TSTR	Format(Point3 value); 
	TSTR	Format(AngAxis value); 
	TSTR	Format(Quat value);
	TSTR	Format(ScaleValue value);


	inline Interface*	GetInterface()		{ return ip; }

private:


	Interface*	ip;
	FILE*		pStream;
	int			nTotalNodeCount;
	int			nCurNode;
	TCHAR		szFmtStr[16];

	MtlKeeper	mtlList;


   OBJFile* m_pObjFile;
   int      m_curObjectId;
};

#endif // __OBJEXP__H

