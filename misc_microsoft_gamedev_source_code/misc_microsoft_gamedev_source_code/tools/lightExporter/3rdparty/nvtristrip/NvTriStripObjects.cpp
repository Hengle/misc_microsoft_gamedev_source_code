// RG: 12/27/00: many bugfixes, optimizations

// NVidia set this to 10
#define NUM_SAMPLES 4
//#define NUM_SAMPLES 10

#include "NvTriStripObjects.h"
#include "VertexCache.h"

//#define ORIG_CODE

NvStripifier::NvStripifier()
{

}

NvStripifier::~NvStripifier()
{

}

///////////////////////////////////////////////////////////////////////////////////////////
// FindEdgeInfo()
//
// find the edge info for these two indices
//
NvEdgeInfo * NvStripifier::FindEdgeInfo(NvEdgeInfoVec &edgeInfos, int v0, int v1){

	// we can get to it through either array
	// because the edge infos have a v0 and v1
	// and there is no order except how it was
	// first created.
	NvEdgeInfo *infoIter = edgeInfos[v0];
	while (infoIter != NULL){
		if (infoIter->m_v0 == v0){
			if (infoIter->m_v1 == v1)
				return infoIter;
			else
				infoIter = infoIter->m_nextV0;
		}
		else {
			assert(infoIter->m_v1 == v0);
			if (infoIter->m_v0 == v1)
				return infoIter;
			else
				infoIter = infoIter->m_nextV1;
		}
	}
	return NULL;
}


///////////////////////////////////////////////////////////////////////////////////////////
// FindOtherFace
//
// find the other face sharing these vertices
// exactly like the edge info above
//
NvFaceInfo * NvStripifier::FindOtherFace(
  NvEdgeInfoVec &edgeInfos, int v0, int v1, NvFaceInfo *faceInfo)
{
	NvEdgeInfo *edgeInfo = FindEdgeInfo(edgeInfos, v0, v1);
	assert(edgeInfo != NULL);
	return (edgeInfo->m_face0 == faceInfo ? edgeInfo->m_face1 : edgeInfo->m_face0);
}

// RG: is NVidia crazy?!
bool NvStripifier::AlreadyExists(NvFaceInfo* faceInfo, NvFaceInfoVec& faceInfos)
{
	for(unsigned int i = 0; i < faceInfos.size(); i++)
	{
		if( (faceInfos[i]->m_v0 == faceInfo->m_v0) &&
			(faceInfos[i]->m_v1 == faceInfo->m_v1) &&
			(faceInfos[i]->m_v2 == faceInfo->m_v2) )
			return true;
	}

	return false;
}

class unique_face
{
  int m_v0, m_v1, m_v2;
public:
  unique_face(int v0, int v1, int v2) : m_v0(v0), m_v1(v1), m_v2(v2) { }

  friend bool operator< (const unique_face& a, const unique_face& b)
  {
    if (a.m_v0 < b.m_v0)
      return (true);
    else if (a.m_v0 == b.m_v0)
    {
      if (a.m_v1 < b.m_v1)
        return (true);
      else if (a.m_v1 == b.m_v1)
      {
        if (a.m_v2 < b.m_v2)
          return (true);
      }
    }
    return (false);
  }
};

///////////////////////////////////////////////////////////////////////////////////////////
// BuildStripifyInfo()
//
// Builds the list of all face and edge infos
//
void NvStripifier::BuildStripifyInfo(NvFaceInfoVec &faceInfos, NvEdgeInfoVec &edgeInfos){

	// reserve space for the face infos, but do not resize them.
	int numIndices = indices.size();
	faceInfos.reserve(numIndices);

	// we actually resize the edge infos, so we must initialize to NULL
	//edgeInfos.resize (numIndices); // RG, wasn't resizing by the proper amount
  edgeInfos.resize(max_indice + 1);
	for (int i = 0; i <= max_indice; i++)
		edgeInfos[i] = NULL;

	// iterate through the triangles of the triangle list
	int numTriangles = numIndices / 3;
	int index        = 0;

  std::set<unique_face> face_set;

  int unique = 0;

	for (i = 0; i < numTriangles; i++)
	{
		// grab the indices
		int v0 = indices[index++];
		int v1 = indices[index++];
		int v2 = indices[index++];

    assert(v0!=v1); // RG: degenerate tris cause this code to crash!!
    assert(v0!=v2);
    assert(v1!=v2);

		//if(!AlreadyExists(faceInfo, faceInfos))
    if (face_set.insert(unique_face(v0, v1, v2)).second)
    {
      unique++;

      // create the face info and add it to the list of faces, but only if this exact face doesn't already
	  	//  exist in the list
		  NvFaceInfo *faceInfo = new NvFaceInfo(v0, v1, v2);

      faceInfos.push_back(faceInfo);

			// grab the edge infos, creating them if they do not already exist
			NvEdgeInfo *edgeInfo01 = FindEdgeInfo(edgeInfos, v0, v1);
			if (edgeInfo01 == NULL){

				// create the info
				edgeInfo01 = new NvEdgeInfo(v0, v1);

				// update the linked list on both
				edgeInfo01->m_nextV0 = edgeInfos[v0];
				edgeInfo01->m_nextV1 = edgeInfos[v1];
				edgeInfos[v0] = edgeInfo01;
				edgeInfos[v1] = edgeInfo01;

				// set face 0
				edgeInfo01->m_face0 = faceInfo;
			}
			else {
				if (edgeInfo01->m_face1 != NULL)
				{
					//G_log().printf("NvStripifier::BuildStripifyInfo: > 2 triangles on an edge... uncertain consequences\n");
				}
				else
					edgeInfo01->m_face1 = faceInfo;
			}

			// grab the edge infos, creating them if they do not already exist
			NvEdgeInfo *edgeInfo12 = FindEdgeInfo(edgeInfos, v1, v2);
			if (edgeInfo12 == NULL){

				// create the info
				edgeInfo12 = new NvEdgeInfo(v1, v2);

				// update the linked list on both
				edgeInfo12->m_nextV0 = edgeInfos[v1];
				edgeInfo12->m_nextV1 = edgeInfos[v2];
				edgeInfos[v1] = edgeInfo12;
				edgeInfos[v2] = edgeInfo12;

				// set face 0
				edgeInfo12->m_face0 = faceInfo;
			}
			else {
				if (edgeInfo12->m_face1 != NULL)
				{
					//G_log().printf("NvStripifier::BuildStripifyInfo: > 2 triangles on an edge... uncertain consequences\n");
				}
				else
					edgeInfo12->m_face1 = faceInfo;
			}

			// grab the edge infos, creating them if they do not already exist
			NvEdgeInfo *edgeInfo20 = FindEdgeInfo(edgeInfos, v2, v0);
			if (edgeInfo20 == NULL){

				// create the info
				edgeInfo20 = new NvEdgeInfo(v2, v0);

				// update the linked list on both
				edgeInfo20->m_nextV0 = edgeInfos[v2];
				edgeInfo20->m_nextV1 = edgeInfos[v0];
				edgeInfos[v2] = edgeInfo20;
				edgeInfos[v0] = edgeInfo20;

				// set face 0
				edgeInfo20->m_face0 = faceInfo;
			}
			else {
				if (edgeInfo20->m_face1 != NULL)
				{
					//G_log().printf("NvStripifier::BuildStripifyInfo: > 2 triangles on an edge... uncertain consequences\n");
				}
				else
					edgeInfo20->m_face1 = faceInfo;
			}

		}
	}

//  G_log().printf("NvStripifier::BuildStripifyInfo: %i out of %i tris where unique\n", unique, numTriangles);
}


