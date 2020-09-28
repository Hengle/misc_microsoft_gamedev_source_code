// utils.cpp
#include "ugfout.h"
#include "iskin.h"
#include "bipexp.h"
#include "simpobj.h"

Matrix3 MakeNormalTM(Matrix3 ntm)
{
	ntm.NoScale();
  ntm.Invert();
  {
    Matrix3 tmp;
    for (int i=0; i<3; ++i)
    {
      Point4 p = ntm.GetRow(i);
      tmp.SetColumn(i, p);
    }
    ntm = tmp;
  }
  ntm.NoTrans();
	return ntm;
}

// Return a pointer to a TriObject given an INode or return NULL
// if the pNode cannot be converted to a TriObject
TriObject* GetTriObjectFromNode(INode* pNode, TimeValue t, int& deleteIt)
{
  deleteIt = FALSE;
  Object *obj = pNode->EvalWorldState(t).obj;
  if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) {
    TriObject *tri = (TriObject *) obj->ConvertToType(t,
      Class_ID(TRIOBJ_CLASS_ID, 0));
    // Note that the TriObject should only be deleted
    // if the pointer to it is not equal to the object
    // pointer that called ConvertToType()
    if (obj != tri) deleteIt = TRUE;
    return tri;
  }
  else {
    return NULL;
  }
}

BOOL TMNegParity(const Matrix3 &m)
{
	return (DotProd(CrossProd(m.GetRow(0),m.GetRow(1)),m.GetRow(2))<0.0)?1:0;
}

Modifier *FindPhysiqueModifier (INode* pNode)
{
	// Get object from pNode. Abort if no object.
	Object *ObjectPtr = pNode->GetObjectRef();
	if (!ObjectPtr) return NULL;

	// Is derived object ?
	if (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject *DerivedObjectPtr = static_cast<IDerivedObject*>(ObjectPtr);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < DerivedObjectPtr->NumModifiers())
		{
			// Get current modifier.
			Modifier *ModifierPtr = DerivedObjectPtr->GetModifier(ModStackIndex);

			// Is this Physique ?
			if (ModifierPtr->ClassID() == Class_ID(	PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B) )
			{
				// Yes -> Exit.
				return ModifierPtr;
			}
			// Next modifier stack entry.
			ModStackIndex++;
		}
	}
	// Not found.
	return NULL;
}

Modifier* FindSkinModifier(INode* pNode)
{
	Object *pObj = pNode->GetObjectRef();
	if(!pObj) return NULL;
	
	while(pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		IDerivedObject *pDerivedObj = static_cast<IDerivedObject*>(pObj);
		
		int stackIdx = 0;
		while(stackIdx < pDerivedObj->NumModifiers())
		{
			Modifier* pMod = pDerivedObj->GetModifier(stackIdx++);
			if(pMod->ClassID() == SKIN_CLASSID)
				return pMod;
		}
		pObj = pDerivedObj->GetObjRef();
	}
	
	return NULL;
}

bool IsNodeBone(INode* pNode)
{
	if(pNode == NULL || pNode->IsRootNode())
		return false;
	
	ObjectState os = pNode->EvalWorldState(0);
	if(os.obj->ClassID() == Class_ID(BONE_CLASS_ID, 0))
		return true;
	// dct 10/18/01, r4 creates bones as objects rather than helpers
	if(os.obj->ClassID() == BONE_OBJ_CLASSID)
		return true;

	// we don't want biped end effectors
	if(os.obj->ClassID() == Class_ID(DUMMY_CLASS_ID, 0))
		return false;

	Control *cont = pNode->GetTMController();
	if(cont->ClassID() == BIPSLAVE_CONTROL_CLASS_ID ||
		cont->ClassID() == BIPBODY_CONTROL_CLASS_ID
//		cont->ClassID() == FOOTPRINT_CLASS_ID
	) return true;
		
	return false;
}

bool IsNodeBipedBone(INode* pNode)
{
	if(pNode == NULL || pNode->IsRootNode())
		return false;

	Control *cont = pNode->GetTMController();
	if(cont->ClassID() == BIPSLAVE_CONTROL_CLASS_ID ||
		cont->ClassID() == BIPBODY_CONTROL_CLASS_ID
	) return true;

	return false;
}

bool IsNodeLight(INode* pNode)
{
	if (pNode == NULL || pNode->IsRootNode())
		return false;

	ObjectState os = pNode->EvalWorldState(0);
	if (os.obj && os.obj->SuperClassID() == LIGHT_CLASS_ID)
		return true;
	
	return false;
}

bool IsNodeCamera(INode* pNode)
{
	if (pNode == NULL || pNode->IsRootNode())
		return false;

	ObjectState os = pNode->EvalWorldState(0);
	if (os.obj && os.obj->SuperClassID() == CAMERA_CLASS_ID)
		return true;
	
	return false;
}

bool IsNodeDummyHelper(INode* pNode)
{
	if (pNode == NULL || pNode->IsRootNode())
		return false;

	ObjectState os = pNode->EvalWorldState(0);
	if (os.obj && os.obj->SuperClassID() == HELPER_CLASS_ID)
	{
		TSTR className;
		os.obj->GetClassName(className);
		if (className == TSTR("Dummy"))
			return true;
	}
	
	return false;
}

Point3 GetRVertexNormal(RVertex* pRVertex, DWORD smGroupFace)
{
	const int cNormals = pRVertex->rFlags & NORCT_MASK;
	
	if ((cNormals <= 1) || (!pRVertex->ern))
		return pRVertex->rn.getNormal();

	Verify(	((cNormals == 1) && (pRVertex->ern == NULL)) || 
					((cNormals > 1) && (pRVertex->ern != NULL)) );
	
	for (int irn = 0; irn < cNormals; irn++)
		if (pRVertex->ern[irn].getSmGroup() & smGroupFace)
			break;

	if (irn >= cNormals)
		irn = 0;

	return pRVertex->ern[irn].getNormal();
}

bool IsStdMulti(Mtl *m) 
{
	return (m->ClassID() == Class_ID(DMTL_CLASS_ID, 0)) != 0;
}

bool IsNodeSphere(INode* pNode)
{
	Object* obj = pNode->GetObjectRef();
	if (!obj)
		return false;
	
	return (
		(obj->SuperClassID() == GEOMOBJECT_CLASS_ID) && 
		(obj->ClassID() == Class_ID(SPHERE_CLASS_ID,0))
		);
}

float GetSphereRadius(INode* pNode, TimeValue t)
{
	if (!IsNodeSphere(pNode))
		return 0;
		
	Object* obj = pNode->GetObjectRef();
	if (!obj) 
		return 0;
		
	SimpleObject* so = reinterpret_cast<SimpleObject*>(obj);
	
	float rad;
	so->pblock->GetValue(SPHERE_RADIUS, t, rad, FOREVER);
	
	return rad;
}

bool GetNodeUDP(INode* pNode, std::vector<char>& dst)
{
	TSTR buf;
	pNode->GetUserPropBuffer(buf);
	
	dst.clear();
	dst.reserve(buf.length());
	
	for (int i = 0; i < buf.length(); i++)
		dst.push_back(buf[i]);
		
	return !buf.isNull();
}

