/**********************************************************************

Filename    :   GMain.cpp
Content     :   Windows main function wrapper
Created     :   July 12, 2003
Authors     :   Brendan Iribe

Copyright   :   (c) 2003 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#if defined(GFC_BUILD_MFC)
#include <afx.h>
#else
#include <windows.h>
#endif

#include <ctype.h>

// Externally defined standard main function
extern "C" int  main(int argc, char **argv);


// *** Windows GUI-application main loop 

#define GFC_WINMAIN_ARG_COUNT           256
#define GFC_WINMAIN_FILENAME_LENGTH     2048

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hprevInst, LPSTR pcmdLine, int cmdShow)
{
    // Unused variables
    (hprevInst); (cmdShow);

    // Allocate and initialize the command line argument list
    int     argc    = 1;
    // Stack based argument list
    int     argsz   = GFC_WINMAIN_ARG_COUNT;
    char*   sargv[GFC_WINMAIN_ARG_COUNT];
    // Dynamic memory based argument list
    char**  dargv   = 0;
    // Pointer the argument list
    // - initially points to the list on stack 
    // - if argument count overflows, then the list is allocated dynamically
    char**  argv    = sargv;

    // Store the filename in the first position of the argument list
    int     fileNameSize = GFC_WINMAIN_FILENAME_LENGTH;
    char    sfileName[GFC_WINMAIN_FILENAME_LENGTH];
    char*   dfileName   = 0;
    char*   pfileName   = sfileName;
    pfileName[0] = 0;
    if ((fileNameSize = ::GetModuleFileName(hinst, pfileName, fileNameSize)) > GFC_WINMAIN_FILENAME_LENGTH)
    {
        dfileName = (char*)malloc(fileNameSize * sizeof(char));
        ::GetModuleFileName(hinst, dfileName, fileNameSize);
        pfileName = dfileName;
    }
    argv[0] = pfileName;

    // Parse the command line text and 
    if (pcmdLine && pcmdLine[0])
    {   
        char*   pch = pcmdLine;

        while (*pch)
        {
            char*   pstartCh = pch;

            if (*pch == '\"')
            {
                pch++;
                pstartCh = pch;
                // Find the closing quotation mark
                pch++;
                while (*pch && (*pch != '\"'))
                    pch++;
            }
            else
            {
                pstartCh = pch;
                pch++;
                // Process all the characters until whitespace is encountered
                while (*pch && !iswspace(*pch))
                    pch++;
            }

            // Add new argument entry to list
            argc++;
            if (argc >= argsz)
            {
                argsz   += GFC_WINMAIN_ARG_COUNT;
                dargv   = (char**)realloc(dargv, argsz * sizeof(char*));
                argv    = dargv;
            }
            argv[argc-1] = pstartCh;

            // If end of command line then we are done processing
            if (!*pch)
                goto done_building_argv;

            // Store zero into ending value to terminate the string
            *pch = 0;
            pch++;

            // Skip the white space
            while (*pch && iswspace(*pch))
                pch++;
        }

    }
done_building_argv:


    // *** Call the programs main function
    int retVal = main(argc, (char**)argv);


    // Free the filename if it required
    // a string to be dynamically allocated
    if (dfileName)
        free(dfileName);
    // Free the dynamic memory based command 
    // line argument list if it was allocated
    if (dargv)
        free(dargv);

    return retVal;
}
