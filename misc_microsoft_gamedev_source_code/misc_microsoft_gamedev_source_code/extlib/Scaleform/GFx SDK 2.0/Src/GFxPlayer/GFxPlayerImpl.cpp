/**********************************************************************

Filename    :   GFxPlayerImpl.cpp
Content     :   MovieRoot and Definition classes
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFile.h"
#include "GZlibFile.h"
#include "GImage.h"
#include "GJPEGUtil.h"
#include "GContainers.h"
#include "GFunctions.h"

#include "GRenderer.h"
#include "GUTF8Util.h"

#include "GFxObject.h"
#include "GFxAction.h"
#include "GFxArray.h"
#include "GFxButton.h"
#include "GFxMouse.h"
#include "GFxPlayerImpl.h"
#include "GFxLoaderImpl.h"
#include "GFxLoadProcess.h"

#include "GFxFontResource.h"
#include "GFxLog.h"
#include "GFxMorphCharacter.h"
#include "GFxShape.h"
#include "GFxStream.h"
#include "GFxStyles.h"
#include "GFxDlist.h"
#include "GFxTimers.h"
#include "GFxSprite.h"
#include "GFxStage.h"
#include "GFxSelection.h"

#include "GFxDisplayContext.h"

#include <string.h> // for memset
#include <float.h>
#include <stdlib.h>
#ifdef GFC_MATH_H
#include GFC_MATH_H
#else
#include <math.h>
#endif

// Increment this when the cache data format changes.
#define CACHE_FILE_VERSION  4


//
// ***** GFxMovieRoot
//
// Global, shared root state for a GFxMovieSub and all its characters.


GFxMovieRoot::GFxMovieRoot()
:       
    ViewportSet(0),    
    PixelScale(1.0f),
    BackgroundColor(0, 0, 0, 255),  
    MouseX(0),
    MouseY(0),
    MouseButtons(0),
    MouseMoved(0),
    MouseWheelScroll(0),
    MouseCursorType(GFxMouseCursorEvent::ARROW),
    pPrevTopMostEntityRawPtr(0),
    UserData(NULL),
    MouseSupportEnabled(GFC_MOUSE_SUPPORT_ENABLED),
    NeedMouseUpdate(1),
    OnEventXmlsocketOndataCalled(false),
    OnEventXmlsocketOnxmlCalled(false),
    OnEventLoadProgressCalled(false)
{
    TimeElapsed     = 0.0f;
    TimeRemainder   = 0.0f;
    // Correct FrameTime is assigned to (1.0f / pLevel0Def->GetFrameRate())
    // when _level0 is set/changed.
    FrameTime       = 1.0f / 12.0f;

    // No entries in load queue.
    pLoadQueueHead  = 0;

    // Create a delegated shared state and ensure that it has a log.
    // Delegation is configured later to Level0Def's shared state.
    pSharedState    = *new GFxSharedStateImpl(0);

    pASMouseListener = 0;
    pKeyboardState  = *new GFxKeyboardState;
    // Create global variable context
    pGlobalContext  = *new GASGlobalContext(this);
    pRetValHolder   = new ReturnValueHolder(pGlobalContext);

    InstanceNameCount = 0;

    VerboseAction       = 0;

    LevelClipsChanged = 1;
    pLevel0Movie      = 0;    
    SuppressActionErrors = 0;

    AdvanceCalled     = 0;


    CachedLogFlag     = 0;

    // Viewport: un-initialized by default.
    ViewportSet = 0;
    ViewScaleX = 1.0f;
    ViewScaleY = 1.0f;
    ViewOffsetX = ViewOffsetY = 0;
    ViewScaleMode = SM_ShowAll;
    ViewAlignment = Align_Center;

    // Focus
    IsShowingRect = false;

    LastIntervalTimerId = 0;
}

GFxMovieRoot::~GFxMovieRoot()
{
    // Set our log to String Manager so that it can report leaks if they happened.
    if (pLevel0Movie)
        pGlobalContext->GetStringManager()->SetLeakReportLog(
        pLevel0Movie->GetLog(), pLevel0Def->GetFileURL());

    // It is important to destroy the sprite before the global context,
    // so that is is not used from OnEvent(unload) in sprite destructor
    // NOTE: We clear the list here first because users can store pointers in _global,
    // which would cause pMovie assignment to not release it early (avoid "aeon.swf" crash).
    UPInt i;

    ShutdownTimers();

    for (i = MovieLevels.size(); i > 0; i--)
        MovieLevels[i-1].pSprite->ClearDisplayList();
    for (i = MovieLevels.size(); i > 0; i--)
        MovieLevels[i-1].pSprite->ForceShutdown();
    // Release all refs.
    MovieLevels.clear();

    ClearStickyVariables();

    delete pRetValHolder;
}


GFxSprite*  GFxMovieRoot::GetLevelMovie(SInt level) const
{
    GASSERT(level >= 0);

    // Exhaustive for now; could do binary.
    for (UInt i = 0; i < MovieLevels.size(); i++)
    {
        if (MovieLevels[i].Level == level)
            return MovieLevels[i].pSprite;
    }   
    return 0;
}

// Sets a movie at level; used for initialization.
bool        GFxMovieRoot::SetLevelMovie(SInt level, GFxSprite *psprite)
{
    UInt i = 0;
    GASSERT(level >= 0);
    GASSERT(psprite);

    LevelClipsChanged = 1;

    for (; i< MovieLevels.size(); i++)
    {
        if (MovieLevels[i].Level >= level)
        {           
            if (MovieLevels[i].Level == level)
            {
                GFC_DEBUG_WARNING1(1, "GFxMovieRoot::SetLevelMovie fails, level %d already occupied", level);
                return 0;
            }           
            // Found insert spot.
            break;              
        }
    }

    // Insert the item.
    LevelInfo li;
    li.Level  = level;
    li.pSprite= psprite;
    MovieLevels.insert(i, li);

    psprite->OnInsertionAsLevel(level);

    if (level == 0)
    {
        pLevel0Movie = psprite;
        pLevel0Def   = psprite->GetResourceMovieDef();
        pSharedState->SetDelegate(pLevel0Def->pSharedState);
        // Frame timing
        FrameTime    = 1.0f / pLevel0Def->GetFrameRate();

        if (!ViewportSet)
        {
            GFxMovieDefImpl* pdef = psprite->GetResourceMovieDef();            
            GViewport desc((SInt)pdef->GetWidth(), (SInt)pdef->GetHeight(), 0,0, (SInt)pdef->GetWidth(), (SInt)pdef->GetHeight());
            SetViewport(desc);
        }        
    }

    NeedMouseUpdate = 1;
    return 1;
}

// Destroys a level and its resources.
bool    GFxMovieRoot::ReleaseLevelMovie(SInt level)
{
    if (level == 0)
    {
        StopDrag();

        ShutdownTimers();

        // Not sure if this unload order is OK
        while (MovieLevels.size())
        {
            GFxSprite* plevel = MovieLevels[MovieLevels.size() - 1].pSprite;
            plevel->OnEventUnload();
            DoActions();
            plevel->ForceShutdown();
            MovieLevels.remove(MovieLevels.size() - 1);
        }

        // Clear vars.
        pLevel0Movie = 0;   
        FrameTime    = 1.0f / 12.0f;        
        // Keep pLevel0Def till next load so that users don't get null from GetMovieDef().
        //  pLevel0Def   = 0;
        //  pSharedState->SetDelegate(0);

        LevelClipsChanged = 1;
        return 1;
    }

    // Exhaustive for now; could do binary.
    for (UInt i = 0; i < MovieLevels.size(); i++)
    {
        if (MovieLevels[i].Level == level)
        {
            GPtr<GFxSprite> plevel = MovieLevels[i].pSprite;

            // Inform old character of unloading.
            plevel->OnEventUnload();
            DoActions();            

            // TBD: Is this right, or should local frames persist till level0 unload?
            plevel->ForceShutdown(); 

            // Remove us.
            MovieLevels.remove(i);

            LevelClipsChanged = 1;
            return 1;
        }
    }   
    return 0;
}

GFxASCharacter* GFxMovieRoot::FindTarget(const GASString& path) const
{
    if (!pLevel0Movie)
        return 0;

    // This will work since environment parses _levelN prefixes correctly.
    // However, it would probably be good to move the general FindTarget logic here.
    return pLevel0Movie->GetASEnvironment()->FindTarget(path);
}


// Helper: parses _levelN tag and returns the end of it.
// Returns level index, of -1 if there is no match.
SInt        GFxMovieRoot::ParseLevelName(const char* pname, const char **ptail, bool caseSensitive)
{
    if (!pname || pname[0] != '_')
        return -1;

    if (caseSensitive)
    {       
        if ( (pname[1] != 'l') ||
            (pname[2] != 'e') ||
            (pname[3] != 'v') ||
            (pname[4] != 'e') ||
            (pname[5] != 'l') )
            return -1;
    }
    else
    {
        if ( ((pname[1] != 'l') && (pname[1] != 'L')) ||
            ((pname[2] != 'e') && (pname[1] != 'E')) ||
            ((pname[3] != 'v') && (pname[1] != 'V')) ||
            ((pname[4] != 'e') && (pname[1] != 'E')) ||
            ((pname[5] != 'l') && (pname[1] != 'L')) )
            return -1;
    }
    // Number must follow the level.
    if ((pname[6] < '0') || (pname[6] > '9'))
        return -1;

    char *ptail2 = 0;
    SInt level = (SInt)strtol(pname + 6, &ptail2, 10);
    *ptail = ptail2;

    return level;
}



// Create new instance names for unnamed objects.
GASString    GFxMovieRoot::CreateNewInstanceName()
{
    InstanceNameCount++; // Start at 0, so first value is 1.
    char pbuffer[48] = { 0 };


    gfc_sprintf(pbuffer, 48, "instance%u", InstanceNameCount);

    GASSERT(pGlobalContext);
    return pGlobalContext->CreateString(pbuffer);
}



// *** Load/Unload movie support

// ****************************************************************************
// Adds load queue entry and takes ownership of it.
//
void    GFxMovieRoot::AddLoadQueueEntry(GFxLoadQueueEntry *pentry)
{
    if (!pLoadQueueHead)
    {
        pLoadQueueHead = pentry;
    }
    else
    {
        // Find tail.
        GFxLoadQueueEntry *ptail = pLoadQueueHead;
        while (ptail->pNext)    
            ptail = ptail->pNext;   

        // Insert at tail.
        ptail->pNext = pentry;
    }   
}

// ****************************************************************************
// Adds load queue entry based on parsed url and target path 
// (Load/unload Movie)
//
void    GFxMovieRoot::AddLoadQueueEntry(const char* ptarget, const char* purl,
                                        GFxLoadQueueEntry::LoadMethod method)
{
    GFxLoadQueueEntry*  pentry        = 0;
    SInt                level         = -1;
    GFxASCharacter*     ptargetChar   = FindTarget(pGlobalContext->CreateString(ptarget));
    GFxSprite*          ptargetSprite = ptargetChar ? ptargetChar->ToSprite() : 0;  

    // If target leads to level, use level loading option.
    if (ptargetSprite)  
    {
        level = ptargetSprite->GetLevel();
        if (level != -1)
        {
            ptargetSprite = 0;
            ptargetChar   = 0;
        }       
    }

    // Otherwise, we have a real target.
    if (ptargetChar)
    {
        pentry = new GFxLoadQueueEntry(ptargetChar->GetCharacterHandle(), purl, method);
    }
    else
    {
        // It must be a level or bad target. 
        if (level == -1)
        {
            // Decode level name.
            const char* ptail = "";
            level = ParseLevelName(ptarget, &ptail, pLevel0Movie->IsCaseSensitive());
            if (*ptail != 0)
                level = -1; // path must end at _levelN.
        }

        if (level != -1)
        {
            pentry = new GFxLoadQueueEntry(level, purl, method);
        }
    }

    if (pentry)
        AddLoadQueueEntry(pentry);
}

// ****************************************************************************
// Adds load queue entry based on parsed url and target path 
// (Load Variables)
//
void    GFxMovieRoot::AddVarLoadQueueEntry(const char* ptarget, const char* purl,
                                           GFxLoadQueueEntry::LoadMethod method)
{
    GFxLoadQueueEntry*  pentry        = 0;
    SInt                level         = -1;
    GFxASCharacter*     ptargetChar   = FindTarget(pGlobalContext->CreateString(ptarget));
    GFxSprite*          ptargetSprite = ptargetChar ? ptargetChar->ToSprite() : 0;  

    // If target leads to level, use level loading option.
    if (ptargetSprite)  
    {
        level = ptargetSprite->GetLevel();
        if (level != -1)
        {
            ptargetSprite = 0;
            ptargetChar   = 0;
        }       
    }

    // Otherwise, we have a real target.
    if (ptargetChar)
    {
        pentry = new GFxLoadQueueEntry(ptargetChar->GetCharacterHandle(), purl, method, true);
    }
    else
    {
        // It must be a level or bad target. 
        if (level == -1)
        {
            // Decode level name.
            const char* ptail = "";
            level = ParseLevelName(ptarget, &ptail, pLevel0Movie->IsCaseSensitive());
            if (*ptail != 0)
                level = -1; // path must end at _levelN.
        }

        if (level != -1)
        {
            pentry = new GFxLoadQueueEntry(level, purl, method, true);
        }
    }

    if (pentry)
        AddLoadQueueEntry(pentry);
}

// ****************************************************************************
// Adds load queue entry based on parsed url and target character 
// (Load/unload Movie)
//
void    GFxMovieRoot::AddLoadQueueEntry(GFxASCharacter* ptargetChar, const char* purl,
                                        GFxLoadQueueEntry::LoadMethod method,
                                        GASMovieClipLoader* pmovieClipLoader)
{   
    if (!ptargetChar)
        return;

    GFxLoadQueueEntry*  pentry  = 0;
    SInt                level   = -1;
    GFxSprite*          ptargetSprite = ptargetChar->ToSprite();

    // If target leads to level, use level loading option.
    if (ptargetSprite)
    {
        level = ptargetSprite->GetLevel();
        if (level != -1)
        {
            ptargetSprite = 0;
            ptargetChar   = 0;
        }   
    }

    // Otherwise, we have a real target.
    if (ptargetChar)
    {
        pentry = new GFxLoadQueueEntry(ptargetChar->GetCharacterHandle(), purl, method);
    }
    else if (level != -1)
    {
        pentry = new GFxLoadQueueEntry(level, purl, method);
    }

    if (pentry)
    {
        pentry->pMovieClipLoader = pmovieClipLoader;
        AddLoadQueueEntry(pentry);
    }
}

// ****************************************************************************
// Adds load queue entry based on parsed url and target character 
// (Load Variables)
//
void    GFxMovieRoot::AddVarLoadQueueEntry(GFxASCharacter* ptargetChar, const char* purl,
                                           GFxLoadQueueEntry::LoadMethod method)
{
    if (!ptargetChar)
        return;

    GFxLoadQueueEntry*  pentry  = 0;
    SInt                level   = -1;
    GFxSprite*          ptargetSprite = ptargetChar->ToSprite();

    // If target leads to level, use level loading option.
    if (ptargetSprite)
    {
        level = ptargetSprite->GetLevel();
        if (level != -1)
        {
            ptargetSprite = 0;
            ptargetChar   = 0;
        }   
    }

    // Otherwise, we have a real target.
    if (ptargetChar)
    {
        pentry = new GFxLoadQueueEntry(ptargetChar->GetCharacterHandle(), purl, method, true);
    }
    else if (level != -1)
    {
        pentry = new GFxLoadQueueEntry(level, purl, method, true);
    }

    if (pentry)
    {
        AddLoadQueueEntry(pentry);
    }
}

// ****************************************************************************
// Adds load queue entry based on parsed url and LoadVars object
// (Load Variables)
//
void GFxMovieRoot::AddVarLoadQueueEntry(GASLoadVarsObject* ploadVars, const char* purl,
                                        GFxLoadQueueEntry::LoadMethod method)
{
    GFxLoadQueueEntry* pentry = new GFxLoadQueueEntry(purl, method, true);
    if (pentry)
    {
        pentry->pLoadVars = ploadVars;
        AddLoadQueueEntry(pentry);
    }
}




GFxMovieDefImpl*  GFxMovieRoot::CreateImageMovieDef(
    GFxImageResource *pimageResource, bool bilinear,
    const char *purl, GFxLoadStates *pls)
{   
    GFxMovieDefImpl*    pdefImpl = 0; 
    GPtr<GFxLoadStates> plsRef;

    // If load states were not passed, create them. This is necessary if
    // we are being called from sprite's attachBitmap call.
    if (!pls)
    {
        plsRef = *new GFxLoadStates(pLevel0Def->pLoaderImpl, pSharedState);
        pls = plsRef.GetPtr();
    }

    // Create a loaded image sprite based on imageInfo.
    if (pimageResource)
    {
        // No file opener here, since it was not used (ok since file openers are
        // used as a key only anyway). Technically this is not necessary since
        // image 'DataDefs' are not stored in the library right now, but the
        // argument must be passed for consistency.
        GFxResourceKey createKey = GFxMovieDataDef::CreateMovieFileKey(purl, 0, 0, 0);

        // Create a MovieDataDef containing our image (and an internal corresponding ShapeDef).
        GPtr<GFxMovieDataDef> pimageMovieDataDef = *new GFxMovieDataDef(createKey);
        if (!pimageMovieDataDef)
            return pdefImpl;        
        pimageMovieDataDef->InitImageFileMovieDef(purl, 0, pimageResource, bilinear);

        // We can alter load states since they are custom for out caller.
        // (ProcessLoadQueue creates a copy of LoadStates)
        pls->SetDataDef(pimageMovieDataDef);

        // TBD: Should we use pSharedState or its delegate??
        pdefImpl = new GFxMovieDefImpl(pls->GetBindStates(),
            pls->pLoaderImpl,
            GFxLoader::LoadAll,
            pSharedState->GetDelegate(),
            true); // Images are fully loaded.
    }
    return pdefImpl;
}

// Processes the load queue handling load/unload instructions.  
void    GFxMovieRoot::ProcessLoadQueue()
{
    while(pLoadQueueHead)
    {
        // Remove from queue.
        GFxLoadQueueEntry *pentry = pLoadQueueHead;
        pLoadQueueHead = pentry->pNext;

        // *** Handle loading/unloading.

        GFxString            url(pentry->URL), urlStrGfx;
        GPtr<GFxASCharacter> poldChar;      
        GFxASCharacter*      pparent   = 0;
        GFxResourceId        newCharId(GFxCharacterDef::CharId_EmptyMovieClip);

        // Capture load states - constructor copies their values from pSharedState;
        // this means that states used for 'loadMovie' can be overridden per movie view.
        // This means that state inheritance chain (GFxLoader -> GFxMovieDefImpl -> GFxMovieRoot)
        // takes effect here, so setting new states in any of above can have effect
        // on loadMovie calls. Note that this is different from import logic, which
        // always uses exactly same states as importee. Such distinction is by design.
        GPtr<GFxLoadStates>  pls = *new GFxLoadStates(pLevel0Def->pLoaderImpl, pSharedState);
        GFxLog*              plog = pls->GetLog();
        GPtr<GFxImageLoader> pimageLoader = pSharedState->GetImageLoader();
        // We take out load flags from pLevel0Def so that its options
        // can be inherited into GFxMovieDef.
        UInt                loadFlags = pLevel0Def->GetLoadFlags();

        GPtr<GFxMovieDefImpl>   pmovieDef;
        GPtr<GFxSprite>         pnewChar;

        // Obtain level0 path before it has a chance to be unloaded below.
        GFxString level0Path;
        GetLevel0Path(&level0Path);

        bool stripped = false;

        bool loadVars = ((pentry->Type & GFxLoadQueueEntry::LTF_VarsFlag) != 0);

        if (pentry->pCharacter)
        {
            // This is loadMovie, not level.
            GASSERT(pentry->Level == -1);
            GASSERT(!(pentry->Type & GFxLoadQueueEntry::LTF_LevelFlag));

            // Make sure we have character targets and parents.
            if (!(poldChar = pentry->pCharacter->ResolveCharacter(this)))
            {
                delete pentry;
                break;
            }
            if ((pparent = poldChar->GetParent())==0)
            {
                delete pentry;
                break;
            }
            stripped = ((poldChar->GetResourceMovieDef()->GetSWFFlags()
                & GFxMovieInfo::SWF_Stripped) != 0);
        }
        else if (pentry->Level != -1)
        {
            GASSERT(pentry->Type & GFxLoadQueueEntry::LTF_LevelFlag);
            if (GetLevelMovie(pentry->Level))
                stripped = ((GetLevelMovie(pentry->Level)->GetResourceMovieDef()->GetSWFFlags()
                & GFxMovieInfo::SWF_Stripped) != 0);
            else if (GetLevelMovie(0))
                stripped = ((GetLevelMovie(0)->GetResourceMovieDef()->GetSWFFlags()
                & GFxMovieInfo::SWF_Stripped) != 0);

            if ( !loadVars )
            {
                // Wipe out level, making it ready for load.
                // Note that this could wipe out all levels!
                // Do this only if loading movie, not variables
                ReleaseLevelMovie(pentry->Level);
                newCharId = GFxResourceId(); // Assign invalid id.
            }
        }
        else
        {
            // If not level or target, then it should be a LoadVars object
            if (!(pentry->pLoadVars))
            {
                // No level or target!
                delete pentry;
                break;
            }
        }

        SInt filelength = 0;
        // vars
        if (loadVars)
        {
            GFxString data;
            if (url.GetLength())
            {
                bool userVarsProtocol = 0;

                // @TODO logic to determine if a user protocol was used

                if (userVarsProtocol)
                {
                    // @TODO
                }
                else
                {
                    // Translate the filename.
                    GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_LoadVars, url, level0Path);
                    GFxString                   fileName;
                    pls->BuildURL(&fileName, loc);

                    // File loading protocol
                    GPtr<GFile> pfile;
                    pfile = *pls->OpenFile(fileName.ToCStr());
                    if (pfile)
                    {
                        if (pentry->pLoadVars)
                        {
                            // LoadVars loading
                            if (!GFx_LoadData(pentry->pLoadVars, pfile.GetPtr(), data))
                            {
                                pentry->pLoadVars = NULL;
                            }

                        }
                        else if (pentry->Level == -1)
                        {
                            // character loading
                            GASSERT(poldChar);
                            // add the variables into poldChar
                            GFx_LoadVariables(pLevel0Movie->GetASEnvironment(), poldChar.GetPtr(), pfile.GetPtr());
                        }
                        else
                        {
                            // Level loading
                            GASSERT(pentry->Type & GFxLoadQueueEntry::LTF_LevelFlag);           
                            // use MovieLevels to get the appropriate level, and
                            // add the variables into the level's sprite
                            // find the right level movie
                            pnewChar = GetLevelMovie(pentry->Level);

                            // If level doesn't exist, create one using an empty MovieDef.
                            if (pnewChar.GetPtr() == 0)
                            {                                
                                // Technically this is not necessary since empty 'DataDefs' are not stored 
                                // in the library right now, but the argument must be passed for consistency.
                                // TBD: Perhaps we could hash it globally, since there should be only one?
                                GFxResourceKey createKey = GFxMovieDataDef::CreateMovieFileKey("", 0, 0, 0);

                                // Create a MovieDataDef containing our image (and an internal corresponding ShapeDef).
                                GPtr<GFxMovieDataDef> pemptyDataDef   = *new GFxMovieDataDef(createKey);
                                GPtr<GFxMovieDefImpl> pemptyMovieImpl;
                                if (pemptyDataDef)
                                {
                                    pemptyDataDef->InitEmptyMovieDef("");
                                    pls->SetDataDef(pemptyDataDef);
                                    pemptyMovieImpl = *new GFxMovieDefImpl(pls->GetBindStates(), pls->pLoaderImpl,
                                                                           GFxLoader::LoadAll, pSharedState->GetDelegate(), 
                                                                           true);
                                }
                                if (!pemptyMovieImpl)
                                    break;

                                pnewChar = *new GFxSprite(pemptyDataDef, pemptyMovieImpl, this,
                                    NULL, GFxResourceId(), true);
                                pnewChar->SetLevel(pentry->Level);
                                SetLevelMovie(pentry->Level, pnewChar);
                            }
                            GFx_LoadVariables(pLevel0Movie->GetASEnvironment(), pnewChar.GetPtr(), pfile.GetPtr());
                        }
                        filelength = pfile->GetLength();
                    }
                }
            }

            // notify LoadVars listeners. TODO: should be done in right places in right time.
            if (pentry->pLoadVars)
            {
                GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
                GASString str = penv->CreateString(data);
                pentry->pLoadVars->NotifyOnData(penv, str);
            }
        }

        // Movie (not LoadVars)
        else
        {        
            // If the file is stripped, its loadMovie may be stripped too,
            // so try to load '.gfx' file first.
            if (stripped && (url.GetSize() > 4) &&
                (GFxString::CompareNoCase(url.ToCStr() + (url.GetSize() - 4), ".swf") == 0) )
            {
                urlStrGfx = url;
                urlStrGfx.Resize(urlStrGfx.GetSize() - 4);
                urlStrGfx += ".gfx";
            }

            // Load movie: if there is no URL or it fails, do 'unloadMovie'.
            if (url.GetLength())
            {               
                bool    bilinearImage     = 0;
                bool    userImageProtocol = 0;

                // Check to see if URL is a user image substitute.
                if (url[0] == 'i' || url[0] == 'I')
                {
                    GFxString urlLowerCase = url.ToLower();

                    if (urlLowerCase.Substring(0, 6) == "img://")
                        userImageProtocol = bilinearImage = 1;
                    else if (urlLowerCase.Substring(0, 8) == "imgps://")
                        userImageProtocol = 1;
                }

                if (userImageProtocol)
                {
                    // Create image through image callback
                    GPtr<GFxImageResource> pimageRes =
                        *GFxLoaderImpl::LoadMovieImage(url.ToCStr(), pimageLoader, plog);

                    if (pimageRes)
                    {
                        pmovieDef = *CreateImageMovieDef(pimageRes, bilinearImage,
                            url.ToCStr(), pls);
                    }
                    else if (plog)
                    {
                        plog->LogScriptWarning(
                            "LoadMovieImageCallback failed to load image \"%s\"\n", url.ToCStr());
                    }
                }
                else
                {
                    // Load the movie, first trying the .gfx file if necessary.
                    // Our file loading can automatically detect and load images as well,
                    // so we take advantage of that by passing the GFxLoader::LoadImageFiles flag.

                    // TBD: We will need to do something with delayed loading here in the future;
                    // i.e. if the CreateMovie is executing in a different thread it will need to
                    // post messages when it is done so that the handlers can be called.

                    if (urlStrGfx.GetLength() > 0)
                    {
                        GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_LoadMovie,
                            urlStrGfx, level0Path);

                        pmovieDef = *GFxLoaderImpl::CreateMovie_LoadState(
                            pls, loc, loadFlags | GFxLoader::LoadImageFiles);
                    }
                    if (!pmovieDef)
                    {
                        GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_LoadMovie,
                            url, level0Path);

                        pmovieDef = *GFxLoaderImpl::CreateMovie_LoadState(
                            pls, loc, loadFlags | GFxLoader::LoadImageFiles);
                    }

                    if (pmovieDef)
                    {
                        // Record file length for progress report below. Once we do
                        // threaded loading this will change.
                        filelength = pmovieDef->GetFileBytes();
                    }
                    else if (plog)
                    {
                        plog->LogScriptWarning("Error loading URL \"%s\"\n", url.ToCStr());
                    }
                }   
            } // end if (url)

            // Create a new sprite of desired type.
            if (pmovieDef)
            {
                // CharId of loaded clip is that of an empty one, so that it can not
                // be duplicated. That is the expected Flash behavior.
                // Also mark clip as loadedSeparately, so that _lockroot works.
                pnewChar = *new GFxSprite(pmovieDef->GetDataDef(), pmovieDef, this,
                    pparent, newCharId, true);
            }
            bool charIsLoadedSuccessfully = (pnewChar.GetPtr() != NULL);

            if (pentry->Level == -1)
            {
                // Nested clip loading; replace poldChar.
                GASSERT(poldChar);

                // If that failed, create an empty sprite. This also handles unloadMovie().
                if (!pnewChar)
                {
                    GFxCharacterCreateInfo ccinfo =
                        pparent->GetResourceMovieDef()->GetCharacterCreateInfo(newCharId);
                    pnewChar = *(GFxSprite*)ccinfo.pCharDef->CreateCharacterInstance(pparent, newCharId,
                        ccinfo.pBindDefImpl);
                }
                // And attach a new character in place of an old one.
                if (pnewChar)
                {
                    pparent->ReplaceChildCharacterOnLoad(poldChar, pnewChar);
                    // In case permanent variables were assigned.. check them.
                    ResolveStickyVariables(pnewChar);
                }
            }
            else
            {
                // Level loading.
                GASSERT(pentry->Type & GFxLoadQueueEntry::LTF_LevelFlag);           
                if (pnewChar)
                {
                    pnewChar->SetLevel(pentry->Level);
                    SetLevelMovie(pentry->Level, pnewChar);
                    // Check if permanent/sticky variables were assigned to level.
                    ResolveStickyVariables(pnewChar);
                }

                if (!pLevel0Movie && plog)
                    plog->LogScriptWarning("_level0 unloaded - no further playback possible\n");
            }

            // Notify MovieClipLoader listeners. TODO: should be done in right places in right time.
            if (pentry->pMovieClipLoader)
            {
                GASSERT(pLevel0Movie);
                GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
                GASSERT(penv);
                if (charIsLoadedSuccessfully)
                {
                    pentry->pMovieClipLoader->NotifyOnLoadStart(penv, pnewChar);
                    pentry->pMovieClipLoader->NotifyOnLoadProgress(penv, pnewChar, filelength, filelength);
                    pentry->pMovieClipLoader->NotifyOnLoadComplete(penv, pnewChar, 0);
                    pentry->pMovieClipLoader->NotifyOnLoadInit(penv, pnewChar);
                }
                else if (url.GetLength())
                {
                    // No new char is loaded - assuming the error occurs
                    pentry->pMovieClipLoader->NotifyOnLoadError(penv, pnewChar, "URLNotFound", 0);
                    // are there any other error codes? (AB)
                }
            }

        } // end else { ** load movie ** }

        // Done.
        delete pentry;
    }
}


// Fills in a file system path relative to _level0.
bool    GFxMovieRoot::GetLevel0Path(GFxString *ppath) const
{       
    if (!pLevel0Movie)
    {
        ppath->Clear();
        return false;
    }

    // Get URL.
    *ppath = pLevel0Def->GetFileURL();

    // Extract path by clipping off file name.
    if (!GFxURLBuilder::ExtractFilePath(ppath))
    {
        ppath->Clear();
        return false;
    }
    return true;
}



// *** Drag State support


void GFxMovieRoot::DragState::InitCenterDelta(bool lockCenter)
{
    LockCenter = lockCenter;

    if (!LockCenter)
    {
        typedef GRenderer::Matrix   Matrix;                     

        // We are not centering on the object, so record relative delta         
        Matrix          parentWorldMat;
        GFxCharacter*   pchar = pCharacter;
        if (pchar->GetParent())
            parentWorldMat = pchar->GetParent()->GetWorldMatrix();

        // Mouse location
        int     x, y, buttons;
        pchar->GetMovieRoot()->GetMouseState(&x, &y, &buttons);
        GPointF worldMouse(GFC_PIXELS_TO_TWIPS(x), GFC_PIXELS_TO_TWIPS(y));

        GPointF parentMouse;
        // Transform to parent coordinates [world -> parent]
        parentWorldMat.TransformByInverse(&parentMouse, worldMouse);

        // Calculate the relative mouse offset, so that we can apply adjustment
        // before bounds checks.
        Matrix  local = pchar->GetMatrix();
        CenterDelta.x = local.M_[0][2] - parentMouse.x;
        CenterDelta.y = local.M_[1][2] - parentMouse.y;
    }
}


// *** Action List management
UInt GFxMovieRoot::ActionQueueType::LastSessionId = 0;

GFxMovieRoot::ActionQueueType::ActionQueueType()
{
    ModId = 1;
    pFreeEntry = NULL;
    CurrentSessionId = ++LastSessionId;
    FreeEntriesCount = 0;
}

GFxMovieRoot::ActionQueueType::~ActionQueueType()
{
    Clear();

    // free pFreeEntry chain
    for(ActionEntry* pcur = pFreeEntry; pcur; )
    {
        ActionEntry* pnext = pcur->pNextEntry;
        delete pcur;
        pcur = pnext;
    }
}

void GFxMovieRoot::ActionQueueType::FlushOnLoadQueues()
{
    ActionQueueEntry& actionQueueEntry = Entries[AP_Normal];
    // here is a special case for onClipEvent(load)/onLoad.
    // onClipEvent(load)[+- onLoad] has higher priority than just onLoad event.
    // So, we allocate two different priorities for onClipEvent(load) and for onLoad.
    if (Entries[GFxMovieRoot::AP_ClipEventLoad].pActionRoot || Entries[GFxMovieRoot::AP_Load].pActionRoot)
    {
        // if either onClipEvent(load) or onLoad events were queued up and we are inserting event with
        // a normal (frame) priority - reinsert all load events in queue with normal priority and insert
        // our event AFTER them.
        if (Entries[GFxMovieRoot::AP_ClipEventLoad].pActionRoot)
        {
            if (actionQueueEntry.pInsertEntry == 0)
            {
                Entries[GFxMovieRoot::AP_ClipEventLoad].pLastEntry->pNextEntry = actionQueueEntry.pActionRoot;
                actionQueueEntry.pActionRoot = Entries[GFxMovieRoot::AP_ClipEventLoad].pActionRoot;
            }
            else
            {
                Entries[GFxMovieRoot::AP_ClipEventLoad].pLastEntry->pNextEntry = actionQueueEntry.pInsertEntry->pNextEntry;
                actionQueueEntry.pInsertEntry->pNextEntry = Entries[GFxMovieRoot::AP_ClipEventLoad].pActionRoot;
            }
            // advance insert entry to the last entry in inserting queue
            actionQueueEntry.pInsertEntry = Entries[GFxMovieRoot::AP_ClipEventLoad].pLastEntry;
            Entries[GFxMovieRoot::AP_ClipEventLoad].Reset();
        }
        if (Entries[GFxMovieRoot::AP_Load].pActionRoot)
        {
            if (actionQueueEntry.pInsertEntry == 0)
            {
                Entries[GFxMovieRoot::AP_Load].pLastEntry->pNextEntry = actionQueueEntry.pActionRoot;
                actionQueueEntry.pActionRoot = Entries[GFxMovieRoot::AP_Load].pActionRoot;
            }
            else
            {
                Entries[GFxMovieRoot::AP_Load].pLastEntry->pNextEntry = actionQueueEntry.pInsertEntry->pNextEntry;
                actionQueueEntry.pInsertEntry->pNextEntry = Entries[GFxMovieRoot::AP_Load].pActionRoot;
            }
            actionQueueEntry.pInsertEntry = Entries[GFxMovieRoot::AP_Load].pLastEntry;
            Entries[GFxMovieRoot::AP_Load].Reset();
        }
    }
}

GFxMovieRoot::ActionEntry*  GFxMovieRoot::ActionQueueType::InsertEntry(GFxActionPriority::Priority prio)
{
    ActionEntry *p = 0;
    if (pFreeEntry)
    {
        p = pFreeEntry;
        pFreeEntry = pFreeEntry->pNextEntry;
        p->pNextEntry = 0;
        --FreeEntriesCount;
    }
    else
    {
        if ((p = new ActionEntry)==0)
            return 0;
    }

    ActionQueueEntry& actionQueueEntry = Entries[prio];

    // Insert and move insert pointer to us.
    if (actionQueueEntry.pInsertEntry == 0) // insert at very beginning
    {
        p->pNextEntry                   = actionQueueEntry.pActionRoot;
        actionQueueEntry.pActionRoot    = p;
    }
    else
    {
        p->pNextEntry = actionQueueEntry.pInsertEntry->pNextEntry;
        actionQueueEntry.pInsertEntry->pNextEntry = p;
    }
    actionQueueEntry.pInsertEntry   = p;
    if (p->pNextEntry == NULL)
        actionQueueEntry.pLastEntry = p;
    p->SessionId = CurrentSessionId;
    ++ModId;

    // Done
    return p;
}

void GFxMovieRoot::ActionQueueType::AddToFreeList(GFxMovieRoot::ActionEntry* pentry)
{
    pentry->ClearAction();
    if (FreeEntriesCount < 50)
    {
        pentry->pNextEntry = pFreeEntry;
        pFreeEntry = pentry;
        ++FreeEntriesCount;
    }
    else
        delete pentry;
}

void    GFxMovieRoot::ActionQueueType::Clear()
{   
    GFxMovieRoot::ActionQueueIterator iter(this);
    while(iter.getNext())
        ;
}

UInt GFxMovieRoot::ActionQueueType::StartNewSession(UInt* pprevSessionId) 
{ 
    if (pprevSessionId) *pprevSessionId = CurrentSessionId;
    CurrentSessionId = ++LastSessionId; 
    return CurrentSessionId; 
}

GFxMovieRoot::ActionQueueIterator::ActionQueueIterator(GFxMovieRoot::ActionQueueType* pactionQueue)
{
    GASSERT(pactionQueue);
    pActionQueue    = pactionQueue;
    ModId           = 0;
    CurrentPrio     = 0;
    pLastEntry      = 0;
}

GFxMovieRoot::ActionQueueIterator::~ActionQueueIterator()
{
    if (pLastEntry)
        pActionQueue->AddToFreeList(pLastEntry); // release entry
}

const GFxMovieRoot::ActionEntry* GFxMovieRoot::ActionQueueIterator::getNext()
{
    if (pActionQueue->ModId != ModId)
    {
        // new actions were added - restart
        CurrentPrio = 0;
        ModId = pActionQueue->ModId;
    }
    ActionEntry* pcurEntry = pActionQueue->Entries[CurrentPrio].pActionRoot;
    if (!pcurEntry)
    {
        while(pcurEntry == NULL && ++CurrentPrio < AP_Count)
        {
            pcurEntry = pActionQueue->Entries[CurrentPrio].pActionRoot;
        }
    }
    if (pcurEntry)
    {
        if (pcurEntry == pActionQueue->Entries[CurrentPrio].pInsertEntry)
            pActionQueue->Entries[CurrentPrio].pInsertEntry = pcurEntry->pNextEntry;
        pActionQueue->Entries[CurrentPrio].pActionRoot = pcurEntry->pNextEntry;
        pcurEntry->pNextEntry = NULL;
    }
    if (pActionQueue->Entries[CurrentPrio].pActionRoot == NULL)
    {
        pActionQueue->Entries[CurrentPrio].pInsertEntry = NULL;
        pActionQueue->Entries[CurrentPrio].pLastEntry = NULL;
    }
    if (pLastEntry)
        pActionQueue->AddToFreeList(pLastEntry); // release entry
    pLastEntry = pcurEntry;
    return pcurEntry;
}

GFxMovieRoot::ActionQueueSessionIterator::ActionQueueSessionIterator(struct GFxMovieRoot::ActionQueueType* pactionQueue, UInt sessionId) :
ActionQueueIterator(pactionQueue), SessionId(sessionId)
{
}

const GFxMovieRoot::ActionEntry* GFxMovieRoot::ActionQueueSessionIterator::getNext()
{
    if (pActionQueue->ModId != ModId)
    {
        // new actions were added - restart
        CurrentPrio = 0;
        ModId = pActionQueue->ModId;
    }
    ActionEntry* pcurEntry = NULL;
    for(;CurrentPrio < AP_Count; ++CurrentPrio)
    {
        pcurEntry = pActionQueue->Entries[CurrentPrio].pActionRoot;
        ActionEntry* pprevEntry = NULL;

        // now search for the node with appropriate session id. Once it is found - 
        // save it and remove from the list
        while(pcurEntry && pcurEntry->SessionId != SessionId)
        {
            pprevEntry = pcurEntry;
            pcurEntry = pcurEntry->pNextEntry;
        }

        if (pcurEntry)
        {
            if (!pprevEntry)
                pActionQueue->Entries[CurrentPrio].pActionRoot = pcurEntry->pNextEntry;
            else
                pprevEntry->pNextEntry = pcurEntry->pNextEntry;

            if (!pcurEntry->pNextEntry)
                pActionQueue->Entries[CurrentPrio].pLastEntry = pprevEntry;

            if (pcurEntry == pActionQueue->Entries[CurrentPrio].pInsertEntry)
            {
                if (!pcurEntry->pNextEntry)
                    pActionQueue->Entries[CurrentPrio].pInsertEntry = pprevEntry;
                else
                    pActionQueue->Entries[CurrentPrio].pInsertEntry = pcurEntry->pNextEntry;
            }

            pcurEntry->pNextEntry = NULL;
            break;
        }
    }
    if (pLastEntry)
        pActionQueue->AddToFreeList(pLastEntry); // release entry
    pLastEntry = pcurEntry;
    return pcurEntry;
}


void    GFxMovieRoot::DoActions()
{
    // Execute the actions in the action list, in the given environment.
    // NOTE: Actions can add other actions to the end of the list while running.
    GFxMovieRoot::ActionQueueIterator iter(&ActionQueue);
    const ActionEntry* paction;
    while((paction = iter.getNext()) != NULL)
    {
        paction->Execute(this);
    }
}

void    GFxMovieRoot::DoActionsForSession(UInt sessionId)
{
    // Execute the actions in the action list, in the given environment.
    // NOTE: Actions can add other actions to the end of the list while running.
    GFxMovieRoot::ActionQueueSessionIterator iter(&ActionQueue, sessionId);
    const ActionEntry* paction;
    while((paction = iter.getNext()) != NULL)
    {
        paction->Execute(this);
    }
}

// Executes actions in this entry
void    GFxMovieRoot::ActionEntry::Execute(GFxMovieRoot *proot) const
{   
    GPtr<GFxASCharacter> pcharacter = pCharacter->ResolveCharacter(proot);
    if (!pcharacter)
        return;

    switch(Type)
    {
    case Entry_Buffer:      
        // Sprite is already AddRefd' so it's guaranteed to stick around.
        pcharacter->ExecuteBuffer(pActionBuffer);
        break;

    case Entry_Event:       
        pcharacter->ExecuteEvent(EventId);      
        break;

    case Entry_Function:        
        pcharacter->ExecuteFunction(Function, FunctionParams);      
        break;

    case Entry_CFunction:        
        pcharacter->ExecuteCFunction(CFunction, FunctionParams);      
        break;

    case Entry_None:
        break;
    }
}


// *** GFxMovieRoot's GFxMovieView interface implementation


void    GFxMovieRoot::SetViewport(const GViewport& viewDesc)
{
    SInt prevWidth = Viewport.Width;
    SInt prevHeight = Viewport.Height;
    Float prevScale = Viewport.Scale;
    Float prevRatio = Viewport.AspectRatio;

    ViewportSet     = 1;
    Viewport        = viewDesc;

    if (Viewport.BufferHeight <= 0 || Viewport.BufferWidth <= 0)
        GFC_DEBUG_WARNING(1, "GFxMovieRoot::SetViewport: buffer size not set");

    UpdateViewport();

    if ((ViewScaleMode == SM_NoScale && (prevWidth != Viewport.Width || prevHeight != Viewport.Height || prevScale != Viewport.Scale || prevRatio != Viewport.AspectRatio)) ||
        (ViewScaleMode != SM_ExactFit && (prevWidth != Viewport.Width || prevHeight != Viewport.Height || prevRatio != Viewport.AspectRatio)))
    {
        if (GetLevelMovie(0))
        {
            // queue onResize
            GFxMovieRoot::ActionEntry* pe = InsertEmptyAction(GFxMovieRoot::AP_Frame);
            if (pe) pe->SetAction(GetLevelMovie(0), GASStageCtorFunction::NotifyOnResize, NULL);
        }
    }
}

void    GFxMovieRoot::UpdateViewport()
{
    // calculate frame rect to be displayed and update    
    if (pLevel0Def)
    {
        // We need to calculate VisibleFrameRect depending on scaleMode and alignment. Alignment is significant only for NoScale mode; 
        // otherwise it is ignored. 
        // Renderer will use VisibleFrameRect to inscribe it into Viewport's rectangle. VisibleFrameRect coordinates are in twips,
        // Viewport's coordinates are in pixels. For simplest case, for ExactFit mode VisibleFrameRect is equal to FrameSize, thus,
        // viewport is filled by the whole scene (the original aspect ratio is ignored in this case, the aspect ratio of viewport
        // is used instead).
        // ViewOffsetX and ViewOffsetY is calculated in stage pixels (not viewport's ones).

        // Viewport rectangle recalculated to twips.
        GRectF viewportRect(PixelsToTwips(Float(Viewport.Left)), 
            PixelsToTwips(Float(Viewport.Top)), 
            PixelsToTwips(Float(Viewport.Left + Viewport.Width)), 
            PixelsToTwips(Float(Viewport.Top + Viewport.Height)));
        const Float viewWidth = viewportRect.Width();
        const Float viewHeight = viewportRect.Height();
        const Float frameWidth = pLevel0Def->GetFrameRectInTwips().Width();
        const Float frameHeight = pLevel0Def->GetFrameRectInTwips().Height();
        switch(ViewScaleMode)
        {
        case SM_NoScale:
            {
                // apply Viewport.Scale and .AspectRatio first
                const Float scaledViewWidth  = viewWidth * Viewport.AspectRatio * Viewport.Scale;
                const Float scaledViewHieght = viewHeight * Viewport.Scale;

                // calculate a VisibleFrameRect
                switch(ViewAlignment)
                {
                case Align_Center:
                    // We need to round centering to pixels in noScale mode, otherwise we lose pixel center alignment.
                    VisibleFrameRect.Left = (Float)PixelsToTwips((int)TwipsToPixels(frameWidth/2 - scaledViewWidth/2));
                    VisibleFrameRect.Top  = (Float)PixelsToTwips((int)TwipsToPixels(frameHeight/2 - scaledViewHieght/2));
                    break;
                case Align_TopLeft:
                    VisibleFrameRect.Left = 0;
                    VisibleFrameRect.Top  = 0;
                    break;
                case Align_TopCenter:
                    VisibleFrameRect.Left = (Float)PixelsToTwips((int)TwipsToPixels(frameWidth/2 - scaledViewWidth/2));
                    VisibleFrameRect.Top  = 0;
                    break;
                case Align_TopRight:
                    VisibleFrameRect.Left = frameWidth - scaledViewWidth;
                    VisibleFrameRect.Top  = 0;
                    break;
                case Align_BottomLeft:
                    VisibleFrameRect.Left = 0;
                    VisibleFrameRect.Top  = frameHeight - scaledViewHieght;
                    break;
                case Align_BottomCenter:
                    VisibleFrameRect.Left = (Float)PixelsToTwips((int)TwipsToPixels(frameWidth/2 - scaledViewWidth/2));
                    VisibleFrameRect.Top  = frameHeight - scaledViewHieght;
                    break;
                case Align_BottomRight:
                    VisibleFrameRect.Left = frameWidth - scaledViewWidth;
                    VisibleFrameRect.Top  = frameHeight - scaledViewHieght;
                    break;
                case Align_CenterLeft:
                    VisibleFrameRect.Left = 0;
                    VisibleFrameRect.Top  = (Float)PixelsToTwips((int)TwipsToPixels(frameHeight/2 - scaledViewHieght/2));
                    break;
                case Align_CenterRight:
                    VisibleFrameRect.Left = frameWidth - scaledViewWidth;
                    VisibleFrameRect.Top  = (Float)PixelsToTwips((int)TwipsToPixels(frameHeight/2 - scaledViewHieght/2));
                    break;
                }
                VisibleFrameRect.SetWidth(scaledViewWidth);
                VisibleFrameRect.SetHeight(scaledViewHieght);
                ViewOffsetX = TwipsToPixels(VisibleFrameRect.Left);
                ViewOffsetY = TwipsToPixels(VisibleFrameRect.Top);
                ViewScaleX = Viewport.AspectRatio * Viewport.Scale;
                ViewScaleY = Viewport.Scale;
                break;
            }
        case SM_ShowAll: 
        case SM_NoBorder: 
            {
                const Float viewWidthWithRatio = viewWidth * Viewport.AspectRatio;
                // For ShowAll and NoBorder we need to apply AspectRatio to viewWidth in order
                // to calculate correct VisibleFrameRect and scales.
                // Scale is ignored for these modes.
                if ((ViewScaleMode == SM_ShowAll && viewWidthWithRatio/frameWidth < viewHeight/frameHeight) ||
                    (ViewScaleMode == SM_NoBorder && viewWidthWithRatio/frameWidth > viewHeight/frameHeight))
                {
                    Float visibleHeight = frameWidth * viewHeight / viewWidthWithRatio;    
                    VisibleFrameRect.Left = 0;
                    VisibleFrameRect.Top = frameHeight/2 - visibleHeight/2;
                    VisibleFrameRect.SetWidth(frameWidth);
                    VisibleFrameRect.SetHeight(visibleHeight);

                    ViewOffsetX = 0;
                    ViewOffsetY = TwipsToPixels(VisibleFrameRect.Top);
                    ViewScaleX = viewWidth ? (frameWidth / viewWidth) : 0.0f;
                    ViewScaleY = ViewScaleX / Viewport.AspectRatio;
                }
                else 
                {
                    Float visibleWidth = frameHeight * viewWidthWithRatio / viewHeight;    
                    VisibleFrameRect.Left = frameWidth/2 - visibleWidth/2;
                    VisibleFrameRect.Top = 0;
                    VisibleFrameRect.SetWidth(visibleWidth);
                    VisibleFrameRect.SetHeight(frameHeight);

                    ViewOffsetX = TwipsToPixels(VisibleFrameRect.Left);
                    ViewOffsetY = 0;
                    ViewScaleY = viewHeight ? (frameHeight / viewHeight) : 0.0f;
                    ViewScaleX = ViewScaleY * Viewport.AspectRatio;
                }
            }
            break;
        case SM_ExactFit: 
            // AspectRatio and Scale is ignored for this mode.
            VisibleFrameRect.Left = VisibleFrameRect.Top = 0;
            VisibleFrameRect.SetWidth(frameWidth);
            VisibleFrameRect.SetHeight(frameHeight);
            ViewOffsetX = ViewOffsetY = 0;
            ViewScaleX = viewWidth  ? (VisibleFrameRect.Width() / viewWidth) : 0.0f;
            ViewScaleY = viewHeight ? (VisibleFrameRect.Height() / viewHeight) : 0.0f;
            break;
        }
        PixelScale = GTL::gmax(1.0f/ViewScaleX, 1.0f/ViewScaleY);
    }
    else
    {
        ViewOffsetX = ViewOffsetY = 0;
        ViewScaleX = ViewScaleY = 1.0f;
        PixelScale = GFC_TWIPS_TO_PIXELS(20.0f);
    }
  //  RendererInfo.ViewportScale = PixelScale;
    //  RendererInfo.ViewportScale = PixelScale;

    ViewportMatrix = GMatrix2D::Translation(-VisibleFrameRect.Left, -VisibleFrameRect.Top);
    ViewportMatrix.AppendScaling(Viewport.Width / VisibleFrameRect.Width(),
                                 Viewport.Height / VisibleFrameRect.Height());
    ViewportMatrix.AppendTranslation(Float(Viewport.Left), Float(Viewport.Top));
  //  RendererInfo.ViewportScale = PixelScale;
}

void    GFxMovieRoot::GetViewport(GViewport *pviewDesc) const
{
    *pviewDesc = Viewport;
}

void    GFxMovieRoot::SetViewScaleMode(ScaleModeType scaleMode)
{
    ViewScaleMode = scaleMode;
    UpdateViewport();
}

void    GFxMovieRoot::SetViewAlignment(AlignType align)
{
    ViewAlignment = align;
    UpdateViewport();
}

GFxASCharacter* GFxMovieRoot::GetTopMostEntity(const GPointF& mousePos, bool testAll)
{
    GFxASCharacter* ptopMouseCharacter = 0;
    for (int movieIndex = (int)MovieLevels.size(); movieIndex > 0; movieIndex--)
    {   
        GFxSprite* pmovie  = MovieLevels[movieIndex-1].pSprite;
        ptopMouseCharacter = pmovie->GetTopMostMouseEntity(mousePos, testAll);
        if (ptopMouseCharacter)
            break;
    }
    return ptopMouseCharacter;
}

Float    GFxMovieRoot::Advance(Float deltaT, bool frameCatchUp)
{    
    if (!MovieLevels.size())
        return pLevel0Def ? (1.0f / pLevel0Def->GetFrameRate()) : 0.0f;

    // Need to restore high precision mode of FPU for X86 CPUs.
    // Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
    // leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
    // be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
    GFxDoublePrecisionGuard dpg;

    // Index used in iterating through root movies.
    UInt   movieIndex;

    if (deltaT < 0.0f)
    {
        GFC_DEBUG_WARNING(1, "GFxMovieView::Advance - negative deltaT argument specified");
        deltaT = 0.0f;
    }

    AdvanceCalled = 1;

    // Advance - Cache states for faster non-locked access.
    GPtr<GFxActionControl>           pac;
    GFxState*                        pstates[5]    = {0,0,0,0,0};    
    const static GFxState::StateType stateQuery[5] =
    { GFxState::State_UserEventHandler, GFxState::State_FSCommandHandler,
      GFxState::State_ExternalInterface, GFxState::State_ActionControl,
      GFxState::State_Log };
  
    // Get multiple states at once to avoid extra locking.
    pSharedState->GetStatesAddRef(pstates, stateQuery, 5);
    pUserEventHandler   = *(GFxUserEventHandler*)  pstates[0];
    pFSCommandHandler   = *(GFxFSCommandHandler*)  pstates[1];
    pExtIntfHandler     = *(GFxExternalInterface*) pstates[2];
    pac                 = *(GFxActionControl*)     pstates[3];
    pCachedLog          = *(GFxLog*)               pstates[4];

    // Set flag indicating that the log is cached.
    CachedLogFlag       = 1;    
    
    if (pac)
    {
        VerboseAction        = (pac->GetActionFlags() & GFxActionControl::Action_Verbose) != 0;
        SuppressActionErrors = (pac->GetActionFlags() & GFxActionControl::Action_ErrorSuppress) != 0;
    }
    else
    {
        VerboseAction        = 0;
        SuppressActionErrors = 0;
    }    

    // Capture Frames loaded so that the value of a loaded frame will be consistent
    GFxSpriteRootNode *pspriteNode = RootSpriteNodes.GetFirst();
    // TBD: Need to query DefImpl only once so that LoadingFrame is consistent for all loaded movies,
    // independt of the order they are processed.
    while(!RootSpriteNodes.IsNull(pspriteNode))
    {        
        // Bytes loaded must always be grabbed before the frame, since there 
        // is a data race between value returned 'getBytesLoaded' and the frame
        // we can seek to.
        if (!pspriteNode->ImportFlag)
        {
            GFxMovieDefImpl* pspriteMovieDefImpl = pspriteNode->pSprite->GetResourceMovieDef();

            pspriteNode->BytesLoaded  = pspriteMovieDefImpl->GetBytesLoaded();
            pspriteNode->LoadingFrame = pspriteMovieDefImpl->GetLoadingFrame();
        }        
        pspriteNode = pspriteNode->pNext;
    }


    if ((pLevel0Movie->GetLoadingFrame() == 0))
    {
        CachedLogFlag = 0;
        return 0;
    }

 

    // Execute loading/frame0 events for root level clips, if necessary.
    if (LevelClipsChanged && (pLevel0Movie->GetLoadingFrame() > 0))
    {
        LevelClipsChanged = false;      
        // Queue up load events, tags and actions for frame 0.
        for (movieIndex = (UInt)MovieLevels.size(); movieIndex > 0; movieIndex--)
        {
            // ExecuteFrame0Events does an internal check, so this will only have an effect once.
            MovieLevels[movieIndex-1].pSprite->ExecuteFrame0Events();
        }

        // It is important to execute frame 0 events here before we get to AdvanceFrame() below,
        // so that actions like stop() are applied in timely manner.
        DoActions();
        ProcessLoadQueue();
    }

    // *** Advance the frame based on time

    TimeElapsed += deltaT;
    TimeRemainder += deltaT;    

    Float minDelta = FrameTime;

    if (IntervalTimers.size() > 0)
    {
        UPInt i, n;
        UInt needCompress = 0;
        for (i = 0, n = IntervalTimers.size(); i < n; ++i)
        {
            if (IntervalTimers[i] && IntervalTimers[i]->IsActive())
            {
                IntervalTimers[i]->Invoke(this, FrameTime);

                Float delta = IntervalTimers[i]->GetNextInvokeTime() - TimeElapsed;
                if (delta < minDelta)
                    minDelta = delta;
            }
            else
                ++needCompress;
        }
        if (needCompress)
        {
            n = IntervalTimers.size(); // size could be changed after Invoke
            UInt j;
            // remove empty entries
            for (i = j = 0; i < n; ++i)
            {
                if (!IntervalTimers[j] || !IntervalTimers[j]->IsActive())
                {
                    delete IntervalTimers[j];
                    IntervalTimers.remove(j);
                }
                else
                    ++j;
            }
        }

    }


    // *** Handle the mouse

    if ( MouseSupportEnabled &&
        (NeedMouseUpdate || MouseMoved || (MouseButtonState.MouseButtonStateLast != (UInt)MouseButtons) || MouseWheelScroll) )
    {
        int     lastState = MouseButtonState.MouseButtonStateLast;
        GPointF mousePos(GFC_PIXELS_TO_TWIPS(MouseX), GFC_PIXELS_TO_TWIPS(MouseY));

        // Find topmost clip from the top level down.
        GPtr<GFxASCharacter> ptopMouseCharacter = GetTopMostEntity(mousePos, false);
        MouseButtonState.TopmostEntity = ptopMouseCharacter;    
        MouseButtonState.MouseButtonStateCurrent = MouseButtons;

        /*//Debug code
        if (lastState != (MouseButtons & 1)) 
        {
        if (ptopMouseCharacter)  //?
        printf("! %s\n", ptopMouseCharacter->GetCharacterHandle()->GetNamePath().ToCStr());
        } 
        if (ptopMouseCharacter)  //?
        {
        printf("? %s\n", ptopMouseCharacter->GetCharacterHandle()->GetNamePath().ToCStr());
        }*/ 

        // Send mouse events.
        if (MouseMoved || ((lastState & MouseButton_Left) != (MouseButtons & MouseButton_Left))) 
        {
            GFxEventId::IdCode buttonEvent = (MouseButtons & MouseButton_Left) ?
                GFxEventId::Event_MouseDown : GFxEventId::Event_MouseUp;

            for (movieIndex = (UInt)MovieLevels.size(); movieIndex > 0; movieIndex--)
            {
                GFxSprite* pmovie = MovieLevels[movieIndex-1].pSprite;

                // Send mouse up/down events.
                if ((lastState & MouseButton_Left) != (MouseButtons & MouseButton_Left))
                    pmovie->PropagateMouseEvent(UByte(buttonEvent));
                // Send move.
                if (MouseMoved)
                    pmovie->PropagateMouseEvent(GFxEventId::Event_MouseMove);
            }
            if (MouseMoved)
            {
                if (!DisableFocusAutoRelease.IsTrue())
                    HideFocusRect();
            }
        }

        if (MouseWheelScroll)
        {
            // no need to propagate the mouse wheel, just sent 
            if (ptopMouseCharacter)
                ptopMouseCharacter->OnMouseWheelEvent(MouseWheelScroll);
        }

        if (pASMouseListener && !pASMouseListener->IsEmpty() && (MouseMoved || lastState != MouseButtons || MouseWheelScroll))
        {
            // Notify Mouse class' AS listeners

            GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
            bool extensions = penv->CheckExtensions();
            if (MouseMoved)
                pASMouseListener->OnMouseMove(penv);
            if (MouseWheelScroll || lastState != MouseButtons)
            {
                GPtr<GFxASCharacter> ptopMouseCharacter = GetTopMostEntity(mousePos, true);
                if (MouseWheelScroll)
                    pASMouseListener->OnMouseWheel(penv, MouseWheelScroll, ptopMouseCharacter);
                if (lastState != MouseButtons)
                {
                    for (UInt mask = 1, buttonId = 1; MouseButton_AllMask & mask; mask <<= 1, ++buttonId)
                    {
                        if ((lastState & mask) != (MouseButtons & mask))
                        {
                            if (MouseButtons & mask)
                                pASMouseListener->OnMouseDown(penv, buttonId, ptopMouseCharacter);
                            else
                                pASMouseListener->OnMouseUp(penv, buttonId, ptopMouseCharacter);
                        }
                        if (!extensions) break; // support only left button, if extensions are not enabled
                    }
                }
            }
        }
        // check for necessity of changing cursor
        if (MouseMoved)
        {
            if (ptopMouseCharacter != pPrevTopMostEntityRawPtr)
            {
                UInt newCursorType = GFxMouseCursorEvent::ARROW;
                if (ptopMouseCharacter)
                    newCursorType = ptopMouseCharacter->GetCursorType();
                if (newCursorType != MouseCursorType)
                {
                    GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
                    if (penv->CheckExtensions())
                    {
                        // extensions enabled
                        // find Mouse.setCursorType and invoke it
                        GASValue objVal;
                        if (penv->GetGC()->pGlobal->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_Mouse), &objVal))
                        {
                            GASObject* pobj = objVal.ToObject();
                            if (pobj)
                            {
                                GASValue scval;
                                if (pobj->GetMember(penv, penv->GetBuiltin(GASBuiltin_setCursorType), &scval))
                                {
                                    GASFunctionRef funcRef = scval.ToFunction();
                                    if (!funcRef.IsNull())
                                    {
                                        GASValue res;
                                        penv->Push(GASNumber(newCursorType));
                                        funcRef(GASFnCall(&res, objVal, penv, 1, penv->GetTopIndex()));
                                        penv->Drop1();    
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        // extensions disabled; just call static method GASMouseCtorFunction::SetCursorType
                        GASMouseCtorFunction::SetCursorType(this, newCursorType);
                    }
                    MouseCursorType = newCursorType;
                }
            }
            pPrevTopMostEntityRawPtr = ptopMouseCharacter;
        }

        // Send onKillFocus, if neccessary
        if (MouseButtonState.MouseButtonStateLast & MouseButton_Left) // mouse button was down
        {
            FocusMouseInputMoved(ptopMouseCharacter);
        }
        MouseMoved = 0;
        MouseWheelScroll = 0;

        //!AB: button's events, such as onPress/onRelease should be fired AFTER
        // appropriate mouse events (onMouseDown/onMouseUp).
        GFx_GenerateMouseButtonEvents(&MouseButtonState);
        // MouseButtonState.MouseButtonStateLast is updated in GenerateMouseButtonEvents
        NeedMouseUpdate = 0;
    }


    // *** Handle keyboard
    {
        ProcessFocusKeyInfo focusKeyInfo;
        GPtr<GFxASCharacter> curFocused = LastFocused;
        while (!pKeyboardState->IsQueueEmpty ())
        {
            short               code;
            UByte               asciiCode;
            UInt32              wcharCode;
            GFxSpecialKeysState specialKeysState;
            GFxEvent::EventType event;
            int                 keyMask = 0;
            GASEnvironment*     penv = pLevel0Movie->GetASEnvironment();
            GASStringContext*   psc = penv->GetSC();

            if (pKeyboardState->GetQueueEntry(&code, &asciiCode, &wcharCode, &event, &specialKeysState))
            {
                if (event == GFxEvent::KeyDown || event == GFxEvent::KeyUp)
                {
                    GFxEventId::IdCode eventIdCode = (event == GFxEvent::KeyDown) ? 
                        GFxEventId::Event_KeyDown : GFxEventId::Event_KeyUp;
                    GFxEventId         eventId((UByte)eventIdCode, code, asciiCode, wcharCode);
                    eventId.SpecialKeysState = specialKeysState;

                    //printf ("Key %d (ASCII %d) is %s\n", code, asciiCode, (event == GFxEvent::KeyDown) ? "Down":"Up");

                    for (movieIndex = (UInt)MovieLevels.size(); movieIndex > 0; movieIndex--)
                        MovieLevels[movieIndex-1].pSprite->PropagateKeyEvent(eventId, &keyMask);

                    pKeyboardState->NotifyListeners(psc, code, asciiCode, wcharCode, event);

                    ProcessFocusKey(code, event, specialKeysState, &focusKeyInfo);
                }
                else if (event == GFxEvent::CharEvent)
                {
                    if (curFocused)
                    {
                        curFocused->OnCharEvent(wcharCode);
                    }
                }
            }
        }
        FinalizeProcessFocusKey(&focusKeyInfo);
    }

    if (TimeRemainder >= FrameTime)
    {
        // Mouse, keyboard and timer handlers above may queue up actions which MUST be executed before the
        // Advance. For example, if a mouse event contains "gotoAndPlay(n)" action and Advance is
        // going to queue up a frame with "stop" action, then that gotoAndPlay will be incorrectly stopped.
        // By executing actions before the Advance we avoid queuing the stop-frame and gotoAndPlay will
        // work correctly. (AB, 12/13/06)
        DoActions();

        do
        {
            if (frameCatchUp)
                TimeRemainder -= FrameTime;
            else
                TimeRemainder = (Float)fmod(TimeRemainder, FrameTime);

            Float framePos = (TimeRemainder >= FrameTime) ? 0.0f : (TimeRemainder / FrameTime);

            // Advance a frame.
            for (movieIndex = (UInt)MovieLevels.size(); movieIndex > 0; movieIndex--)
            {
                GFxSprite* pmovie = MovieLevels[movieIndex-1].pSprite;
                pmovie->AdvanceFrame(1, framePos);
            }       

            // Execute actions queued up due to actions and mouse.
            DoActions();
            ProcessLoadQueue();

            // Advance hit-test cache. Used mostly for mouse, but also from ActionScript.
            PointTestCache.NextFrame();

        } while(TimeRemainder >= FrameTime);

        // Force GetTopmostMouse update in next Advance so that buttons detect change, if any.
        NeedMouseUpdate = 1;        
    }
    else
    {
        // Fractional advance only.
        Float framePos = TimeRemainder / FrameTime;

        for (movieIndex = (UInt)MovieLevels.size(); movieIndex > 0; movieIndex--)
        {
            GFxSprite* pmovie = MovieLevels[movieIndex-1].pSprite;
            pmovie->AdvanceFrame(0, framePos);
        }   

        TimeRemainder = (Float)fmod(TimeRemainder, FrameTime);

        // Technically AdvanceFrame(0) above should not generate any actions.
        // However, we need to execute actions queued up due to mouse.
        DoActions();
        ProcessLoadQueue();
    }    

    CachedLogFlag = 0;

    return GTL::gmin(minDelta, FrameTime - TimeRemainder);
}



