//============================================================================
//
//  LoadXBOXTerrain.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "TerrainPCH.h"

#include "LoadXBOXTerrain.h"

#include "file.h"

// FIXME - This makes xterrain dependant on xgame!
// xgame
#include "..\xgame\gameDirectories.h"

// xrender
#include "renderThread.h"

// terrain
#include "terrain.h"
#include "TerrainIO.h"
#include "TerrainMetric.h"
#include "TerrainSimRep.h"
#include "TerrainTexturing.h"

#include "file.h"

BTerrainIOLoader gLoader;

// rg [2/1/06] - This needs to be changed to load in the background in some fashion soon, but I want to wait 
// until the async file manager is fleshed out more first. Also, the XTD file should be Inflated on the fly (this one change alone 
// would make a big difference.)
// cm [2/15/06] Virtual alloc portion finished.
bool loadXBOXTerrain(long dirID, const char *filename, long terrainDirID)
{
}

