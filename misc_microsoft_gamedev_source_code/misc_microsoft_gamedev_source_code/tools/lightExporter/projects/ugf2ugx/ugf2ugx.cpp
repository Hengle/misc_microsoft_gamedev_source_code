//-----------------------------------------------------------------------------
// File: ugf2ugx.cpp
// Rich Geldreich
// June 12, 2003 RG Initial coding
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include <d3dx9effect.h>

#include "common/geom/ugx_instancer.h"

using namespace gr;

int main(int argc, char* argv[])
{
	if (3 != argc)
	{
		printf("Usage: %s [InputFile] [OutputFile]\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	const char* pInputFile = argv[1];
	const char* pOutputFile = argv[2];
		
	LogFile log(true);
		
	FILEStream inStream(pInputFile);
	FILEStream outStream(pOutputFile, false);
		
	if (inStream)
	{
		printf("Unable to read from file: \"%s\"!\n", pInputFile);
		return EXIT_FAILURE;
	}
	
	if (outStream)
	{
		printf("Unable to write to file: \"%s\"!\n", pOutputFile);
		return EXIT_FAILURE;
	}
	
	Unigeom::Geom geom;
	
	printf("Reading file: \"%s\".\n", pInputFile);
	
	Verify(!geom.read(inStream));
	
	UGXInstancer instancer(geom, log);
	
	instancer.geom().log(log);
			
	printf("Writing file: \"%s\".\n", pOutputFile);
	
	outStream << instancer.geom();
	
	printf("Output file size: %i\n", outStream.size());

	return EXIT_SUCCESS;
}