void    GFxMovieRoot::HandleEvent(const GFxEvent &event)
{
    union
    {
        const GFxEvent *        pevent;
        const GFxKeyEvent *     pkeyEvent;
        const GFxMouseEvent *   pmouseEvent;
        const GFxCharEvent *    pcharEvent;
    };
    pevent = &event;

    // Process the event type

    switch(event.Type)
    {
        // Mouse.
    case GFxEvent::MouseMove:
        GFC_DEBUG_WARNING(event.EventClassSize != sizeof(GFxMouseEvent),
            "GFxMouseEvent class required for mouse events");
        NotifyMouseState(pmouseEvent->x, pmouseEvent->y, MouseButtons);
        break;

    case GFxEvent::MouseUp:
        GFC_DEBUG_WARNING(event.EventClassSize != sizeof(GFxMouseEvent),
            "GFxMouseEvent class required for mouse events");
        MouseButtons &= ~(1 << pmouseEvent->Button);
        NotifyMouseState(pmouseEvent->x, pmouseEvent->y, MouseButtons);
        break;

    case GFxEvent::MouseDown:
        GFC_DEBUG_WARNING(event.EventClassSize != sizeof(GFxMouseEvent),
            "GFxMouseEvent class required for mouse events");
        MouseButtons |= 1 << pmouseEvent->Button;
        NotifyMouseState(pmouseEvent->x, pmouseEvent->y, MouseButtons);
        break;

    case GFxEvent::MouseWheel:
        GFC_DEBUG_WARNING(event.EventClassSize != sizeof(GFxMouseEvent),
            "GFxMouseEvent class required for mouse events");
        MouseX      = (SInt)(pmouseEvent->x * ViewScaleX + ViewOffsetX);
        MouseY      = (SInt)(pmouseEvent->y * ViewScaleY + ViewOffsetY);
        MouseWheelScroll = (SInt)pmouseEvent->ScrollDelta;
        break;


        // Keyboard.
    case GFxEvent::KeyDown:
        GFC_DEBUG_WARNING(event.EventClassSize != sizeof(GFxKeyEvent),
            "GFxKeyEvent class required for keyboard events");
        if (pKeyboardState)
        {
            if (pkeyEvent->WcharCode < 32 || pkeyEvent->WcharCode == 127)
            {
                pKeyboardState->SetKeyToggled(GFxKey::NumLock, pkeyEvent->SpecialKeysState.IsNumToggled());
                pKeyboardState->SetKeyToggled(GFxKey::CapsLock, pkeyEvent->SpecialKeysState.IsCapsToggled());
                pKeyboardState->SetKeyToggled(GFxKey::ScrollLock, pkeyEvent->SpecialKeysState.IsScrollToggled());
                pKeyboardState->SetKeyDown(pkeyEvent->KeyCode, pkeyEvent->AsciiCode, pkeyEvent->SpecialKeysState);
            }
            else
            {
                pKeyboardState->SetChar(pkeyEvent->WcharCode);
            }
        }
        break;

    case GFxEvent::KeyUp:
        GFC_DEBUG_WARNING(event.EventClassSize != sizeof(GFxKeyEvent),
            "GFxKeyEvent class required for keyboard events");
        if (pKeyboardState)
        {
            if (pkeyEvent->WcharCode < 32 || pkeyEvent->WcharCode == 127)
            {
                pKeyboardState->SetKeyToggled(GFxKey::NumLock, pkeyEvent->SpecialKeysState.IsNumToggled());
                pKeyboardState->SetKeyToggled(GFxKey::CapsLock, pkeyEvent->SpecialKeysState.IsCapsToggled());
                pKeyboardState->SetKeyToggled(GFxKey::ScrollLock, pkeyEvent->SpecialKeysState.IsScrollToggled());
                pKeyboardState->SetKeyUp(pkeyEvent->KeyCode, pkeyEvent->AsciiCode, pkeyEvent->SpecialKeysState);
            }
        }
        break;

    case GFxEvent::CharEvent:
        GFC_DEBUG_WARNING(event.EventClassSize != sizeof(GFxCharEvent),
            "GFxCharEvent class required for keyboard events");
        if (pKeyboardState)
        {
            pKeyboardState->SetChar(pcharEvent->WcharCode);
        }
        break;

    default:
        break;
    }
}


