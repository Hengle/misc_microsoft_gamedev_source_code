/**********************************************************************

Filename    :   GFxMovieDef.cpp
Content     :   SWF Player Core movie data structures.
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   This file contains class declarations used in
                GFxPlayerImpl.cpp only. Declarations that need to be
                visible by other player files should be placed
                in GFxCharacter.h.


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxMovieDef.h"
#include "GFxLoadProcess.h"
#include "GFile.h"

// For GFxSprite and GFxMovieRoot
#include "GFxSprite.h"
#include "GFxPlayerImpl.h"
#include "GFxText.h"

#include "GFxFontGlyphPacker.h"
#include "GFxFontManager.h"

#include "GFxShape.h"


// This is the key string that will cause import substitution
// to use GFxFontLib instead in order to support internationalization.
#define GFX_FONTLIB_IMPORT_KEY  "gfxfontlib.swf"


// ***** GFxDataAllocator

// Allocation class used for tag data in GFxMovieDataDef.
GFxDataAllocator::GFxDataAllocator()
{
    pAllocations = 0;
    pCurrent     = 0;
    BytesLeft    = 0;
}

GFxDataAllocator::~GFxDataAllocator()
{
    while(pAllocations)
    {
        Block*  pnext = pAllocations->pNext;
        GFREE(pAllocations);
        pAllocations = pnext;
    }
}

void*   GFxDataAllocator::OverflowAlloc(size_t bytes)
{       
    // If the request is big, give it an individual chunk.
    if (bytes > BlockSize / 2)    
        return AllocIndividual(bytes);    

	const size_t AlignedBlockHeader = (sizeof(Block*) + (SYSTEMALIGNMENT - 1)) & ~(SYSTEMALIGNMENT - 1);

    if (bytes > BytesLeft)
    {
        Block* pblock = (Block*) GALLOC(BlockSize + AlignedBlockHeader);
        if (!pblock)
            return 0;
        // Insert to allocated list.
        pblock->pNext = pAllocations;
        pAllocations = pblock;
        // Assign free space.
		pCurrent = (UByte*) (pblock);
		pCurrent+=AlignedBlockHeader;
        BytesLeft= BlockSize;
    }

    void* pmem = pCurrent;
    pCurrent += bytes;
    BytesLeft -= bytes;

    return pmem;
}

void*    GFxDataAllocator::AllocIndividual(size_t bytes)
{
    Block* pblock = (Block*) GALLOC(bytes + sizeof(Block));
    if (!pblock)
        return 0;
    // Insert to allocated list.
    pblock->pNext = pAllocations;
    pAllocations = pblock;
    return (pblock + 1);
}


// ***** GFxMovieDataDef


GFxMovieDataDef::GFxMovieDataDef(const GFxResourceKey &creationKey)
    : ResourceKey(creationKey),
      GradientIdGenerator(GFxResourceId::IdType_GradientImage)
{
    FileAttributes  = 0;
    pMetadata       = 0;
    MetadataSize    = 0;

    pPathAllocator  = new GFxPathAllocator();
  
    LoadState       = LS_Unitialized;
    LoadingFrame    = 0;
    TagCount        = 0;

    ResIndexCounter = 0;

    MovieType       = MT_Empty;

    InitActionsCnt  = 0;

    // Add an empty character id, for createEmptyMovieClip.
    GPtr<GFxSpriteDef> pdef = *new GFxSpriteDef(this);
    pdef->InitEmptyClipDef();
    AddResource(GFxResourceId(CharId_EmptyMovieClip), pdef);

    GPtr<GFxEditTextCharacterDef> ptextDef = *new GFxEditTextCharacterDef(this);
    ptextDef->InitEmptyTextDef();
    AddResource(GFxResourceId(CharId_EmptyTextField), ptextDef);
}

// Initialize GFxMovieDataDef() with a header loaded into processInfo.
GFxMovieDataDef::GFxMovieDataDef(GFxLoaderImpl::SWFProcessInfo& pi, const GFxResourceKey &creationKey)
    : ResourceKey(creationKey),
      GradientIdGenerator(GFxResourceId::IdType_GradientImage)
{
    FileAttributes  = 0;
    pMetadata       = 0;
    MetadataSize    = 0;

    pPathAllocator  = new GFxPathAllocator();

    LoadState       = LS_LoadingFrames;
    LoadingFrame    = 0;
    TagCount        = 0; // Should be based on export?

    ResIndexCounter = 0;

    MovieType       = MT_Flash;

    InitActionsCnt  = 0;

    // Store the file path & other variables.
    FileURL     = pi.Stream.GetUnderlyingFile()->GetFilePath();
    Header      = pi.Header;

    // Add an empty character id, for createEmptyMovieClip.
    GPtr<GFxSpriteDef> pdef = *new GFxSpriteDef(this);
    pdef->InitEmptyClipDef();
    AddResource(GFxResourceId(CharId_EmptyMovieClip), pdef);

    GPtr<GFxEditTextCharacterDef> ptextDef = *new GFxEditTextCharacterDef(this);
    ptextDef->InitEmptyTextDef();
    AddResource(GFxResourceId(CharId_EmptyTextField), ptextDef);
}


GFxMovieDataDef::~GFxMovieDataDef()
{

    // Destroy frame data.
    UInt i;
    for(i=0; i<Playlist.size(); i++)
        Playlist[i].DestroyTags();
    for(i=0; i<InitActionList.size(); i++)
        InitActionList[i].DestroyTags();

    // Destroy imports
    for(i=0; i<ImportData.size(); i++)
        delete ImportData[i];

    if (pPathAllocator)
        delete pPathAllocator;

    if (pMetadata)
    {
        GFREE(pMetadata);
        pMetadata = 0;
    }   
}




// Initialize an empty movie definition.
void GFxMovieDataDef::InitEmptyMovieDef(const char *purl)
{
    GASSERT(LoadState == LS_Unitialized);

    FileURL           = purl;
    TagCount          = 0;

    Header.FileLength = 0;
    Header.FPS        = 1.0f;
    Header.SWFFlags   = 0;
    Header.Version    = GFC_MAX_UINT;

    // Set FrameCount = 1; that is the default for an empty clip.
    Header.FrameCount = 1;

    Playlist.resize(Header.FrameCount);
    InitActionList.resize(Header.FrameCount);
    InitActionsCnt  = 0;

    LoadingFrame = Header.FrameCount;
    LoadState    = LS_LoadFinished;
}

// Create a definition describing an image file.
void    GFxMovieDataDef::InitImageFileMovieDef(const char *purl, UInt fileLength,
                                               GFxImageResource *pimageResource, bool bilinear)
{
    // Configure us to have one frame - same as empty initialization.
    InitEmptyMovieDef(purl);
    Header.FileLength = fileLength;

    GASSERT(Header.FrameCount == 1);
    GASSERT(Playlist.size() == 1);

    // Create image resource and shape character using it.    
    AddResource(GFxResourceId(CharId_ImageMovieDef_ImageResource), pimageResource);

    GPtr<GFxShapeCharacterDef> pshapeDef = *new GFxShapeCharacterDef();
    pshapeDef->SetToImage(pimageResource, bilinear);
    AddCharacter(GFxResourceId(CharId_ImageMovieDef_ShapeDef), pshapeDef); 

    // Add a PlaceObject command, so that it creates a shape.
    // We use an individual allocator since we don't want a block of memory to be wasted.
    void* ptagMem = TagMemAllocator.AllocIndividual(
                            sizeof(GFxPlaceObject2) + sizeof(GASExecuteTag*));
    if (ptagMem)
    {
        GASExecuteTag**  pptagArray = (GASExecuteTag**) ptagMem;
        GFxPlaceObject2* ptag       = (GFxPlaceObject2*) (pptagArray + 1);
        GTL::gconstruct<GFxPlaceObject2>(ptag);

        // Verified: Flash assigns depth -16383 to image shapes (depth value = 1 in our list).
        SInt            depth = 1;
        GFxCharPosInfo  pos = GFxCharPosInfo(GFxResourceId(GFxCharacterDef::CharId_ImageMovieDef_ShapeDef),
                            depth, 0, GRenderer::Cxform(), 1, GRenderer::Matrix());
        ptag->InitializeToAdd(pos);

        // Add playlist frame.
        GASSERT(Playlist.size() == 1);
        pptagArray[0] = ptag;
        Playlist[0].pTagPtrList = pptagArray;
        Playlist[0].TagCount = 1;
    }
    else
    {
        // Not enough memory for tags.
        GASSERT(0);
    }
}



// Read a .SWF pMovie.
void    GFxMovieDataDef::Read(GFxLoadProcess *plp, GFxMovieBindProcess* pbp)
{
    GASSERT(LoadState == LS_LoadingFrames);
    // LoadState's DataDef must be initialized to us.
    GASSERT(plp->GetDataDef() == this);


    // Reference stream for easy access
    GFxStream   &stream = *plp->GetStream();
       
 
    // Create list and process tags
    Playlist.resize(Header.FrameCount);
    InitActionList.resize(Header.FrameCount);

    stream.LogParseClass(Header.FrameRect); 
    stream.LogParse("Note: SWF Frame Rate = %f, Frames = %d\n",
                    Header.FPS, Header.FrameCount);

    LoadState = LS_LoadingFrames;

    UInt    prevFrameResourceIndex = ResIndexCounter;

    TagCount = 0;
    while ((UInt32) stream.Tell() < plp->ProcessInfo.FileEndPos)
    {
        GFxTagInfo tagInfo;
        GFxTagType tagType = stream.OpenTag(&tagInfo);

        plp->ReportProgress(FileURL, tagInfo, (UInt32) stream.Tell(), plp->ProcessInfo.FileEndPos);

        GFxLoaderImpl::LoaderFunction   lf = NULL;
        //in->LogParse("tagType = %d\n", tagType);
        
        if (tagType == GFxTag_EndFrame)
        {            
           // We handle EndFrame logic after the tag is closed below.
           // That is helpful to ensure that reported bytesLoaded position is correct.
        }
        else if (GFxLoaderImpl::GetTagLoader(tagType, &lf))
        {
            /* Tag decoding helper code
          //  if ((tagType == 83) || (tagType == 2) || (tagType == 32) ||
                // Morph
          //      (tagType == 46) || (tagType == 84) )
            {
                int start = stream.Tell();
                stream.LogParse("*** Tag data for tag %d:\n", tagType);
                stream.LogTagBytes();
                stream.SetPosition(start);
            }
            */

            // call the tag loader.  The tag loader should add
            // characters or tags to the GFxMovieSub data structure.
            (*lf)(plp, tagInfo);
        }
        else
        {
            // no tag loader for this tag type.
            stream.LogParse("*** no tag loader for type %d\n", tagType);
            stream.LogTagBytes();
        }

        stream.CloseTag();
        TagCount++;

        // Handle EndFrame after it's tag is closed.
        if (tagType == GFxTag_EndFrame)
        {            
            plp->CommitFrameTags();

            // Record the number of resources in frame so that they can be bound.
            // TBD: This will need thread-safe block/release semantic in the future.
            ResourcesInFrame.push_back(ResIndexCounter - prevFrameResourceIndex);
            FrameBytesLoaded.push_back(stream.Tell() - plp->ProcessInfo.FileStartPos);
            prevFrameResourceIndex = ResIndexCounter;

            // Show frame tag -- advance to the next frame.
            stream.LogParse("  ShowFrame\n");
            IncrementLoadingFrame();

            // Transfer loaded resources to frame
            // If binding was requested, do binding.
            if (pbp)
            {
                pbp->BindNextFrame();
                // Do we need to do anything about bind errors? Technically
                // they will be sticky in the bind process.
            }
        }

        else if (tagType == 0)
        {
            if ((unsigned)stream.Tell() != plp->ProcessInfo.FileEndPos)
            {
                // Safety break, so we don't read past the end of the pMovie.               
                stream.LogWarning("Warning: GFxLoader - GFxStream-end tag hit, but not at the "
                    "end of the file yet; stopping for safety\n");
                break;
            }
        }
    }

    // TBD: Shouldn't we verify the number of frames loaded and set a different state otherwise?

    // Set state before BindNextFrame is called.
    LoadState = LS_LoadFinished;

    if (plp->FrameTagsAvailable())
    {
        plp->CommitFrameTags();
        // Record the number of resources in frame so that they can be bound.
        // TBD: This will need thread-safe block/release semantic in the future.
        ResourcesInFrame.push_back(ResIndexCounter - prevFrameResourceIndex);
        FrameBytesLoaded.push_back(stream.Tell() - plp->ProcessInfo.FileStartPos);
        prevFrameResourceIndex = ResIndexCounter;
        
        IncrementLoadingFrame();
        if (pbp)            
            pbp->BindNextFrame();
        // Do we need to do anything about bind errors? Technically
        // they will be sticky in the bind process.
    }
    
}


