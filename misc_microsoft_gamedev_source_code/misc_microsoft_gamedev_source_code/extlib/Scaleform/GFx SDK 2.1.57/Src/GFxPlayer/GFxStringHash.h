/**********************************************************************

Filename    :   GFxStringHash.h
Content     :   String hash table used when optional case-insensitive
                lookup is required.
Created     :
Authors     :
Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxStringHash_H
#define INC_GFxStringHash_H

#include "GFxString.h"


// *** GFxStringHash

// This is a custom string hash table that supports case-insensitive
// searches through special functions such as get_CaseInsensitive, etc.
// This class is used for Flash labels, exports and other case-insensitive tables.

template<class U>
class GFxStringHash : public GTL::ghash<GFxString, U, GFxString::NoCaseHashFunctor>
{
    typedef GFxStringHash<U>                                        self;
    typedef GTL::ghash<GFxString, U, GFxString::NoCaseHashFunctor>  base_class;
public:    

    void    operator = (const self& src) { base_class::operator = (src); }

    bool    get_CaseInsensitive(const GFxString& key, U* pvalue) const
    {
        GFxString::NoCaseKey ikey(key);
        return base_class::get_alt(ikey, pvalue);
    }
    // Pointer-returning get variety.
    const U* get_CaseInsensitive(const GFxString& key) const   
    {
        GFxString::NoCaseKey ikey(key);
        return base_class::get_alt(ikey);
    }
    U*  get_CaseInsensitive(const GFxString& key)
    {
        GFxString::NoCaseKey ikey(key);
        return base_class::get_alt(ikey);
    }

    
    typedef typename base_class::iterator base_iterator;

    base_iterator    find_CaseInsensitive(const GFxString& key)
    {
        GFxString::NoCaseKey ikey(key);
        return base_class::find_alt(ikey);
    }

    // Set just uses a find and assigns value if found. The key is not modified;
    // this behavior is identical to Flash string variable assignment.    
    void    set_CaseInsensitive(const GFxString& key, const U& value)
    {
        base_iterator it = find_CaseInsensitive(key);
        if (it != base_class::end())
        {
            it->second = value;
        }
        else
        {
            base_class::add(key, value);
        }
    } 
};

#endif