// The host app uses this to tell the GFxMovieSub where the
// user's mouse pointer is.
void    GFxMovieRoot::NotifyMouseState(int x, int y, int buttons)
{
    x   = (SInt)(x * ViewScaleX + ViewOffsetX);
    y   = (SInt)(y * ViewScaleY + ViewOffsetY);

    if ((MouseX != x) || (MouseY != y))
    {
        MouseX = x;
        MouseY = y;
        MouseMoved = 1;
    }   
    MouseButtons = buttons;
}


// Use this to retrieve the last state of the mouse, as set via
// NotifyMouseState().  Coordinates are in PIXELS, NOT TWIPS.
void    GFxMovieRoot::GetMouseState(int* x, int* y, int* buttons)
{
    GASSERT(x);
    GASSERT(y);
    GASSERT(buttons);

    *x = MouseX;
    *y = MouseY;
    *buttons = MouseButtons;
}

bool    GFxMovieRoot::HitTest(int x, int y, HitTestType testCond)
{
    int mx  = (SInt)(x * ViewScaleX + ViewOffsetX);
    int my  = (SInt)(y * ViewScaleY + ViewOffsetY);

    UInt movieIndex;
    for (movieIndex = (UInt)MovieLevels.size(); movieIndex > 0; movieIndex--)
    {
        GFxSprite*  pmovie  = MovieLevels[movieIndex-1].pSprite;

        GRectF      movieLocalBounds = pmovie->GetBounds(GRenderer::Matrix());

        GPointF     pt( (Float)GFC_PIXELS_TO_TWIPS(mx),
            (Float)GFC_PIXELS_TO_TWIPS(my) );

        GPointF     ptMv = pmovie->GetWorldMatrix().TransformByInverse(pt);

        if (movieLocalBounds.Contains(ptMv))
        {
            switch(testCond)
            {
            case HitTest_Bounds:
                if (pmovie->PointTestLocal(ptMv, false))
                    return 1;
                break;
            case HitTest_Shapes:
                if (pmovie->PointTestLocal(ptMv, true))
                    return 1;
                break;
            case HitTest_ButtonEvents:
                if (pmovie->GetTopMostMouseEntity(ptMv))
                    return 1;
                break;
            }
        }
    }
    return 0;
}

