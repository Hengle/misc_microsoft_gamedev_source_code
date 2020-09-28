/**********************************************************************

Filename    :   Main.cpp
Content     :   SWF to GFX resource extraction and conversion tool
Created     :   October, 2006
Authors     :   Artyom Bolgar
Copyright   :   (c) 2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#include "GFxExport.h"

GFxImageExporterFactoryBase* GFxImageExporterFactoriesArray[GFX_MAX_NUM_OF_IMAGE_EXPORTERS];
unsigned                     GFxImageExporterFactoriesArrayCount = 0; 

int main(int argc, const char** const argv)
{
    if (argc > 0)
    {
        GFxDataExporter exporter;
        for (UInt i = 0; i < GFxImageExporterFactoriesArrayCount; ++i)
        {
            // fill 'exporter' by all registered image exporters.
            GFxImageExporter* pexp = GFxImageExporterFactoriesArray[i]->Create();
            exporter.AddImageExporter(pexp->GetFormatId(), pexp);
        }
        if (argc == 1)
        {
            exporter.ShowHelpScreen();
            return -1;
        }
        exporter.ParseCommandLine(argc, argv);
        exporter.Process();
    }
    GMemory::DetectMemoryLeaks();
    return 0;
}
