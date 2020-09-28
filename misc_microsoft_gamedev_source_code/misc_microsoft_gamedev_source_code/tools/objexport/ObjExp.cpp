//************************************************************************** 
//* Asciiexp.cpp	- Ascii File Exporter
//* 
//* By Christer Janson
//* Kinetix Development
//*
//* January 20, 1997 CCJ Initial coding
//*
//* This module contains the DLL startup functions
//*
//* Copyright (c) 1997, All Rights Reserved. 
//***************************************************************************

#include "objexp.h"


HINSTANCE hInstance;
int controlsInit = FALSE;

static BOOL exportSelected;



#define OUTPUT_BINARY_OBJ

#define OBJEXP_CLASS_ID	Class_ID(0x27e17e24, 0x3b970b76)

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) 
{
	hInstance = hinstDLL;

	// Initialize the custom controls. This should be done only once.
	if (!controlsInit) {
		controlsInit = TRUE;
		InitCustomControls(hInstance);
		InitCommonControls();
	}
	
	return (TRUE);
}


__declspec( dllexport ) const TCHAR* LibDescription() 
{
	return GetString(IDS_LIBDESCRIPTION);
}

/// MUST CHANGE THIS NUMBER WHEN ADD NEW CLASS 
__declspec( dllexport ) int LibNumberClasses() 
{
	return 1;
}


__declspec( dllexport ) ClassDesc* LibClassDesc(int i) 
{
	switch(i) {
	case 0: return GetObjExpDesc();
	default: return 0;
	}
}

__declspec( dllexport ) ULONG LibVersion() 
{
	return VERSION_3DSMAX;
}

// Let the plug-in register itself for deferred loading
__declspec( dllexport ) ULONG CanAutoDefer()
{
	return 1;
}

class ObjExpClassDesc:public ClassDesc {
public:
	int				   IsPublic()  { return 1; }
	void*			      Create(BOOL loading = FALSE) { return new OBJExp; } 
	const TCHAR*	   ClassName() { return GetString(IDS_OBJEXP); }
	SClass_ID		   SuperClassID() { return SCENE_EXPORT_CLASS_ID; } 
	Class_ID		      ClassID()   { return OBJEXP_CLASS_ID; }
	const TCHAR*	   Category()  { return GetString(IDS_CATEGORY); }
};

static ObjExpClassDesc ObjExpDesc;

ClassDesc* GetObjExpDesc()
{
	return &ObjExpDesc;
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance)
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;

	return NULL;
}

OBJExp::OBJExp()
{
   m_pObjFile = NULL;
   m_curObjectId = 0;
}

OBJExp::~OBJExp()
{
   delete(m_pObjFile);
   m_pObjFile = NULL;
}

int OBJExp::ExtCount()
{
	return 1;
}

const TCHAR * OBJExp::Ext(int n)
{
	switch(n) {
	case 0:
		// This cause a static string buffer overwrite
		// return GetString(IDS_EXTENSION1);
		return _T("OBX");
	}
	return _T("");
}

const TCHAR * OBJExp::LongDesc()
{
	return GetString(IDS_LONGDESC);
}

const TCHAR * OBJExp::ShortDesc()
{
	return GetString(IDS_SHORTDESC);
}

const TCHAR * OBJExp::AuthorName() 
{
	return _T("Sergio Tacconi");
}

const TCHAR * OBJExp::CopyrightMessage() 
{
	return GetString(IDS_COPYRIGHT);
}

const TCHAR * OBJExp::OtherMessage1() 
{
	return _T("");
}

const TCHAR * OBJExp::OtherMessage2() 
{
	return _T("");
}

unsigned int OBJExp::Version()
{
	return 100;
}

static INT_PTR CALLBACK AboutBoxDlgProc(HWND hWnd, UINT msg, 
	WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		CenterWindow(hWnd, GetParent(hWnd)); 
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hWnd, 1);
			break;
		}
		break;
		default:
			return FALSE;
	}
	return TRUE;
}       

void OBJExp::ShowAbout(HWND hWnd)
{
	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutBoxDlgProc, 0);
}