///////////////////////////////////////////////////////////////////////////////////////////
// FindStartPoint()
//
// Finds a good starting point, namely one which has only one neighbor
//
int NvStripifier::FindStartPoint(NvFaceInfoVec &faceInfos, NvEdgeInfoVec &edgeInfos)
{
	for(unsigned int i = 0; i < faceInfos.size(); i++)
	{
		int ctr = 0;

		if(FindOtherFace(edgeInfos, faceInfos[i]->m_v0, faceInfos[i]->m_v1, faceInfos[i]) == NULL)
			ctr++;
		if(FindOtherFace(edgeInfos, faceInfos[i]->m_v1, faceInfos[i]->m_v2, faceInfos[i]) == NULL)
			ctr++;
		if(FindOtherFace(edgeInfos, faceInfos[i]->m_v2, faceInfos[i]->m_v0, faceInfos[i]) == NULL)
			ctr++;
		if(ctr > 1)
			return i;
	}
	return -1;
}


///////////////////////////////////////////////////////////////////////////////////////////
// FindGoodResetPoint()
//
// A good reset point is one near other commited areas so that
// we know that when we've made the longest strips its because
// we're stripifying in the same general orientation.
//
NvFaceInfo* NvStripifier::FindGoodResetPoint(NvFaceInfoVec &faceInfos, NvEdgeInfoVec &edgeInfos){
	// we hop into different areas of the mesh to try to get
	// other large open spans done.  Areas of small strips can
	// just be left to triangle lists added at the end.
	NvFaceInfo *result = NULL;

	if(result == NULL)
	{
		int numFaces   = faceInfos.size();
		int startPoint;
		if(bFirstTimeResetPoint)
		{
			//first time, find a face with few neighbors (look for an edge of the mesh)
			startPoint = FindStartPoint(faceInfos, edgeInfos);
			bFirstTimeResetPoint = false;
		}
		else
			startPoint = (int)(((float) numFaces - 1) * meshJump);

		if(startPoint == -1)
			startPoint = (int)(((float) numFaces - 1) * meshJump);

		int i = startPoint;
		do {

			// if this guy isn't visited, try him
			if (faceInfos[i]->m_stripId < 0){
				result = faceInfos[i];
				break;
			}

			// update the index and clamp to 0-(numFaces-1)
			if (++i >= numFaces)
				i = 0;

		} while (i != startPoint);

		// update the meshJump
		meshJump += 0.1f;
		if (meshJump > 1.0f)
			meshJump = .05f;
	}

	// return the best face we found
	return result;
}


///////////////////////////////////////////////////////////////////////////////////////////
// GetUniqueVertexInB()
//
// Returns the vertex unique to faceB
//
int NvStripifier::GetUniqueVertexInB(NvFaceInfo *faceA, NvFaceInfo *faceB){

	int facev0 = faceB->m_v0;
	if (facev0 != faceA->m_v0 &&
		facev0 != faceA->m_v1 &&
		facev0 != faceA->m_v2)
		return facev0;

	int facev1 = faceB->m_v1;
	if (facev1 != faceA->m_v0 &&
		facev1 != faceA->m_v1 &&
		facev1 != faceA->m_v2)
		return facev1;

	int facev2 = faceB->m_v2;
	if (facev2 != faceA->m_v0 &&
		facev2 != faceA->m_v1 &&
		facev2 != faceA->m_v2)
		return facev2;

	// nothing is different
	return -1;
}


///////////////////////////////////////////////////////////////////////////////////////////
// GetSharedVertex()
//
// Returns the vertex shared between the two input faces
//
int NvStripifier::GetSharedVertex(NvFaceInfo *faceA, NvFaceInfo *faceB){

	int facev0 = faceB->m_v0;
	if (facev0 == faceA->m_v0 ||
		facev0 == faceA->m_v1 ||
		facev0 == faceA->m_v2)
		return facev0;

	int facev1 = faceB->m_v1;
	if (facev1 == faceA->m_v0 ||
		facev1 == faceA->m_v1 ||
		facev1 == faceA->m_v2)
		return facev1;

	int facev2 = faceB->m_v2;
	if (facev2 == faceA->m_v0 ||
		facev2 == faceA->m_v1 ||
		facev2 == faceA->m_v2)
		return facev2;

	// nothing is shared
	return -1;
}


///////////////////////////////////////////////////////////////////////////////////////////
// GetNextIndex()
//
// Returns vertex of the input face which is "next" in the input index list
//
inline int NvStripifier::GetNextIndex(const IndexVec &indices, NvFaceInfo *face){

	int numIndices = indices.size();
	assert(numIndices >= 2);

	int v0  = indices[numIndices-2];
	int v1  = indices[numIndices-1];

	int fv0 = face->m_v0;
	int fv1 = face->m_v1;
	int fv2 = face->m_v2;

	if (fv0 != v0 && fv0 != v1){
		if ((fv1 != v0 && fv1 != v1) || (fv2 != v0 && fv2 != v1)){
//			G_log().printf("NvStripifier::GetNextIndex: Triangle doesn't have all of its vertices\n");
//			G_log().printf("NvStripifier::GetNextIndex: Duplicate triangle probably got us derailed\n");
		}
		return fv0;
	}
	if (fv1 != v0 && fv1 != v1){
		if ((fv0 != v0 && fv0 != v1) || (fv2 != v0 && fv2 != v1)){
//			G_log().printf("NvStripifier::GetNextIndex: Triangle doesn't have all of its vertices\n");
//			G_log().printf("NvStripifier::GetNextIndex: Duplicate triangle probably got us derailed\n");
		}
		return fv1;
	}
	if (fv2 != v0 && fv2 != v1){
		if ((fv0 != v0 && fv0 != v1) || (fv1 != v0 && fv1 != v1)){
//			G_log().printf("NvStripifier::GetNextIndex: Triangle doesn't have all of its vertices\n");
//			G_log().printf("NvStripifier::GetNextIndex: Duplicate triangle probably got us derailed\n");
		}
		return fv2;
	}

	// shouldn't get here
//	G_log().printf("NvStripifier::GetNextIndex: Duplicate triangle sent\n");
	return -1;
}


///////////////////////////////////////////////////////////////////////////////////////////
// IsMarked()
//
// If either the faceInfo has a real strip index because it is
// already assign to a committed strip OR it is assigned in an
// experiment and the experiment index is the one we are building
// for, then it is marked and unavailable
inline bool NvStripInfo::IsMarked(NvFaceInfo *faceInfo){
	return (faceInfo->m_stripId >= 0) || (IsExperiment() && faceInfo->m_experimentId == m_experimentId);
}