// Wait for the specified loading frame, returning 1 if succeeded and 0 if loading failed.
bool    GFxMovieDataDef::WaitForLoadingFrame(UInt frame)
{
    if (frame < LoadingFrame)
        return 1;
    if (LoadState == LS_LoadError)
        return 0;

    // For now, just verify that frame is available.
    GASSERT(frame >= LoadingFrame);
    return 0;
}


// Sets MetaData of desired size.
void    GFxMovieDataDef::SetMetadata(UByte *pdata, UInt size)
{
    // Should only set metadata once.
    GASSERT(pMetadata == 0);
    if ((pMetadata = (UByte*)GALLOC(size))!=0)
    {
        MetadataSize = size;
        memcpy(pMetadata, pdata, size);
    }   
}

UInt        GFxMovieDataDef::GetMetadata(char *pbuff, UInt buffSize) const
{
    if (!pbuff)
        return MetadataSize;
    buffSize = GTL::gmin<UInt>(buffSize, MetadataSize);
    if (pMetadata)
        memcpy(pbuff, pMetadata, buffSize);
    return buffSize;
}




// Add a resource during loading.
void    GFxMovieDataDef::AddResource(GFxResourceId rid, GFxResource* pres)
{
    Resources.add(rid, GFxResourceHandle(pres));
}
/*
// May not be necessary since ResourceData object is always used at this point
void    GFxMovieDataDef::AddResource(GFxResourceId rid, GFxResourceFileInfo* pfileInfo)
{    
    Resources.add(rid,
        GFxResourceHandle(GFxResourceHandle::RH_Index, ResIndexCounter));
    ResIndexCounter++;
    //ResourceFileData.add(resourceId, pfileInfo);
}
*/

// Adds a new resource and generates a handle for it.
GFxResourceHandle   GFxMovieDataDef::AddDataResource(GFxResourceId rid, const GFxResourceData &resData)
{    
    GFxResourceHandle rh(GFxResourceHandle::RH_Index, ResIndexCounter);
    ResIndexCounter++;
   
    Resources.add(rid, rh);
    ResourceBindData.push_back(resData);
    
    GASSERT(ResourceBindData.size() == ResIndexCounter);

    return rh;
}

// Add import resource and generate a handle for it.
GFxResourceHandle   GFxMovieDataDef::AddImportResource(GFxResourceId rid, GFxImportData *pimportData)
{
    GUNUSED(pimportData);

    GFxResourceHandle rh(GFxResourceHandle::RH_Index, ResIndexCounter);
    ResIndexCounter++;
    Resources.add(rid, rh);

    // Do we need to add some particular node to ResourceBindData for imports?
    // or can we just leave it null?

    // Just add empty node for now.
    ResourceBindData.push_back(GFxResourceData());
    GASSERT(ResourceBindData.size() == ResIndexCounter);
    return rh;   
}




//
// GFxFontResource loaders
//


class GFxFontResourceCreator : public GFxResourceData::DataInterface
{
    typedef GFxResourceData::DataHandle DataHandle;

    // Creates/Loads resource based on data and loading process
    virtual bool    CreateResource(DataHandle hdata, GFxResourceBindData *pbindData,
                                   GFxLoadStates *plp) const;
};