// Dummy function for progress bar
DWORD WINAPI fn(LPVOID arg)
{
	return(0);
}

// Start the exporter!
// This is the real entrypoint to the exporter. After the user has selected
// the filename (and he's prompted for overwrite etc.) this method is called.
int OBJExp::DoExport(const TCHAR *name,ExpInterface *ei, Interface *i, BOOL suppressPrompts, DWORD options) 
{
	exportSelected = (options & SCENE_EXPORT_SELECTED) ? TRUE : FALSE;

	// Grab the interface pointer.
	ip = i;



   m_pObjFile = new OBJFile();
   m_pObjFile->setName(name);
   
	
	// Startup the progress bar.
	ip->ProgressStart(GetString(IDS_PROGRESS_MSG), TRUE, fn, NULL);

	// Get a total node count by traversing the scene
	// We don't really need to do this, but it doesn't take long, and
	// it is nice to have an accurate progress bar.
	nTotalNodeCount = 0;
	nCurNode = 0;
	PreProcess(ip->GetRootNode(), nTotalNodeCount);

/*
	// First we write out a file header with global information. 
	ExportGlobalInfo();

	// Export list of material definitions
	ExportMaterialList();
*/
	int numChildren = ip->GetRootNode()->NumberOfChildren();
	


   // Count needed number of vertices, normals, etc.
   int numPositions = 0;
   int numTexCoords = 0;
   int numNormals = 0;
   PreProcessCount(ip->GetRootNode(), numPositions, numTexCoords, numNormals);

   m_pObjFile->setNumPositions(numPositions);
   m_pObjFile->setNumTextureCoords(numTexCoords);
   m_pObjFile->setNumNormals(numNormals);


	// Call our node enumerator.
	// The nodeEnum function will recurse into itself and 
	// export each object found in the scene.
	
	for (int idx=0; idx<numChildren; idx++) {
		if (ip->GetCancel())
			break;
		nodeEnum(ip->GetRootNode()->GetChildNode(idx), 0);
	}


#ifndef OUTPUT_BINARY_OBJ

   // Write ASCII OBJ file
   pStream = fopen(name,"w");
   if (!pStream) {
		return 0;
	}

	m_pObjFile->write(pStream);

	fclose(pStream);

#else

   // Write binary OBJ file
   pStream = fopen(name, "wb");
   if (!pStream) {
      return 0;
   }

	m_pObjFile->writeBinary(pStream);

	fclose(pStream);

#endif

	// We're done. Finish the progress bar.
	ip->ProgressEnd();
	return 1;
}

BOOL OBJExp::SupportsOptions(int ext, DWORD options) {
	assert(ext == 0);	// We only support one extension
	return(options == SCENE_EXPORT_SELECTED) ? TRUE : FALSE;
	}

// This method is the main object exporter.
// It is called once of every node in the scene. The objects are
// exported as they are encountered.

// Before recursing into the children of a node, we will export it.
// The benefit of this is that a nodes parent is always before the
// children in the resulting file. This is desired since a child's
// transformation matrix is optionally relative to the parent.

