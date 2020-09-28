//============================================================================
// flashtranslator.cpp
// Copyright 2008 (c) Ensemble Studios
//============================================================================

#include "xgameRender.h"
#include "scaleformIncludes.h"
#include "flashtranslator.h"
#include "wordwrap.h"

//============================================================================
//============================================================================
BFlashTranslator::BFlashTranslator(uint wwMode) :
   GFxTranslator(wwMode)
{
}

//============================================================================
//============================================================================
BFlashTranslator::~BFlashTranslator()
{
}

//============================================================================
//============================================================================
bool BFlashTranslator::mgsWordWrap(LineFormatDesc* pDesc)
{
   if (!pDesc)
      return false;

   /*
   struct LineFormatDesc
    {
        const wchar_t*  pParaText;              // [in] paragraph text
        UPInt           ParaTextLen;            // [in] length, in chars
        const Float*    pWidths;                // [in] array of line widths before char at corresponding index, in pixels, size = NumCharsInLine + 1
        UPInt           LineStartPos;           // [in] text position in paragraph of first char in line
        UPInt           NumCharsInLine;         // [in] count of chars currently in the line        
        Float           VisibleRectWidth;       // [in] width of client rect, in pixels
        Float           CurrentLineWidth;       // [in] current line width, in pixels
        Float           LineWidthBeforeWordWrap;// [in] line width till ProposedWordWrapPoint, in pixels
        Float           DashSymbolWidth;        // [in] may be used to calculate hyphenation, in pixels

        UPInt           ProposedWordWrapPoint;  // [in,out] text index of proposed word wrap pos,
                                                //          callback may change it to move wordwrap point
        bool            UseHyphenation;         // [out]    callback may set it to indicate to use hyphenation
    };
   */
   static bool bUseDefault = false;
   if (bUseDefault)
      return false;

   LPCWSTR pLineText = pDesc->pParaText + pDesc->LineStartPos;
   LPCWSTR pCurTextPtr = pLineText;
   int numCharsInLine = Math::Min(pDesc->NumCharsInLine, pDesc->ParaTextLen);
   int lineBreakPos = pDesc->ProposedWordWrapPoint;
   bool bCanBreak = false;
   for (int i = 0; i < numCharsInLine; ++i)
   {  
      //-- if the current character doesn't fit into our viewable rectangle
      //-- bail out.
      if (pDesc->pWidths[i] > pDesc->VisibleRectWidth)
         break;

      // if the character is "\r" or "\n" or "\0", line break.
		if(L'\n'==(*pCurTextPtr) || L'\r'==(*pCurTextPtr) || L'\0'==(*pCurTextPtr))
      {
         bCanBreak = true;
         lineBreakPos = i;
         break;
      }
      
      //-- check whether we can break at this location
      //-- if we can break at this location mark this as the current
      //-- best break point and mark up that we could break.
      if (WordWrap_CanBreakLineAt(pCurTextPtr, pLineText))
      {
         bCanBreak = true;
         lineBreakPos = i;
      }

      // go to the next character
      pCurTextPtr++;
   }

   //-- if we found a break location then update the ProposedWordWrapPoint and 
   //-- return it.
   if (bCanBreak)
   {
      int whiteSpaceOffest = WordWrap_FindNonWhiteSpaceForward_Pos(pLineText+lineBreakPos);
      pDesc->ProposedWordWrapPoint = Math::Min((lineBreakPos+whiteSpaceOffest), (int) (pDesc->ParaTextLen-1));
      // tell scaleform we found a better location
      return true;
   }

   return false;
}

//============================================================================
//============================================================================
bool BFlashTranslator::OnWordWrapping(LineFormatDesc* pdesc)
{
   if (WWMode == WWT_Default)
   {
      return false;
   }
   else if (WWMode == WWT_Custom)
   {
      return mgsWordWrap(pdesc);      
   }
   else if ((WWMode & (WWT_Asian | WWT_NoHangulWrap | WWT_Prohibition)) && pdesc->NumCharsInLine > 0)
   {
      const wchar_t* pstr = pdesc->pParaText + pdesc->LineStartPos;
      UPInt remainingLen  = pdesc->ParaTextLen - pdesc->LineStartPos;
      UPInt pos = pdesc->NumCharsInLine - 1;
      UPInt wordWrapPos = pdesc->ProposedWordWrapPoint;
      for (; pos > 0; --pos)
      {
         if (GFxWWHelper::IsWhiteSpaceChar(pstr[pos]))
         {
             UPInt eolPos = GFxWWHelper::FindPrevNonWhiteSpace(pstr, pos);
             if (eolPos == GFC_MAX_UPINT)
             {
                 pos = GFxWWHelper::FindNextNonWhiteSpace (pstr, pos + 1, remainingLen - 1);
                 break;
             }
             pos = eolPos + 1;
         }
         if (GFxWWHelper::IsLineBreakOpportunityAt(WWMode, pstr, pos))
             break;
      }            
      if (pos == 0)
      {
         // couldn't find any character to break the line
         wordWrapPos = pdesc->NumCharsInLine - 1;
      }
      else
      {
         wordWrapPos = GFxWWHelper::FindNextNonWhiteSpace (pstr, pos, remainingLen - 1);
         if (wordWrapPos == GFC_MAX_UPINT)
             wordWrapPos = pdesc->NumCharsInLine - 1; //?
      }
      pdesc->ProposedWordWrapPoint = wordWrapPos;
      return true;
   }
   else if ((WWMode & WWT_Hyphenation))
   {
      if (pdesc->ProposedWordWrapPoint == 0)
         return false;
      const wchar_t* pstr = pdesc->pParaText + pdesc->LineStartPos;
      // determine if we need hyphenation or not. For simplicity,
      // we just will put dash only after vowels.
      UPInt hyphenPos = pdesc->NumCharsInLine;
      // check if the proposed word wrapping position is at the space.
      // if so, this will be the ending point in hyphenation position search.
      // Otherwise, will look for the position till the beginning of the line.
      // If we couldn't find appropriate position - just leave the proposed word
      // wrap point unmodified.
      UPInt endingHyphenPos = (gfc_iswspace(pstr[pdesc->ProposedWordWrapPoint - 1])) ? 
         pdesc->ProposedWordWrapPoint : 0;
      for (; hyphenPos > endingHyphenPos; --hyphenPos)
      {
         if (GFxWWHelper::IsVowel(pstr[hyphenPos - 1]))
         {
             // check if we have enough space for putting dash symbol
             // we need to summarize all widths up to hyphenPos + pdesc->DashSymbolWidth
             // and this should be less than view rect width
             Float lineW = pdesc->pWidths[hyphenPos - 1];
             lineW += pdesc->DashSymbolWidth;
             if (lineW < pdesc->VisibleRectWidth)
             {
                 // ok, looks like we can do hyphenation
                 pdesc->ProposedWordWrapPoint = hyphenPos;
                 pdesc->UseHyphenation = true;
                 return true;
             }
             else
             {
                 // oops, we have no space for hyphenation mark
                 continue;
             }
             break;
         }
      }
   }
   return false;
}