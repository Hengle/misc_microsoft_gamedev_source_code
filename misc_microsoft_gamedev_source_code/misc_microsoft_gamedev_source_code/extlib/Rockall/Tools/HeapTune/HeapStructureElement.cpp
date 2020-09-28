#include "StdAfx.h"
#include "HeapStructureElement.h"

CHeapStructureElement::CHeapStructureElement(void)
{
	Set(0,0); // can omit
}
CHeapStructureElement::CHeapStructureElement(int nLogDelta, int nStart)
{
	Set(nLogDelta, nStart);
}
bool CHeapStructureElement::Set(int nLogDelta, int nStart)
{
	m_nLogDelta=nLogDelta;
	m_nStart=nStart;
	return true; // todo
}
bool CHeapStructureElement::SetLinear(int nDelta, int nStart)
{
	m_nDelta=nDelta;
	m_nStart=nStart;
	return true; // todo
}
CHeapStructureElement::~CHeapStructureElement(void)
{
}
