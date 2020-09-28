/**********************************************************************

Filename    :   GFxCharacterDef.cpp
Content     :   Implementation of basic GFxCharacterDef functionality.
Created     :   
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :  

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxCharacterDef.h"

#include "GFxCharacter.h"
#include "GFxAction.h"

// For GASExecutTag().
#include "GFxMovieDef.h"




// ***** GFxCharacterDef

// Create a most basic instance through GFxGenericCharacter
GFxCharacter*   GFxCharacterDef::CreateCharacterInstance(GFxASCharacter* parent, GFxResourceId rid,
                                                         GFxMovieDefImpl *pbindingImpl)
{
    GUNUSED(pbindingImpl);
    return new GFxGenericCharacter(this, parent, rid);
}

void    GFxCharacterDef::Display(GFxDisplayContext &, GFxCharacter* pinstanceInfo, StackData stackData)  
{ 
    GUNUSED(pinstanceInfo); 
	GUNUSED(stackData);
}


// *****  GFxTimelineDef

// Calls destructors on all of the tag objects.
void    GFxTimelineDef::Frame::DestroyTags()
{
    for (UInt i=0; i<TagCount; i++)
        pTagPtrList[i]->~GASExecuteTag();
    // Clear count and pointers.
    pTagPtrList = 0;
    TagCount    = 0;
}


