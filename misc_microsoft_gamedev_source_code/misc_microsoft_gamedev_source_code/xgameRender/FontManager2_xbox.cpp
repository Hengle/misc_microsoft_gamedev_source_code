//============================================================================
// FontManager2_xbox.cpp
//
// Copyright (c) 200b Ensemble Studios
//============================================================================

//============================================================================
//  INCLUDES
//============================================================================
#include "xgameRender.h"
#include "FontSystem2.h"
#include "xmlreader.h"
#include "render.h"

BFontManager2 gFontManager;



//============================================================================
//============================================================================
BTextData::BTextData()
{
   reset();
}


//============================================================================
//============================================================================
void BTextData::reset()
{
   mPos.zero();
   mText.empty();
   mhFont = NULL;
   mColor = cDWORDWhite;
   mScale = 1.0f;
   mJustification = BFontManager2::cJustifyLeft;

   mFlag3DPos = false;
   mFlagDeleteOnSimUpdate = false;
}


//============================================================================
//============================================================================
void BTextData::copy(const BTextData& src)
{
   if (this == &src)
      return;

   mPos = src.mPos;
   mText = src.mText;
   mhFont = src.mhFont;
   mColor = src.mColor;
   mScale = src.mScale;
   mJustification = src.mJustification;

   mFlag3DPos = src.mFlag3DPos;
   mFlagRenderIn3DViewport = src.mFlagRenderIn3DViewport;
   mFlagDeleteOnSimUpdate = src.mFlagDeleteOnSimUpdate;
}