bool        GFxMovieRoot::EnableMouseSupport(bool mouseEnabled)
{
    bool oldState = MouseSupportEnabled;
    MouseSupportEnabled = mouseEnabled;
    return oldState;
}


bool    GFxMovieRoot::AttachDisplayCallback(const char* pathToObject, void (*callback)(void*), void* userPtr)
{   
    if (!pLevel0Movie)
        return 0;

    GASEnvironment*     penv    = pLevel0Movie->GetASEnvironment();
    GASValue            obj;
    if (penv->GetVariable(penv->CreateString(pathToObject), &obj))
    {
        GFxASCharacter*     pchar   = obj.ToASCharacter(penv);

        if (pchar)
        {
            pchar->SetDisplayCallback(callback, userPtr);
            return true;
        }
    }
    return false;
}


int    GFxMovieRoot::AddIntervalTimer(GASIntervalTimer *timer)
{
    timer->SetId(++LastIntervalTimerId);
    IntervalTimers.push_back(timer);
    return LastIntervalTimerId;
}

void    GFxMovieRoot::ClearIntervalTimer(int timerId)
{
    for (UPInt i = 0, n = IntervalTimers.size(); i < n; ++i)
    {
        if (IntervalTimers[i] && IntervalTimers[i]->GetId() == timerId)
        {
            // do not remove the timer's array's entry here, just null it;
            // it will be removed later in Advance.
            IntervalTimers[i]->Clear();
            return;
        }
    }
}