// Creates/Loads resource based on data and loading process.
bool   GFxFontResourceCreator::CreateResource(DataHandle hdata,
                                              GFxResourceBindData *pbindData,
                                              GFxLoadStates *pls) const
{

    GFxFontData *pfd = (GFxFontData*) hdata;
    GASSERT(pfd);

    // 1) Traverse through potential substitute fonts to see if substitution
    //    needs to take place.

    // FontResourceCreator may be responsible for substituting glyphs from
    // other files with '_glyphs' in their name.
    typedef GFxMovieDataDef::FontDataUse FontDataUse;

    GTL::garray<GPtr<GFxMovieDefImpl> >& fontDefs = pls->SubstituteFontMovieDefs;
    UInt    ifontDef, isourceFont;    

    for(ifontDef=0; ifontDef<fontDefs.size(); ifontDef++)
    {
        GFxMovieDefImpl*            pdefImpl = fontDefs[ifontDef];
        GTL::garray<FontDataUse>&   sourceFonts = pdefImpl->GetDataDef()->Fonts;

        // Search imports for the font with the same name which has glyphs.       
        for(isourceFont = 0; isourceFont < sourceFonts.size(); isourceFont++)
        {
            GFxFontData *psourceFontData = sourceFonts[isourceFont].pFontData;

            if (psourceFontData->GetGlyphShapeCount() > 0)
            {
                if (pfd->MatchSubstituteFont(psourceFontData))
                {                    
                    // Set our binding.
                    // Note: Unlike us, the source is guaranteed to be fully loaded when
                    // this takes place, so we can just look up its binding.
                    // Set binding together with its internal table.
                    pdefImpl->ResourceBinding.
                        GetResourceData(pbindData,
                                        sourceFonts[isourceFont].ResourceBindIndex);
                    return 1;
                }
            }
        }
    }

    // 2) Not substitution. Create a font based on our data.
    
    
    // First, try to create a system font if the shape has no glyphs.      
    if (!pfd->HasVectorOrRasterGlyphs() && pfd->GetName())
    {           
        // Lookup an alternative font name for the font.
        GFxFontMap* pfontMap        = pls->pBindStates->pFontMap;
        const char* pfontName       = pfd->GetName();
        const char* plookupFontName = pfontName;
        UInt        lookupFontFlags = pfd->GetCreateFontFlags();
        GFxFontMap::MapEntry fontEntry;

        if (pfontMap && pfontMap->GetFontMapping(&fontEntry, pfontName))
        {   
            plookupFontName = fontEntry.Name.ToCStr();
            lookupFontFlags = fontEntry.UpdateFontFlags(lookupFontFlags);
        }

        // Check GFxFontLib first.
        GFxFontLib *pfontLib = pls->pBindStates->pFontLib;
        if (pfontLib)
        {
            GFxMovieDefImpl* pdefImpl = pbindData->pBinding->GetOwnerDefImpl();
            GASSERT(pdefImpl != 0);
            
            // Override shared state so that we can provide our own LoadStates's
            // value for log & task manager. 
            class SharedState : public GFxSharedState
            {
                GFxLoadStates*   pLoadStates;
                GFxMovieDefImpl* pDefImpl;   
            public:

                SharedState(GFxMovieDefImpl* pdefImpl, GFxLoadStates* ploadStates)
                    : pLoadStates(ploadStates), pDefImpl(pdefImpl) { }

                // Delegate to DefImpl.
                virtual GFxSharedState* GetSharedImpl() const { return pDefImpl->pSharedState; }

                virtual GFxState*   GetStateAddRef(GFxState::StateType state) const
                {
                    if (state == GFxState::State_Log)
                    {
                        if (pLoadStates->pLog)
                            pLoadStates->pLog->AddRef();
                        return pLoadStates->pLog;
                    }
                    else if (state == GFxState::State_TaskManager)
                    {
                        if (pLoadStates->pTaskManager)
                            pLoadStates->pTaskManager->AddRef();
                        return pLoadStates->pTaskManager;
                    }

                    return GFxSharedState::GetStateAddRef(state);
                }
            };

            // Find/create compatible font through FontLib.
            GFxFontLib::FontResult fr;
            SharedState            ss(pdefImpl, pls);

            if (pfontLib->FindFont(&fr, plookupFontName, lookupFontFlags,
                                   pdefImpl, &ss))
            {
                
                // AddDefImpl to our list of dependencies.
                if (fr.GetMovieDef() != pdefImpl)
                    pdefImpl->ResourceImports.push_back((GFxMovieDefImpl*)fr.GetMovieDef());
                // Store resource pointer.
                pbindData->pResource = fr.GetFontResource();
            }
        }

        // Then GFxFontProvider.
        if (!pbindData->pResource && pls->pBindStates->pFontProvider)
        {
            // Create a system font from the provider (the font may be looked up from
            // resource lib if it already exists).
            pbindData->pResource =
                *GFxFontResource::CreateFontResource(plookupFontName, lookupFontFlags,
                                                     pls->pBindStates->pFontProvider,
                                                     pls->GetLib());
        }
    }
   
    // If this is not a system font, create one based on out FontData.
    if (!pbindData->pResource)
    {
        // Our GFxMovieDefImpl's pBinding should have been provided by caller.
        pbindData->pResource = *new GFxFontResource(pfd, pbindData->pBinding);
    }


    // We could do font glyph rendering here, however it is better to delay it
    // till the end of frame binding so that textures from several fonts
    // can be potentially combined.

    return pbindData->pResource ? 1 : 0;
}




// Adds a font data resource. Fonts are special because we need to be able to
// look them up by frames and do substitution even when they are not imported.
GFxResourceHandle   GFxMovieDataDef::AddFontDataResource(GFxResourceId rid, GFxFontData *pfontData)
{
    GASSERT(pfontData);

    static GFxFontResourceCreator creatorInstance;
    GFxResourceData fontResourceData(&creatorInstance, pfontData);

    GFxResourceHandle rh = AddDataResource(rid, fontResourceData);

    // Add FontDataUse
    FontDataUse fduse;
    fduse.pFontData = pfontData;    
    fduse.Id        = rid;    
    fduse.ResourceBindIndex = rh.GetBindIndex();
    Fonts.push_back(fduse);

    return rh;
}

// Get font data bi ResourceId.
GFxFontData* GFxMovieDataDef::GetFontData(GFxResourceId rid)
{
    for(UInt i=0; i<Fonts.size(); i++)    
        if (Fonts[i].Id == rid)
            return Fonts[i].pFontData;
    return 0;
}


bool    GFxMovieDataDef::GetResourceHandle(GFxResourceHandle* phandle, GFxResourceId rid) const
{
    ResourceHash::const_iterator ir = Resources.find(rid);
    if (ir != Resources.end())
    {
        *phandle = ir->second;
        return 1;
    }
    return 0;
}



// Helper used to look up labeled frames and/or translate frame numbers from a string.
bool    GFxMovieDataDef::TranslateFrameString(
    const GFxStringHash<UInt>	  &namedFrames,
    const char* label, UInt* frameNumber, bool translateNumbers)
{
    if (!label || !label[0])
        return 0;

    // First, determines if a string is a frame number. Generally,
    // if it is NOT a frame number, the string is treated as a label.
    // However, there are some cases when a string number check is
    // not done (i.e. op 0x8C), so that is treated as an option.
    if (translateNumbers)
    {
        // The string must evaluate to an integer to be converted to
        // frame number; frames like "8.5" are treated as labels. Whitespace,
        // '+' or '-' characters, however, are allowed in front of a number string.
        // However, any other characters in the string would cause it to be a label.
        // TBD: May be that matches ECMA ActionScript number conversion ?

        bool        digitFound  = 0;            
        int         i;

        // Check whether to interpret as a number or a label.
        for (i=0; label[i] != 0; i++)
        {
            UByte ch = label[i];
            if ((ch>='0') && (ch <= '9'))
            {
                digitFound = 1;
                continue;
            }
            if (ch == '+' || ch == '-')
            {
                if (!digitFound)
                    continue;
            }
            if (ch == ' ' || ch == '\t')
                continue;
            // Any other char? It's a label.
            goto translate_frame_label;
        }

        // This must be  a frame number: do conversion.
        char*   tail   = 0;
        UInt    number = (UInt) strtod(label, &tail);
        // Check for conversion failure.
        if (tail == label || *tail != 0)
            return 0;

        // Frames number arguments start with 1, so make it 0-based.
        *frameNumber = number - 1;
        return 1;
    }

translate_frame_label:

    // We have found a label, do lookup.
	return namedFrames.get_CaseInsensitive(GFxString(label), frameNumber);
}



// Labels the frame currently being loaded with the given name.
// A copy of the name string is made and kept in this object.    
void    GFxMovieDataDef::AddFrameName(const char* name, GFxLog *plog)
{
    GUNUSED(plog);
    GASSERT(LoadingFrame >= 0 && LoadingFrame < Header.FrameCount);

    GFxString   n = name;
    //GASSERT(NamedFrames.get(n, NULL) == false);   // frame should not already have a Name (?)
    NamedFrames.set_CaseInsensitive(n, LoadingFrame);   // stores 0-based frame #
}


void    GFxMovieDataDef::SetLoadingPlaylistFrame(const Frame& frame)
{
    // We should not call SetLoadingPlaylistFrame() for the same frame multiple times.
    GASSERT(Playlist[LoadingFrame].TagCount == 0);
    Playlist[LoadingFrame] = frame;
}

void    GFxMovieDataDef::SetLoadingInitActionFrame(const Frame& frame)
{
    GASSERT(InitActionList[LoadingFrame].TagCount == 0);
    InitActionList[LoadingFrame] = frame;
    ++InitActionsCnt;
}

// Adds ImportData, ownership passed to the Imports array.
void    GFxMovieDataDef::AddImport(GFxImportData* pimportData)
{
    ImportData.push_back(pimportData);
}

// Expose one of our resources under the given symbol,
// for export.  Other movies can import it.    
void    GFxMovieDataDef::ExportResource(const GFxString& symbol, GFxResourceId rid, GFxResourceHandle hres)
{
    // SWF sometimes exports the same thing more than once!
	Exports.set_CaseInsensitive(symbol, hres);
    InvExports.set(rid, symbol);
}    


const GFxTimelineDef::Frame&   GFxMovieDataDef::GetPlaylist(int frameNumber) const
{
	// Thread-support TBD
	// We will need to introduce a lock
	// or copy on write access when returning frames while
	// the movie is still loading

	return Playlist[frameNumber];
}
const GFxTimelineDef::Frame*	GFxMovieDataDef::GetInitActions(int frameNumber) const
{
	if (((UInt)frameNumber) >= InitActionList.size())
		return 0;

	return &InitActionList[frameNumber];
}





// *** Creating MovieDefData file keys