BOOL OBJExp::nodeEnum(INode* node, int indentLevel) 
{
	//if(exportSelected && node->Selected() == FALSE)
	//	return TREE_CONTINUE;

	nCurNode++;
	ip->ProgressUpdate((int)((float)nCurNode/nTotalNodeCount*100.0f)); 

	// Stop recursing if the user pressed Cancel 
	if (ip->GetCancel())
		return FALSE;
	
	TSTR indent = GetIndent(indentLevel);
	
	// If this node is a group head, all children are 
	// members of this group. The node will be a dummy node and the node name
	// is the actually group name.
	if (node->IsGroupHead()) {
		indentLevel++;
	}
	
	// Only export if exporting everything or it's selected
	if(!exportSelected || node->Selected()) {

		// The ObjectState is a 'thing' that flows down the pipeline containing
		// all information about the object. By calling EvalWorldState() we tell
		// max to eveluate the object at end of the pipeline.
		ObjectState os = node->EvalWorldState(0); 

		// The obj member of ObjectState is the actual object we will export.
		if (os.obj) {

			// We look at the super class ID to determine the type of the object.
			switch(os.obj->SuperClassID()) {
			case GEOMOBJECT_CLASS_ID: 
				ExportGeomObject(node, indentLevel); 
				break;
/*
			case CAMERA_CLASS_ID:
				if (GetIncludeObjCamera()) ExportCameraObject(node, indentLevel); 
				break;
			case LIGHT_CLASS_ID:
				if (GetIncludeObjLight()) ExportLightObject(node, indentLevel); 
				break;
			case SHAPE_CLASS_ID:
				if (GetIncludeObjShape()) ExportShapeObject(node, indentLevel); 
				break;
			case HELPER_CLASS_ID:
				if (GetIncludeObjHelper()) ExportHelperObject(node, indentLevel); 
				break;
*/
			}
		}
	}	
	
	// For each child of this node, we recurse into ourselves 
	// until no more children are found.
	for (int c = 0; c < node->NumberOfChildren(); c++) {
		nodeEnum(node->GetChildNode(c), indentLevel);
	}
	
	// If thie is true here, it is the end of the group we started above.
	if (node->IsGroupHead()) 
   {
		indentLevel--;
	}

	return TRUE;
}



void OBJExp::PreProcessCount(INode* node, int& positionCount, int& texCoordCount, int& normalCount)
{
   // Only export if exporting everything or it's selected
   if(!exportSelected || node->Selected()) {

      // The ObjectState is a 'thing' that flows down the pipeline containing
      // all information about the object. By calling EvalWorldState() we tell
      // max to eveluate the object at end of the pipeline.
      ObjectState os = node->EvalWorldState(0); 

      // The obj member of ObjectState is the actual object we will export.
      if (os.obj) {

         // We look at the super class ID to determine the type of the object.
         switch(os.obj->SuperClassID()) {
         case GEOMOBJECT_CLASS_ID: 
            
            // Targets are actually geomobjects, skip here
            if (os.obj->ClassID() != Class_ID(TARGET_CLASS_ID, 0))
            {
               BOOL needDel;
               TriObject* tri = GetTriObjectFromNode(node, 0, needDel);
               if (!tri) {
                  return;
               }

               Mesh* mesh = &tri->GetMesh();
               int numFaces = mesh->getNumFaces();

               positionCount += numFaces * 3;
               normalCount += numFaces * 3;

               if (needDel) {
                  delete tri;
               }
            }
            break;
         }
      }
   }	

   // For each child of this node, we recurse into ourselves 
   // until no more children are found.
   for (int c = 0; c < node->NumberOfChildren(); c++) {
      PreProcessCount(node->GetChildNode(c), positionCount, texCoordCount, normalCount);
   }
}


void OBJExp::PreProcess(INode* node, int& nodeCount)
{
	nodeCount++;

	// Add the nodes material to out material list
	// Null entries are ignored when added...
	mtlList.AddMtl(node->GetMtl());

	// For each child of this node, we recurse into ourselves 
	// and increment the counter until no more children are found.
	for (int c = 0; c < node->NumberOfChildren(); c++) {
		PreProcess(node->GetChildNode(c), nodeCount);
	}
}


BOOL MtlKeeper::AddMtl(Mtl* mtl)
{
	if (!mtl) {
		return FALSE;
	}

	int numMtls = mtlTab.Count();
	for (int i=0; i<numMtls; i++) {
		if (mtlTab[i] == mtl) {
			return FALSE;
		}
	}
	mtlTab.Append(1, &mtl, 25);

	return TRUE;
}

int MtlKeeper::GetMtlID(Mtl* mtl)
{
	int numMtls = mtlTab.Count();
	for (int i=0; i<numMtls; i++) {
		if (mtlTab[i] == mtl) {
			return i;
		}
	}
	return -1;
}

int MtlKeeper::Count()
{
	return mtlTab.Count();
}

Mtl* MtlKeeper::GetMtl(int id)
{
	return mtlTab[id];
}