void    GFxMovieRoot::ShutdownTimers()
{
    for (UPInt i = 0, n = IntervalTimers.size(); i < n; ++i)
    {
        delete IntervalTimers[i];
    }
    IntervalTimers.clear();
}


// *** GFxMovieRoot's GFxMovie interface implementation

// These methods mostly delegate to _level0 movie.

GFxMovieDef*    GFxMovieRoot::GetMovieDef() const
{   
    return pLevel0Def;
}
UInt    GFxMovieRoot::GetCurrentFrame() const
{
    return pLevel0Movie ? pLevel0Movie->GetCurrentFrame() : 0;
}
bool    GFxMovieRoot::HasLooped() const
{
    return pLevel0Movie ? pLevel0Movie->HasLooped() : 0; 
}
void    GFxMovieRoot::Restart()                         
{       
    if (pLevel0Movie)
    {
        GPtr<GFxMovieDefImpl> prootMovieDef = pLevel0Movie->GetResourceMovieDef();
        ReleaseLevelMovie(0);        

        // Unregister all classes
        pGlobalContext->UnregisterAllClasses();

        // Note: Log is inherited dynamically from Def, so we don't set it here
        GPtr<GFxSprite> prootMovie = *new GFxSprite(prootMovieDef->GetDataDef(), prootMovieDef,
            this, NULL, GFxResourceId());

        if (!prootMovie)
        {
            return;
        }

        // reset mouse state and cursor
        if (pUserEventHandler)
        {
            pUserEventHandler->HandleEvent(this, GFxEvent(GFxEvent::DoShowMouse));
            pUserEventHandler->HandleEvent(this, GFxMouseCursorEvent(GFxMouseCursorEvent::ARROW));
        }
        // reset keyboard state
        pKeyboardState->ResetState();

        // Assign level and _level0 name.
        prootMovie->SetLevel(0);
        SetLevelMovie(0, prootMovie);

        // In case permanent variables were assigned.. check them.
        ResolveStickyVariables(prootMovie);

        Advance(0.0f, 0);
    }
}

void    GFxMovieRoot::GotoFrame(UInt targetFrameNumber) 
{   // 0-based!!
    if (pLevel0Movie) pLevel0Movie->GotoFrame(targetFrameNumber); 
}

bool    GFxMovieRoot::GotoLabeledFrame(const char* label, SInt offset)
{
    if (!pLevel0Movie)
        return 0;

    UInt    targetFrame = GFC_MAX_UINT;
    if (pLevel0Def->GetDataDef()->GetLabeledFrame(label, &targetFrame, 0))
    {
        GotoFrame((UInt)((SInt)targetFrame + offset));
        return 1;
    }
    else
    {
        if (GetLog())
            GetLog()->LogScriptError("Error: MovieImpl::GotoLabeledFrame('%s') unknown label\n", label);
        return 0;
    }
}

void    GFxMovieRoot::Display()
{
    // If there is no _level0, there are no other levels either.
    if (!pLevel0Movie)
        return;

    // Warn in case the user called Display() before Advance().
    GFC_DEBUG_WARNING(!AdvanceCalled, "GFxMovieView::Display called before Advance - no rendering will take place");

    UInt    movieIndex;
    bool    haveVisibleMovie = 0;

    for (movieIndex = 0; movieIndex < MovieLevels.size(); movieIndex++)
    {
        if (MovieLevels[movieIndex].pSprite->GetVisible())
        {
            haveVisibleMovie = 1;
            break;
        }
    }

    // Avoid rendering calls if no movies are visible.
    if (!haveVisibleMovie)
        return;

    GFxDisplayContext context(pSharedState, 
                              pLevel0Def->GetWeakLib(),
                              &pLevel0Def->ResourceBinding,
                              GetPixelScale(),
                              ViewportMatrix);

    // Need to check RenderConfig before GetRenderer.
    if (!context.GetRenderConfig() || !context.GetRenderer())
    {
        GFC_DEBUG_WARNING(1, "GFxMovie::Display failed, no renderer specified");
        return;
    }

    // Cache log to avoid expensive state access.
    CachedLogFlag = 1;
    pCachedLog    = context.pLog;

    context.GetRenderer()->BeginDisplay(   
        BackgroundColor,
        Viewport,
        VisibleFrameRect.Left, VisibleFrameRect.Right,
        VisibleFrameRect.Top, VisibleFrameRect.Bottom);

    
	// Pass matrix on the stack
	StackData rootStackData;
	rootStackData.stackMatrix.SetIdentity();
	rootStackData.stackColor = GRenderer::Cxform();

    // Display all levels. _level0 is at the bottom.
    for (movieIndex = 0; movieIndex < MovieLevels.size(); movieIndex++)
    {
        GFxSprite* pmovie = MovieLevels[movieIndex].pSprite;
        pmovie->Display(context,rootStackData);
    }

    // Display focus rect
    DisplayFocusRect(context);

    context.GetRenderer()->EndDisplay();

    CachedLogFlag = 0;
}


void    GFxMovieRoot::SetPlayState(PlayState s)
{   
    if (pLevel0Movie)
        pLevel0Movie->SetPlayState(s); 
}
GFxMovie::PlayState GFxMovieRoot::GetPlayState() const
{   
    if (!pLevel0Movie) return Stopped;
    return pLevel0Movie->GetPlayState(); 
}

void    GFxMovieRoot::SetVisible(bool visible)
{
    if (pLevel0Movie) pLevel0Movie->SetVisible(visible);
}
bool    GFxMovieRoot::GetVisible() const
{
    return pLevel0Movie ? pLevel0Movie->GetVisible() : 0;
}

void GFxMovieRoot::GFxValue2ASValue(const GFxValue& gfxVal, GASValue* pdestVal) const
{
    GASSERT(pdestVal);
    switch(gfxVal.GetType())
    {
    case GFxValue::VT_Undefined: pdestVal->SetUndefined(); break;
    case GFxValue::VT_Boolean:   pdestVal->SetBool(gfxVal.GetBool()); break;
    case GFxValue::VT_Null:      pdestVal->SetNull(); break;
    case GFxValue::VT_Number:    pdestVal->SetNumber(gfxVal.GetNumber()); break;
    case GFxValue::VT_String:    
        {
            pdestVal->SetString(pGlobalContext->CreateString(gfxVal.GetString())); break;
        }
        break;
    case GFxValue::VT_StringW:    
        {
            pdestVal->SetString(pGlobalContext->CreateString(gfxVal.GetStringW())); break;
        }
        break;

    default:
        break;
    }
}

// Action Script access.
bool GFxMovieRoot::SetVariable(const char* ppathToVar, const GFxValue& value, SetVarType setType)
{
    if (!pLevel0Movie)
        return 0;
    if (!ppathToVar)
    {
        GetLog()->LogError("Error: NULL pathToVar passed to SetVariable/SetDouble()\n");
        return 0;
    }

    GASEnvironment*                 penv = pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(ppathToVar));

    GASValue val;
    GFxValue2ASValue(value, &val);
    bool setResult = pLevel0Movie->GetASEnvironment()->SetVariable(path, val, NULL, 
        (setType == SV_Normal));

    if ( (!setResult && (setType != SV_Normal)) ||
        (setType == SV_Permanent) )
    {
        // Store in sticky hash.
        AddStickyVariable(path, val, setType);
    }
    return setResult;
}

void GFxMovieRoot::ASValue2GFxValue(GASEnvironment* penv, const GASValue& value, GFxValue* pdestVal) const
{
    GASSERT(pdestVal);
    int toType;
    if (pdestVal->GetType() & GFxValue::VT_ConvertBit)
        toType = pdestVal->GetType() & (~GFxValue::VT_ConvertBit);
    else
    {
        switch(value.GetType())
        {
        case GASValue::UNDEFINED:    toType = GFxValue::VT_Undefined; break;
        case GASValue::BOOLEAN:      toType = GFxValue::VT_Boolean; break;
        case GASValue::NULLTYPE:     toType = GFxValue::VT_Null; break;
        case GASValue::NUMBER:       toType = GFxValue::VT_Number; break;
        default:                     toType = GFxValue::VT_String; break;
        }
    }
    switch(toType)
    {
    case GFxValue::VT_Undefined:    pdestVal->SetUndefined(); break;
    case GFxValue::VT_Boolean:      pdestVal->SetBoolean(value.ToBool(penv)); break;
    case GFxValue::VT_Null:         pdestVal->SetNull(); break;
    case GFxValue::VT_Number:       pdestVal->SetNumber(value.ToNumber(penv)); break;
    case GFxValue::VT_String:    
        {
            // Store string into a return value holder, so that it is not released till next call.
            // This is thread safe from the multiple GFxMovieRoot point of view.
            UInt pos = pRetValHolder->StringArrayPos++;
            if (pRetValHolder->StringArray.size() < pRetValHolder->StringArrayPos)
                pRetValHolder->ResizeStringArray(pRetValHolder->StringArrayPos);
            pRetValHolder->StringArray[pos] = value.ToString(penv);
            pdestVal->SetString(pRetValHolder->StringArray[pos].ToCStr());
        }
        break;
    case GFxValue::VT_StringW:    
        {
            GASString str = value.ToString(penv);
            UInt len = str.GetLength();
            wchar_t* pdestBuf = (wchar_t*)pRetValHolder->PreAllocateBuffer((len+1)*sizeof(wchar_t));

            GUTF8Util::DecodeString(pdestBuf,str.ToCStr());
            pdestVal->SetStringW(pdestBuf);
        }
        break;
    }
}

bool GFxMovieRoot::GetVariable(GFxValue *pval, const char* ppathToVar) const
{
    if (!pLevel0Movie || !pval)
        return 0;

    GASEnvironment*                 penv = pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(ppathToVar));

    pRetValHolder->ResetPos();
    pRetValHolder->ResizeStringArray(0);

    GASValue retVal;
    if (penv->GetVariable(path, &retVal))
    {
        ASValue2GFxValue(penv, retVal, pval);
        return true;
    }
    return false;
}

bool GFxMovieRoot::SetVariableArray(SetArrayType type, const char* ppathToVar,
                                    UInt index, const void* pdata, UInt count, SetVarType setType)
{
    if (!pLevel0Movie)
        return false;

    GASEnvironment*                 penv = pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(ppathToVar));

    GPtr<GASArrayObject> parray;
    GASValue retVal;
    if(penv->GetVariable(path, &retVal) && retVal.IsObject())
    {
        GASObject* pobj = retVal.ToObject();
        if (pobj && pobj->GetObjectType() == GASObject::Object_Array)
            parray = static_cast<GASArrayObject*>(pobj);
    }

    if (!parray)
    {
        parray = *new GASArrayObject(pLevel0Movie->GetASEnvironment());
    }
    if (count + index > UInt(parray->GetSize()))
        parray->Resize(count + index);

    switch(type)
    {
    case SA_Int:
        {
            const int* parr = static_cast<const int*>(pdata);
            for (UInt i = 0; i < count; ++i)
            {
                parray->SetElement(i + index, GASValue(parr[i]));
            }
            break;
        }
    case SA_Float:
        {
            const Float* parr = static_cast<const Float*>(pdata);
            for (UInt i = 0; i < count; ++i)
            {
                parray->SetElement(i + index, GASValue(GASNumber(parr[i])));
            }
            break;
        }
    case SA_Double:
        {
            const Double* parr = static_cast<const Double*>(pdata);
            for (UInt i = 0; i < count; ++i)
            {
                parray->SetElement(i + index, GASValue(GASNumber(parr[i])));
            }
            break;
        }
    case SA_Value:
        {
            const GFxValue* parr = static_cast<const GFxValue*>(pdata);
            for (UInt i = 0; i < count; ++i)
            {
                GASValue asVal;
                GFxValue2ASValue(parr[i], &asVal);
                parray->SetElement(i + index, asVal);
            }
            break;
        }
    case SA_String:
        {
            const char* const* parr = static_cast<const char* const*>(pdata);
            for (UInt i = 0; i < count; ++i)
            {
                parray->SetElement(i + index, GASValue(pGlobalContext->CreateString(parr[i])));
            }
            break;
        }
    case SA_StringW:
        {
            const wchar_t* const* parr = static_cast<const wchar_t* const*>(pdata);
            for (UInt i = 0; i < count; ++i)
            {
                parray->SetElement(i + index, GASValue(pGlobalContext->CreateString(parr[i])));
            }
            break;
        }
    }

    GASValue val;
    val.SetAsObject(parray);

    bool setResult = pLevel0Movie->GetASEnvironment()->SetVariable(path, val, NULL, 
        (setType == SV_Normal));

    if ( (!setResult && (setType != SV_Normal)) ||
        (setType == SV_Permanent) )
    {
        // Store in sticky hash.
        AddStickyVariable(path, val, setType);
    }
    return setResult;
}

bool GFxMovieRoot::SetVariableArraySize(const char* ppathToVar, UInt count, SetVarType setType)
{
    if (!pLevel0Movie)
        return false;

    GASEnvironment*                 penv = pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(ppathToVar));

    GPtr<GASArrayObject> parray;
    GASValue retVal;
    if(penv->GetVariable(path, &retVal) && retVal.IsObject())
    {
        GASObject* pobj = retVal.ToObject();
        if (pobj && pobj->GetObjectType() == GASObject::Object_Array)
        {
            // Array already exists - just resize
            parray = static_cast<GASArrayObject*>(pobj);
            if (count != UInt(parray->GetSize()))
                parray->Resize(count);
            return true;
        }
    }

    // no array found - allocate new one, resize and set
    parray = *new GASArrayObject(pLevel0Movie->GetASEnvironment());
    parray->Resize(count);
    GASValue val;
    val.SetAsObject(parray);

    bool setResult = pLevel0Movie->GetASEnvironment()->SetVariable(path, val, NULL, 
        (setType == SV_Normal));

    if ( (!setResult && (setType != SV_Normal)) ||
        (setType == SV_Permanent) )
    {
        // Store in sticky hash.
        AddStickyVariable(path, val, setType);
    }
    return setResult;
}

UInt GFxMovieRoot::GetVariableArraySize(const char* ppathToVar)
{
    if (!pLevel0Movie)
        return 0;

    GASEnvironment*                 penv = pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(ppathToVar));

    GASValue retVal;
    if (penv->GetVariable(path, &retVal) && retVal.IsObject())
    {
        GASObject* pobj = retVal.ToObject();
        if (pobj && pobj->GetObjectType() == GASObject::Object_Array)
        {
            GASArrayObject* parray = static_cast<GASArrayObject*>(pobj);
            return parray->GetSize();
        }
    }
    return 0;
}