// GFxMovieDataDef key: {FileName, pFileOpener, optional pImageCreator}
class GFxMovieDataDefFileKeyData : public GRefCountBase<GFxMovieDataDefFileKeyData>
{
    friend class GFxMovieDataDefFileKeyInterface;

    GFxString               FileName;
    GPtr<GFxFileOpener>     pFileOpener;
    
    SInt64                  ModifyTime;
    
    // pImageCreator is unique in that it is only used for GFxMovieDataDef binding
    // if GFxImageCreator::IsKeepingImageData returns false and GFxLoader::LoadKeepBindData
    // flag is not specified. If one of those flags are set, GFxImageCreator is only
    // used as an argument for GFxMovieDefImpl and not MovieDataDef. In that case,
    // its value here will be null and image resources are not created until binding time.
    GPtr<GFxImageCreator>   pImageCreator;    

public:

    GFxMovieDataDefFileKeyData(const char* pfilename, SInt64 modifyTime,
                               GFxFileOpener* pfileOpener, GFxImageCreator *pimageCreator)
    {
        FileName     = pfilename;
        ModifyTime   = modifyTime;
        pFileOpener  = pfileOpener;
        pImageCreator= pimageCreator;
    }

    bool operator == (GFxMovieDataDefFileKeyData& other) const
    {
        return (pFileOpener == other.pFileOpener &&
                pImageCreator == other.pImageCreator &&
                ModifyTime == other.ModifyTime &&
                FileName == other.FileName);
    }
    bool operator != (GFxMovieDataDefFileKeyData& other) const
    {
        return !operator == (other);
    }

    size_t  GetHashCode() const
    {
        size_t fileHashCode = GFxString::BernsteinHashFunction(FileName.ToCStr(),
                                                               FileName.GetSize());
        return fileHashCode ^
            ((size_t)ModifyTime) ^
            ((size_t)pFileOpener.GetPtr()) ^ (((size_t)pFileOpener.GetPtr()) >> 7) ^
            ((size_t)pImageCreator.GetPtr()) ^ (((size_t)pImageCreator.GetPtr()) >> 7);
    }
};


class GFxMovieDataDefFileKeyInterface : public GFxResourceKey::KeyInterface
{
public:
    typedef GFxResourceKey::KeyHandle KeyHandle;

    virtual void    AddRef(KeyHandle hdata)
    {
        GASSERT(hdata); ((GFxMovieDataDefFileKeyData*) hdata)->AddRef();
    }
    virtual void    Release(KeyHandle hdata)
    {        
        GASSERT(hdata); ((GFxMovieDataDefFileKeyData*) hdata)->Release();
    }

    // Key/Hash code implementation.
    virtual GFxResourceKey::KeyType GetKeyType(KeyHandle hdata) const
    {
        GUNUSED(hdata);
        return GFxResourceKey::Key_File;
    }

    virtual size_t  GetHashCode(KeyHandle hdata) const
    {
        GASSERT(hdata);        
        return ((GFxMovieDataDefFileKeyData*) hdata)->GetHashCode();
    }

    virtual bool    KeyEquals(KeyHandle hdata, const GFxResourceKey& other)
    {
        if (this != other.GetKeyInterface())
            return 0;        
        return *((GFxMovieDataDefFileKeyData*) hdata) ==
               *((GFxMovieDataDefFileKeyData*) other.GetKeyData());
    }

    const char* GetFileURL(KeyHandle hdata) const
    {
        GFxMovieDataDefFileKeyData* pdata = (GFxMovieDataDefFileKeyData*) hdata;
        GASSERT(pdata);
        return pdata->FileName.ToCStr();
    }
};


static GFxMovieDataDefFileKeyInterface GFxMovieDataDefFileKeyInterface_Instance;

// Create a key for an SWF file corresponding to GFxMovieDef.
GFxResourceKey  GFxMovieDataDef::CreateMovieFileKey(const char* pfilename,
                                                    SInt64 modifyTime,
                                                    GFxFileOpener* pfileOpener,
                                                    GFxImageCreator* pimageCreator)
{
    GPtr<GFxMovieDataDefFileKeyData> pdata =
        *new GFxMovieDataDefFileKeyData(pfilename, modifyTime, pfileOpener, pimageCreator);

    return GFxResourceKey(&GFxMovieDataDefFileKeyInterface_Instance,
                          (GFxResourceKey::KeyHandle)pdata.GetPtr() );
}



// ***** GFxMovieDefBindStates

void    GFxMovieDefBindStates::SetDataDef(GFxMovieDataDef* pdef)
{
    // DataDef can only be set once and should be valid from that point on.
    GASSERT((pDataDef.GetPtr() == 0) && (pdef != 0));
    pDataDef = pdef;

    // Extract and store relative path from parent def.
    RelativePath = pdef->GetFileURL();    
    if (!GFxURLBuilder::ExtractFilePath(&RelativePath))
        RelativePath.Clear();
}



//
// ***** GFxMovieDefImpl
//

GFxMovieDefImpl::GFxMovieDefImpl(GFxMovieDefBindStates* pstates,
                                 GFxLoaderImpl* ploaderImpl,
                                 UInt loadConstantFlags,
                                 GFxSharedStateImpl *pdelegateState,
                                 bool fullyLoaded)    
{    
    ResourceBinding.SetOwnerDefImpl(this);

    LoadFlags   = loadConstantFlags;
    pLoaderImpl = ploaderImpl;

    // We MUST have states and DataDef
    GASSERT(pstates);

    pBindStates  = pstates;
    BindingFrame = 0;
    BytesLoaded  = 0;

    // Create a delegated shared state.
    pSharedState = *new GFxSharedStateImpl(pdelegateState);        
   

    if (fullyLoaded)
    {
        BindingFrame = GetDataDef()->GetLoadingFrame();
        BytesLoaded  = GetDataDef()->GetFileBytes();
    }
}

GFxMovieDefImpl::~GFxMovieDefImpl()
{
    // Release all binding references before ImportSourceMovies is
    // cleared. This is required because GFxSpriteDef references contain
    // GASExecuteTag tags that must be destroyed before MovieDefImpl is
    // released; thus all imported binding references must be cleared first.    
    // If this is not done tag destructors may crash if MovieDefImpl was
    // released first.
    ResourceBinding.Destroy();
}



// *** GFxMovieDefBindProcess - used for GFxMovieDefImpl Binding
    
GFxMovieBindProcess::GFxMovieBindProcess(GFxLoadStates *pls, GFxMovieDefImpl* pdefImpl)
    : GlyphTextureIdGen(GFxResourceId::IdType_DynFontImage)
{
    // The frame we are currently binding.
    State           = Bind_NotStarted;

    BindIndex       = 0;
    ImportIndex     = 0;
    FontUseIndex    = 0; 
    pLoadStates     = pls;

    pDataDef = pls->GetDataDef();
    pDefImpl = pdefImpl;
    GASSERT(pDataDef == pdefImpl->GetDataDef());
    Stripped = ((pDataDef->GetSWFFlags() & GFxMovieInfo::SWF_Stripped) != 0);
}


