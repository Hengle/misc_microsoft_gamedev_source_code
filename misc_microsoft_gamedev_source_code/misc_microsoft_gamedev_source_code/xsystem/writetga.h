//==============================================================================
// writetage.h
//
// Copyright (c) 1997-2001 Ensemble Studios
//==============================================================================

#ifndef _WRITETGA_H_
#define _WRITETGA_H_

class BImage;

bool writeTGA(long dirID, const BCHAR_T *filename, BImage *image);
bool writeTGA(const BCHAR_T* path, BImage *image);
bool writeTGA(BFile& file, BImage* image);


#endif