bool GFxMovieRoot::GetVariableArray(SetArrayType type, const char* ppathToVar,
                                    UInt index, void* pdata, UInt count)
{
    if (!pLevel0Movie)
        return false;

    GASEnvironment*                 penv = pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(ppathToVar));

    GASValue retVal;
    if(penv->GetVariable(path, &retVal) && retVal.IsObject())
    {
        GASObject* pobj = retVal.ToObject();
        if (pobj && pobj->GetObjectType() == GASObject::Object_Array)
        {
            pRetValHolder->ResetPos();
            pRetValHolder->ResizeStringArray(0);

            GASArrayObject* parray = static_cast<GASArrayObject*>(pobj);
            UInt arrSize = parray->GetSize();
            switch(type)
            {
            case SA_Int:
                {
                    int* parr = static_cast<int*>(pdata);
                    for (UInt i = 0, n = GTL::gmin(arrSize, count); i < n; ++i)
                    {
                        GASValue* pval = parray->GetElementPtr(i + index);
                        if (pval)
                            parr[i] = (int)pval->ToNumber(penv);
                        else
                            parr[i] = 0;
                    }
                }
                break;
            case SA_Float:
                {
                    Float* parr = static_cast<Float*>(pdata);
                    for (UInt i = 0, n = GTL::gmin(arrSize, count); i < n; ++i)
                    {
                        GASValue* pval = parray->GetElementPtr(i + index);
                        if (pval)
                            parr[i] = (Float)pval->ToNumber(penv);
                        else
                            parr[i] = 0;
                    }
                }
                break;
            case SA_Double:
                {
                    Double* parr = static_cast<Double*>(pdata);
                    for (UInt i = 0, n = GTL::gmin(arrSize, count); i < n; ++i)
                    {
                        GASValue* pval = parray->GetElementPtr(i + index);
                        if (pval)
                            parr[i] = (Double)pval->ToNumber(penv);
                        else
                            parr[i] = 0;
                    }
                }
                break;
            case SA_Value:
                {
                    GFxValue* parr = static_cast<GFxValue*>(pdata);
                    for (UInt i = 0, n = GTL::gmin(arrSize, count); i < n; ++i)
                    {
                        GASValue* pval = parray->GetElementPtr(i + index);
                        GFxValue* pdestVal = &parr[i];
                        pdestVal->SetUndefined();
                        if (pval)
                            ASValue2GFxValue(penv, *pval, pdestVal);
                        else
                            pdestVal->SetUndefined();
                    }
                }
                break;
            case SA_String:
                {
                    const char** parr = static_cast<const char**>(pdata);
                    UInt n = GTL::gmin(arrSize, count);
                    pRetValHolder->ResizeStringArray(n);
                    for (UInt i = 0; i < n; ++i)
                    {
                        GASValue* pval = parray->GetElementPtr(i + index);
                        if (pval)
                        {
                            GASString str = pval->ToString(penv);
                            parr[i] = str.ToCStr();
                            pRetValHolder->StringArray[pRetValHolder->StringArrayPos++] = str;   
                        }
                        else
                            parr[i] = NULL;
                    }
                }
                break;
            case SA_StringW:
                {
                    const wchar_t** parr = static_cast<const wchar_t**>(pdata);
                    // pre-calculate the size of required buffer
                    UInt i, bufSize = 0;
                    UInt n = GTL::gmin(arrSize, count);
                    pRetValHolder->ResizeStringArray(n);
                    for (i = 0; i < arrSize; ++i)
                    {
                        GASValue* pval = parray->GetElementPtr(i + index);
                        if (pval)
                        {
                            GASString str = pval->ToString(penv);
                            pRetValHolder->StringArray[i] = str;
                            bufSize += str.GetLength()+1;
                        }
                    }
                    wchar_t* pwBuffer = (wchar_t*)pRetValHolder->PreAllocateBuffer(bufSize * sizeof(wchar_t));

                    for (i = 0; i < n; ++i)
                    {
                        const char* psrcStr = pRetValHolder->StringArray[i].ToCStr();
                        wchar_t* pwdstStr = pwBuffer;
                        UInt32 code;
                        while ((code = GUTF8Util::DecodeNextChar(&psrcStr)) != 0)
                        {
                            *pwBuffer++ = (wchar_t)code;
                        }
                        *pwBuffer++ = 0;

                        parr[i] = pwdstStr;
                    }
                    pRetValHolder->ResizeStringArray(0);
                }
                break;
            }
            return true;
        }
    }
    return false;
}

void GFxMovieRoot::AddStickyVariable(const GASString& fullPath, const GASValue &val, SetVarType setType)
{
    // Parse the path first.
    GASStringContext sc(pGlobalContext, 8);
    GASString   path(sc.GetBuiltin(GASBuiltin_empty_));
    GASString   name(sc.GetBuiltin(GASBuiltin_empty_));

    if (!GASEnvironment::ParsePath(&sc, fullPath, &path, &name))
    {
        if (name.IsEmpty())
            return;

        // This can happen if a sticky variable did not have a path. In that
        // case assume it is _level0.
        path = sc.GetBuiltin(GASBuiltin__level0);
    }
    else
    {
        // TBD: Verify/clean up path integrity?
        //  -> no _parent, _global, "..", ".", "this"
        //  -> _root converts to _level0
        // "/" converts to "."
        bool    needLevelPrefix = 0;

        if (path.GetSize() >= 5)
        {           
            if (!memcmp(path.ToCStr(), "_root", 5))
            {
                // Translate _root.
                path = sc.GetBuiltin(GASBuiltin__level0) + path.Substring(5, path.GetLength());
            }

            // Warn about invalid path components in debug builds.
#ifdef  GFC_BUILD_DEBUG         
            const char *p = strstr(path.ToCStr(), "_parent");
            if (!p) p = strstr(path.ToCStr(), "_global");

            if (p)
            {
                if ((p == path.ToCStr()) || *(p-1) == '.')
                {
                    GFC_DEBUG_WARNING(1, "SetVariable - Sticky/Permanent variable path can not include _parent or _global");
                }
            }
#endif

            // Path must start with _levelN.
            if (memcmp(path.ToCStr(), "_level", 6) != 0)
            {
                needLevelPrefix = 1;
            }
        }
        else
        {
            needLevelPrefix = 1;
        }

        if (needLevelPrefix)
            path = sc.GetBuiltin(GASBuiltin__level0dot_) + path;
    }

    StickyVarNode* pnode = 0;
    StickyVarNode* p = 0;

    if (StickyVariables.get(path, &pnode) && pnode)
    {
        p = pnode;

        while (p)
        {
            // If there is a name match, overwrite value.
            if (p->Name == name)
            {
                p->Value     = val;
                if (!p->Permanent) // Permanent sticks.
                    p->Permanent = (setType == SV_Permanent);
                return;
            }
            p = p->pNext;
        };

        // Not found, add new node
        if ((p = new StickyVarNode(name, val, (setType == SV_Permanent)))!=0)
        {          
            // Link prev node to us: order does not matter.
            p->pNext = pnode->pNext;
            pnode->pNext = p;
        }
        // Done.
        return;
    }

    // Node does not exit, create it
    if ((p = new StickyVarNode(name, val, (setType == SV_Permanent)))!=0)
    {        
        // Save node.
        StickyVariables.set(path, p);
    }
}

void    GFxMovieRoot::ResolveStickyVariables(GFxASCharacter *pcharacter)
{
    GASSERT(pcharacter);

    const GASString &path  = pcharacter->GetCharacterHandle()->GetNamePath();
    StickyVarNode*   pnode = 0;
    StickyVarNode*   p, *pnext;

    if (StickyVariables.get(path, &pnode))
    {       
        // If found, add variables.     
        // We also remove sticky nodes while keeping permanent ones.
        StickyVarNode*  ppermanent = 0;
        StickyVarNode*  ppermanentTail = 0;

        p = pnode;

        while (p)
        {
            pcharacter->SetMember(pcharacter->GetASEnvironment(), p->Name, p->Value);
            pnext = p->pNext;

            if (p->Permanent)
            {
                // If node is permanent, create a permanent-only linked list.
                if (!ppermanent)
                {
                    ppermanent = p;
                }
                else
                {
                    ppermanentTail->pNext = p;
                }

                ppermanentTail = p;
                p->pNext = 0;
            }
            else
            {
                // If not permanent, delete this node.
                delete p;
            }

            p = pnext;
        }


        if (ppermanent)
        {
            // If permanent list was formed, keep it.
            if (ppermanent != pnode)
                StickyVariables.set(path, ppermanent);
        }
        else
        {
            // Otherwise delete hash key.
            StickyVariables.remove(path);
        }
    }
}

void    GFxMovieRoot::ClearStickyVariables()
{
    GASStringHash<StickyVarNode*>::iterator ihash = StickyVariables.begin();

    // Travers hash and delete nodes.
    for( ;ihash != StickyVariables.end(); ++ihash)
    {
        StickyVarNode* pnode = ihash->second;
        StickyVarNode* p;

        while (pnode)
        {
            p = pnode;
            pnode = p->pNext;
            delete p;
        }
    }

    StickyVariables.clear();
}

bool    GFxMovieRoot::IsAvailable(const char* pathToVar) const
{
    if (!pLevel0Movie)
        return 0;

    GASEnvironment*                 penv = pLevel0Movie->GetASEnvironment();
    GASString                       path(penv->CreateString(pathToVar));

    return penv->IsAvailable(path);
}


// For ActionScript interfacing convenience.
bool        GFxMovieRoot::Invoke(const char* pmethodName, GFxValue *presult, const GFxValue* pargs, UInt numArgs)
{
    if (!pLevel0Movie) return 0;
    GASValue resultVal;
    GASEnvironment* penv = pLevel0Movie->GetASEnvironment();
    GASSERT(penv);

    // push arguments directly into environment
    for (int i = (int)numArgs - 1; i >= 0; --i)
    {
        GASSERT(pargs); // should be checked only if numArgs > 0
        const GFxValue& gfxVal = pargs[i];
        switch(gfxVal.GetType())
        {
        case GFxValue::VT_Boolean:   penv->Push(gfxVal.GetBool()); break;
        case GFxValue::VT_Null:      penv->Push(GASValue::NULLTYPE); break;
        case GFxValue::VT_Number:    penv->Push(gfxVal.GetNumber()); break;
        case GFxValue::VT_String:    
            {
                penv->Push(pGlobalContext->CreateString(gfxVal.GetString())); break;
            }
            break;
        case GFxValue::VT_StringW:    
            {
                penv->Push(pGlobalContext->CreateString(gfxVal.GetStringW())); break;
            }
            break;
        default:
            penv->Push(GASValue::UNDEFINED);
        }
    }
    bool retVal = pLevel0Movie->Invoke(pmethodName, &resultVal, numArgs);
    penv->Drop(numArgs); // release arguments
    if (retVal && presult)
    {
        pRetValHolder->ResetPos();
        // convert result to GFxValue
        ASValue2GFxValue(penv, resultVal, presult);
    }
    return retVal;
}

bool        GFxMovieRoot::Invoke(const char* pmethodName, GFxValue *presult, const char* pargFmt, ...)
{
    if (!pLevel0Movie) return 0;
    GASValue resultVal;
    va_list args;
    va_start(args, pargFmt);
    bool retVal = pLevel0Movie->InvokeArgs(pmethodName, &resultVal, pargFmt, args);
    va_end(args);
    if (retVal && presult)
    {
        pRetValHolder->ResetPos();
        // convert result to GFxValue
        ASValue2GFxValue(pLevel0Movie->GetASEnvironment(), resultVal, presult);
    }
    return retVal;
}

bool        GFxMovieRoot::InvokeArgs(const char* pmethodName, GFxValue *presult, const char* pargFmt, va_list args)
{
    if (pLevel0Movie)
    {
        GASValue resultVal;
        bool retVal = pLevel0Movie->InvokeArgs(pmethodName, &resultVal, pargFmt, args);
        if (retVal && presult)
        {
            pRetValHolder->ResetPos();
            // convert result to GFxValue
            ASValue2GFxValue(pLevel0Movie->GetASEnvironment(), resultVal, presult);
        }
        return retVal;
    }
    return false;
}

void        GFxMovieRoot::SetExternalInterfaceRetVal(const GFxValue& retVal)
{
    GFxValue2ASValue(retVal, &ExternalIntfRetVal);
}

// Focus management
class GASTabIndexSortFunctor
{
public:
    inline bool operator()(const GFxASCharacter* a, const GFxASCharacter* b) const
    {
        return (a->GetTabIndex() < b->GetTabIndex());
    }
};

class GASAutoTabSortFunctor
{
public:
    inline bool operator()(const GFxASCharacter* a, const GFxASCharacter* b) const
    {
        GFxCharacter::Matrix ma = a->GetLevelMatrix();
        GFxCharacter::Matrix mb = b->GetLevelMatrix();
        GRectF aRect  = ma.EncloseTransform(a->GetFocusRect());
        GRectF bRect  = mb.EncloseTransform(b->GetFocusRect());
        bool sameRow = (((aRect.Top >= bRect.Top && aRect.Top <= bRect.Bottom) ||
            (aRect.Top <= bRect.Top && bRect.Top <= aRect.Bottom)) &&
            (aRect.Right < bRect.Left || aRect.Left > bRect.Right));
        if (sameRow)
            return aRect.Left < bRect.Left;

        return (aRect.Top < bRect.Top);
    }
};

void GFxMovieRoot::ProcessFocusKey(short keycode, GFxEvent::EventType event, const GFxSpecialKeysState& specialKeysState, ProcessFocusKeyInfo* pfocusInfo)
{
    GASSERT(pfocusInfo);
    if (event == GFxEvent::KeyDown)
    {
        if (keycode == GFxKey::Tab || 
            (IsShowingRect && (keycode == GFxKey::Left || keycode == GFxKey::Right || keycode == GFxKey::Up || keycode == GFxKey::Down)))
        {
            int tabableArraySize = 0;
            if (!pfocusInfo->Initialized)
            {
                pfocusInfo->TabableArray.resize(0);
                pfocusInfo->TabIndexed = false;

                // fill array by focusable characters
                for (int movieIndex = (int)MovieLevels.size(); movieIndex > 0; movieIndex--)
                {
                    MovieLevels[movieIndex-1].pSprite->FillTabableArray(pfocusInfo->TabableArray, &pfocusInfo->TabIndexed);
                }

                tabableArraySize = (int)pfocusInfo->TabableArray.size();

                if (pfocusInfo->TabIndexed)
                {
                    // sort by tabIndex if tabIndexed == true
                    static GASTabIndexSortFunctor sf;
                    GAlg::QuickSort(pfocusInfo->TabableArray, sf);
                }
                else
                {
                    // sort for automatic order
                    static GASAutoTabSortFunctor sf;
                    GAlg::QuickSort(pfocusInfo->TabableArray, sf);
                }

                /* // Debug code, do not remove!
                for(int ii = 0; ii < tabableArraySize; ++ii)
                {
                printf("Focus[%d] = %s\n", ii, tabableArray[ii]->GetCharacterHandle()->GetNamePath().ToCStr());
                }*/

                pfocusInfo->CurFocusIdx = -1;
                pfocusInfo->CurFocused = LastFocused;
                if (pfocusInfo->CurFocused)
                {
                    // find the index of currently focused
                    for (int i = 0; i < tabableArraySize; ++i)
                    {
                        if (pfocusInfo->TabableArray[i] == pfocusInfo->CurFocused)
                        {
                            pfocusInfo->CurFocusIdx = i;
                            break;
                        }
                    }
                }
                pfocusInfo->Initialized = true;
            }
            else
            {
                tabableArraySize = (int)pfocusInfo->TabableArray.size();
            }
            if (keycode == GFxKey::Tab)
            {
                if (specialKeysState.IsShiftPressed())
                {
                    if (--pfocusInfo->CurFocusIdx < 0)
                        pfocusInfo->CurFocusIdx = tabableArraySize-1;
                }
                else
                {
                    if (++pfocusInfo->CurFocusIdx >= tabableArraySize)
                        pfocusInfo->CurFocusIdx = 0;
                }
            }
            else if (pfocusInfo->CurFocusIdx > -1 && pfocusInfo->CurFocused)
            {
                if (pfocusInfo->CurFocused->IsFocusRectEnabled() || AlwaysEnableFocusArrowKeys.IsTrue())
                {
                    GFxCharacter::Matrix ma = pfocusInfo->CurFocused->GetLevelMatrix();
                    GRectF aRect  = ma.EncloseTransform(pfocusInfo->CurFocused->GetFocusRect());

                    if (keycode == GFxKey::Right || keycode == GFxKey::Left)
                    {
                        int newFocusIdx = pfocusInfo->CurFocusIdx;
                        GRectF newFocusRect;
                        newFocusRect.Left = newFocusRect.Right = (keycode == GFxKey::Right) ? Float(INT_MAX) : Float(INT_MIN);
                        newFocusRect.Top = newFocusRect.Bottom = Float(INT_MAX);
                        // find nearest from right or left side

                        bool hitStipe = false;
                        for (int i = 0; i < tabableArraySize - 1; ++i)
                        {
                            newFocusIdx = (keycode == GFxKey::Right) ? newFocusIdx + 1 : newFocusIdx - 1;
                            if (newFocusIdx >= tabableArraySize)
                                newFocusIdx = 0;
                            else if (newFocusIdx < 0)
                                newFocusIdx = tabableArraySize - 1;
                            GPtr<GFxASCharacter> b = pfocusInfo->TabableArray[newFocusIdx];
                            GFxCharacter::Matrix mb = b->GetLevelMatrix();
                            GRectF bRect  = mb.EncloseTransform(b->GetFocusRect());

                            bool curHitStripe = false;
                            // check the "stripe zone"
                            if (keycode == GFxKey::Right)
                            {
                                const GRectF stripeRect(aRect.Right, aRect.Top, GFC_MAX_FLOAT, aRect.Bottom);
                                if (bRect.Intersects(stripeRect))
                                    curHitStripe = true;
                            }
                            else // Left
                            {
                                const GRectF stripeRect(GFC_MIN_FLOAT, aRect.Top, aRect.Left, aRect.Bottom);
                                if (bRect.Intersects(stripeRect))
                                    curHitStripe = true;
                            }
                            if (curHitStripe)
                            {
                                if (!hitStipe)
                                { // first hit - save the current char index and rect
                                    pfocusInfo->CurFocusIdx = newFocusIdx;
                                    newFocusRect = bRect;
                                    hitStipe = true;
                                    continue;
                                }
                            }
                            // if we already hit stripe once - ignore all chars NOT in the stripe zone
                            if (hitStipe && !curHitStripe)
                                continue;

                            bool closerByY = (GTL::gabs(bRect.Top + bRect.Height()/2 - aRect.Top + aRect.Height()/2) < 
                                GTL::gabs(newFocusRect.Top + newFocusRect.Height()/2 - aRect.Top + aRect.Height()/2));
                            if (closerByY && 
                                ((keycode == GFxKey::Right && (bRect.Left - aRect.Right >= 0 && bRect.Left - aRect.Right <= GTL::gabs(newFocusRect.Left - aRect.Right))) ||
                                (keycode == GFxKey::Left  && (aRect.Left - bRect.Right >= 0 && aRect.Left - bRect.Right <= GTL::gabs(aRect.Left - newFocusRect.Right)))))
                            {
                                pfocusInfo->CurFocusIdx = newFocusIdx;
                                newFocusRect = bRect;
                            }
                        }
                    }
                    else if (keycode == GFxKey::Up || keycode == GFxKey::Down)
                    {
                        int newFocusIdx = pfocusInfo->CurFocusIdx;
                        GRectF newFocusRect(0);
                        newFocusRect.Left = Float(INT_MAX);
                        newFocusRect.Top = (keycode == GFxKey::Down) ? Float(INT_MAX) : Float(INT_MIN);
                        // find nearest from top and bottom side
                        // The logic is as follows:
                        // 1. The highest priority characters are ones, which boundary rectangles intersect with the "stripe zone" 
                        // above or below the currently selected character. I.e. for Down key the "stripe zone" will be 
                        // ((aRect.Left, aRect.Right), (aRect.Bottom, Infinity)).
                        //   a) if there are more than one characters in the "stripe zone", then the best candidate should
                        //      have shortest distance by Y axis.
                        // 2. Otherwise, the closest character will be chosen by comparing Y-distance and only then X-distance.
                        bool hitStipe = false;

                        for (int i = 0; i < tabableArraySize - 1; ++i)
                        {
                            newFocusIdx = (keycode == GFxKey::Down) ? newFocusIdx + 1 : newFocusIdx - 1;
                            if (newFocusIdx >= tabableArraySize)
                                newFocusIdx = 0;
                            else if (newFocusIdx < 0)
                                newFocusIdx = tabableArraySize - 1;
                            GPtr<GFxASCharacter> b = pfocusInfo->TabableArray[newFocusIdx];
                            GFxCharacter::Matrix mb = b->GetLevelMatrix();
                            GRectF bRect  = mb.EncloseTransform(b->GetFocusRect());

                            bool curHitStripe = false;
                            // check the "stripe zone"
                            if (keycode == GFxKey::Down)
                            {
                                const GRectF stripeRect(aRect.Left, aRect.Bottom, aRect.Right, GFC_MAX_FLOAT);
                                if (bRect.Intersects(stripeRect))
                                    curHitStripe = true;
                            }
                            else
                            {
                                const GRectF stripeRect(aRect.Left, GFC_MIN_FLOAT, aRect.Right, aRect.Top);
                                if (bRect.Intersects(stripeRect))
                                    curHitStripe = true;
                            }
                            //printf("bRect = %s\n",  b->GetCharacterHandle()->GetNamePath().ToCStr());
                            //printf ("curHitStripe is %d\n", (int)curHitStripe);
                            if (curHitStripe)
                            {
                                if (!hitStipe)
                                { // first hit - save the current char index and rect
                                    pfocusInfo->CurFocusIdx = newFocusIdx;
                                    newFocusRect = bRect;
                                    hitStipe = true;
                                    continue;
                                }
                            }
                            // if we already hit stripe once - ignore all chars NOT in the stripe zone
                            if (hitStipe && !curHitStripe)
                                continue;

                            bool sameRow = (((aRect.Top >= bRect.Top && aRect.Top <= bRect.Bottom) ||
                                (aRect.Top <= bRect.Top && bRect.Top <= aRect.Bottom)) &&
                                (aRect.Right < bRect.Left || aRect.Left > bRect.Right));
                            //printf ("SAMEROW is %d\n", (int)sameRow);
                            if (!sameRow || curHitStripe)
                            {
                                bool closerByX = (GTL::gabs(bRect.Left + bRect.Width()/2 - aRect.Left + aRect.Width()/2) < 
                                    GTL::gabs(newFocusRect.Left + newFocusRect.Width()/2 - aRect.Left + aRect.Width()/2));
                                Float yDistDiff = (bRect.Top - aRect.Top) - (newFocusRect.Top - aRect.Top);
                                if (((keycode == GFxKey::Down && bRect.Top > aRect.Top && (yDistDiff < 0 || (yDistDiff == 0 && closerByX))) ||
                                    (keycode == GFxKey::Up   && bRect.Top < aRect.Top && (yDistDiff > 0 || (yDistDiff == 0 && closerByX)))))
                                {
                                    //printf("newFocus = %s\n",  b->GetCharacterHandle()->GetNamePath().ToCStr());
                                    pfocusInfo->CurFocusIdx = newFocusIdx;
                                    newFocusRect = bRect;
                                }
                            }
                        }
                    }
                }
            }
            if (pfocusInfo->CurFocusIdx >= 0 && pfocusInfo->CurFocusIdx < tabableArraySize)
            {
                pfocusInfo->CurFocused = pfocusInfo->TabableArray[pfocusInfo->CurFocusIdx];
            }
            else
            {
                pfocusInfo->CurFocused = 0;
            }
        }
    }
}