GFxMovieBindProcess::BindState GFxMovieBindProcess::BindNextFrame()
{    
    // Check state and return correct value if appropriate.
    if (State != Bind_InProgress)
    {
        if (State == Bind_NotStarted)
        {
            State = Bind_InProgress;
            
            // Binding can only take place once.
            GASSERT(pDefImpl->BindingFrame == 0);
        }
        else
        {
            return State;
        }
    }

    // Make sure that the frame we are interested in is available.
    if (!pDataDef->WaitForLoadingFrame(pDefImpl->BindingFrame))
    {
        // Error occurred in load, turn it into error in processing.
        State = Bind_Error;
        return State;
    }

    // Main task of binding:
    //  - populate the ResourceBinding table to resolve Handles
    //  - for all imported movies store references in ImportSourceMovies
    
    GFxLoadStates *pls        = pLoadStates;
    UInt           loadFlags  = pDefImpl->GetLoadFlags();
    bool           fontImportsSucceeded = true;

    // Resolve all imports for this frame.
    if (!(loadFlags & GFxLoader::LoadDisableImports))
    {  
        while ((ImportIndex < pDataDef->ImportData.size()) &&
               (pDataDef->ImportData[ImportIndex]->Frame <= pDefImpl->BindingFrame))
        {
            // Load imports at index.
            GFxImportData*   pimportData = pDataDef->ImportData[ImportIndex];
            const GFxString& sourceURL   = pimportData->SourceUrl;           

            // See if the source URL ends in 'gfxfontlib.swf', in which
            // case we do import font substitution instead.
            static const char fontlibKey[]   = GFX_FONTLIB_IMPORT_KEY;
            const UPInt       fontlibKeySize = sizeof(fontlibKey) - 1;

            if (pLoadStates->pBindStates->pFontLib &&
                (sourceURL.GetSize() >= fontlibKeySize) &&
                (GFxString::CompareNoCase(sourceURL.ToCStr() +
                    (sourceURL.GetSize() - fontlibKeySize), fontlibKey) == 0))
            {
                // This if a substituted font import, handle it in a custom manner.
                bool allSucceed = pDefImpl->ResolveImportThroughFontLib(ImportIndex);
                if (!allSucceed)
                    fontImportsSucceeded = 0;

                // TO DO: Font import substitution could cause font resource to be assigned 
                // to resource data slots of different type (import, etc.) in pathological
                // cases. We should check for that in the future perhaps by verifying the
                // type during binding load time...
                ImportIndex++;
                continue;
            }


            GFxMovieDefImpl*    pdef = 0;

            // LoadStates are separate in import because import has a separate MovieDef.
            GPtr<GFxLoadStates> pimportLoadStates = *pls->CloneForImport();
            // Load flags must be the same, but we wait for load completion with imports.
            UInt                importLoadFlags = loadFlags | GFxLoader::LoadWaitCompletion;

            // Try both SWF and GFX files.
            // If the file is stripped, its imports may be stripped too,
            // so try to load '.gfx' import file first.
    	
		    // We don't use GetLength() because ".swf" is represented as 4 bytes in UTF8
	        if (Stripped && (sourceURL.GetSize() > 4) &&
		            (GFxString::CompareNoCase(sourceURL.ToCStr() +
		            (sourceURL.GetSize() - 4), ".swf") == 0) )
            {           
                GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_Import,
                                                sourceURL, pls->GetRelativePath());
			    loc.FileName.Resize(sourceURL.GetSize() - 4);
                loc.FileName += ".gfx";

                // Use our captured load state to load the imported movie. This
                // guarantees that the states are consistent.
                pdef = GFxLoaderImpl::CreateMovie_LoadState(pimportLoadStates,
                                                            loc, importLoadFlags);
            }

            if (!pdef)
            {
                GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_Import,
                                                sourceURL, pls->GetRelativePath());
                pdef = GFxLoaderImpl::CreateMovie_LoadState(pimportLoadStates,
                                                            loc, importLoadFlags);
            }

            if (!pdef)
            {
                // Failed loading; leave us at correct bind frame.
                State = Bind_Error;
                FinishBinding();
                return State;
            }
            else
            {
                // It is possible for SWF files to import themselves, which Flash
                // seems to handle gracefully. What happens with indirect recursiveness?
                // TBD: For now we the pass the recursive flag to avoid leaks,
                // but we will need to investigate this in detail in the future.
                bool recursive = (pdef == pDefImpl);
                if (recursive && pls->GetLog())
                    pls->GetLog()->LogWarning("Warning: Recursive import detected in '%s'\n", sourceURL.ToCStr());

                pDefImpl->ResolveImport(ImportIndex, pdef, pls, recursive);
                // Invoke default import visitor to notify it of resolve.
                GFxImportVisitor* pimportVistor = pls->GetBindStates()->pImportVisitor;
                if (pimportVistor)
                    pimportVistor->Visit(pDefImpl, pdef, sourceURL.ToCStr());

                pdef->Release();
            }

            ImportIndex++;
        }
    }

    
    // Create & Resolve all resources in the frame.
    UInt resourceCount = pDataDef->ResourcesInFrame[pDefImpl->BindingFrame];
    UInt ri;

    for (ri = 0; ri < resourceCount; ri++, BindIndex++)
    {
        GASSERT(BindIndex <= pDataDef->ResourceBindData.size());

        GFxResourceData& data = pDataDef->ResourceBindData[BindIndex];

        if (!data.IsValid())
        {
            // Must be an import handle index.
            // For imports, the handle must have already been resolved.
#ifdef GFC_BUILD_DEBUG
            if (!(loadFlags & GFxLoader::LoadDisableImports) && fontImportsSucceeded)
            {
                GFxResourceBindData rbd;
                pDefImpl->ResourceBinding.GetResourceData(&rbd, BindIndex);
                GASSERT(rbd.pResource.GetPtr() != 0);
            }
#else
            GUNUSED(fontImportsSucceeded);
#endif
        }
        else
        {
            // Create a resource based on its loaded data in DataDef.
            GFxResourceBindData bd;
            bd.pBinding = &pDefImpl->ResourceBinding;
            bd.pResource= 0;

            if (data.CreateResource(&bd, pls))
            {
                // SetBindData AddRefs to the resource.
                pDefImpl->ResourceBinding.SetBindData(BindIndex, bd);     
            }
            else
            {
                // Set empty binding otherwise: this allows for indexing
                // without problems.
                pDefImpl->ResourceBinding.SetBindData(BindIndex, bd);            

                // Fail the rest of loading/binding?
                // Do we block on a certain frame?
                // Who emits the error message?
            }
        }
    }


    // Make a list of fonts generated in this frame.
    // We can rely on FontUse do do this?
    GTL::garray<GFxFontResource*>   fonts;

    while(FontUseIndex < pDataDef->Fonts.size() &&
        pDataDef->Fonts[FontUseIndex].ResourceBindIndex < BindIndex)
    {            
        GFxResourceBindData rbd;
        pDefImpl->ResourceBinding.GetResourceData(
            &rbd, pDataDef->Fonts[FontUseIndex].ResourceBindIndex);

        if (rbd.pResource)
        {
            GASSERT(rbd.pResource->GetResourceType() == GFxResource::RT_Font);
            GFxFontResource* pfont = (GFxFontResource*) rbd.pResource.GetPtr();
            fonts.push_back(pfont);           
        }

        FontUseIndex ++;
    }

    /*
    This check used to be in GenerateFontBitmaps. Useful? TBD.

    const GFxExporterInfo* pexpInfo = GetExporterInfo();
    if (pexpInfo && (pexpInfo->ExportFlags & GFxExporterInfo::EXF_GlyphTexturesExported))
    return;
    */

    // Generate font bitmaps for the frame.
    if (fonts.size())
    {
        // NOTE: Some fonts may have already been rasterized if they were imported 
        // through '_glyphs' substitution, which can happen during CreateResource above.

        if (pls->GetFontPackParams() != 0)
        {            
            GFx_GenerateFontBitmaps(pls->GetFontPackParams(), fonts,
                                    pls->GetBindStates()->pImageCreator, 
                                    pls->GetRenderConfig(),
                                    pls->pLog,
                                    &GlyphTextureIdGen);
        }
        else
        {
            // Warn if there is no pack params and cache; this would cause glyphs to be rendered as shapes.
            GFC_DEBUG_WARNING(!pls->GetFontCacheManager() ||
                              !pls->GetFontCacheManager()->IsDynamicCacheEnabled(),
                "Both font packing and dynamic cache disabled - text will be vectorized");
        }                
    }


    // This frame done, advance and update state.
    pDefImpl->BytesLoaded = pDataDef->FrameBytesLoaded[pDefImpl->BindingFrame];
    pDefImpl->BindingFrame++;

    if (pDefImpl->BindingFrame == pDataDef->GetFrameCount())
    {  
        // Update bytes loaded for the end of the file. This is necessary
        // because we don't count the size of final tag 0 during loading.
        pDefImpl->BytesLoaded = pDataDef->GetFileBytes();

        State = Bind_Finished;
        FinishBinding();
    }

    return State;
}

void    GFxMovieBindProcess::FinishBinding()
{

    pDefImpl->ResourceBinding.Freeze();
}