//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
BFontManager2::BFontManager2()
{
   mpCurrentFont = NULL;
   mBaseDirectoryID = -1;
   mFontArial10 = NULL;
   mFontCourier10 = NULL;
   mFontDenmark14 = NULL;
   mFontDenmark16 = NULL;
   mFontDenmark18 = NULL;
   mFontDenmark24 = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFontManager2::~BFontManager2()
{
   //-- Kill all the fonts.
   destroyAllFonts();
}


//============================================================================
//  FONT CONTROL
//============================================================================
BHandle BFontManager2::createFont(const BString& name, const BFontInfo& fontInfo)
{
   //-- Make sure this font name is unique.
   BHandle hFont = findFont(name);
   if (hFont)
      return NULL;

   //-- See if an equivalent font already exists.
   hFont = findFont(fontInfo);
   if (hFont)
   {
      BFont2* pFont = getFont(hFont);
      if (!pFont)
         return NULL;
      pFont->addName(name);
      return hFont;
   }
    
   //-- Create the font.
   BFont2* pFont = new BFont2;
   if (!pFont->create(fontInfo, this))
   {
      delete pFont;
      return NULL;   
   }

   //-- We're good to go.
   pFont->addName(name);
   BHandle hFont1 = mFonts.addToTail(pFont);

   //-- Create the safe handles.
   long index = mSafeHandles.add(hFont1);

   //-- Return a safe handle.
   return ((BHandle)(index + 1));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BHandle BFontManager2::findFont(const BString& name)
{
   BHandle hFont;
   BFont2* pFont = mFonts.getHead(hFont);
   while (pFont)
   {
      if (pFont->hasName(name))
      {
         long index = mSafeHandles.find(hFont);
         return ((BHandle)(index + 1));
      }

      pFont = mFonts.getNext(hFont);
   }
   return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BHandle BFontManager2::findFont(const BString& name, long pixelHeight)
{
   BString fontName;
   fontName.format(B("%s %d"), name.getPtr(), pixelHeight);

   BHandle hFont = findFont(fontName);
   if (hFont)
      return hFont;

   //-- Try giving back a default font.
   return findFont(B("Arial 10"));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BHandle BFontManager2::findFont(const BFontInfo& fontInfo)
{
   BHandle hFont;
   BFont2* pFont = mFonts.getHead(hFont);
   while (pFont)
   {
      if (*pFont == fontInfo)
      {
         long index = mSafeHandles.find(hFont);
         return ((BHandle)(index + 1));
      }

      pFont = mFonts.getNext(hFont);
   }
   return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFontManager2::destroyFont(const BString& name)
{
   //-- Remove the font.
   BHandle hFont = findFont(name);
   if (hFont)
   {
      BFont2* pFont = getFont(hFont);
      if (pFont)
      {
         pFont->removeName(name);
         if (pFont->getNumNames() == 0)
         {
            //-- This font is no longer used, kill it.

            //-- Validate the safe handle.
            long index = ((long)hFont) - 1;
            if ((index >= 0) && (index < mSafeHandles.getNumber()))
            {
               BHandle hReal = mSafeHandles[index];
               mSafeHandles[index] = 0;
               if (hReal)
                  mFonts.remove(hReal);
               else
                  hReal = hReal;
               delete pFont;
            }
         }
      }
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFontManager2::setFont(BHandle hFont)
{
   //-- Validate the safe handle.
   long index = (long)hFont;
   if ((index <= 0) || (index > mSafeHandles.getNumber()))
   {
      mpCurrentFont = NULL;
      return;
   }

   //-- Validate the real handle.
   BHandle hReal = mSafeHandles[index - 1];
   if (hReal == NULL)
   {
      mpCurrentFont = NULL;
      return;
   }

   //-- Set the font.
   mpCurrentFont = mFonts.getItem(hReal);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFont2* BFontManager2::getFont(BHandle hFont)
{
   //-- Validate the safe handle.
   long index = (long)hFont;
   if ((index <= 0) || (index > mSafeHandles.getNumber()))
      return NULL;

   //-- Validate the real handle.
   BHandle hReal = mSafeHandles[index - 1];
   if (hReal == NULL)
      return NULL;

   //-- Get the font.
   return mFonts.getItem(hReal);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFontSource* BFontManager2::getFontSource(const BString& sourceName)
{
   BHandle handle;
   BFontSource* pSource = mFontSources.getHead(handle);
   while (pSource)
   {
      if (pSource->mName == sourceName)
         return pSource;
      pSource = mFontSources.getNext(handle);
   }
   return NULL;
}


//============================================================================
//  STRING RENDERING
//============================================================================
void BFontManager2::renderString(float x, float y, DWORD dwColor, const WCHAR* pStr, BJustification just, float scale)
{
   if (mpCurrentFont)
   {
      mpCurrentFont->begin();
      mpCurrentFont->drawText(x, y, dwColor, pStr, just, scale);
      mpCurrentFont->end();
   }
}


//============================================================================
//  SYSTEM INTERFACE
//============================================================================
void BFontManager2::destroyAllMaterials()
{
   //-- Tell all the fonts.
   BHandle hFont;
   BFont2* pFont = mFonts.getHead(hFont);
   while (pFont)
   {
      pFont->destroy();
      pFont = mFonts.getNext(hFont);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFontManager2::destroyAllFonts()
{
   //-- Kill ALL the fonts.
   BHandle hFont;
   BFont2* pFont = mFonts.getHead(hFont);
   while (pFont)
   {
      delete pFont;
      pFont = mFonts.getNext(hFont);
   }
   mFonts.reset();

   //-- Whack all safe handles.
   for (long index = 0; index < mSafeHandles.getNumber(); ++index)
      mSafeHandles[index] = 0;

   //-- Kill all the font sources.
   BFontSource* pSource = mFontSources.removeHead();
   while (pSource)
   {
      pSource->mFont.Destroy();
      delete pSource;
      pSource = mFontSources.removeHead();
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFontManager2::createFontsFromXML()
{
   //-- Validate dependencies.
//	if (!BRenderDevice::valid())
  //    return;

   if (mBaseDirectoryID==-1)
      return;

   //-- Open file.
   BXMLReader reader;
   bool result = reader.load(mBaseDirectoryID, B("fonts.xml"));
   BASSERT(result);
   if (!result)
   {
      BFAIL("XML parsing error in xfonts.xml.  No fonts will be loaded.");
      return;
   }

   //-- Grab the root node.
   BXMLNode rootNode(reader.getRootNode());
      
   //-- Run through all the child nodes and parse them.
   BFontInfo fontInfo;
   for(long nodeIndex = 0; nodeIndex < rootNode.getNumberChildren(); nodeIndex++)
   {
      //-- Look for fonts or MSK file names.
      BXMLNode node(rootNode.getChild(nodeIndex));
      
      //-- See if its an Source file name.
      if (node.getName().compare(B("SourceFile"))==0)
      {
         BXMLAttribute attribSourceName;
         BXMLAttribute attribTypeFace;
         BXMLAttribute attribPixelHeight;
         BXMLAttribute attribFileName;
         
         if (!node.getAttribute("name", &attribSourceName) || 
             !node.getAttribute("typeFace", &attribTypeFace) || 
             !node.getAttribute("ascentHeight", &attribPixelHeight))
            continue;

         if (gRender.getViewParams().getViewportWidth()==640)
         {
            if (!node.getAttribute("file640", &attribFileName))
               continue;
         }
         else
         {
            if (!node.getAttribute("file", &attribFileName))
               continue;
         }

         BFontSource* pSource=new BFontSource;
         if(!pSource)
         {
            BASSERT(0);
            continue;
         }
           
         attribSourceName.getValue(pSource->mName);
         attribTypeFace.getValue(pSource->mTypeFace);
         attribFileName.getValue(pSource->mFileName);
         attribPixelHeight.getValueAsLong(pSource->mPixelHeight);

         if(FAILED(pSource->mFont.Create(mBaseDirectoryID, pSource->mFileName)))
         {
            BASSERT(0);
            delete pSource;
            continue;
         }

         if(mFontSources.addToHead(pSource)==NULL)
         {
            BASSERT(0);
            delete pSource;
            continue;
         }

         continue;
      }

      //-- Otherwise, it should be a font.
      if (node.getName().compare(B("Font"))!=0)
         continue;

      //-- Start a new font.
      clearFontInfo(fontInfo);

      //-- Get the font name.
      BString name;
      BXMLAttribute attrib;
      if (node.getAttribute("Name", &attrib))
         attrib.getValue(name);

      BDynamicArray<BString> otherNamesList;
      otherNamesList.clear();

      //-- Parse this node's children.
      for (long childIndex = 0; childIndex < node.getNumberChildren(); childIndex++)
      {
         const BXMLNode child(node.getChild(childIndex));
         
         //-- See if its a parameter.
         if (child.getName().compare(B("param"))==0)
         {
            parseParameter(child, fontInfo);
            continue;
         }

         //-- See if it is an alternate name for the font.
         if (child.getName().compare(B("OtherName")) == 0)
         {
            BString otherName;
            child.getText(otherName);
            if (!otherName.isEmpty())
               otherNamesList.add(otherName);
         }

         //-- See if its an override block.
         if (child.getName().compare(B("ResolutionOverride"))==0)
         {
            //-- See if we care about this override.
            DWORD res = 0;
            if (!child.getAttribValueAsDWORD("ResX", res) || (res != gRender.getWidth()))
               continue;
               
            //-- We care.
            parseOverride(child, fontInfo);
         }
      }

      //-- Validate font.
      if (name.isEmpty())               continue;
      if (fontInfo.mTypeFace.isEmpty()) continue;
      if (fontInfo.mPixelHeight <= 0)   continue;
      
      //-- Remove the old font.
      destroyFont(name);

      //-- Create this font.
      BHandle hFont = createFont(name, fontInfo);
      if (hFont == NULL)
      {
         {setBlogError(4196); blogerror("Error creating font: %s", name.getPtr());}
      }
      else
      {
         // Success, so add all the other names to it.
         BFont2 *pFont = getFont(hFont);
         long numOtherNames = otherNamesList.getNumber();
         if (pFont && numOtherNames > 0)
         {
            for (long i=0; i<numOtherNames; i++)
               pFont->addName(otherNamesList.get(i));
         }         
      }
   }   
}


//============================================================================
//============================================================================
void BFontManager2::initFontHandles()
{
   mFontArial10 = findFont("Arial 10");
   mFontCourier10 = findFont("Courier 10");
   mFontDenmark14 = findFont("Denmark 14");
   mFontDenmark16 = findFont("Denmark 16");
   mFontDenmark18 = findFont("Denmark 18");
   mFontDenmark24 = findFont("Denmark 24");
}


//============================================================================
//  STATIC FUNCTIONS
//============================================================================
void BFontManager2::clearFontInfo(BFontInfo& fontInfo)
{
   fontInfo.mTypeFace.empty();
   fontInfo.mPixelHeight       = 0;
   fontInfo.mAntialiased       = false;
   fontInfo.mBold              = false;
   fontInfo.mItalic            = false;
   fontInfo.mBackgroundOutline = false;
   fontInfo.mBackgroundShadow  = false;
   fontInfo.mBackgroundOffset  = 0;
   fontInfo.mBackgroundAlpha   = 0.0f;
   fontInfo.mSpacingAdjustment = 0.0f;
   fontInfo.mHorizontalOffset  = 0;
   fontInfo.mVerticalOffset    = 0;
}


//============================================================================
//  LEGACY COMPATABILITY INTERFACE
//============================================================================
BVector BFontManager2::stringExtents(BHandle hFont, const BString& str)
{
   setFont(hFont);

   BVector extents;
   BUString ustr;
   extents.x = getLineLength(str.asUnicode(ustr), str.length());
   extents.y = getLineHeight();
   extents.z = 0.0f;
   return extents;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BCharacterExtent2 BFontManager2::getCharacterExtent(BHandle hFont, WCHAR character)
{
   setFont(hFont);

   BCharacterExtent2 extent;

   
   BFont2* pFont = getFont(hFont);
   if (!pFont)
   {      
      extent.mExtentX      = 0.0f;
      extent.mExtentY      = 0.0f;
      extent.mPreAdvanceX  = 0.0f;
      extent.mPostAdvanceX = 0.0f;
      return extent;
   }

   WCHAR text[2];
   text[0]=character;
   text[1]=NULL;
   extent.mExtentX = getLineLength(text, 1);
   extent.mExtentY = getLineHeight();
   
   float scale=pFont->getScale();
   if(scale!=1.0f)
   {
      extent.mExtentX *= scale;
      extent.mExtentY *= scale;
   }
   
   extent.mPreAdvanceX  = 0.0f;
   extent.mPostAdvanceX = extent.mExtentX;

   return extent;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float BFontManager2::getMaximumCharacterHeight(BHandle hFont)
{
   setFont(hFont);

//   return getLineHeight();

   if (!mpCurrentFont)
      return 0.0f;
   return mpCurrentFont->getHeightFloat();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float BFontManager2::getMaximumCharacterAscent(BHandle hFont)
{
   setFont(hFont);
   if (!mpCurrentFont)
      return 0.0f;
   return (float)mpCurrentFont->getAscentHeight();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float BFontManager2::getMaximumCharacterDescent(BHandle hFont)
{
   setFont(hFont);
   if (!mpCurrentFont)
      return 0.0f;
   return (float)(mpCurrentFont->getHeight() - mpCurrentFont->getAscentHeight());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float BFontManager2::getCharPosInString(BHandle hFont, WCHAR character, const BString& text)
{
   BUString utext;
   const WCHAR* pText = text.asUnicode(utext);
   const WCHAR* pos=wcschr(pText, (WCHAR)character);   
   if (!pos)
      return(-1.0f);

   long offset = pos-pText;

   BString temp;
   temp.copy(text, offset + 1);
   BVector v = stringExtents(hFont, temp);
   return v.x;
}


//============================================================================
//  HELPERS
//============================================================================
void BFontManager2::parseParameter(BXMLNode node, BFontInfo& fontInfo)
{
   BXMLAttribute attrib;

   if (node.getAttribute("SourceFile",        &attrib)) attrib.getValue(fontInfo.mSourceFile);
   if (node.getAttribute("TypeFace",          &attrib)) attrib.getValue(fontInfo.mTypeFace);
   if (node.getAttribute("AscentHeight",      &attrib)) attrib.getValueAsLong(fontInfo.mPixelHeight);
   if (node.getAttribute("Antialiased",       &attrib)) attrib.getValueAsBool(fontInfo.mAntialiased);
   if (node.getAttribute("Bold",              &attrib)) attrib.getValueAsBool(fontInfo.mBold);
   if (node.getAttribute("Italic",            &attrib)) attrib.getValueAsBool(fontInfo.mItalic);
   if (node.getAttribute("BackgroundOutline", &attrib)) attrib.getValueAsBool(fontInfo.mBackgroundOutline);
   if (node.getAttribute("BackgroundShadow",  &attrib)) attrib.getValueAsBool(fontInfo.mBackgroundShadow);
   if (node.getAttribute("BackgroundOffset",  &attrib)) attrib.getValueAsLong(fontInfo.mBackgroundOffset);
   if (node.getAttribute("BackgroundAlpha",   &attrib)) attrib.getValueAsFloat(fontInfo.mBackgroundAlpha);
   if (node.getAttribute("SpacingAdjustment", &attrib)) 
   {  
      long val = 0;
      attrib.getValueAsLong(val);
      fontInfo.mSpacingAdjustment = (float)val;
   }
   if (node.getAttribute("HorizontalOffset",  &attrib)) attrib.getValueAsLong(fontInfo.mVerticalOffset);
   if (node.getAttribute("VerticalOffset",    &attrib)) attrib.getValueAsLong(fontInfo.mVerticalOffset);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFontManager2::parseOverride(BXMLNode node, BFontInfo& fontInfo)
{
   //-- Parse this node's children.
   for (long childIndex = 0; childIndex < node.getNumberChildren(); childIndex++)
   {
      const BXMLNode child(node.getChild(childIndex));
      
      //-- See if its a parameter.
      if (child.getName().compare(B("param"))==0)
         parseParameter(child, fontInfo);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float BFontManager2::getLineLength(const WCHAR* pChars, long numChars)
{
   //FIXME XBOX
   if(!mpCurrentFont)
      return 0.0f;
   return mpCurrentFont->getTextWidth(pChars);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float BFontManager2::getLineLength(const char* pChars, long numChars)
{
   //FIXME XBOX
   if(!mpCurrentFont)
      return 0.0f;
   return mpCurrentFont->getTextWidth(pChars);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float BFontManager2::getLineHeight()
{
   if (!mpCurrentFont)
      return 0.0f;

   return mpCurrentFont->getHeightFloat();
}


//============================================================================
//============================================================================
BTextData* BFontManager2::drawText(BHandle hFont, float x, float y, const char* pStr, DWORD dwColor, BJustification j, float scale, bool renderIn3DViewport)
{
   BTextData* pTD = mQueuedText.acquire(true);
   pTD->reset();
   pTD->setFont(hFont);
   pTD->setPos2D(x, y);
   pTD->setText(pStr);
   pTD->setColor(dwColor);
   pTD->setJustification(j);
   pTD->setScale(scale);
   pTD->setFlagRenderIn3DViewport(renderIn3DViewport);
   return (pTD);
}


//============================================================================
//============================================================================
BTextData* BFontManager2::drawText(BHandle hFont, float x, float y, const WCHAR* pStr, DWORD dwColor, BJustification j, float scale, bool renderIn3DViewport)
{
   BTextData* pTD = mQueuedText.acquire(true);
   pTD->reset();
   pTD->setFont(hFont);
   pTD->setPos2D(x, y);
   pTD->setText(pStr);
   pTD->setColor(dwColor);
   pTD->setJustification(j);
   pTD->setScale(scale);
   pTD->setFlagRenderIn3DViewport(renderIn3DViewport);
   return (pTD);
}


//============================================================================
//============================================================================
BTextData* BFontManager2::drawText(BHandle hFont, BVector worldPos, const char* pStr, DWORD dwColor, BJustification j, float scale)
{
   BTextData* pTD = mQueuedText.acquire(true);
   pTD->reset();
   pTD->setFont(hFont);
   pTD->setPos3D(worldPos);
   pTD->setText(pStr);
   pTD->setColor(dwColor);
   pTD->setJustification(j);
   pTD->setScale(scale);
   return (pTD);
}


//============================================================================
//============================================================================
BTextData* BFontManager2::drawText(BHandle hFont, BVector worldPos, const WCHAR* pStr, DWORD dwColor, BJustification j, float scale)
{
   BTextData* pTD = mQueuedText.acquire(true);
   pTD->reset();
   pTD->setFont(hFont);
   pTD->setPos3D(worldPos);
   pTD->setText(pStr);
   pTD->setColor(dwColor);
   pTD->setJustification(j);
   pTD->setScale(scale);
   return (pTD);
}


//============================================================================
//============================================================================
BTextData* BFontManager2::drawText(const BTextData& src)
{
   BTextData* pTD = mQueuedText.acquire(true);
   pTD->copy(src);
   return (pTD);
}


//============================================================================
//============================================================================
void BFontManager2::_renderText(BHandle hFont, float x, float y, const BUString& str, DWORD dwColor, BJustification j, float scale)
{
   // Make sure we have a string.
   if (str.isEmpty())
      return;

   // Make sure we have a font.
   if (!hFont && !mDefaultFont.isEmpty())
      hFont = findFont(mDefaultFont);
   if (!hFont)
      hFont = findFont(B("Arial"), 10);
   setFont(hFont);
   if (!mpCurrentFont)
      return;

   // A different justification constant.
   long just = JUSTIFY_LEFT;
   switch (j)
   {
   case cJustifyLeft:         just = JUSTIFY_LEFT;    break;
   case cJustifyCenter:       just = JUSTIFY_CENTER;  break;
   case cJustifyXCenter:      just = JUSTIFY_CENTER;  break;
   case cJustifyYCenter:      just = JUSTIFY_LEFT;    break;
   case cJustifyRight:        just = JUSTIFY_RIGHT;   break;
   case cJustifyRightYCenter: just = JUSTIFY_RIGHT;   break;
   case cJustifyLeftRight:    just = JUSTIFY_LEFT;    break;
   }

   // Double check the scale.
   if (scale <= 0.0f)
      scale = 1.0f;

   // Render the string.
   renderString(x, y, dwColor, str, just, scale);
}

//============================================================================
//============================================================================
void BFontManager2::render3D()
{
   // Draw Queued Text
   uint highWaterMark = mQueuedText.getHighWaterMark();
   for (uint i=0; i<highWaterMark; i++)
   {
      if (mQueuedText.isInUse(i))
      {
         const BTextData& td = mQueuedText[i];
         if (td.getFlag3DPos())
         {
            float sx, sy;
            gRender.getViewParams().calculateWorldToScreen(td.getPos(), sx, sy);
            _renderText(td.getFont(), sx, sy, td.getText(), td.getColor(), td.getJustification(), td.getScale());

            if (!td.getFlagDeleteOnSimUpdate())
               mQueuedText.release(i);
         }               
         else if (td.getFlagRenderIn3DViewport())
         {
            _renderText(td.getFont(), td.getPos().x, td.getPos().y, td.getText(), td.getColor(), td.getJustification(), td.getScale());

            if (!td.getFlagDeleteOnSimUpdate())
               mQueuedText.release(i);
         }       
      }
   }
}

//============================================================================
//============================================================================
void BFontManager2::render2D()
{
   // Draw Queued Text
   uint highWaterMark = mQueuedText.getHighWaterMark();
   for (uint i=0; i<highWaterMark; i++)
   {
      if (mQueuedText.isInUse(i))
      {
         const BTextData& td = mQueuedText[i];
         if (!td.getFlagRenderIn3DViewport())
         {
            _renderText(td.getFont(), td.getPos().x, td.getPos().y, td.getText(), td.getColor(), td.getJustification(), td.getScale());

            if (!td.getFlagDeleteOnSimUpdate())
               mQueuedText.release(i);
         }               
      }
   }
}


//============================================================================
//============================================================================
void BFontManager2::onUpdateSimStart()
{
   // Draw Queued Text
   uint highWaterMark = mQueuedText.getHighWaterMark();
   for (uint i=0; i<highWaterMark; i++)
   {
      if (mQueuedText.isInUse(i))
      {
         if (mQueuedText[i].getFlagDeleteOnSimUpdate())
            mQueuedText.release(i);
      }
   }
}