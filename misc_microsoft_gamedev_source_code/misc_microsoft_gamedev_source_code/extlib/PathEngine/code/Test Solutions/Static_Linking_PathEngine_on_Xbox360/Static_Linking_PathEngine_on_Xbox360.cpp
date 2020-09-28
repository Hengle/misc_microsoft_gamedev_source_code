
#include "libs/PathEngine_Interface/interface/Singletons.h"
#include "i_pathengine.h"
#include <xtl.h>

void
PathEngine_DebugBreak()
{
    DebugBreak();
}
void
PathEngine_Abort()
{
    _exit(1);
}

bool MemoryTrackingIsEnabled() {return false;}
unsigned long GetTotalMemoryAllocated() {return 0;}
unsigned long GetMaximumMemoryAllocated() {return 0;}
void ResetMaximumMemoryAllocated() {}
unsigned long GetTotalNumberOfAllocations() {return 0;}

class cMinimalErrorHandler : public iErrorHandler
{
public:
    eAction
    handle(const char* type, const char* description, const char *const* attributes)
    {
        DebugBreak();
        return CONTINUE;
    }
};

void __cdecl
main()
{
//******* initialise the SDK
    nSingletons::init_StandAlone();
    iPathEngine* pathEngine = &nSingletons::pathEngineI();

//******* make sure you setup a suitable error handler - this one just debug breaks
    cMinimalErrorHandler errorHandler;
    pathEngine->setErrorHandler(&errorHandler);


//******* do stuff with the SDK
    iMesh *mesh;
    {
        const char meshString[]=
"<mesh>"
	"<mesh3D>"
		"<verts>"
			"<vert x=\"-100\" y=\"100\" z=\"51\"/>"
			"<vert x=\"100\" y=\"100\" z=\"51\"/>"
			"<vert x=\"-100\" y=\"-100\" z=\"51\"/>"
			"<vert x=\"100\" y=\"-100\" z=\"45\"/>"
		"</verts>"
		"<tris>"
			"<tri sectionID=\"0\" surfaceType=\"1\" edge0StartVert=\"0\" edge1StartVert=\"1\" edge2StartVert=\"2\"/>"
			"<tri edge0StartVert=\"1\" edge1StartVert=\"3\" edge2StartVert=\"2\" edge2Connection=\"0\" edge0StartZ=\"50\" edge2StartZ=\"35\"/>"
		"</tris>"
	"</mesh3D>"
"</mesh>";
        mesh = pathEngine->loadMeshFromBuffer("xml", meshString, strlen(meshString), 0);
        if(mesh == 0)
        {
            DebugBreak();
        }
    }
    if(mesh->getNumberOf3DFaces() != 2)
    {
        DebugBreak();
    }




//******* shutdown the SDK
    nSingletons::shutDown_StandAlone();
}