void GFxMovieRoot::FinalizeProcessFocusKey(ProcessFocusKeyInfo* pfocusInfo)
{
    GASSERT(pfocusInfo);

    if (pfocusInfo->Initialized)
    {
        if (pfocusInfo->CurFocusIdx >= 0 && pfocusInfo->CurFocusIdx < (int)pfocusInfo->TabableArray.size())
        {
            QueueSetFocusTo(pfocusInfo->TabableArray[pfocusInfo->CurFocusIdx]);
            IsShowingRect = true;
        }
    }
}

void GFxMovieRoot::ActivateFocusCapture()
{
    ProcessFocusKeyInfo focusKeyInfo;
    ProcessFocusKey(GFxKey::Tab, GFxKeyEvent::KeyDown, GFxSpecialKeysState(0), &focusKeyInfo);
    FinalizeProcessFocusKey(&focusKeyInfo);
}

void GFxMovieRoot::DisplayFocusRect(const GFxDisplayContext& context)
{
    GPtr<GFxASCharacter> curFocused = LastFocused;
    if (curFocused  && IsShowingRect && curFocused->IsFocusRectEnabled())
    {
        GRenderer::Matrix   mat = curFocused->GetWorldMatrix();         
        GRectF focusLocalRect = curFocused->GetFocusRect();

        if (focusLocalRect.IsNull())
            return;
        // Do viewport culling if bounds are available.
        if (!VisibleFrameRect.Intersects(mat.EncloseTransform(focusLocalRect)))
            if (!(context.GetRenderFlags() & GFxRenderConfig::RF_NoViewCull))
                return;

        GRenderer* prenderer   = context.GetRenderer();

        GRectF focusWorldRect = mat.EncloseTransform(focusLocalRect);

        prenderer->SetCxform(GRenderer::Cxform()); // reset color transform for focus rect
        prenderer->SetMatrix(GRenderer::Matrix()); // reset matrix to work in world coords


        GPointF         coords[4];
        static const UInt16 indices[24] = { 
            0, 1, 2,    2, 1, 3,    2, 4, 5,    5, 4, 6,    7, 3, 8,    8, 3, 9,    5, 9, 10,   10, 9, 11 
        };
#define LIMIT_COORD(c) ((c>32767)?32767.f:((c < -32768)?-32768.f:c))
#define LIMITED_POINT(p) GPointF(LIMIT_COORD(p.x), LIMIT_COORD(p.y))

        coords[0] = LIMITED_POINT(focusWorldRect.TopLeft());
        coords[1] = LIMITED_POINT(focusWorldRect.TopRight());
        coords[2] = LIMITED_POINT(focusWorldRect.BottomRight());
        coords[3] = LIMITED_POINT(focusWorldRect.BottomLeft());

        GRenderer::VertexXY16i icoords[12];
            // Strip (fill in)
            icoords[0].x = (SInt16) coords[0].x;      icoords[0].y = (SInt16) coords[0].y;        // 0
            icoords[1].x = (SInt16) coords[1].x;      icoords[1].y = (SInt16) coords[1].y;        // 1
            icoords[2].x = (SInt16) coords[0].x;      icoords[2].y = (SInt16) coords[0].y + 40;   // 2
            icoords[3].x = (SInt16) coords[1].x;      icoords[3].y = (SInt16) coords[1].y + 40;   // 3
            icoords[4].x = (SInt16) coords[0].x + 40; icoords[4].y = (SInt16) coords[0].y + 40;   // 4
            icoords[5].x = (SInt16) coords[3].x;      icoords[5].y = (SInt16) coords[3].y - 40;   // 5
            icoords[6].x = (SInt16) coords[3].x + 40; icoords[6].y = (SInt16) coords[3].y - 40;   // 6
            icoords[7].x = (SInt16) coords[1].x - 40; icoords[7].y = (SInt16) coords[1].y + 40;   // 7
            icoords[8].x = (SInt16) coords[2].x - 40; icoords[8].y = (SInt16) coords[2].y - 40;   // 8
            icoords[9].x = (SInt16) coords[2].x;      icoords[9].y = (SInt16) coords[2].y - 40;   // 9 
            icoords[10].x = (SInt16) coords[3].x;     icoords[10].y = (SInt16) coords[3].y;       // 10
            icoords[11].x = (SInt16) coords[2].x;     icoords[11].y = (SInt16) coords[2].y;       // 11

        prenderer->SetVertexData(icoords, 12, GRenderer::Vertex_XY16i);

        prenderer->FillStyleColor(GColor(255, 255, 0, 255));

        // Fill the inside
        prenderer->SetIndexData(indices, 24, GRenderer::Index_16);
        prenderer->DrawIndexedTriList(0, 0, 24, 0, 8);

        // Done
        prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
        prenderer->SetIndexData(0, 0, GRenderer::Index_None);
    }
}

void GFxMovieRoot::HideFocusRect()
{
    if (IsShowingRect)
    {
        GPtr<GFxASCharacter> curFocused = LastFocused;
        if (curFocused)
            curFocused->OnLosingKeyboardFocus();
    }
    IsShowingRect = false;
}

void GFxMovieRoot::SetFocusTo(GFxASCharacter* ch)
{
    // the order of events, if Selection.setFocus is invoked is as follows:
    // Instantly:
    // 1. curFocus.onKillFocus, curFocus = oldFocus
    // 2. curFocus = newFocus
    // 3. curFocus.onSetFocus, curFocus = newFocus
    // 4. Selection focus listeners, curFocus = newFocus
    // Queued:
    // 5. oldFocus.onRollOut, curFocus = newFocus
    // 6. newFocus.onRollOver, curFocus = newFocus
    GPtr<GFxASCharacter> curFocused = LastFocused;

    if (curFocused == ch) return;

    if (curFocused)
    {
        // queue onRollOut, step 5 (since it is queued up, we may do it before TransferFocus)
        curFocused->OnLosingKeyboardFocus();
    }

    // Do instant focus transfer (steps 1-4)
    TransferFocus(ch);

    // invoke onSetFocus for newly set LastFocused
    if (ch)
    {
        // queue onRollOver, step 6
        ch->OnGettingKeyboardFocus();
    }
}

void GFxMovieRoot::QueueSetFocusTo(GFxASCharacter* ch)
{
    // the order of events, if focus key is pressed is as follows:
    // 1. curFocus.onRollOut, curFocus = oldFocus
    // 2. newFocus.onRollOver, curFocus = oldFocus
    // 3. curFocus.onKillFocus, curFocus = oldFocus
    // 4. curFocus = newFocus
    // 5. curFocus.onSetFocus, curFocus = newFocus
    // 6. Selection focus listeners, curFocus = newFocus
    GPtr<GFxASCharacter> curFocused = LastFocused;

    if (curFocused == ch) return;

    if (curFocused)
    {
        // queue onRollOut (step 1)
        curFocused->OnLosingKeyboardFocus();
    }

    // invoke onSetFocus for newly set LastFocused
    if (ch)
    {
        // queue onRollOver (step 2)
        ch->OnGettingKeyboardFocus();
    }

    // Queue setting focus to ch (steps 3-6)
    GASSelection::QueueSetFocus(GetLevelMovie(0)->GetASEnvironment(), ch);
}

// Instantly transfers focus w/o any queuing
void GFxMovieRoot::TransferFocus(GFxASCharacter* pNewFocus)
{
    GPtr<GFxASCharacter> curFocused = LastFocused;

    if (curFocused == pNewFocus) return;

    if (curFocused)
    {
        // invoke onKillFocus for LastFocused
        curFocused->OnFocus(GFxASCharacter::KillFocus, pNewFocus);
    }

    LastFocused = pNewFocus;

    // invoke onSetFocus for newly set LastFocused
    if (pNewFocus)
    {
        pNewFocus->OnFocus(GFxASCharacter::SetFocus, curFocused);
    }
    GASSelection::BroadcastOnSetFocus(GetLevelMovie(0)->GetASEnvironment(), curFocused, pNewFocus);
}


void GFxMovieRoot::SetKeyboardFocusTo(GFxASCharacter* ch)
{
    IsShowingRect = true;
    SetFocusTo(ch);
    // here is a difference with the Flash: in Flash, if you do Selection.setFocus on invisible character or
    // character with invisible parent then Flash sets the focus on it and shows the focusRect.
    // GFxPlayer sets the focus, but doesn't show the focusRect. Probably, it shouldn't set focus at all.
    // (AB, 02/22/07).
    if (IsShowingRect)
    {
        GFxASCharacter* cur = ch;
        for (; cur && cur->GetVisible(); cur = cur->GetParent())
            ;
        IsShowingRect = !cur;
    }
}

void GFxMovieRoot::FocusMouseInputMoved(GFxASCharacter* newCharWithInput)
{
    GPtr<GFxASCharacter> curFocused = LastFocused;
    if (curFocused && curFocused != newCharWithInput)
        curFocused->OnLosingMouseInput();
}

//
// ***** GFxSwfEvent
//
// For embedding event handlers in GFxPlaceObject2

void    GFxSwfEvent::Read(GFxStream* in, UInt32 flags)
{
    GASSERT(flags != 0);

    // Scream if more than one bit is set, since we're not set up to handle
    // that, and it doesn't seem possible to express in ActionScript source,
    // so it's important to know if this ever occurs in the wild.
    if (flags & (flags - 1))
    {
        in->LogError("Error: GFxSwfEvent::Read() - more than one event type encoded!  "
            "unexpected! flags = 0x%x\n", flags);
    }

    // 14 bits reserved, 18 bits used

    static const GFxEventId CodeBits[19] =
    {
        GFxEventId::Event_Load,
        GFxEventId::Event_EnterFrame,
        GFxEventId::Event_Unload,
        GFxEventId::Event_MouseMove,
        GFxEventId::Event_MouseDown,
        GFxEventId::Event_MouseUp,
        GFxEventId::Event_KeyDown,
        GFxEventId::Event_KeyUp,
        GFxEventId::Event_Data,
        GFxEventId::Event_Initialize,
        GFxEventId::Event_Press,
        GFxEventId::Event_Release,
        GFxEventId::Event_ReleaseOutside,
        GFxEventId::Event_RollOver,
        GFxEventId::Event_RollOut,
        GFxEventId::Event_DragOver,
        GFxEventId::Event_DragOut,
        GFxEventId::Event_KeyPress,
        GFxEventId::Event_Construct
    };

    for (int i = 0, mask = 1; i < int(sizeof(CodeBits)/sizeof(CodeBits[0])); i++, mask <<= 1)
    {
        if (flags & mask)
        {
            Event = CodeBits[i];
            break;
        }
    }

    UInt32  eventLength = in->ReadU32();

    // if Event_KeyPress, read keyCode (UI8) and decrement eventLength.
    if (Event.Id == GFxEventId::Event_KeyPress)
    {
        Event.KeyCode = in->ReadU8();
        --eventLength;
        in->LogParse("---- GFxSwfEvent::Read -- Event_KeyPress found, key code is %d\n", Event.KeyCode);
    }

    // Read the actions.
#ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
    // Use extra #ifndef check to ensure Event.GetFunctionName() is not called
    if (in->IsVerboseParseAction())
    {        
        in->LogParseAction("---- actions for event 0x%X (%s)\n",
            Event.Id,
            GASStringBuiltinManager::GetBuiltinCStr(Event.GetFunctionNameBuiltinType()));
    }
#endif
    if (Event.Id == 19)
        Event.Id = Event.Id;
    //        GASSERT(0);
    pActionOpData = *new GASActionBufferData();
    pActionOpData->Read(in);

    if (pActionOpData->GetLength() != eventLength)
    {
        in->LogError("Error: GFxSwfEvent::Read - EventLength = %d, but read %d\n",
            eventLength,
            pActionOpData->GetLength());

        // This occasionally happens, so we need to adjust
        // the stream so the remaining entries are correctly loaded.
        UInt i, remainder = eventLength - pActionOpData->GetLength();        
        for (i = 0; i<remainder; i++)
            in->ReadS8();

        // MA: Seems that ActionScript can actually jump outside of action buffer
        // bounds  into the binary content stored within other tags. Handling this
        // would complicate things. Ex: f1.swf 
        // For event tags this also means that data can actually be stored 
        // after ActionId == 0, hence we get this "remainder".
    }
}

void    GFxSwfEvent::AttachTo(GFxASCharacter* ch) const
{
    // Create a function to execute the actions.
    if (pActionOpData && !pActionOpData->IsNull())
    {            
        GTL::garray<GASWithStackEntry>  emptyWithStack;
        GASEnvironment*                 penv = ch->GetASEnvironment();
        GPtr<GASActionBuffer>           pbuff = *new GASActionBuffer(penv->GetSC(), pActionOpData);
        GASAsFunctionDef*               pfunc = new GASAsFunctionDef(pbuff.GetPtr(), penv, 0,
            emptyWithStack, GASActionBuffer::Exec_Event);
        pfunc->SetLength(pActionOpData->GetLength());

        GASValue method(GASFunctionRef(*new GASFunctionObject(pfunc)));
        ch->SetEventHandler(Event, method);
    }
}





//
// ***** GFxPlaceObject2
//

GFxPlaceObject2::GFxPlaceObject2()  
{
    TagType     = GFxTag_End;
    Name        = NULL;
    PlaceType   = Place_Add;
}

GFxPlaceObject2::~GFxPlaceObject2()
{    
    for (UPInt i = 0, n = EventHandlers.size(); i < n; i++)
    {
        delete EventHandlers[i];
    }
    EventHandlers.resize(0);

    if (Name)        
        GFREE(Name);
}

