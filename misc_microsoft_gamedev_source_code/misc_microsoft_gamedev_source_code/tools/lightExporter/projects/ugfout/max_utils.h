// max_utils.h
#ifndef MAX_UTILS_H
#define MAX_UTILS_H

typedef std::vector<TimeValue> TimeValueVec;
typedef std::vector<INode*> INodePtrVec;

TriObject* GetTriObjectFromNode(INode* pNode, TimeValue t, int &deleteIt);
BOOL TMNegParity(const Matrix3& m);
Modifier *FindPhysiqueModifier (INode* pNode);
Modifier* FindSkinModifier(INode* pNode);
bool IsNodeBone(INode* pNode);
bool IsNodeBipedBone(INode* pNode);
bool IsNodeLight(INode* pNode);
bool IsNodeCamera(INode* pNode);
bool IsNodeDummyHelper(INode* pNode);
Point3 GetRVertexNormal(RVertex* pRVertex, DWORD smGroupFace);
Matrix3 MakeNormalTM(Matrix3 ntm);
bool IsStdMulti(Mtl *m);
bool IsNodeSphere(INode* obj);
float GetSphereRadius(INode* pNode, TimeValue t);
bool GetNodeUDP(INode* pNode, std::vector<char>& dst);

class AutoTriObject
{
public:	
	AutoTriObject(INode* pNode, TimeValue time)
	{
		mpTri = GetTriObjectFromNode(pNode, time, mNeedDel);
	}
	
	~AutoTriObject()
	{
		clear();
	}
	
	TriObject* get(void) const { return mpTri; }
	
	void clear(void)
	{
		if (mNeedDel)
			delete mpTri;
		
		mpTri = NULL;
		mNeedDel = 0;
	}
	
private:
	TriObject* mpTri;
	BOOL mNeedDel;
};

#endif // MAX_UTILS_H