///////////////////////////////////////////////////////////////////////////////////////////
// MarkTriangle()
//
// Marks the face with the current strip ID
//
inline void NvStripInfo::MarkTriangle(NvFaceInfo *faceInfo){
	assert(!IsMarked(faceInfo));
	if (IsExperiment()){
		faceInfo->m_experimentId = m_experimentId;
		faceInfo->m_testStripId  = m_stripId;
    }
	else{
		assert(faceInfo->m_stripId == -1);
		faceInfo->m_experimentId = -1;
		faceInfo->m_stripId      = m_stripId;
	}
}

bool NvStripInfo::Unique(
  NvFaceInfoVec& faceVec,
  const BoolVec& bmap,
  NvFaceInfo* face)
{
#ifndef ORIG_CODE
  bool tbv0 = bmap.at(face->m_v0);
  bool tbv1 = bmap.at(face->m_v1);
  bool tbv2 = bmap.at(face->m_v2);
  if (tbv0 && tbv1 && tbv2)
    return (false);
  return true;

#else
	bool bv0, bv1, bv2; //bools to indicate whether a vertex is in the faceVec or not
	bv0 = bv1 = bv2 = false;

	for(int i = 0; i < faceVec.size(); i++)
	{
		if(!bv0)
		{
			if( (faceVec[i]->m_v0 == face->m_v0) ||
				(faceVec[i]->m_v1 == face->m_v0) ||
				(faceVec[i]->m_v2 == face->m_v0) )
				bv0 = true;
		}

		if(!bv1)
		{
			if( (faceVec[i]->m_v0 == face->m_v1) ||
				(faceVec[i]->m_v1 == face->m_v1) ||
				(faceVec[i]->m_v2 == face->m_v1) )
				bv1 = true;
		}

		if(!bv2)
		{
			if( (faceVec[i]->m_v0 == face->m_v2) ||
				(faceVec[i]->m_v1 == face->m_v2) ||
				(faceVec[i]->m_v2 == face->m_v2) )
				bv2 = true;
		}

		//the face is not unique, all it's vertices exist in the face vector
		if(bv0 && bv1 && bv2)
    {
      return false;
    }
	}

	//if we get out here, it's unique
	return true;
#endif
}

