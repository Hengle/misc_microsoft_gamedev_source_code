//=============================================================================
// Copyright (c) 1997-2002 Ensemble Studios
//
// Targa file reader
//=============================================================================

#pragma once

#ifndef _READTGA_H_
#define _READTGA_H_


//=============================================================================
class BImage;

//=============================================================================
BImage   *readTGA(long dirID, const BCHAR_T *filename);

bool     getTGADimensions(long dirID, const BCHAR_T *filename, long &width, long &height);
bool     readTGA(long dirID, const BCHAR_T *filename, DWORD *data);
bool     readTGABlock(long dirID, const BCHAR_T *filename, DWORD *data, long xOffset, long yOffset, long xSize, long ySize);

#endif