// Grabs the stuff we want from the source pMovie.
void    GFxMovieDefImpl::ResolveImport(UInt importIndex, GFxMovieDefImpl* pdefImpl,
                                       GFxLoadStates *pls, bool recursive)
{
    GFxImportData*  pimport = GetDataDef()->ImportData[importIndex];
    bool            imported = false;

    // Go through all character ids and resolve their handles.
    for (UInt i=0; i< pimport->Imports.size(); i++)
    {
        GFxImportData::Symbol &symbol = pimport->Imports[i];

        // Look for exports and substitute.

        // Do the import.
        GFxResourceBindData bindData;        

        if (!pdefImpl->GetExportedResource(&bindData, symbol.SymbolName))
        {               
            LogError("Import error: GFxResource '%s' is not exported from movie '%s'\n",
                symbol.SymbolName.ToCStr(), pimport->SourceUrl.ToCStr());
        }
        else
        {
            // Add it to binding for the character.
            // Set imported flag so that we can AddRef to movieDef of an import,
            // allowing resources such as fonts to work right.
            imported = SetResourceBindData(GFxResourceId(symbol.CharacterId),
                                           bindData, symbol.SymbolName.ToCStr());
        }
    }

    // Hold a ref, to keep this source GFxMovieDef alive.
    // TBD: Why only add if imported ??
    if (imported)
    {
        if (!recursive)
            ImportSourceMovies.push_back(pdefImpl);
    }

    // No font substitution for recursive imports.
    if (recursive)
        return;


    // Name-based font import substitution:
    // The Font EXPORT feature in Flash Studio ignores the embedded glyphs and instead,
    // exports a fixed number of glyphs (243 or larger, depending on studio version), 
    // which makes it unstable for font imports. 
    // This name-based substitution can be used instead, replacing "Device Fonts" with
    // identically named font from the shared font.swf file.

    // To make exported fonts match up using export name we need glyphs to come
    // from a different file. Font substitution occurs if source import file name
    // has '_glyphs' in it.
    bool        forceFontSubstitution = 0;    
    GFxString   lowerURL = pimport->SourceUrl.ToLower();

    if (strstr(lowerURL.ToCStr(), "_glyphs") != 0)
    {
        forceFontSubstitution = 1;

        // Save substitution so that it can be potentially checked.
        pls->SubstituteFontMovieDefs.push_back(pdefImpl);
    }


    /*  Font substitution difficulties and how they are addressed:

    Full font substitution can not take place here because some of the
    FontUse entries (for future frames) may not yet be created. Similarly
    there might be some thread considerations having to do with overwriting
    fonts bound in previous frames.

    Specifically, there are two substitution problems:

    1) Previously bound fonts could have already been used by text field
    instances; substituting them may be too late.

    For now: Go through list, substitute anyway. This would not be
    a problem if we could force all imports to take place before
    font definitions. It is not clear if this is practical and can be
    controlled in Flash studio to the extent we need - TBD (need
    to research this).

    For now, we can have text field cached texture glyph batches store
    original font pointers and compare them to indexed font handle lookup
    result. Text fields would then be updated in the event of change.
    TBD - any thread issues with this?


    2) Future fonts may not have FontUse array entries and thus need to 
    be substituted later on.

    Solution: we add all forced substitutes sources to the
    pls->SubstituteFontMovieDefs array. This array will need to be checked
    when GFxFontResourceCreator::CreateResource is called to create
    GFxFontResource from GFxFontData.

    */


    // Iterate through fonts and resolve fonts with no glyphs from the def.
    typedef GFxMovieDataDef::FontDataUse FontDataUse;

    UInt                        ifont, isourceFont;
    GTL::garray<FontDataUse>&   fonts = GetDataDef()->Fonts;
    GTL::garray<FontDataUse>&   sourceFonts = pdefImpl->GetDataDef()->Fonts;

    for(ifont = 0; ifont < fonts.size(); ifont++)
    {        
        GFxFontData *pfontData = fonts[ifont].pFontData;

        if ((pfontData->GetGlyphShapeCount() == 0) || forceFontSubstitution)
        {
            // Search imports for the font with the same name which has glyphs.       
            for(isourceFont = 0; isourceFont < sourceFonts.size(); isourceFont++)
            {
                GFxFontData *psourceFontData = sourceFonts[isourceFont].pFontData;

                if (psourceFontData->GetGlyphShapeCount() > 0)
                {
                    if (pfontData->MatchSubstituteFont(psourceFontData))
                    {
                        // With DefData separation, we do not need to ReplaceExport
                        // any more; updating the binding should be good enough.

                        // Set our binding.
                        // Note: Unlike us, the source is guaranteed to be fully loaded when
                        // this takes place, so we can just look up its binding.
                        GFxResourceBindData sourceBindData;
                        pdefImpl->ResourceBinding.GetResourceData(&sourceBindData,
                            sourceFonts[isourceFont].ResourceBindIndex);

                        if (sourceBindData.pResource)
                        {
                            ResourceBinding.SetBindData(
                                fonts[ifont].ResourceBindIndex, sourceBindData);
                        }
                        
                        break;
                    }
                }
            }
        }
    }


}



// Resolves an import of 'gfxfontlib.swf' through the GFxFontLib object.
bool    GFxMovieDefImpl::ResolveImportThroughFontLib(UInt importIndex)
{
    GFxImportData*  pimport = GetDataDef()->ImportData[importIndex];    
    GFxFontLib*     pfontLib = pBindStates->pFontLib;
    GFxFontMap*     pfontMap = pBindStates->pFontMap;
    bool            allSucceeded = 1;

    // Go through all character ids and resolve their handles.
    for (UInt i=0; i< pimport->Imports.size(); i++)
    {
        GFxImportData::Symbol &symbol = pimport->Imports[i];

        // Lookup an alternative font name for the font; this should almost always 
        // be used so that a game import font such as $FontNormal can be translated
        // to a specific internationalized font name such as "Arial Unicode MS".
        const char* pfontName       = symbol.SymbolName.ToCStr();
        const char* plookupFontName = pfontName;
        UInt        lookupFontFlags = 0;
        GFxFontMap::MapEntry fontEntry;

        if (pfontMap && pfontMap->GetFontMapping(&fontEntry, pfontName))
        {            
            plookupFontName = fontEntry.Name.ToCStr();
            lookupFontFlags = fontEntry.UpdateFontFlags(lookupFontFlags);
        }

        // Find/create compatible font through FontLib.
        // We can pass our own pSharedState here because it must match the
        // current LoadState while the imports are loading.
        GFxFontLib::FontResult fr;
        if (pfontLib->FindFont(&fr, plookupFontName, lookupFontFlags, this, pSharedState))
        {
            // AddDefImpl to our list of dependencies.
            GFxMovieDefImpl *pfontDefImpl = (GFxMovieDefImpl*)fr.GetMovieDef();
            if (pfontDefImpl != this)
                ResourceImports.push_back(pfontDefImpl);
            
            // Store font resource binding entry.
            GFxResourceBindData bindData;
            bindData.pBinding = &pfontDefImpl->ResourceBinding;
            bindData.pResource= fr.GetFontResource();
                    
            SetResourceBindData(GFxResourceId(symbol.CharacterId), bindData,
                                symbol.SymbolName.ToCStr());
        }
        else
        {
            // Complain.
            allSucceeded = 0;

            if (plookupFontName == pfontName)
                LogError("Import error: Font '%s' not found in GFxFontMap or GFxFontLib\n",
                         pfontName);
            else
            {
                const char* pstyle = "";
                switch(fontEntry.Flags)
                {
                    case GFxFontMap::MFF_Normal:    pstyle = " Normal"; break;
                    case GFxFontMap::MFF_Bold:      pstyle = " Bold"; break;
                    case GFxFontMap::MFF_Italic:    pstyle = " Italic"; break;
                    case GFxFontMap::MFF_BoldItalic: pstyle = " Bold Italic"; break;
                    default: pstyle = ""; break;
                }
                
                LogError("Import error: Font '%s' mapped to '%s'%s not found in GFxFontLib\n",
                         pfontName, plookupFontName, pstyle);
            }
        }
    }

    // Add an empty import so that ImportSourceMovies still matches ImportData.
    ImportSourceMovies.push_back(0);
    return allSucceeded;
}


// Internal helper for import updates.
bool    GFxMovieDefImpl::SetResourceBindData(GFxResourceId rid, GFxResourceBindData& bindData,
                                             const char* pimportSymbolName)
{
    GFxResourceHandle rh;
    if (GetDataDef()->GetResourceHandle(&rh, rid))
    {
        GASSERT(rh.IsIndex());

        // Establish an association for BindIndex; note that
        // ResourceBindData is not used here. For imported characters
        // ResourceData elements will be empty.
        ResourceBinding.SetBindData(rh.GetBindIndex(), bindData);
        
        // Return imported flag so that the caller knows when to AddRef to the MovieDef
        return true;
    }
   
    // The handle for import MUST exists in DataDef since it is created 
    // during the Read of import tags. If it doesn't, there is an error or
    // perhaps we are out of memory.   
    GFC_DEBUG_WARNING1(1,
        "Internal import bind error: bind handle not found for resource '%s'",
        pimportSymbolName);
    GUNUSED(pimportSymbolName); // For release build.
   
    return false;
}


// Create a playable GFxMovieSub instance from a def.
GFxMovieView*   GFxMovieDefImpl::CreateInstance(bool initFirstFrame)    
{
    GFxMovieRoot* proot = new GFxMovieRoot();
    if (!proot)
        return 0;

    // Note: Log is inherited dynamically from Def, so we don't set it here
    GPtr<GFxSprite> prootMovie = *new GFxSprite(GetDataDef(), this, proot,
        NULL, GFxResourceId());

    if (!prootMovie)
    {
        proot->Release();
        return 0;
    }

    // Assign level and _level0 name.
    prootMovie->SetLevel(0);
    proot->SetLevelMovie(0, prootMovie);

    if (initFirstFrame)
        proot->Advance(0.0f, 0);

    // AddRef unnecessary, refCount == 1 on new
    return proot;
}




// *** GFxMovieDefImpl Key 

