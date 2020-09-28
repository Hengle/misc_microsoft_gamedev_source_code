/**********************************************************************

Filename    :   FxPlayerSettings.h
Content     :   Command line parsing logic header for FxPlayer.
Created     :   January 15, 2008
Authors     :   Michael Antonov, Maxim Didenko

Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_FxPlayerSettings_H
#define INC_FxPlayerSettings_H


// ***** Player Settings class

#include "GTypes.h"
#include "FontConfigParser.h"

// Settings class stores playback settings determined
// based on the command-line parameters.
class   FxPlayerSettings
{
public:

    enum AAModeType
    {
        AAMode_None,        // No anti-aliasing is used.
        AAMode_EdgeAA,      // Edge AA is used (if supported by GRenderer).
        AAMode_FSAA         // HW Full-screen AA is used (if supported by device).
    };


    UInt        BitDepth;
    Float       ScaleX, ScaleY;
    Float       TexLodBias;
    AAModeType  AAMode;
    bool        Background;
    bool        MeasurePerformance;
    bool        FullScreen;
    UInt32      VMCFlags;
    bool        VSync;

    enum    HudStateType
    {
        Hud_Hidden,
        Hud_Stats,
        Hud_Help
    };

    // Display Hud at startup
    HudStateType    HudState;

    // Verbose options
    bool    VerboseParse;
    bool    VerboseParseShape;
    bool    VerboseParseAction;
    bool    VerboseAction;
    bool    LogRootFilenames;
    bool    LogChildFilenames;
    bool    LogLongFilename;
    bool    Quiet;
    bool    NoActionErrors;
    bool    NoHud;

    // Rendering state
    bool    DoLoop;
    bool    DoRender;
    bool    DoSound;

    // Set to disable system font.
    bool        NoSystemFont;
    bool        NoControlKeys;
    // If not empty, specifies a file to load int FontLib.
    GFxString   FontLibFile;
    // Set to disable automatic load of fontconfig.txt
    bool        NoFontConfigLoad;

    // Font configurations, if specified.
    FontConfigSet   FontConfigs;
    // Index of currently applied FontConfig, -1 for none.
    SInt            FontConfigIndex;
    // The requested language
    GFxString   RequestedLanguage;

    // Set to play movie as fast as possible
    bool    FastForward;

    Float   ExitTimeout;
    UInt    SleepDelay; // Some consoles use this

    // PlaybackFile
    char    FileName[256];
    char    ShortFileName[64];

    // FontConfigFile
    GFxString   FontConfigFilePath;
    GFileStat   FontConfigFileStats;


    FxPlayerSettings()
    {
        // Default values
        ScaleX = ScaleY     = 1.0f;
        TexLodBias          = -0.5f;
        AAMode              = AAMode_EdgeAA;
        BitDepth            = 32;
        Background          = 1;
#if defined(GFC_OS_PS2) || defined(GFC_OS_PS3) || defined(GFC_OS_WII) || defined(GFC_OS_XBOX360) || defined(GFC_OS_PSP)
        MeasurePerformance  = 1;
        FullScreen          = 1;
#else 
        MeasurePerformance  = 0;
        FullScreen          = 0;
#endif
        VSync               = 0;

        HudState            = Hud_Hidden;
        VMCFlags            = 0;

        VerboseParse        = 0;
        VerboseParseShape   = 0;
        VerboseParseAction  = 0;
        VerboseAction       = 0;
        LogRootFilenames    = 0;
        LogLongFilename     = 0;
        LogChildFilenames   = 1;

        Quiet               = 0;
        NoActionErrors      = 0;
        NoHud               = 0;

        DoLoop              = 1;
        DoRender            = 1;
        DoSound             = 0;

        NoSystemFont        = 0;
        NoControlKeys       = 0;
        NoFontConfigLoad    = 0;

        // No font config used by default.
        FontConfigIndex     = -1;

        FastForward         = 0;

        ExitTimeout         = 0.0f;
        SleepDelay          = 31;

        // Clear file
        FileName[0]         = 0;
        ShortFileName[0]    = 0;
    }

    // Initializes settings based on the command line.
    // Returns 1 if parse was successful, otherwise 0 if usage should be displayed.
    bool        ParseCommandLine(int argc, char *argv[]);

    void        LoadFontConfigs(ConfigParser *parser);
    FontConfig* GetCurrentFontConfig();
    int         GetFontConfigIndexByName(const char* pname);

    bool        LoadDefaultFontConfigFromPath(char* path);

    // Displays Playback / Usage information
    static void PrintUsage();
};

#endif  // INC_FXPLAYERSETTINGS_H
