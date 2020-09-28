//============================================================================
//
//  ScanPolygon.h
//
//  Copyright (c) 1999-2001, Ensemble Studios
//
//============================================================================


#ifndef __SCAN_POLYGON_H__
#define __SCAN_POLYGON_H__


//----------------------------------------------------------------------------
//  Externs
//----------------------------------------------------------------------------
class BTileCoordinates;
struct ClipRect;

//----------------------------------------------------------------------------
//  Callback Prototypes
//----------------------------------------------------------------------------
typedef void (CALLBACK POLYGON_FUNC)(long x, long z, void* pParam);


//----------------------------------------------------------------------------
//  Clipping Functions
//----------------------------------------------------------------------------
BTileCoordinates *clipPolygon(const BTileCoordinates *pPoints, long inSize, long &outSize, long xmin, long ymin, long xmax, long ymax);


//----------------------------------------------------------------------------
//  Polygon Functions
//----------------------------------------------------------------------------
void scanPolygon(BTileCoordinates *pPoints, long numPoints, const ClipRect *pClip, POLYGON_FUNC *pFunc, void *pParam);


#endif