void    GFxPlaceObject2::Read(GFxLoadProcess* p, GFxTagType tagType)
{   
    GASSERT(tagType == GFxTag_PlaceObject ||
            tagType == GFxTag_PlaceObject2 ||
            tagType == GFxTag_PlaceObject3);

    GFxStream*  pin          = p->GetStream();
    UInt        movieVersion = p->GetVersion();
    Read(pin, tagType, movieVersion);
}

void    GFxPlaceObject2::Read(GFxStream*  pin, GFxTagType tagType, UInt movieVersion)
{   
    GASSERT(tagType == GFxTag_PlaceObject ||
            tagType == GFxTag_PlaceObject2 ||
            tagType == GFxTag_PlaceObject3);
    TagType = tagType;

    bool        verboseParse = pin->IsVerboseParse();

    if (TagType == GFxTag_PlaceObject)
    {
        // Original PlaceObject tag; very simple.
        Pos.CharacterId = GFxResourceId(pin->ReadU16());
        Pos.Depth       = pin->ReadU16();
        pin->ReadMatrix(&Pos.Matrix_1);

        if (verboseParse)
        {        
            pin->LogParse("  CharId = %d\n"
                "  depth = %d\n"
                "  mat = \n",
                Pos.CharacterId.GetIdIndex(), Pos.Depth);
            pin->LogParseClass(Pos.Matrix_1);
        }

        if (pin->Tell() < pin->GetTagEndPosition())
        {
            pin->ReadCxformRgb(&Pos.ColorTransform);

            if (verboseParse)
            {
                pin->LogParse("  cxform:\n");
                pin->LogParseClass(Pos.ColorTransform);
            }
        }
    }
    // TagType == 26: PlaceObject2
    // TagType == 70: PlaceObject3
    else if ((TagType == GFxTag_PlaceObject2) ||
        (TagType == GFxTag_PlaceObject3))
    {   
        // Bit packings for tag bytes.
        enum PlaceObject2Flags
        {
            PO2_HasActions      = 0x80,
            PO2_HasClipBracket  = 0x40,
            PO2_HasName         = 0x20,
            PO2_HasRatio        = 0x10,
            PO2_HasCxfrom       = 0x08,
            PO2_HasMatrix       = 0x04,
            PO2_HasChar         = 0x02,
            PO2_FlagMove        = 0x01
        };
        enum PlaceObject3Flags
        {
            PO3_BitmapCaching   = 0x04,
            PO3_HasBlendMode    = 0x02,
            PO3_HasFilters      = 0x01,
        };

        UByte   po2Flags = pin->ReadU8();
        UByte   po3Flags = 0;
        // Commonly used flags.
        bool    hasChar         = (po2Flags & PO2_HasChar) != 0;
        bool    flagMove        = (po2Flags & PO2_FlagMove) != 0;

        // PlaceObject3 options.
        if (TagType == 70)
            po3Flags = pin->ReadU8();


        Pos.Depth = pin->ReadU16();                

        if (hasChar)
            Pos.CharacterId = GFxResourceId(pin->ReadU16());

        if (po2Flags & PO2_HasMatrix)
        {
            Pos.HasMatrix = true;
            pin->ReadMatrix(&Pos.Matrix_1);
        }
        if (po2Flags & PO2_HasCxfrom)
        {
            Pos.HasCxform = true;
            pin->ReadCxformRgba(&Pos.ColorTransform);
        }

        if (po2Flags & PO2_HasRatio)
            Pos.Ratio = (Float)pin->ReadU16() / (Float)65535;

        if (po2Flags & PO2_HasName)
            Name = pin->ReadString();

        if (po2Flags & PO2_HasClipBracket)
            Pos.ClipDepth = pin->ReadU16(); 

        // Separate parse logging to avoid extra tests/logic during execution.
        // Particularly important because VC++ does not compile out var-args.
        if (verboseParse)
        {         
            pin->LogParse("  depth = %d\n", Pos.Depth);

            if (po2Flags & PO2_HasChar)
                pin->LogParse("  char id = %d\n", Pos.CharacterId.GetIdIndex());
            if (po2Flags & PO2_HasMatrix)
            {
                pin->LogParse("  mat:\n");
                pin->LogParseClass(Pos.Matrix_1);
            }
            if (po2Flags & PO2_HasCxfrom)
            {
                pin->LogParse("  cxform:\n");
                pin->LogParseClass(Pos.ColorTransform);
            }
            if (po2Flags & PO2_HasRatio)
                pin->LogParse("  ratio: %f\n", Pos.Ratio);
            if (po2Flags & PO2_HasName)            
                pin->LogParse("  name = %s\n", Name ? Name : "<null>");
            if (po2Flags & PO2_HasClipBracket)            
                pin->LogParse("  ClipDepth = %d\n", Pos.ClipDepth);
        }

        // PlaceObject3
        if (po3Flags & PO3_HasFilters)
        {
            // This loads only Blur and DropShadow/Glow filters only.
            GFxFilterDesc filters[GFxFilterDesc::MaxFilters];
            UInt numFilters = GFx_LoadFilters(pin, filters);
            if (numFilters)
            {
                if (Pos.TextFilter == 0)
                    Pos.TextFilter = new GFxTextFilter;
                for (UInt i = 0; i < numFilters; ++i)
                {
                    const GFxFilterDesc& filter = filters[i];
                    if ((filter.Flags & 0xF) == GFxFilterDesc::Filter_Blur)
                    {
                        Pos.TextFilter->BlurX        = filter.BlurX;
                        Pos.TextFilter->BlurY        = filter.BlurY;
                        Pos.TextFilter->BlurStrength = filter.Strength;
                        continue;
                    }
                    if ((filter.Flags & 0xF) == GFxFilterDesc::Filter_DropShadow ||
                        (filter.Flags & 0xF) == GFxFilterDesc::Filter_Glow)
                    {
                        Pos.TextFilter->ShadowFlags    = filter.Flags & 0xF0;
                        Pos.TextFilter->ShadowBlurX    = filter.BlurX;
                        Pos.TextFilter->ShadowBlurY    = filter.BlurY;
                        Pos.TextFilter->ShadowStrength = filter.Strength;
                        Pos.TextFilter->ShadowAlpha    = filter.Color.GetAlpha();
                        Pos.TextFilter->ShadowAngle    = filter.Angle;
                        Pos.TextFilter->ShadowDistance = filter.Distance;
                        Pos.TextFilter->ShadowOffsetX  = 0;
                        Pos.TextFilter->ShadowOffsetY  = 0;
                        Pos.TextFilter->ShadowColor    = filter.Color;
                        Pos.TextFilter->UpdateShadowOffset();
                        continue;
                    }
                }
            }
        }
        if (po3Flags & PO3_HasBlendMode)
        {
            UByte   blendMode = pin->ReadU8();
            if (verboseParse)
                pin->LogParse("  blend mode = %d\n", (SInt)blendMode);

            if ((blendMode < 1) || (blendMode>14))
            {
                GFC_DEBUG_WARNING(1, "PlaceObject3::Read - loaded blend mode out of range");
                blendMode = 1;
            }
            // Assign the mode.
            Pos.BlendMode = (GRenderer::BlendType) blendMode;
        }
        if (po3Flags & PO3_BitmapCaching)
        {
            pin->ReadU8();
        }


        if (po2Flags & PO2_HasActions)
        {
            UInt16  reserved = pin->ReadU16();
            GUNUSED(reserved);

            // The logical 'or' of all the following handlers.
            // I don't think we care about this...
            UInt32  allFlags = 0;
            bool    u32Flags = (movieVersion >= 6) || (TagType == GFxTag_PlaceObject3);

            if (u32Flags)
            {
                allFlags = pin->ReadU32();
            }
            else
            {
                allFlags = pin->ReadU16();
            }
            GUNUSED(allFlags);

            if (verboseParse)
                pin->LogParse("  actions: flags = 0x%X\n", allFlags);

            // Read SwfEvents.
            for (;;)
            {
                // Read event.
                pin->Align();

                UInt32  thisFlags = 0;
                if (u32Flags)
                    thisFlags = pin->ReadU32();
                else                    
                    thisFlags = pin->ReadU16();

                if (thisFlags == 0)
                {
                    // Done with events.
                    break;
                }

                GFxSwfEvent*    ev = new GFxSwfEvent;
                ev->Read(pin, thisFlags);

                EventHandlers.push_back(ev);
            }
        }


        if (hasChar && flagMove)
        {
            // Remove whatever's at Depth, and put GFxCharacter there.
            PlaceType = Place_Replace;
            if (verboseParse)
                pin->LogParse("    * (replace)\n");
        }
        else if (!hasChar && flagMove)
        {
            // Moves the object at Depth to the new location.
            PlaceType = Place_Move;
            if (verboseParse)
                pin->LogParse("    * (move)\n");
        }
        else if (hasChar && !flagMove)
        {
            // Put GFxCharacter at Depth.
            PlaceType = Place_Add;
        }

        //LogMsg("place object at depth %i\n", Depth);
    }
}

// Initialize to a specified value (used when generating MovieDataDef for images).
void    GFxPlaceObject2::InitializeToAdd(const GFxCharPosInfo& posInfo)
{
    TagType = GFxTag_PlaceObject;
    Pos     = posInfo;
}


// Place/move/whatever our object in the given pMovie.
void    GFxPlaceObject2::ExecuteWithPos(const GFxCharPosInfo& pos, GFxSprite* m, UInt createFrame)  
{
    switch (PlaceType)
    {
    case Place_Add:
        // Original PlaceObject (tag 4) doesn't do replacement
        {
            GASEnvironment *penv = m->GetASEnvironment();
            GASString       sname((Name == NULL) ? penv->GetBuiltin(GASBuiltin_empty_) : penv->CreateString(Name));
            m->AddDisplayObject(pos, sname, EventHandlers, 0, TagType != 4, createFrame, true);
        }
        break;

    case Place_Move:
        m->MoveDisplayObject(pos);
        break;

    case Place_Replace:
        {
            GASEnvironment *penv = m->GetASEnvironment();
            GASString       sname((Name == NULL) ? penv->GetBuiltin(GASBuiltin_empty_) : penv->CreateString(Name));
            m->ReplaceDisplayObject(pos, sname);
        }        
        break;
    }
}


void    GFxPlaceObject2::ExecuteStateReverse(GFxSprite* m, int frame)
{
    switch (PlaceType)
    {
    case Place_Add:
        // Reverse of add is remove
        m->RemoveDisplayObject(Pos.Depth, TagType == GFxTag_PlaceObject ?
            Pos.CharacterId : GFxResourceId());
        break;

    case Place_Move:        
        {       
            // Reverse of move is previous move.
            GFxCharPosInfo      pos;
            UInt                tagFrame = (UInt)frame;
            UInt                findPrevFlags = GFxSprite::FindPrev_AllowMove;
            // If we had set Matrix/CXForm, force the search to find
            // correct previous values for it (even it it identity).
            if (Pos.HasCxform)  findPrevFlags |= GFxSprite::FindPrev_NeedCXForm;
            if (Pos.HasMatrix)  findPrevFlags |= GFxSprite::FindPrev_NeedMatrix;
            if (Pos.BlendMode)  findPrevFlags |= GFxSprite::FindPrev_NeedBlend;

            GFxPlaceObject2*    ptag = m->FindPreviousPlaceObjectInfo(&pos, &tagFrame, Pos.Depth,
                GFxResourceId(), findPrevFlags);

            if (ptag)
            {   
                // Previous tag might have been Add/Replace, but to go back to it
                // we still do a move, just to the 'Previous' position.
                ExecuteWithPos(pos, m);
            }
            else
            {
                GFC_DEBUG_WARNING2(1, "Reverse MOVE can't find previous replace or add tag(%d, %d)",
                    frame, Pos.Depth);
            }
            break;
        }

    case Place_Replace:
        {
            // Find a previous replace/add tag and compute the necessary object position. This
            // position may not be the same as in ptag, since it can come from even earlier frame.
            GFxCharPosInfo      pos;
            UInt                tagFrame = (UInt)frame;
            UInt                findPrevFlags = 0;
            if (Pos.HasCxform)  findPrevFlags |= GFxSprite::FindPrev_NeedCXForm;
            if (Pos.HasMatrix)  findPrevFlags |= GFxSprite::FindPrev_NeedMatrix;
            if (Pos.BlendMode)  findPrevFlags |= GFxSprite::FindPrev_NeedBlend;
            GFxPlaceObject2*    ptag = m->FindPreviousPlaceObjectInfo(&pos, &tagFrame, Pos.Depth,
                GFxResourceId(), findPrevFlags);

            if (ptag)
            {       
                ptag->ExecuteWithPos(pos, m, tagFrame);
            }
            else
            {
                GFC_DEBUG_WARNING2(1, "Reverse REPLACE can't find previous replace or add Tag(%d, %d)",
                    frame, Pos.Depth);
            }
            break;
        }
    }
}

// DepthResI" is the depth + id packed into one comparable value.
GASExecuteTag::DepthResId  GFxPlaceObject2::GetPlaceObject2_DepthId() const
{    
    GFxResourceId id;

    if (TagType == GFxTag_PlaceObject)
    {
        // Old-style PlaceObject tag; the corresponding Remove
        // is specific to the CharacterId.          
        id = Pos.CharacterId;        

        // Since they are loaded, character ids on stage should not include high bits.
        GASSERT((Pos.CharacterId.GetIdValue() & 0xFFFF) == 0);
    }
    return DepthResId(GFxResourceId(),Pos.Depth);
}




//
// ***** GFxRemoveObject2
//

void    GFxRemoveObject2::Read(GFxLoadProcess* p, GFxTagType tagType)
{
    GASSERT(tagType == GFxTag_RemoveObject || tagType == GFxTag_RemoveObject2);

    if (tagType == GFxTag_RemoveObject)
    {
        // Older SWF's allow multiple objects at the same depth;
        // this Id disambiguates.  Later SWF's just use one
        // object per depth.
        Id = GFxResourceId(p->ReadU16());
    }

    Depth = p->ReadU16();
}

void    GFxRemoveObject2::Execute(GFxSprite* m)
{
    m->RemoveDisplayObject(Depth, Id);
}

void    GFxRemoveObject2::ExecuteStateReverse(GFxSprite* m, int frame)
{
    // Find a previous replace/add tag and compute the necessary object position. This
    // position may not be the same as in ptag, since it can come from even earlier frame.
    GFxCharPosInfo      pos;
    UInt                tagFrame = (UInt)frame;
    GFxPlaceObject2*    ptag = m->FindPreviousPlaceObjectInfo(&pos, &tagFrame, Depth, Id, 0);

    // Reverse of remove is to re-add the previous object.

    if (ptag)
    {
        // If this tag adds object, they need to be tagged with the tags createFrame;
        // so that it is not accidentally unloaded during a seek.
        ptag->ExecuteWithPos(pos, m, tagFrame);
    }   
    else
    {
        GFC_DEBUG_WARNING2(1, "Reverse REMOVE can't find previous replace or add Tag(%d, %d)",
            frame, Depth);
    }
}



//
// ***** GFxSetBackgroundColor
//

void    GFxSetBackgroundColor::Execute(GFxSprite* m)
{
    GFxMovieRoot *pmroot = m->GetMovieRoot();
    Float         alpha  = pmroot->GetBackgroundAlpha();
    Color.SetAlpha( UByte(gfrnd(alpha * 255.0f)) );
    pmroot->SetBackgroundColor(Color);
}

void    GFxSetBackgroundColor::Read(GFxLoadProcess* p)
{
    p->GetStream()->ReadRgb(&Color);

    p->LogParse("  SetBackgroundColor: (%d %d %d)\n",
        (int)Color.GetRed(), (int)Color.GetGreen(), (int)Color.GetBlue());
}


// ****************************************************************************
// Helper function to load variables from a file
//
bool GFx_LoadVariables(GASEnvironment* penv, GASObjectInterface* pchar, GFile *pfile)
{
    // bail if empty file
    SInt flen;
    if ((flen = pfile->GetLength()) == 0)
        return false;


    // read everything and strip url encoding
    UByte* td = (UByte*) GALLOC(flen);
    pfile->Read(td, flen);

    if (pchar->GetObjectType() == GASObjectInterface::Object_LoadVars)
    {
        GASLoadVarsObject* lvobj = (GASLoadVarsObject*) pchar;
        lvobj->SetLoadedBytes(flen);
    }

    // PPS: the following does not support UTF
    GFxString data;
    GASGlobalContext::Unescape((const char*)td, flen, &data);
    GFREE(td);

    return GASLoadVarsProto::LoadVariables(penv, pchar, data);
}

// ****************************************************************************
// Helper function to load data from a file
//
bool GFx_LoadData(GASObjectInterface* pchar, GFile *pfile, GFxString& data)
{
    // bail if empty file
    SInt flen;
    if ((flen = pfile->GetLength()) == 0)
        return false;

    // read everything and return
    UByte* td = (UByte*) GALLOC(flen);
    pfile->Read(td, flen);

    if (pchar->GetObjectType() == GASObjectInterface::Object_LoadVars)
    {
        GASLoadVarsObject* lvobj = (GASLoadVarsObject*) pchar;
        lvobj->SetLoadedBytes(flen);
    }

    // PPS: the following converts byte stream to appropriate endianness
    // PPS: only for UTF16 (assuming sizeof(wchar_t) == sizeof(UInt16))
    UInt16* prefix = (UInt16*)td;
    if (prefix[0] == GByteUtil::BEToSystem((UInt16)0xFFFE)) // little endian
    {
        for (SInt i=0; i < flen>>1; i++)
            prefix[i] = GByteUtil::LEToSystem(prefix[i]);
        data.AppendString( ((const wchar_t*)td)+1, (flen>>1)-1 );
    }
    else if (prefix[0] == GByteUtil::BEToSystem((UInt16)0xFEFF)) // big endian
    {
        for (SInt i=0; i < flen>>1; i++)
            prefix[i] = GByteUtil::BEToSystem(prefix[i]);
        data.AppendString( ((const wchar_t*)td)+1, (flen>>1)-1 );
    }
    else
    {
        data.AppendString((const char*)td, flen);
    }

    GFREE(td);
    return true;

}