// GFxMovieDefBindStates is used as a key object in GFxMovieDefImpl.
size_t  GFxMovieDefBindStates::GetHashCode() const
{
    return 
        ((size_t)pDataDef.GetPtr()) ^ (((size_t)pDataDef.GetPtr()) >> 7) ^
        ((size_t)pFileOpener.GetPtr()) ^ (((size_t)pFileOpener.GetPtr()) >> 7) ^
        ((size_t)pURLBulider.GetPtr()) ^ (((size_t)pURLBulider.GetPtr()) >> 7) ^
        ((size_t)pImageCreator.GetPtr()) ^ (((size_t)pImageCreator.GetPtr()) >> 7) ^
        ((size_t)pImportVisitor.GetPtr()) ^ (((size_t)pImportVisitor.GetPtr()) >> 7) ^
       // ((size_t)pImageVisitor.GetPtr()) ^ (((size_t)pImageVisitor.GetPtr()) >> 7) ^
        ((size_t)pGradientParams.GetPtr()) ^ (((size_t)pGradientParams.GetPtr()) >> 7) ^
        ((size_t)pFontPackParams.GetPtr()) ^ (((size_t)pFontPackParams.GetPtr()) >> 7) ^
        ((size_t)pFontProvider.GetPtr()) ^ (((size_t)pFontProvider.GetPtr()) >> 7) ^
        ((size_t)pFontLib.GetPtr()) ^ (((size_t)pFontLib.GetPtr()) >> 7) ^
        ((size_t)pFontMap.GetPtr()) ^ (((size_t)pFontMap.GetPtr()) >> 7);
}

bool GFxMovieDefBindStates::operator == (GFxMovieDefBindStates& other) const
{
    // For bind key to be identical, both pDataDef and all of the binding
    // states must match.
    return pDataDef == other.pDataDef &&
           pFileOpener == other.pFileOpener &&
           pURLBulider == other.pURLBulider &&
           pImageCreator == other.pImageCreator &&
           pImportVisitor == other.pImportVisitor &&
     //      pImageVisitor == other.pImageVisitor &&
           pGradientParams == other.pGradientParams &&
           pFontPackParams == other.pFontPackParams &&           
           pFontProvider == other.pFontProvider &&
           pFontLib == other.pFontLib &&
           pFontMap == other.pFontMap;
}

class GFxMovieDefImplKeyInterface : public GFxResourceKey::KeyInterface
{
public:
    typedef GFxResourceKey::KeyHandle KeyHandle;

    virtual void    AddRef(KeyHandle hdata)
    {
        GASSERT(hdata); ((GFxMovieDefBindStates*) hdata)->AddRef();
    }
    virtual void    Release(KeyHandle hdata)
    {        
        GASSERT(hdata); ((GFxMovieDefBindStates*) hdata)->Release();
    }
    virtual GFxResourceKey::KeyType GetKeyType(KeyHandle hdata) const
    {
        GUNUSED(hdata);
        return GFxResourceKey::Key_Unique;
    }

    virtual size_t  GetHashCode(KeyHandle hdata) const
    {
        GASSERT(hdata);        
        return ((GFxMovieDefBindStates*) hdata)->GetHashCode();
    }
    virtual bool    KeyEquals(KeyHandle hdata, const GFxResourceKey& other)
    {
        if (this != other.GetKeyInterface())
            return 0;        
        return *((GFxMovieDefBindStates*) hdata) ==
               *((GFxMovieDefBindStates*) other.GetKeyData());
    }
};

static GFxMovieDefImplKeyInterface GFxMovieDefImplKeyInterface_Instance;

// Create a key for an SWF file corresponding to GFxMovieDef.
GFxResourceKey  GFxMovieDefImpl::CreateMovieKey(GFxMovieDefBindStates* pbindStates)
{
    return GFxResourceKey(&GFxMovieDefImplKeyInterface_Instance,
                          (GFxResourceKey::KeyHandle)pbindStates );
}




// Fill in the binding resource information together with its binding.
bool GFxMovieDefImpl::GetExportedResource(GFxResourceBindData *pdata, const GFxString& symbol)
{

    // TBD: Should we block for availability depending on whether the source
    // has finished loading?

    GFxResourceHandle hres;
    if (GetDataDef()->Exports.get_CaseInsensitive(symbol, &hres))
    {
        // Determine the exact binding
        if (hres.IsIndex())
        {
            ResourceBinding.GetResourceData(pdata, hres.GetBindIndex());
        }
        else
        {
            pdata->pBinding = &ResourceBinding;
            pdata->pResource= hres.GetResource(&ResourceBinding);
        }
        return (pdata->pResource.GetPtr() != 0);
    }

    else
    {
        // Nested import symbols are also visible in parent, although
        // our own Exports table has precedence (checked for above).

        // TBD: Any particular traversal order ?
        for (UInt i = 0; i<ImportSourceMovies.size(); i++)
        {
            GFxMovieDefImpl* pdef = ImportSourceMovies[i].GetPtr();
            if (pdef && pdef->GetExportedResource(pdata, symbol))
                return 1;
        }
    }

    return 0;
}

const GFxString*   GFxMovieDefImpl::GetNameOfExportedResource(GFxResourceId rid) const
{
    return GetDataDef()->InvExports.get(rid);
}


// *** Resource Lookup


// Obtains a resource based on its id. If resource is not yet resolved,
// NULL is returned. Should be used only before creating an instance.
// Type checks the resource based on specified type.
GFxResource*        GFxMovieDefImpl::GetResource(GFxResourceId rid, GFxResource::ResourceType rtype)
{
    GASSERT(GetDataDef() != 0);

    GFxResourceHandle rh;
    if (GetDataDef()->GetResourceHandle(&rh, rid))
    {
        GFxResource* pres = rh.GetResource(&ResourceBinding);
        if (pres)
        {
            if (pres->GetResourceType() == rtype)
                return pres;
        }
    }
    return 0;    
}


// Obtains full character creation information, including GFxCharacterDef.
GFxCharacterCreateInfo     GFxMovieDefImpl::GetCharacterCreateInfo(GFxResourceId rid)
{
    GFxResourceHandle      rh;
    
    GFxCharacterCreateInfo ccinfo;
    ccinfo.pCharDef     = 0;
    ccinfo.pBindDefImpl = 0;

    if (GetDataDef()->GetResourceHandle(&rh, rid))
    {        
        GFxResourceBinding* pbinding;
        GFxResource*        pres = rh.GetResourceAndBinding(&ResourceBinding, &pbinding);
        if (pres)
        {
            if (pres->GetResourceType() & GFxResource::RT_CharacterDef_Bit)
            {
                ccinfo.pBindDefImpl = pbinding->GetOwnerDefImpl();
                ccinfo.pCharDef     = (GFxCharacterDef*) pres;                
            }
        }
    }
    
    return ccinfo;
}





/*
// Debug helper; returns true if the given
// CharacterId is listed in the import table.
bool    GFxMovieDefImpl::InImportTable(int CharacterId)
{
    for (UInt i = 0, n = Imports.size(); i < n; i++)
    {
        if (Imports[i].CharacterId == CharacterId)
            return true;
    }
    return false;
}
*/

// Calls back the visitor for each GFxMovieSub that we
// import symbols from.
void    GFxMovieDefImpl::VisitImportedMovies(GFxMovieDefImpl::ImportVisitor* visitor)
{

    // Thread TBD: Ensure that all imports are loaded
    
    GTL::garray<GFxImportData*>& importData = GetDataDef()->ImportData;

    if (importData.size())
    {
        // TBD: Visited may no longer be necessary in 2.0. However,
        // we need to verify that it's not possible to produce to identical import
        // file references in the Flash studio.
        GFxStringHash<bool>        visited;  
    
        for (UInt i = 0; i < importData.size(); i++)
        {            
            if (i < ImportSourceMovies.size())
            {
                // We may not want to allow this. The only time this could happen is
                // if file is still being bound by another thread that has not caught
                // up to this point, or binding error has occurred.
                GASSERT(0);
                break;
            }
          
            if (visited.find_CaseInsensitive(importData[i]->SourceUrl) == visited.end())
            {                           
                // Call back the visitor.
                if (ImportSourceMovies[i])
                    visitor->Visit(this, ImportSourceMovies[i], importData[i]->SourceUrl.ToCStr());
                visited.set_CaseInsensitive(importData[i]->SourceUrl, true);
            }
        }
    }
}





// Fill in cached data in MovieDef.
// @@@@ NEEDS TESTING -- MIGHT BE BROKEN!!!
void    GFxMovieDefImpl::PrecomputeCachedData()
{
    // Temporarily install null render and sound handlers,
    // so we don't get output during preprocessing.
    // .. no longer necessary, as instance is created without a renderer

    // Need an instance.
    GFxMovieView*   m = CreateInstance();
    if (m == NULL)
    {
        LogError("Error: PrecomputeCachedData can't create instance of GFxMovieSub\n");
        return;
    }

    // Run through the GFxMovieSub's frames.
    //
    // @@ there might be cleaner ways to do this; e.G. do
    // ExecuteFrameTags(i, true) on GFxMovieSub and all child
    // sprites.
    int KickCount = 0;
    for (;;)
    {
        // @@ do we also have to run through all sprite frames
        // as well?
        //
        // @@ also, ActionScript can rescale things
        // dynamically -- we can't really do much about that I
        // guess?
        //
        // @@ Maybe we should allow the user to specify some
        // safety margin on scaled shapes.

        UInt    LastFrame = m->GetCurrentFrame();
        m->Advance(0.010f);
        m->Display();

        if (m->GetCurrentFrame() == GetFrameCount() - 1)
        {
            // Done.
            break;
        }

        if (m->GetPlayState() == GFxMovie::Stopped)
        {
            // Kick the pMovie.
            //printf("kicking GFxMovieSub, kick ct = %d\n", KickCount);
            m->GotoFrame(LastFrame + 1);
            m->SetPlayState(GFxMovie::Playing);
            KickCount++;

            if (KickCount > 10)
            {
                //printf("GFxMovieSub is stalled; giving up on playing it through.\n");
                break;
            }
        }
        else if (m->GetCurrentFrame() < LastFrame)
        {
            // apparently we looped back.  Skip ahead...
            LogError("loop back; jumping to frame %d\n", LastFrame);
            m->GotoFrame(LastFrame + 1);
        }
        else
        {
            KickCount = 0;
        }
    }

    m->Release();
}


