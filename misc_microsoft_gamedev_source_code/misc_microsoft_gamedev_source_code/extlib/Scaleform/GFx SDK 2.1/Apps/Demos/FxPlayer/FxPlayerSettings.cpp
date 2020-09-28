/**********************************************************************

Filename    :   FxPlayerSettings.cpp
Content     :   Command line parsing logic implementation for FxPlayer.
Created     :   January 15, 2008
Authors     :   Michael Antonov, Maxim Didenko

Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "FxPlayerSettings.h"


// ***** FxPlayerSettings Implementation

#ifdef FXPLAYER_RENDER_DIRECT3D
    #if (GFC_D3D_VERSION == 8)
        #include "GRendererD3D8.h"
        #define D3DPLAYER GRendererD3D8
    #else
        #include "GRendererD3D9.h"
        #define D3DPLAYER GRendererD3D9
    #endif
#endif

// Initializes settings based on the command line.
// Returns 1 if parse was successful, otherwise 0 if usage should be displayed.
bool    FxPlayerSettings::ParseCommandLine(int argc, char *argv[])
{
    if (argc <= 1)
        return 1;

    for (int arg = 1; arg < argc; arg++)
    {
        if ( (argv[arg][0] == '-')
#ifdef GFC_OS_WIN32
            || (argv[arg][0] == '/')
#endif            
            )
        {
#if defined(GFC_OS_MAC)
            if (strncmp(argv[arg], "-psn", 4))
                continue;
#endif

            // Looks like an option.

            if (argv[arg][1] == '?')
            {
                // Help.
                PrintUsage();
                return 0;
            }

            else if (argv[arg][1] == 's')
            {
                if (argv[arg][2] == 'y') //VSync
                {
                    VSync = 1;
                }
                else // Scale.
                {
                    arg++;
                    if (arg < argc)
                        ScaleX = ScaleY = GTL::gclamp<float>((float) atof(argv[arg]), 0.01f, 100.f);
                    else
                    {
                        fprintf(stderr, "-s option requires a scale value.\n");
                        return 0;
                    }
                }
            }

            else if ((argv[arg][1] == 'n') && (argv[arg][2] == 'a'))
            {
                // Disable antialiasing - '-na'
                AAMode = AAMode_None;
            }
            else if ((argv[arg][1] == 'f') && (argv[arg][2] == 's') && (argv[arg][3] == 'a'))
            {
                // Force HW FASS - '-fsa'
                AAMode = AAMode_FSAA;
            }

            else if ((argv[arg][1] == 'f') && (argv[arg][2] == 'c'))
            {
                // Font Config file use - '-fc filename'
                arg++;
                // Load the font config file
                ConfigParser parser(argv[arg]);
                if (!parser.IsValid())
                {
                    fprintf(stderr, "-fc - cannot find specified font config.\n" );
                    return 0;
                }
                LoadFontConfigs(&parser);
                NoFontConfigLoad = 1; // disable autoloading
            }

            else if (argv[arg][1] == 'n' && argv[arg][2] == 'f' && argv[arg][3] == 'c')
            {
                // Disables the player from looking for the fontconfig.txt file in the movie's dir - '/nfc'
                NoFontConfigLoad = 1;
            }

            else if (argv[arg][1] == 'b')
            {
                // Set default bit depth.
                arg++;
                if (arg < argc)
                {
                    BitDepth = atoi(argv[arg]);
                    if (BitDepth != 16 && BitDepth != 32)
                    {
                        fprintf(stderr, "-b - specified bit depth must be 16 or 32.\n");
                        return 0;
                    }
                }
                else
                {
                    fprintf(stderr, "-b requires bit depth, ex: \"-b 16\"  or \"-b 32\".\n");
                    return 0;
                }
            }

            else if (argv[arg][1] == 'p')
            {
                // Enable frame-rate/performance logging.
                MeasurePerformance = 1;
            }


            else if (argv[arg][1] == 'i')
            {
                // Display Info hud screen at at startup.
                HudState = Hud_Stats;
            }

            else if (argv[arg][1] == 'f')
            {
                // Enable fast-forward playback
                if (argv[arg][2] == 'f')
                {
                    FastForward = 1;
                }
                else if (argv[arg][2] == 0)
                {
                    FullScreen = 1;
                }
                else if (argv[arg][2] == 'l')
                {
                    // Install fontlib file: -fl filename.swf
                    arg++;
                    if (arg < argc)
                    {
                        FontLibFile = argv[arg];
                    }
                    else
                    {
                        fprintf(stderr, "-fl option requires a font source SWF/GFX file name.\n");
                        return 0;
                    }
                }
            }

            else if ((argv[arg][1] == 'n') && (argv[arg][2] == 's') && (argv[arg][3] == 'f'))
            {
                // No System Font: -nsf.
                NoSystemFont = 1;
            }

            else if ((argv[arg][1] == 'n') && (argv[arg][2] == 'c') && (argv[arg][3] == 'k'))
            {
                // No Control Keys: -nck.
                NoControlKeys = 1;
            }
            else if ((argv[arg][1] == 'n') && (argv[arg][2] == 'h'))
            {
                NoHud = 1;
            }

            else if (argv[arg][1] == '1')
            {
                // Play once; don't loop.
                DoLoop = 0;
            }

            else if (argv[arg][1] == 'r')
            {
                // Set rendering on/off.
                arg++;
                if (arg < argc)
                {
                    const int render_arg = atoi(argv[arg]);
                    switch (render_arg)
                    {
                    case 0:
                        // Disable both
                        DoRender    = 0;
                        DoSound     = 0;
                        break;
                    case 1:
                        // Enable both
                        DoRender    = 1;
                        DoSound     = 1;
                        break;
                    case 2:
                        // Disable just sound
                        DoRender    = 1;
                        DoSound     = 0;
                        break;

                    default:
                        fprintf(stderr, "-r requires a value of 0, 1 or 2 (%d is invalid).\n",
                            render_arg);
                        return 0;
                    }
                }
                else
                {
                    fprintf(stderr, "-r requires a value of 0/1 to disable/enable rendering.\n");
                    return 0;
                }
            }

            else if (argv[arg][1] == 't')
            {
                // Set timeout.
                arg++;
                if (arg < argc)
                    ExitTimeout = (float) atof(argv[arg]);
                else
                {
                    fprintf(stderr, "-t requires a timeout value, in seconds.\n");
                    return 0;
                }
            }

            else if (argv[arg][1] == 'v')
            {
                if (Quiet)
                {
                    fprintf(stderr, "Quiet option -q conflicts with %s verbose option.\n", argv[arg]);
                    return 0;
                }

                // Be verbose; i.e. print log messages to stdout.
                if (argv[arg][2] == 'a')
                {
                    // Enable spew re: action.
                    VerboseAction = 1;
                }
                else if (argv[arg][2] == 'f')
                {
                    if (argv[arg][3] == 'q')
                        LogChildFilenames = 0;
                    else if (argv[arg][3] == 'p')
                        LogLongFilename = 1;
                    else if (argv[arg][3] == 'r')
                        LogRootFilenames = 1;                   
                }
                else if ((argv[arg][2] == 'p') && (argv[arg][3] == 0))
                {
                    // Enable parse spew.
                    VerboseParse = 1;
                }
                else if ((argv[arg][2] == 'p') && (argv[arg][3] == 's'))
                {
                    // Enable parse shape.
                    VerboseParseShape = 1;
                }
                else if ((argv[arg][2] == 'p') && (argv[arg][3] == 'a'))
                {
                    // Enable parse action.
                    VerboseParseAction = 1;
                }
                else
                {
                    fprintf(stderr, "Unknown -v option type.\n");
                    return 0;
                }
            }

            else if (argv[arg][1] == 'q' && argv[arg][2] == 'a' && argv[arg][3] == 'e')
            {
                // Quiet. Opposite to verbose, will not display any messages.
                NoActionErrors = 1;

                if (VerboseAction)
                {
                    fprintf(stderr, "Quiet option -qse conflicts with specified verbose options.\n");
                    return 0;
                }
            }
            else if (argv[arg][1] == 'q')
            {
                // Quiet. Opposite to verbose, will not display any messages.
                Quiet = 1;

                if (VerboseAction || VerboseParseAction || VerboseParseShape || VerboseParse)
                {
                    fprintf(stderr, "Quiet option -q conflicts with specified verbose options.\n");
                    return 0;
                }
            }

            else if (argv[arg][1] == 'm')
            {
                if (argv[arg][2] == 'l')
                {
                    arg++;
                    TexLodBias = (float) atof(argv[arg]);
                    //printf("Texture LOD Bais is no %f\n", tex_lod_bias);
                }
                else
                {
                    fprintf(stderr, "Unknown -m option type.\n");
                    return 0;
                }
            }
#if defined(FXPLAYER_RENDER_DIRECT3D) && (GFC_D3D_VERSION != 10)
            else if ((argv[arg][1] == 'd') && (argv[arg][2] == 'y') && (argv[arg][3] == 'n'))
            {
                VMCFlags |= D3DPLAYER::VMConfig_UseDynamicTex;
            }
#endif
        }
        else
        {
            if (argv[arg])
            {
                gfc_strcpy(FileName, 256, argv[arg]);
            }
        }
    }

    if (FileName[0])
    {
        // if filename is set, then we try to load the font config
        LoadDefaultFontConfigFromPath(FileName);
    }

    if (!FileName[0])
    {
        printf("Note: No input file specified. Use -? option for help. \n");
        return 1;
    }

    return 1;
}

void    FxPlayerSettings::LoadFontConfigs(ConfigParser *parser)
{
    FontConfigs.Parse(parser);
    if (FontConfigs.size() > 0)
    {
        FontConfigIndex = 0;
    }
    else
    {
        FontConfigIndex = -1;
    }
}


int     FxPlayerSettings::GetFontConfigIndexByName(const char* pname)
{
    for (UInt i=0; i < FontConfigs.size(); i++)
    {
        if (gfc_stricmp(pname, FontConfigs[i]->ConfigName) == 0)
            return i;
    }   
    return -1;
}


FontConfig*  FxPlayerSettings::GetCurrentFontConfig()
{
    // we are skipping over invalid fontconfigs until one is found.
    // else return NULL.

    if (FontConfigIndex == -1)
        return NULL;

    FontConfig* fc = NULL;
    SInt sIdx = FontConfigIndex;
    bool ok = false;

    while (!ok)
    {
        ok = true;
        fc = FontConfigs[FontConfigIndex];
        // check if all fontlib files exist
        for (UInt i=0; i < fc->FontLibFiles.size(); i++)
        {
            // check if file exists
            GSysFile file(fc->FontLibFiles[i]);
            if (!file.IsValid())
            {
                ok = false;
                fprintf(stderr, "Fontlib file '%s' cannot be found. Skipping config '%s'..\n", fc->FontLibFiles[i].ToCStr(), fc->ConfigName.ToCStr());
                break;
            }
        }

        if (!ok)
        {
            FontConfigIndex++;
            FontConfigIndex %= (SInt)FontConfigs.size();
            if (FontConfigIndex == sIdx)
                return NULL;
        }
    }

    return FontConfigs[FontConfigIndex];
}


bool  FxPlayerSettings::LoadDefaultFontConfigFromPath(char *path)
{
    if (!NoFontConfigLoad)
    {
        // load fontconfig.txt if it exists in the movie's path
        GFxString fontConfigFilePath;

        // [PPS] The absolute check is unneccessary
        // ExtractFilePath will return the correct parent path:
        // C:/folder/filename.ext - C:/folder/
        // folder/filename.ext - folder/
        // filename.ext - empty string
        //if (GFxURLBuilder::IsPathAbsolute(path))
        //{
            fontConfigFilePath.AppendString(path);
            if ( !GFxURLBuilder::ExtractFilePath(&fontConfigFilePath) )
            {
                fontConfigFilePath = "";
            }
        //}

        fontConfigFilePath += "fontconfig.txt";
        bool maintainIndex = false;

        // store font config file related info
        if (FontConfigFilePath.GetLength() == 0)   // if no file was previously loaded
        {
            GFileStat fileStats;
            if ( GSysFile::GetFileStat(&fileStats, fontConfigFilePath.ToCStr()) )
            {
                FontConfigFilePath = fontConfigFilePath;
                FontConfigFileStats = fileStats;
            }
        }
        else // if the file was previously loaded and is modified
        {
            if (fontConfigFilePath == FontConfigFilePath)
            {
                // if modified time is not the same, then reload config file
                GFileStat fileStats;
                if ( GSysFile::GetFileStat(&fileStats, fontConfigFilePath.ToCStr()) )
                {
                    if ( !(fileStats == FontConfigFileStats) )
                    {
                        FontConfigFileStats = fileStats;
                        maintainIndex = true;
                    }
                }
            }
        }

        // parse the config file
        ConfigParser parser(fontConfigFilePath.ToCStr());
        SInt oldIdx = FontConfigIndex;
        LoadFontConfigs(&parser);

        // try to maintain previous font config index
        if ( maintainIndex &&
            (FontConfigIndex == 0) &&
            (oldIdx != -1) )
        {
            FontConfigIndex = oldIdx;
            FontConfigIndex %= (SInt)FontConfigs.size();
        }

        return true;
    }
    return false;
}



// Brief instructions.
void    FxPlayerSettings::PrintUsage()
{

    printf(
        "GFxPlayer - a sample SWF/GFX file player for the GFx library.\n"
        "\n"
        "Copyright (c) 2006-2007 Scaleform Corp. All Rights Reserved.\n"
        "Contact sales@scaleform.com for licensing information.\n"
        "\n"
        "Usage:        gfxplayer [options] movie_file.swf\n"
        "Options:\n"
        "  -?          Display this help info.\n"
        "  -s <factor> Scale the movie window size by the specified factor.\n"
        "  -na, -fsa   Use no anti-aliasing; use fullscreen HW AA.\n"
        "  -f          Run in full-screen mode.\n"
        "  -sy          Enable vertical synchronization.\n"
        "  -nsf        No system font - disables GFxFontProviderWin32.\n"
        "  -fc <fname> Load a font config file.\n"
        "  -nfc        Disable autoloading of font config file in movie's path.\n"
        "  -fl <fname> Specifies a SWF/GFX file to load into GFxFontLib.\n"
        "  -i          Display info HUD on startup.\n"
        "  -vp         Verbose parse - print SWF parse log.\n"
        "  -vps        Verbose parse shape - print SWF shape parse log.\n"
        "  -vpa        Verbose parse action - print SWF actions during parse.\n"
        "  -vfq        Do not print SWF filename for ActionScript errors in child movies \n"
        "  -vfr        Print SWF filename for ActionScript errors in root movies. \n"
        "  -vfp        Use print full file path for ActionScript errors\n"
        "  -va         Verbose actions - display ActionScript execution log.\n"
        "  -q          Quiet. Do not display errors or trace statements.\n"
        "  -qae        Suppress ActionScript errors.\n"
        "  -ml <bias>  Specify the texture LOD bias (float, default -0.5).\n"
        "  -p          Performance test - run without delay and log FPS.\n"
        "  -ff         Fast forward - run one frame per update.\n"
        "  -1          Play once; exit when/if movie reaches the last frame.\n"
        "  -r <0|1>    0 disables rendering  (for batch tests).\n"
        "              1 enables rendering (default setting).\n"
        //"              2 enables rendering & disables sound\n"
        "  -t <sec>    Timeout and exit after the specified number of seconds.\n"
        "  -b <bits>   Bit depth of output window (16 or 32, default is 16).\n"
        "  -nck        Disable all player related control keys.\n"
        "  -nh         Do not load HUD SWF.\n"
#if defined(FXPLAYER_RENDER_DIRECT3D) && (GFC_D3D_VERSION != 10)
        "  -dyn        Use dynamic textures (D3D)"
#endif
        "\n"
        "Keys:\n"
        "  CTRL N          Cycle through loaded font configs.\n"
        "  CTRL S          Toggle scaled display\n"
        "  CTRL W          Toggle wireframe\n"
        "  CTRL A          Toggle antialiasing mode\n"
        "  CTRL U          Toggle fullscreen\n"
        "  CTRL F          Toggle fast mode (FPS)\n"
        "  CTRL G          Toggle fast forward\n"
        "  CTRL P          Toggle pause\n"
        "  CTRL R          Restart the movie\n"
        "  CTRL D          Toggle stage clipping\n"
        "  CTRL C          Toggle stage culling\n"
        "  CTRL O          Toggle triangle optimization\n"
#if defined(FXPLAYER_RENDER_DIRECT3D) && (GFC_D3D_VERSION != 10)
        "  CTRL V          Toggle dynamic textures\n"
#endif
        "  CTRL Right      Step backward one frame\n"
        "  CTRL Left       Step forward one frame\n"
        "  CTRL PageUp     Step back 10 frames\n"
        "  CTRL PageDown   Step forward 10 frames\n"
        "  CTRL -,+        Curve tolerance down, up\n"
        "  F1              Toggle info help\n"
        "  F2              Toggle info stats\n"
        "  CTRL Q          Quit\n"
        );
}