inline void update_bmap(BoolVec& bmap, const NvFaceInfo* Pface)
{
  bmap.at(Pface->m_v0) = true;
  bmap.at(Pface->m_v1) = true;
  bmap.at(Pface->m_v2) = true;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Build()
//
// Builds a strip forward as far as we can go, then builds backwards, and joins the two lists
//
void NvStripInfo::Build(NvEdgeInfoVec &edgeInfos, NvFaceInfoVec &faceInfos){

	// used in building the strips forward and backward
	static IndexVec scratchIndices;
	scratchIndices.resize(0);

	// build forward... start with the initial face
	NvFaceInfoVec forwardFaces, backwardFaces;
  BoolVec forwardFaces_bmap(m_nvs.max_indice+1);

	forwardFaces.push_back(m_startInfo.m_startFace);
  update_bmap(forwardFaces_bmap, m_startInfo.m_startFace);

	MarkTriangle(m_startInfo.m_startFace);

	int v0 = (m_startInfo.m_toV1 ? m_startInfo.m_startEdge->m_v0 : m_startInfo.m_startEdge->m_v1);
	int v1 = (m_startInfo.m_toV1 ? m_startInfo.m_startEdge->m_v1 : m_startInfo.m_startEdge->m_v0);

	// easiest way to get v2 is to use this function which requires the
	// other indices to already be in the list.
	scratchIndices.push_back(v0);
	scratchIndices.push_back(v1);
	int v2 = NvStripifier::GetNextIndex(scratchIndices, m_startInfo.m_startFace);
	scratchIndices.push_back(v2);

	//
	// build the forward list
	//
	int nv0 = v1;
	int nv1 = v2;

	NvFaceInfo *nextFace = NvStripifier::FindOtherFace(edgeInfos, nv0, nv1, m_startInfo.m_startFace);
	while (nextFace != NULL && !IsMarked(nextFace))
	{
		//this tests to see if a face is "unique", meaning that its vertices aren't already in the list
		// so, strips which "wrap-around" are not allowed
		if(!Unique(forwardFaces, forwardFaces_bmap, nextFace))
			break;

		// add this to the strip
		forwardFaces.push_back(nextFace);
    update_bmap(forwardFaces_bmap, nextFace);

		MarkTriangle(nextFace);

		// add the index
		nv0 = nv1;
		nv1 = NvStripifier::GetNextIndex(scratchIndices, nextFace);
		scratchIndices.push_back(nv1);

		// and get the next face
		nextFace = NvStripifier::FindOtherFace(edgeInfos, nv0, nv1, nextFace);

	}

	// tempAllFaces is going to be forwardFaces + backwardFaces
	// it's used for Unique()
	NvFaceInfoVec tempAllFaces;
  BoolVec tempAllFaces_bmap(m_nvs.max_indice+1);

	for(unsigned int i = 0; i < forwardFaces.size(); i++)
  {
		tempAllFaces.push_back(forwardFaces[i]);
    update_bmap(tempAllFaces_bmap, forwardFaces[i]);
  }

	//
	// reset the indices for building the strip backwards and do so
	//
	scratchIndices.resize(0);
	scratchIndices.push_back(v2);
	scratchIndices.push_back(v1);
	scratchIndices.push_back(v0);
	nv0 = v1;
	nv1 = v0;
	nextFace = NvStripifier::FindOtherFace(edgeInfos, nv0, nv1, m_startInfo.m_startFace);
	while (nextFace != NULL && !IsMarked(nextFace))
	{
		//this tests to see if a face is "unique", meaning that its vertices aren't already in the list
		// so, strips which "wrap-around" are not allowed
		if(!Unique(tempAllFaces, tempAllFaces_bmap, nextFace))
			break;

		// add this to the strip
		backwardFaces.push_back(nextFace);

		//this is just so Unique() will work
		tempAllFaces.push_back(nextFace);
    update_bmap(tempAllFaces_bmap, nextFace);

		MarkTriangle(nextFace);

		// add the index
		nv0 = nv1;
		nv1 = NvStripifier::GetNextIndex(scratchIndices, nextFace);
		scratchIndices.push_back(nv1);

		// and get the next face
		nextFace = NvStripifier::FindOtherFace(edgeInfos, nv0, nv1, nextFace);
	}

	// Combine the forward and backwards stripification lists and put into our own face vector
	Combine(forwardFaces, backwardFaces);
}


///////////////////////////////////////////////////////////////////////////////////////////
// Combine()
//
// Combines the two input face vectors and puts the result into m_faces
//
void NvStripInfo::Combine(const NvFaceInfoVec &forward, const NvFaceInfoVec &backward){

	// add backward faces
	int numFaces = backward.size();
	for (int i = numFaces - 1; i >= 0; i--)
		m_faces.push_back(backward[i]);

	// add forward faces
	numFaces = forward.size();
	for (i = 0; i < numFaces; i++)
		m_faces.push_back(forward[i]);
}


///////////////////////////////////////////////////////////////////////////////////////////
// SharesEdge()
//
// Returns true if the input face and the current strip share an edge
//
bool NvStripInfo::SharesEdge(const NvFaceInfo* faceInfo, NvEdgeInfoVec &edgeInfos)
{
	//check v0->v1 edge
	NvEdgeInfo* currEdge = NvStripifier::FindEdgeInfo(edgeInfos, faceInfo->m_v0, faceInfo->m_v1);

	if(IsInStrip(currEdge->m_face0) || IsInStrip(currEdge->m_face1))
		return true;

	//check v1->v2 edge
	currEdge = NvStripifier::FindEdgeInfo(edgeInfos, faceInfo->m_v1, faceInfo->m_v2);

	if(IsInStrip(currEdge->m_face0) || IsInStrip(currEdge->m_face1))
		return true;

	//check v2->v0 edge
	currEdge = NvStripifier::FindEdgeInfo(edgeInfos, faceInfo->m_v2, faceInfo->m_v0);

	if(IsInStrip(currEdge->m_face0) || IsInStrip(currEdge->m_face1))
		return true;

	return false;

}


///////////////////////////////////////////////////////////////////////////////////////////
// CommitStrips()
//
// "Commits" the input strips by setting their m_experimentId to -1 and adding to the allStrips
//  vector
//
void NvStripifier::CommitStrips(NvStripInfoVec &allStrips, const NvStripInfoVec &strips)
{
	// Iterate through strips
	int numStrips = strips.size();
	for (int i = 0; i < numStrips; i++){

		// Tell the strip that it is now real
		NvStripInfo *strip = strips[i];
		strip->m_experimentId = -1;

		// add to the list of real strips
		allStrips.push_back(strip);

		// Iterate through the faces of the strip
		// Tell the faces of the strip that they belong to a real strip now
		const NvFaceInfoVec &faces = strips[i]->m_faces;
		int numFaces = faces.size();

//RG: umm, old debug code from NVidia?
//		if( (faces[0]->m_v0 == 2302) &&
//			(faces[0]->m_v1 == 3215) &&
//			(faces[0]->m_v2 == 2603) )
//			printf("BLEH");

		for (int j = 0; j < numFaces; j++)
		{
			strip->MarkTriangle(faces[j]);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////
// FindTraversal()
//
// Finds the next face to start the next strip on.
//
bool NvStripifier::FindTraversal(NvFaceInfoVec &faceInfos,
								 NvEdgeInfoVec    &edgeInfos,
								 NvStripInfo      *strip,
								 NvStripStartInfo &startInfo){

	// if the strip was v0->v1 on the edge, then v1 will be a vertex in the next edge.
	int v = (strip->m_startInfo.m_toV1 ? strip->m_startInfo.m_startEdge->m_v1 : strip->m_startInfo.m_startEdge->m_v0);

	NvFaceInfo *untouchedFace = NULL;
	NvEdgeInfo *edgeIter      = edgeInfos[v];
	while (edgeIter != NULL){
		NvFaceInfo *face0 = edgeIter->m_face0;
		NvFaceInfo *face1 = edgeIter->m_face1;
		if ((face0 != NULL && !strip->IsInStrip(face0)) && face1 != NULL && !strip->IsMarked(face1))
		{
			untouchedFace = face1;
			break;
		}
		if ((face1 != NULL && !strip->IsInStrip(face1)) && face0 != NULL && !strip->IsMarked(face0)){
			untouchedFace = face0;
			break;
		}

		// find the next edgeIter
		edgeIter = (edgeIter->m_v0 == v ? edgeIter->m_nextV0 : edgeIter->m_nextV1);
	}

	startInfo.m_startFace = untouchedFace;
	startInfo.m_startEdge = edgeIter;
	if (edgeIter != NULL)
	{
		if(strip->SharesEdge(startInfo.m_startFace, edgeInfos))
			startInfo.m_toV1 = (edgeIter->m_v0 == v);  //note! used to be m_v1
		else
			startInfo.m_toV1 = (edgeIter->m_v1 == v);
	}
	return (startInfo.m_startFace != NULL);
}


////////////////////////////////////////////////////////////////////////////////////////
// RemoveSmallStrips()
//
// allStrips is the whole strip vector...all small strips will be deleted from this list, to avoid leaking mem
// allBigStrips is an out parameter which will contain all strips above minStripLength
// faceList is an out parameter which will contain all faces which were removed from the striplist
//
void NvStripifier::RemoveSmallStrips(NvStripInfoVec& allStrips, NvStripInfoVec& allBigStrips, NvFaceInfoVec& faceList)
{
	faceList.clear();
	allBigStrips.clear();  //make sure these are empty
	NvFaceInfoVec tempFaceList;

	for(unsigned int i = 0; i < allStrips.size(); i++)
	{
		if(allStrips[i]->m_faces.size() < (size_t)minStripLength)
		{
			//strip is too small, add faces to faceList
			for(unsigned int j = 0; j < allStrips[i]->m_faces.size(); j++)
				tempFaceList.push_back(allStrips[i]->m_faces[j]);

			//and free memory
			delete allStrips[i];
		}
		else
		{
			allBigStrips.push_back(allStrips[i]);
		}
	}

#ifdef ORIG_CODE
	bool *bVisitedList = new bool[tempFaceList.size()];
	memset(bVisitedList, 0, tempFaceList.size()*sizeof(bool));
	VertexCache* vcache = new VertexCache(cacheSize);

	int bestNumHits = -1;
	int numHits;
	int bestIndex;

	while(1)
	{
		bestNumHits = -1;

		//find best face to add next, given the current cache
		for(int i = 0; i < tempFaceList.size(); i++)
		{
			if(bVisitedList[i])
				continue;

			numHits = CalcNumHitsFace(vcache, tempFaceList[i]);
			if(numHits > bestNumHits)
			{
				bestNumHits = numHits;
				bestIndex = i;
			}
		}

		if(bestNumHits == -1.0f)
			break;
		bVisitedList[bestIndex] = true;
		UpdateCacheFace(vcache, tempFaceList[bestIndex]);
		faceList.push_back(tempFaceList[bestIndex]);
	}

	delete vcache;
	delete[] bVisitedList;
#else
	VertexCache vcache(cacheSize);
  std::vector<int> unvisted(tempFaceList.size());
  int max_vert = -1;

  for (unsigned int i = 0; i < tempFaceList.size(); i++)
  {
    unvisted[i] = i;
    if (tempFaceList[i]->m_v0 > max_vert) max_vert = tempFaceList[i]->m_v0;
    if (tempFaceList[i]->m_v1 > max_vert) max_vert = tempFaceList[i]->m_v1;
    if (tempFaceList[i]->m_v2 > max_vert) max_vert = tempFaceList[i]->m_v2;
  }

  std::vector<int> vert_hist(max_vert + 1);

  for (int i = 0; i <= max_vert; i++)
    vert_hist[i] = 0;

  for (unsigned int i = 0; i < tempFaceList.size(); i++)
  {
    vert_hist[tempFaceList[i]->m_v0]++;
    vert_hist[tempFaceList[i]->m_v1]++;
    vert_hist[tempFaceList[i]->m_v2]++;
  }

  int unvisted_left = tempFaceList.size();

  // Greedy search.. definitely could be improved.
  while (unvisted_left)
  {
    struct local
    {
      static int popularity(
        const NvFaceInfo* face,
        const std::vector<int>& vert_hist)
      {
        return vert_hist[face->m_v0] +
               vert_hist[face->m_v1] +
               vert_hist[face->m_v2];
      }

      static int face_cost(
        const VertexCache& vcache,
        const NvFaceInfo* face,
        const std::vector<int>& vert_hist)
      {
        int cost = 0;

	      if (!vcache.InCache(face->m_v0)) cost++;
      	if (!vcache.InCache(face->m_v1)) cost++;
      	if (!vcache.InCache(face->m_v2)) cost++;

        if (cost > 0)
        {
          int removed_vert = vcache.At(vcache.Size() - 1);
          if ((removed_vert != -1) && (vert_hist.at(removed_vert))) cost++;

          if (cost > 1)
          {
            removed_vert = vcache.At(vcache.Size() - 2);
            if ((removed_vert != -1) && (vert_hist.at(removed_vert))) cost++;

            if (cost > 2)
            {
              removed_vert = vcache.At(vcache.Size() - 3);
              if ((removed_vert != -1) && (vert_hist.at(removed_vert))) cost++;
            }
          }
        }

        return cost;
      }
    };

    int best_cost = INT_MAX;
    int best_tri, best_index, best_pop;

    for (int i = 0; i < unvisted_left; i++)
    {
      int tri = unvisted[i];
      bool better = false;

			int cost = local::face_cost(vcache, tempFaceList[tri], vert_hist);
      if (cost < best_cost)
        better = true;
      else if (cost == best_cost)
      {
        //if (local::popularity(tempFaceList[tri], vert_hist) < best_pop)
        //  better = true;
      }

      if (better)
      {
        best_cost = cost;
        best_index = i;
        best_tri = tri;
        best_pop = local::popularity(tempFaceList[best_tri], vert_hist);
        if (!best_cost)
          break;
      }
    }

    assert(best_cost != INT_MAX);

		UpdateCacheFace(&vcache, tempFaceList[best_tri]);
		faceList.push_back(tempFaceList[best_tri]);

    vert_hist[tempFaceList[best_tri]->m_v0]--;
    vert_hist[tempFaceList[best_tri]->m_v1]--;
    vert_hist[tempFaceList[best_tri]->m_v2]--;

    unvisted[best_index] = unvisted[unvisted_left - 1];
    unvisted_left--;
  }

#ifdef _DEBUG
  for (int i = 0; i <= max_vert; i++)
    assert(vert_hist[i] == 0);
#endif

#endif

}


///////////////////////////////////////////////////////////////////////////////////////////
// Stripify()
//
//
// in_indices are the input indices of the mesh to stripify
// in_cacheSize is the target cache size
//
void NvStripifier::Stripify(const IndexVec &in_indices, const int in_cacheSize,
							const int in_minStripLength, NvStripInfoVec &outStrips, NvFaceInfoVec& outFaceList)
{
	meshJump = 0.0f;
	bFirstTimeResetPoint = true; //used in FindGoodResetPoint()

	//the number of times to run the experiments
	int numSamples = NUM_SAMPLES;  //<--------------
	cacheSize = in_cacheSize;
	minStripLength = in_minStripLength;  //this is the strip size threshold below which we dump the strip into a list

	indices = in_indices;

  max_indice = INT_MIN;
  for (unsigned int k = 0; k < indices.size(); k++)
  {
    if ((int)indices[k] > max_indice)
      max_indice = indices[k];
  }

	// build the stripification info
	NvFaceInfoVec allFaceInfos;
	NvEdgeInfoVec allEdgeInfos;

	BuildStripifyInfo(allFaceInfos, allEdgeInfos);

	NvStripInfoVec allStrips;

	// stripify
	FindAllStrips(allStrips, allFaceInfos, allEdgeInfos, numSamples);

	//split up the strips into cache friendly pieces, optimize them, then dump these into outStrips
	SplitUpStripsAndOptimize(allStrips, outStrips, allEdgeInfos, outFaceList);

	//clean up
	for(unsigned int i = 0; i < allStrips.size(); i++)
	{
		delete allStrips[i];
	}

	for (unsigned int i = 0; i < allEdgeInfos.size(); i++)
	{
		NvEdgeInfo *info = allEdgeInfos[i];
		while (info != NULL){
			NvEdgeInfo *next = (info->m_v0 == i ? info->m_nextV0 : info->m_nextV1);
			info->Unref();
			info = next;
		}
	}

}

// RG: maps vert indice into list of strips that use that vert
class vert_mapper
{
public:
  typedef std::list<int> StripList;
  typedef std::vector<StripList> StripListVec;
private:
  StripListVec strip_lists;      // one list of stripinfo's for each possible indice

public:
  vert_mapper(int num_verts) : strip_lists(num_verts)
  {
  }

  void add(int strip_index, int indice)
  {
    StripList& l = strip_lists.at(indice);

    for (StripList::const_iterator i = l.begin(); i != l.end(); i++)
    {
      if (*i == strip_index)
        return;
    }

    l.push_back(strip_index);
  }

  const StripList& list(int indice)
  {
    return strip_lists.at(indice);
  }
};

///////////////////////////////////////////////////////////////////////////////////////////
// SplitUpStripsAndOptimize()
//
// Splits the input vector of strips (allBigStrips) into smaller, cache friendly pieces, then
//  reorders these pieces to maximize cache hits
// The final strips are output through outStrips
//

void NvStripifier::SplitUpStripsAndOptimize(NvStripInfoVec &allStrips, NvStripInfoVec &outStrips,
                                            NvEdgeInfoVec& edgeInfos, NvFaceInfoVec& outFaceList)
{
	fprintf(stderr, "NvStripifier::SplitUpStripsAndOptimize\n");

  int threshold = cacheSize - 4; //= 999999;
	NvStripInfoVec tempStrips;

	//split up strips into threshold-sized pieces
	for(unsigned int i = 0; i < allStrips.size(); i++)
	{
		NvStripInfo* currentStrip;
		NvStripStartInfo startInfo(NULL, NULL, false);

		if(allStrips[i]->m_faces.size() > (size_t)threshold)
		{

			int numTimes    = allStrips[i]->m_faces.size() / threshold;
			int numLeftover = allStrips[i]->m_faces.size() % threshold;

			for(int j = 0; j < numTimes; j++)
			{
				currentStrip = new NvStripInfo(*this, startInfo, 0, -1);

				for(int faceCtr = j*threshold; faceCtr < threshold+(j*threshold); faceCtr++)
				{
					currentStrip->m_faces.push_back(allStrips[i]->m_faces[faceCtr]);
				}

				tempStrips.push_back(currentStrip);
			}

			int leftOff = j * threshold;

			if(numLeftover != 0)
			{
				currentStrip = new NvStripInfo(*this, startInfo, 0, -1);

				for(int k = 0; k < numLeftover; k++)
				{
					currentStrip->m_faces.push_back(allStrips[i]->m_faces[leftOff++]);
				}

				tempStrips.push_back(currentStrip);
			}
		}
		else
		{
			//we're not just doing a tempStrips.push_back(allBigStrips[i]) because
			// this way we can delete allBigStrips later to free the memory
			currentStrip = new NvStripInfo(*this, startInfo, 0, -1);

			for(unsigned int j = 0; j < allStrips[i]->m_faces.size(); j++)
				currentStrip->m_faces.push_back(allStrips[i]->m_faces[j]);

			tempStrips.push_back(currentStrip);
		}
	}

	//add small strips to face list
	NvStripInfoVec tempStrips2;
	RemoveSmallStrips(tempStrips, tempStrips2, outFaceList);

	outStrips.clear();
	if(tempStrips2.size() != 0)
	{
    vert_mapper vmapper(max_indice + 1);

		for(i = 0; i < tempStrips2.size(); i++)
    {
      NvStripInfo* strip = tempStrips2[i];

	    for(unsigned int j = 0; j < strip->m_faces.size(); j++)
      {
		    vmapper.add(i, strip->m_faces[j]->m_v0);
		    vmapper.add(i, strip->m_faces[j]->m_v1);
		    vmapper.add(i, strip->m_faces[j]->m_v2);
      }
    }

		//Optimize for the vertex cache
		VertexCache* vcache = new VertexCache(cacheSize);

		float bestNumHits = -1.0f;
		float numHits;
		int bestIndex;
		bool done = false;

		int firstIndex = 0;
		float minCost = 10000.0f;

		for(i = 0; i < tempStrips2.size(); i++)
		{
			int numNeighbors = 0;

			//find strip with least number of neighbors per face
			for(unsigned int j = 0; j < tempStrips2[i]->m_faces.size(); j++)
			{
				numNeighbors += NumNeighbors(tempStrips2[i]->m_faces[j], edgeInfos);
			}

			float currCost = (float)numNeighbors / (float)tempStrips2[i]->m_faces.size();
			if(currCost < minCost)
			{
				minCost = currCost;
				firstIndex = i;
			}
		}

		UpdateCacheStrip(vcache, tempStrips2[firstIndex]);
		outStrips.push_back(tempStrips2[firstIndex]);

		tempStrips2[firstIndex]->visited = true;


		//this n^2 algo is what slows down stripification so much....
		// needs to be improved
    // RG: Search optimized, not completely but MUCH better than orig code
    int counter = 0;
		while(1)
		{
      if ((counter&1023)==0) { fprintf(stderr, "."); fflush(stderr); }
      counter++;

			bestIndex = -1;
      bestNumHits = -1.0f;

#ifdef ORIG_CODE
			//find best strip to add next, given the current cache
			for(int i = 0; i < tempStrips2.size(); i++)
			{
				if(tempStrips2[i]->visited)
					continue;

				numHits = CalcNumHitsStrip(vcache, tempStrips2[i]);
				if(numHits > bestNumHits)
				{
					bestNumHits = numHits;
					bestIndex = i;
				}
			}

//      int orig_bestIndex = bestIndex;
//      bestIndex = -1;
//      bestNumHits = -1.0f;

#else
      vert_mapper::StripList candidate_list;
      for (int i = 0; i < vcache->Size(); i++)
      {
        int v = vcache->At(i);
        if (v != -1)
        {
          const vert_mapper::StripList& l = vmapper.list(v);
          candidate_list.insert(candidate_list.begin(), l.begin(), l.end());
        }
      }

      candidate_list.sort();
      //vert_mapper::StripList::iterator end = candidate_list.unique();
      candidate_list.unique();
      vert_mapper::StripList::const_iterator p;

			//find best strip to add next, given the current cache
			for(p = candidate_list.begin(); p != candidate_list.end(); p++)
			{
				if(tempStrips2[*p]->visited)
					continue;

				numHits = CalcNumHitsStrip(vcache, tempStrips2[*p]);
				if(numHits > bestNumHits)
				{
					bestNumHits = numHits;
				  bestIndex = *p;
				}
			}

      if (bestIndex == -1)
      {
        for(unsigned int i = 0; i < tempStrips2.size(); i++)
			  {
				  if(tempStrips2[i]->visited) continue;
				  bestNumHits = CalcNumHitsStrip(vcache, tempStrips2[i]);
          bestIndex = i;
          break;
			  }
      }
#endif

//      if(bestIndex != orig_bestIndex) _asm int 3;

			if(bestNumHits == -1.0f)
				break;

			tempStrips2[bestIndex]->visited = true;
			UpdateCacheStrip(vcache, tempStrips2[bestIndex]);
			outStrips.push_back(tempStrips2[bestIndex]);
		}

    fprintf(stderr, "\n");

		delete vcache;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////
// UpdateCacheStrip()
//
// Updates the input vertex cache with this strip's vertices
//
void NvStripifier::UpdateCacheStrip(VertexCache* vcache, NvStripInfo* strip)
{
	for(unsigned int i = 0; i < strip->m_faces.size(); i++)
	{
		if(!vcache->InCache(strip->m_faces[i]->m_v0))
			vcache->AddEntry(strip->m_faces[i]->m_v0);

		if(!vcache->InCache(strip->m_faces[i]->m_v1))
			vcache->AddEntry(strip->m_faces[i]->m_v1);

		if(!vcache->InCache(strip->m_faces[i]->m_v2))
			vcache->AddEntry(strip->m_faces[i]->m_v2);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// UpdateCacheFace()
//
// Updates the input vertex cache with this face's vertices
//
void NvStripifier::UpdateCacheFace(VertexCache* vcache, NvFaceInfo* face)
{
	if(!vcache->InCache(face->m_v0))
		vcache->AddEntry(face->m_v0);

	if(!vcache->InCache(face->m_v1))
		vcache->AddEntry(face->m_v1);

	if(!vcache->InCache(face->m_v2))
		vcache->AddEntry(face->m_v2);
}


///////////////////////////////////////////////////////////////////////////////////////////
// CalcNumHitsStrip()
//
// returns the number of cache hits per face in the strip
// RG: When this method is called the strips have already been segmented
// into smaller pieces no larger than the vcache.
//
float NvStripifier::CalcNumHitsStrip(VertexCache* vcache, NvStripInfo* strip)
{
//#ifdef ORIG_CODE
#if (1)
// This method does not even try to take into account pushed out verts!?

	int numHits = 0;
	int numFaces = 0;

	for(unsigned int i = 0; i < strip->m_faces.size(); i++)
	{
		if(vcache->InCache(strip->m_faces[i]->m_v0))
			numHits++;

		if(vcache->InCache(strip->m_faces[i]->m_v1))
			numHits++;

		if(vcache->InCache(strip->m_faces[i]->m_v2))
			numHits++;

		numFaces++;
	}

	return ((float)numHits / (float)numFaces);
#else
  VertexCache sim_cache(*vcache);
	int numHits = 0;

	for(int i = 0; i < strip->m_faces.size(); i++)
  {
#if (0)
    if(sim_cache.InCache(strip->m_faces[i]->m_v0)) numHits++;
		if(sim_cache.InCache(strip->m_faces[i]->m_v1)) numHits++;
		if(sim_cache.InCache(strip->m_faces[i]->m_v2)) numHits++;
    if(!sim_cache.InCache(strip->m_faces[i]->m_v0)) sim_cache.AddEntry(strip->m_faces[i]->m_v0);
		if(!sim_cache.InCache(strip->m_faces[i]->m_v1)) sim_cache.AddEntry(strip->m_faces[i]->m_v1);
		if(!sim_cache.InCache(strip->m_faces[i]->m_v2)) sim_cache.AddEntry(strip->m_faces[i]->m_v2);
#else
    if(sim_cache.InCache(strip->m_faces[i]->m_v0))
			numHits++;
    else
      sim_cache.AddEntry(strip->m_faces[i]->m_v0);

		if(sim_cache.InCache(strip->m_faces[i]->m_v1))
			numHits++;
    else
      sim_cache.AddEntry(strip->m_faces[i]->m_v1);

		if(sim_cache.InCache(strip->m_faces[i]->m_v2))
			numHits++;
    else
      sim_cache.AddEntry(strip->m_faces[i]->m_v2);
#endif
  }

	return (float)numHits / (float)strip->m_faces.size();
#endif
}


///////////////////////////////////////////////////////////////////////////////////////////
// CalcNumHitsFace()
//
// returns the number of cache hits in the face
//
int NvStripifier::CalcNumHitsFace(VertexCache* vcache, NvFaceInfo* face)
{
	int numHits = 0;

	if(vcache->InCache(face->m_v0))
		numHits++;

	if(vcache->InCache(face->m_v1))
		numHits++;

	if(vcache->InCache(face->m_v2))
		numHits++;

	return numHits;
}


///////////////////////////////////////////////////////////////////////////////////////////
// NumNeighbors()
//
// Returns the number of neighbors that this face has
//
int NvStripifier::NumNeighbors(NvFaceInfo* face, NvEdgeInfoVec& edgeInfoVec)
{
	int numNeighbors = 0;

	if(FindOtherFace(edgeInfoVec, face->m_v0, face->m_v1, face) != NULL)
	{
		numNeighbors++;
	}

	if(FindOtherFace(edgeInfoVec, face->m_v1, face->m_v2, face) != NULL)
	{
		numNeighbors++;
	}

	if(FindOtherFace(edgeInfoVec, face->m_v2, face->m_v0, face) != NULL)
	{
		numNeighbors++;
	}

	return numNeighbors;
}


///////////////////////////////////////////////////////////////////////////////////////////
// AvgStripSize()
//
// Finds the average strip size of the input vector of strips
//
float NvStripifier::AvgStripSize(const NvStripInfoVec &strips){
	int sizeAccum = 0;
	int numStrips = strips.size();
	for (int i = 0; i < numStrips; i++){
		NvStripInfo *strip = strips[i];
		sizeAccum += strip->m_faces.size();
	}
	return ((float)sizeAccum) / ((float)numStrips);
}


///////////////////////////////////////////////////////////////////////////////////////////
// FindAllStrips()
//
// Does the stripification, puts output strips into vector allStrips
//
// Works by setting runnning a number of experiments in different areas of the mesh, and
//  accepting the one which results in the longest strips.  It then accepts this, and moves
//  on to a different area of the mesh.  We try to jump around the mesh some, to ensure that
//  large open spans of strips get generated.
//
void NvStripifier::FindAllStrips(NvStripInfoVec &allStrips,
								 NvFaceInfoVec &allFaceInfos,
								 NvEdgeInfoVec &allEdgeInfos,
								 int numSamples){
	// the experiments
	int experimentId = 0;
	int stripId      = 0;
	bool done        = false;

	int loopCtr = 0;

	fprintf(stderr, "NvStripifier::FindAllStrips\n");

  while (!done)
	{
    fprintf(stderr, ".");
    fflush(stderr);

		loopCtr++;

		//
		// PHASE 1: Set up numSamples * numEdges experiments
		//
		NvStripInfoVec *experiments = new NvStripInfoVec [numSamples * 6];
		int experimentIndex = 0;
		std::set   <NvFaceInfo*>  resetPoints;
		for (int i = 0; i < numSamples; i++)
		{

			// Try to find another good reset point.
			// If there are none to be found, we are done
			NvFaceInfo *nextFace = FindGoodResetPoint(allFaceInfos, allEdgeInfos);
			if (nextFace == NULL){
				done = true;
				break;
			}

			// If we have already evaluated starting at this face in this slew
			// of experiments, then skip going any further
			else if (resetPoints.find(nextFace) != resetPoints.end()){
				continue;
			}

			// trying it now...
			resetPoints.insert(nextFace);

			// otherwise, we shall now try experiments for starting on the 01,12, and 20 edges
			assert(nextFace->m_stripId < 0);

			// build the strip off of this face's 0-1 edge
			NvEdgeInfo *edge01 = FindEdgeInfo(allEdgeInfos, nextFace->m_v0, nextFace->m_v1);
			NvStripInfo *strip01 = new NvStripInfo(*this, NvStripStartInfo(nextFace, edge01, true), stripId++, experimentId++);
			experiments[experimentIndex++].push_back(strip01);

			// build the strip off of this face's 1-0 edge
			NvEdgeInfo *edge10 = FindEdgeInfo(allEdgeInfos, nextFace->m_v0, nextFace->m_v1);
			NvStripInfo *strip10 = new NvStripInfo(*this, NvStripStartInfo(nextFace, edge10, false), stripId++, experimentId++);
			experiments[experimentIndex++].push_back(strip10);

			// build the strip off of this face's 1-2 edge
			NvEdgeInfo *edge12 = FindEdgeInfo(allEdgeInfos, nextFace->m_v1, nextFace->m_v2);
			NvStripInfo *strip12 = new NvStripInfo(*this, NvStripStartInfo(nextFace, edge12, true), stripId++, experimentId++);
			experiments[experimentIndex++].push_back(strip12);

			// build the strip off of this face's 2-1 edge
			NvEdgeInfo *edge21 = FindEdgeInfo(allEdgeInfos, nextFace->m_v1, nextFace->m_v2);
			NvStripInfo *strip21 = new NvStripInfo(*this, NvStripStartInfo(nextFace, edge21, false), stripId++, experimentId++);
			experiments[experimentIndex++].push_back(strip21);

			// build the strip off of this face's 2-0 edge
			NvEdgeInfo *edge20 = FindEdgeInfo(allEdgeInfos, nextFace->m_v2, nextFace->m_v0);
			NvStripInfo *strip20 = new NvStripInfo(*this, NvStripStartInfo(nextFace, edge20, true), stripId++, experimentId++);
			experiments[experimentIndex++].push_back(strip20);

			// build the strip off of this face's 0-2 edge
			NvEdgeInfo *edge02 = FindEdgeInfo(allEdgeInfos, nextFace->m_v2, nextFace->m_v0);
			NvStripInfo *strip02 = new NvStripInfo(*this, NvStripStartInfo(nextFace, edge02, false), stripId++, experimentId++);
			experiments[experimentIndex++].push_back(strip02);
		}

		//
		// PHASE 2: Iterate through that we setup in the last phase
		// and really build each of the strips and strips that follow to see how
		// far we get
		//
		int numExperiments = experimentIndex;
		for (i = 0; i < numExperiments; i++){

			// get the strip set

			// build the first strip of the list
			experiments[i][0]->Build(allEdgeInfos, allFaceInfos);
			int experimentId = experiments[i][0]->m_experimentId;

			NvStripInfo *stripIter = experiments[i][0];
			NvStripStartInfo startInfo(NULL, NULL, false);
			while (FindTraversal(allFaceInfos, allEdgeInfos, stripIter, startInfo)){

				// create the new strip info
				stripIter = new NvStripInfo(*this, startInfo, stripId++, experimentId);

				// build the next strip
				stripIter->Build(allEdgeInfos, allFaceInfos);

				// add it to the list
				experiments[i].push_back(stripIter);
			}
		}

		//
		// Phase 3: Find the experiment that has the most promise
		//
		int bestIndex = 0;
		double bestValue = 0;
		for (i = 0; i < numExperiments; i++)
		{
			const float avgStripSizeWeight = 1.0f;
			const float numTrisWeight      = 1.0f;
			float avgStripSize = AvgStripSize(experiments[i]);
			float numStrips    = (float) experiments[i].size();
			float value        = avgStripSize * avgStripSizeWeight + (avgStripSize * numStrips * numTrisWeight);

			if (value > bestValue)
			{
				bestValue = value;
				bestIndex = i;
			}
		}

		//
		// Phase 4: commit the best experiment of the bunch
		//
		CommitStrips(allStrips, experiments[bestIndex]);

		// and destroy all of the others
		for (i = 0; i < numExperiments; i++)
		{
			if (i != bestIndex)
			{
				int numStrips = experiments[i].size();
				for (int j = 0; j < numStrips; j++)
				{
					delete experiments[i][j];
				}
			}
		}

		// delete the array that we used for all experiments
		delete [] experiments;
  }
  fprintf(stderr, "\n");
}


///////////////////////////////////////////////////////////////////////////////////////////
// CountRemainingTris()
//
// This will count the number of triangles left in the
// strip list starting at iter and finishing up at end
//
int NvStripifier::CountRemainingTris(std::list<NvStripInfo*>::iterator iter,
											std::list<NvStripInfo*>::iterator  end){
	int count = 0;
	while (iter != end){
		count += (*iter)->m_faces.size();
		iter++;
	}
	return count;
}

///////////////////////////////////////////////////////////////////////////////////////////
// next_is_cw()
//
// Returns true if the next face should be ordered in CW fashion
//
bool NvStripifier::NextIsCw(const int numIndices)
{
	return ((numIndices % 2) == 0);
}


///////////////////////////////////////////////////////////////////////////////////////////
// is_cw()
//
// Returns true if the face is ordered in CW fashion
//
bool NvStripifier::IsCw(NvFaceInfo *faceInfo, int v0, int v1)
{
	if (faceInfo->m_v0 == v0)
		return (faceInfo->m_v1 == v1);

	else if (faceInfo->m_v1 == v0)
		return (faceInfo->m_v2 == v1);

	else
		return (faceInfo->m_v0 == v1);

	// shouldn't get here
	assert(0);
	return false;
}

//used in create_strips
template<class T>
inline void SWAP(T& first, T& second)
{
	T temp = first;
	first = second;
	second = temp;
}

///////////////////////////////////////////////////////////////////////////////////////////
// create_strips()
//
// Up until now, the strips had been strips at heart, but tri lists in reality.
// Now, remove redundant indices, and stitch together strips to form one, huge uber-strip
//  using degenerate tris.
//
void NvStripifier::CreateStrips(
  const NvStripInfoVec& strips,
  IndexVec& stripIndices)
{
	NvFaceInfo tLastFace(0, 0, 0);
	int nStripCount = strips.size();

	for (int i = 0; i < nStripCount; i++)
	{
		NvStripInfo *strip = strips[i];
		int nStripFaceCount = strip->m_faces.size();
		assert(nStripFaceCount > 0);

		// Handle the first face in the strip
		{
			NvFaceInfo tFirstFace(strip->m_faces[0]->m_v0, strip->m_faces[0]->m_v1, strip->m_faces[0]->m_v2);

			// If there is a second face, reorder vertices such that the
			// unique vertex is first
			if (nStripFaceCount > 1)
			{
				int nUnique = NvStripifier::GetUniqueVertexInB(strip->m_faces[1], &tFirstFace);
				if (nUnique == tFirstFace.m_v1)
				{
					SWAP(tFirstFace.m_v0, tFirstFace.m_v1);
				}
				else if (nUnique == tFirstFace.m_v2)
				{
					SWAP(tFirstFace.m_v0, tFirstFace.m_v2);
				}

				// If there is a third face, reorder vertices such that the
				// shared vertex is last
				if (nStripFaceCount > 2)
				{
					int nShared = NvStripifier::GetSharedVertex(strip->m_faces[2], &tFirstFace);
					if (nShared == tFirstFace.m_v1)
					{
						SWAP(tFirstFace.m_v1, tFirstFace.m_v2);
					}
				}
      }

			if (i != 0)
			{
				// Double tap the first in the new strip
				stripIndices.push_back(tFirstFace.m_v0);

				// Check CW/CCW ordering
				if (NextIsCw(stripIndices.size()) != IsCw(strip->m_faces[0], tFirstFace.m_v0, tFirstFace.m_v1))
				{
					stripIndices.push_back(tFirstFace.m_v0);
				}
			}
			else
			{
				if(!IsCw(strip->m_faces[0], tFirstFace.m_v0, tFirstFace.m_v1))
					stripIndices.push_back(tFirstFace.m_v0);
			}

			stripIndices.push_back(tFirstFace.m_v0);
			stripIndices.push_back(tFirstFace.m_v1);
			stripIndices.push_back(tFirstFace.m_v2);

			// Update last face info
			tLastFace = tFirstFace;
		}

		for (int j = 1; j < nStripFaceCount; j++)
		{
			int nUnique = NvStripifier::GetUniqueVertexInB(&tLastFace, strip->m_faces[j]);
			if (nUnique != -1)
			{
				stripIndices.push_back(nUnique);

				// Update last face info
				tLastFace.m_v0 = tLastFace.m_v1;
				tLastFace.m_v1 = tLastFace.m_v2;
				tLastFace.m_v2 = nUnique;
			}
		}

		// Double tap between strips.
		if (i != nStripCount - 1)
      stripIndices.push_back(tLastFace.m_v2);

		// Update last face info
		tLastFace.m_v0 = tLastFace.m_v1;
		tLastFace.m_v1 = tLastFace.m_v2;
		tLastFace.m_v2 = tLastFace.m_v2;
	}
}