void GFxMovieDefImpl::VisitResources(ResourceVisitor* pvisitor, UInt visitMask)
{
    
    if (visitMask & (GFxMovieDef::ResVisit_AllImages |
                     GFxMovieDef::ResVisit_Fonts | 
                     GFxMovieDef::ResVisit_EditTextFields) )
    {

        GFxMovieDataDef::ResourceHash::const_iterator ihash = GetDataDef()->Resources.begin();

        for(; ihash != GetDataDef()->Resources.end(); ++ihash)
        {
            GFxResource *pres          = ihash->second.GetResource(&ResourceBinding);
            bool         resourceMatch = 0;

            if (pres)
            {
                GFxResource::ResourceUse use = pres->GetResourceUse();

                switch(pres->GetResourceType())
                {
                case GFxResource::RT_Image:
                    {                       
                        if (use == GFxResource::Use_Bitmap)
                        {
                            if (visitMask & GFxMovieDef::ResVisit_Bitmaps)
                                resourceMatch = 1;
                        }
                        else if (use == GFxResource::Use_Gradient)
                        {
                            if (visitMask & GFxMovieDef::ResVisit_GradientImages)
                                resourceMatch = 1;
                        }
                    }                    
                    break;

                case GFxResource::RT_Font:
                    if (visitMask & GFxMovieDef::ResVisit_Fonts)
                        resourceMatch = 1;
                    break;

                case GFxResource::RT_EditTextDef:
                    if (visitMask & GFxMovieDef::ResVisit_EditTextFields)
                        resourceMatch = 1;
                    break;

                default: break;
                }
            }

            if (resourceMatch)
            {
                // Determine export name by doing a search.
                const char* pexportName = 0;

				GFxStringHash< GFxResourceHandle >::iterator iexport
                                                = GetDataDef()->Exports.begin();
                while (iexport != GetDataDef()->Exports.end())
                {
                    if (iexport->second.Equals(ihash->second))
                    {
                        pexportName = iexport->first.ToCStr();
                        break;
                    }
                    ++iexport;
                }                            

                pvisitor->Visit(this, pres, ihash->first, pexportName);
            }
        }
    }

    // Check to see if we need to traverse nested files.
    if (visitMask & GFxMovieDef::ResVisit_NestedMovies)
    {
        // visit imported movies
        for(UPInt i = 0, n = ImportSourceMovies.size(); i < n; ++i)
        {
            if (ImportSourceMovies[i])
                ImportSourceMovies[i]->VisitResources(pvisitor, visitMask);
        }
    }   
}


GFxResource*     GFxMovieDefImpl::GetResource(const char *pexportName) const
{
    if (!pexportName)
        return 0;

    // Find the export string
    GFxString str(pexportName);
    GFxStringHash< GFxResourceHandle >::const_iterator iexport = GetDataDef()->Exports.find_CaseInsensitive(str);
    if ( iexport == GetDataDef()->Exports.end())
        return 0;

    // TBD: Thread issues?
    // Check if the resource has been resolved yet. Not quite right since
    // pointer-based handles are possible.
    //if (iexport->second.GetBindIndex() >= ResourceBinding.GetResourceCount())    
    //    return 0;

    GFxResource *pres = iexport->second.GetResource(&ResourceBinding);
    return pres;    
}

/*
bool   GFxMovieDefImpl::GetImageFileInfo(GFxImageFileInfo *pinfo, int characterId)
{
    GTL::ghash_identity<int, GPtr<GFxBitmapResource> >::const_iterator ihash = Bitmaps.find(characterId);
    // Image file info is stored in the bitmap resource.
    if (ihash != Bitmaps.end())    
        return ihash->second->GetImageFileInfo(pinfo, GetExporterInfo());
    return 0;
}

*/


// Locate a font resource by name and style.
// It's ok to return GFxFontResource* without the binding because pBinding
// is embedded into font resource allowing imports to be handled properly.
GFxFontResource*    GFxMovieDefImpl::GetFontResource(const char* pfontName, UInt styleFlags)
{
    GFxMovieDataDef* pdataDef = GetDataDef();

    // TBD: We will need to do something about threading here, since it is legit
    // to call GetFontResource while loading still hasn't completed.

    UInt i, j;

    for(i=0; i<pdataDef->Fonts.size(); i++)
    {
        if (pdataDef->Fonts[i].pFontData->MatchFont(pfontName, styleFlags))
        {
            // TBD: Wait for resource binding?
            GFxResourceBindData rbd;
            ResourceBinding.GetResourceData(&rbd, pdataDef->Fonts[i].ResourceBindIndex);
            if (rbd.pResource)
            {
                GASSERT(rbd.pResource->GetResourceType() == GFxResource::RT_Font);
            }           
            return (GFxFontResource*)rbd.pResource.GetPtr();
        }
    }
    
    // Export names also look like font names for lookup purposes.
    // In particular, we need to do this to support imported shared font,
    // where export names are used during lookup.
    for(i=0; i<pdataDef->ImportData.size(); i++)
    {
        GFxImportData* pimportData = pdataDef->ImportData[i];

        for (j = 0; j<pimportData->Imports.size(); j++)
        {
            // Compare font name.
            if (!pimportData->Imports[j].SymbolName.CompareNoCase(pfontName))
            {
                // If wound, locate the font and match style flags.
                // We will also have to report the new name so that it can be
                // detected in the font manager.

                GFxResourceHandle rh;
                if (pdataDef->GetResourceHandle(&rh, GFxResourceId(pimportData->Imports[j].CharacterId)))
                {
                    GFxResource* pres = rh.GetResource(&ResourceBinding);

                    // The resource must be a font.
                    if (pres && (pres->GetResourceType() == GFxResource::RT_Font))
                    {
                        GFxFontResource* pfontRes = (GFxFontResource*)pres;
                        if (pfontRes->GetFont()->MatchFontFlags(styleFlags))
                        {
                            // We found it!
                            // Fill in name, since font will not have its export name.
                            return pfontRes;
                        }
                    }
                }
            }
        }
    }

    return 0;   
}


// *** GASExecuteTag - debug counters

#ifdef GFC_BUILD_DEBUG

class ASClassCounter
{
public:
    GAtomicInt<UInt> Count;

    ASClassCounter()  { Count = 0; }
    // If this assertion is hit, it means that some allocated
    // GASExecuteTag(s) weren't properly cleaned up.
    ~ASClassCounter() { GASSERT(Count == 0); }
};

ASClassCounter AS_Counter_Intance;

GASExecuteTag::GASExecuteTag()
{
    AS_Counter_Intance.Count++;
    //Sentinel = 0xE465F1A9;
}
GASExecuteTag::~GASExecuteTag()
{
    //GASSERT(Sentinel == 0xE465F1A9);
    //Sentinel = 0;
    AS_Counter_Intance.Count--;
}

#endif



// ***** GFxInitImportActions - created for import tags.

// Queues up logic to execute InitClip actions from the import,
// using Highest priority by default.
void    GFxInitImportActions::Execute(GFxSprite* m)
{
    // For regular execute our context is the same as of target sprite,
    // but it gets overriden if called from GFxSprite::ExecuteImportedInitActions.
    ExecuteInContext(m, m->GetResourceMovieDef(), 0);
}

// InitImportActions that come from imported files need to be executed
// in the MovieDefImpl binding context (otherwise we would index parent's
// source movies incorrectly, resulting in recursive loop).
void    GFxInitImportActions::ExecuteInContext(GFxSprite* m, GFxMovieDefImpl *pbindDef, bool recursiveCheck)
{
    if (ImportIndex >= pbindDef->ImportSourceMovies.size())
        return;
    GFxMovieDefImpl* psourceImportMovie = pbindDef->ImportSourceMovies[ImportIndex];
    if (psourceImportMovie &&
        psourceImportMovie->GetDataDef()->HasInitActions())
    {
        // It is possible that SWF files will import themselves or other files recursively.
        // Check for that and don't run sprite's ResourceMovieDef's actions again.
        if (recursiveCheck && (psourceImportMovie == m->GetResourceMovieDef()))
            return;

        m->ExecuteImportedInitActions(psourceImportMovie);
    }    
}

