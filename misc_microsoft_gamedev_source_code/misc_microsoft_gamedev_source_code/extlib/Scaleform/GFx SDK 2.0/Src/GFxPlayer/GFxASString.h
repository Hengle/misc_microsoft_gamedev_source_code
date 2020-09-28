/**********************************************************************

Filename    :   GFxASString.h
Content     :   String manager implementation for Action Script
Created     :   Movember 7, 2006
Authors     :   Michael Antonov

Notes       :    Implements optimized GASString class, which acts as a
hash key for strings allocated from GASStringManager.

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXASSTRING_H
#define INC_GFXASSTRING_H

#include "GTypes.h"
#include "GTLTypes.h"
#include "GRefCount.h"

#include "GUTF8Util.h"
#include "GFxString.h"

#include <string.h>
#ifdef GFC_OS_PS3
# include <wctype.h>
#endif // GFC_OS_PS3


// ***** Classes Declared

class   GASString;
class   GASStringManager;
struct  GASStringNode;

// Log forward declaration - for leak report
class   GFxLog;


// String node - stored in the manager table.

struct GASStringNode
{        

    const char*         pData;
    GASStringManager*   pManager;
    union {
        GASStringNode*  pLower;
        GASStringNode*  pNextAlloc;
    };
	UInt32              RefCount;
	UInt32              HashFlags;
	UInt                Size;


	// *** Utility functions

	GASStringManager*   GetManager() const
	{
		return pManager;
	}
	inline UInt32  GetHashCode()
	{
		return HashFlags & 0x00FFFFFF;
	}

	void    AddRef()
	{
		RefCount++;
	}
	void    Release()
	{
		if (--RefCount == 0)
			ReleaseNode();
	}

	// Releases the node to the manager.
	void    ReleaseNode();    
	// Resolves pLower to a valid value.    
	void    ResolveLowercase_Impl();

	void    ResolveLowercase()
	{
		// This should not be called if lowercase was already resolved,
		// or we would overwrite it.
		GASSERT(pLower == 0);
		ResolveLowercase_Impl();
	}

};




// ***** GASString - ActionScript string implementation.

// GASString is represented as a pointer to a unique node so that
// comparisons can be implemented very fast. Nodes are allocated by
// and stored within GASStringManager.
// GASString objects can be created only by string manager; default
// constructor is not implemented.

struct GASStringNodeHolder
{
	GASStringNode*  pNode;
};

// Do not derive from GNewOverrideBase and do not new!
class GASString : public GASStringNodeHolder
{
	friend class GASStringManager;
	friend class GASStringBuiltinManager;

public:

	// *** Create/Destroy: can

	explicit GASString(GASStringNode *pnode)
	{
		pNode = pnode;
		pNode->AddRef();
	}

	GASString(const GASString& src)
	{
		pNode = src.pNode;
		pNode->AddRef();
	}
	~GASString()
	{        
		pNode->Release();
	}

	// *** General Functions

	void        Clear();

	// Pointer to raw buffer.
	const char* ToCStr() const          { return pNode->pData; }
	const char* GetBuffer() const       { return pNode->pData; }

	// Size of string characters without 0. Raw count, not UTF8.
	UInt        GetSize() const         { return pNode->Size; }
	bool        IsEmpty() const         { return pNode->Size == 0; }

	UInt        GetHashFlags() const    { return pNode->HashFlags; }
	UInt        GetHash() const         { return GetHashFlags() & Flag_HashMask; }

	GASStringNode* GetNode() const      { return pNode; }
	GASStringManager* GetManager() const { return pNode->GetManager(); }


	static UInt32   HashFunction(const char *pchar, size_t length);


	// *** UTF8 Aware functions.

	// Returns length in characters.
	UInt        GetLength() const;
	UInt32      GetCharAt(UInt index) const;

	// The rest of the functions here operate in UTF8. For example,    
	GASString   AppendChar(UInt32 ch);

	/*
	GFxString& Insert(const GFxString& substr, int posAt, int len = -1);
	GFxString& Insert(const char* substr, int posAt, int len = -1);
	GFxString& Insert(char ch, int posAt);

	// posAt is in bytes
	GFxString& Insert(const wchar_t* substr, int posAt, int len = -1);
	// posAt is in UTF8 chars
	GFxString& InsertUTF8(const wchar_t* substr, int posAt, int len = -1);
	// posAt is in bytes
	GFxString& Insert(const UInt32* substr, int posAt, int len = -1);
	// posAt is in UTF8 chars
	GFxString& InsertUTF8(const UInt32* substr, int posAt, int len = -1);

	GFxString& Remove(int posAt = -1, int len = 1);
	*/

	void        Remove(int posAt, int len = 1);

	// Returns a GFxString that's a substring of this.
	//  -start is the index of the first UTF8 character you want to include.    
	//  -end is the index one past the last UTF8 character you want to include.
	GASString   Substring(int start, int end) const;

	// Case-converted strings.
	GASString   ToUpper() const; 
	GASString   ToLower() const;



	// *** Operators

	// Assignment.
	void    operator = (const char* str);
	void    operator = (const GASString& src)
	{
		AssignNode(src.pNode);        
	}

	// Concatenation of string / UTF8.
	void        operator += (const char* str);
	void        operator += (const GASString& str);
	GASString   operator + (const char* str) const;
	GASString   operator + (const GASString& str) const;

	// Comparison.
	bool    operator == (const GASString& str) const
	{
		return pNode == str.pNode;
	}    
	bool    operator != (const GASString& str) const
	{
		return pNode != str.pNode;
	}
	bool    operator == (const char* str) const
	{
		return gfc_strcmp(pNode->pData, str) == 0;
	}
	bool    operator != (const char* str) const
	{
		return !operator == (str);
	}

	// Compares provide the same ordering for UTF8 due to
	// the nature of data.
	bool    operator<(const char* pstr) const
	{
		return gfc_strcmp(ToCStr(), pstr) < 0;
	}
	bool    operator<(const GASString& str) const
	{
		return *this < str.ToCStr();
	}
	bool    operator>(const char* pstr) const
	{
		return gfc_strcmp(ToCStr(), pstr) > 0;
	}
	bool    operator>(const GASString& str) const
	{
		return *this > str.ToCStr();
	}

	// Accesses raw bytes returned by GetSize.
	const char operator [] (UInt index) const
	{
		GASSERT(index < pNode->Size);
		return pNode->pData[index];
	}




	// ***** Custom Methods

	// Path flag access inlines.
	bool    IsNotPath() const                 { return (GetHashFlags() & GASString::Flag_IsNotPath) != 0;  }
	void    SetPathFlags(UInt32 flags) const  { pNode->HashFlags |= flags; }

	// Determines if this string is a built-in.
	bool    IsBuiltin() const           { return (GetHashFlags() & GASString::Flag_Builtin) != 0;  }
	// Determines if this string is a standard member.
	bool    IsStandardMember() const    { return (GetHashFlags() & GASString::Flag_StandardMember) != 0;  }


	void    AssignNode(GASStringNode *pnode)
	{
		pnode->AddRef();
		pNode->Release();
		pNode = pnode;
	}

	enum FlagConstants
	{
		Flag_HashMask       = 0x00FFFFFF,
		Flag_FlagMask       = 0xFF000000,
		// Flags
		Flag_Builtin        = 0x80000000,
		Flag_ConstData      = 0x40000000,
		Flag_StandardMember = 0x20000000,

		// This flag is set if GetLength() == GetSize() for a string.
		// Avoid extra scanning is Substring and indexing logic.
		Flag_LengthIsSize   = 0x10000000,

		// Determining whether a string a path is pricey, so we cache this information
		// in a string node. Flag_PathCheck is set if a check was made for a path.
		// Flag_IsNotPath is set if we have determined that this string is not to be a path.
		// If a check was not made yet, Flag_IsNotPath is always cleared.
		Flag_PathCheck      = 0x08000000,
		Flag_IsNotPath      = 0x04000000,
	};

	inline void     ResolveLowercase() const
	{
		if (pNode->pLower == 0)
			pNode->ResolveLowercase();
	}


	inline bool    Compare_CaseCheck(const GASString &str, bool caseSensitive) const
	{
		if (caseSensitive)
			return pNode == str.pNode;
		// For case in-sensitive strings we need to resolve lowercase.
		ResolveLowercase();
		str.ResolveLowercase();
		return pNode->pLower == str.pNode->pLower;
	}

	// Compares constants, assumes that 
	inline bool    CompareBuiltIn_CaseCheck(const GASString &str, bool caseSensitive) const
	{
		GASSERT((pNode->HashFlags & Flag_Builtin) != 0);
		if (caseSensitive)
			return pNode == str.pNode;
		// For case in-sensitive strings we need to resolve lowercase.
		str.ResolveLowercase();
		return pNode->pLower == str.pNode->pLower;
	}


	// Compares constants case-insensitively
	inline bool    CompareBuiltIn_CaseInsensitive(const GASString &str) const
	{
		GASSERT((pNode->HashFlags & Flag_Builtin) != 0);        
		// For case in-sensitive strings we need to resolve lowercase.
		str.ResolveLowercase();        
		return pNode->pLower == str.pNode->pLower;
	}

	// Compares constants case-insensitively
	inline bool    Compare_CaseInsensitive_Resolved(const GASString &str) const
	{
		GASSERT(pNode->pLower != 0);
		// For case in-sensitive strings we need to resolve lowercase.
		str.ResolveLowercase();        
		return pNode->pLower == str.pNode->pLower;
	}

	// Compares constants case-insensitively.
	// Assumes that pLower MUST be resolved in both strings.
	inline bool    CompareBuiltIn_CaseInsensitive_Unchecked(const GASString &str) const
	{
		GASSERT((pNode->HashFlags & Flag_Builtin) != 0);
		GASSERT(str.pNode->pLower != 0);
		return pNode->pLower == str.pNode->pLower;
	}


	// ***** Case Insensitive wrapper support

	// Case insensitive keys are used to look up insensitive string in hash tables
	// for SWF files with version before SWF 7.
	struct NoCaseKey
	{
		const GASString *pStr;        
		NoCaseKey(const GASString &str) : pStr(&str)
		{
			str.ResolveLowercase();
		}
	};

	bool    operator == (const NoCaseKey& strKey) const
	{
		return strKey.pStr->Compare_CaseInsensitive_Resolved(*this);
	}    
	bool    operator != (const NoCaseKey& strKey) const
	{
		return !strKey.pStr->Compare_CaseInsensitive_Resolved(*this);
	}

	// Raw compare is used to look up a string is extremely rare cases when no 
	// string context is available for GASString creation.    
	struct RawCompareKey
	{
		const char *pStr;
		size_t      Hash;

		RawCompareKey(const char *pstr, size_t length)
		{
			pStr = pstr;
			Hash = GASString::HashFunction(pstr, length);            
		}
	};

	bool    operator == (const RawCompareKey& strKey) const
	{
		return operator == (strKey.pStr);
	}    
	bool    operator != (const RawCompareKey& strKey) const
	{
		return operator != (strKey.pStr);
	}

};



// ***** String Manager implementation

struct GASStringKey
{
	const char* pStr;
	size_t      HashValue;
	size_t      Length;

	GASStringKey(const GASStringKey &src)
		: pStr(src.pStr), HashValue(src.HashValue), Length(src.Length)
	{ }
	GASStringKey(const char* pstr, size_t hashValue, size_t length)
		: pStr(pstr), HashValue(hashValue), Length(length)
	{ }
};


template<class C>
class GASStringNodeHashFunc
{
public:
	typedef C value_type;

	// Hash code is stored right in the node
	size_t  operator() (const C& data) const
	{
		return data->HashFlags;
	}

	size_t  operator() (const GASStringKey &str) const
	{
		return str.HashValue;
	}

	// Hash update - unused.
	static size_t  get_cached_hash(const C& data)               { return data->HashFlags; }
	static void    set_cached_hash(C& data, size_t hashValue)   { GUNUSED2(data, hashValue); }
	// Value access.
	static C&      get_value(C& data)                           { return data; }
	static const C& get_value(const C& data)                    { return data; }

};

// String node hash set - keeps track of all strings currently existing in the manager.
typedef GTL::ghash_set_uncached<GASStringNode*, GASStringNodeHashFunc<GASStringNode*> > GASStringNodeHash;




class GASStringManager
{    
	friend class GASString;  
	friend struct GASStringNode;

	GASStringNodeHash StringSet;


	// Allocation Page structures, used to avoid many small allocations.
	struct StringNodePage
	{
		enum { StringNodeCount = 127 };     
		// Node array starts here.
		GASStringNode       Nodes[StringNodeCount];
		// Next allocated page; save so that it can be released.
		StringNodePage  *   pNext;
	};

	// Strings text is also allocated in pages, to make small string management efficient.
	struct TextPage
	{
		// The size of buffer is usually 12 or 16, depending on platform.
		enum  {
			BuffSize   = (sizeof(void*) <= 4) ? (sizeof(void*) * 3) : (sizeof(void*) * 2),
			// Use -2 because we do custom alignment and we want to fit in allocator block.
			BuffCount  = (2048 / BuffSize) - 2
		};

		struct Entry
		{
			union
			{   // Entry for free node list.
				Entry*  pNextAlloc;
				char    Buff[BuffSize];
			};
		};

		Entry       Entries[BuffCount];
		TextPage*   pNext;
		void*       pMem;
	};


	// Free string nodes that can be used.
	GASStringNode*      pFreeStringNodes;
	// List of allocated string node pages, so that they can be released.
	// Note that these are allocated for the duration of GASStringManager lifetime.
	StringNodePage*     pStringNodePages;

	// Free string buffers that can be used, together with their owner nodes.
	TextPage::Entry*    pFreeTextBuffers;
	TextPage*           pTextBufferPages;

	// Pointer to the available string node.
	GASStringNode       EmptyStringNode;

	// Log object used for reporting AS leaks.
	GPtr<GFxLog>        pLog;
	GFxString           FileName;


	void            AllocateStringNodes();
	void            AllocateTextBuffers();

	GASStringNode*  AllocStringNode()
	{
		if (!pFreeStringNodes)
			AllocateStringNodes();
		// Grab next node from list.
		GASStringNode* pnode = pFreeStringNodes;
		if (pFreeStringNodes)
			pFreeStringNodes = pnode->pNextAlloc;
		return pnode;
	}

	void            FreeStringNode(GASStringNode* pnode)
	{
		if (pnode->pData)
		{
			if (!(pnode->HashFlags & GASString::Flag_ConstData))
				FreeTextBuffer(const_cast<char*>(pnode->pData), pnode->Size);
			pnode->pData = 0;
		}
		// Insert into free list.
		pnode->pNextAlloc = pFreeStringNodes;
		pFreeStringNodes  = pnode;
	}

	// Get buffer for text (adds 0 to length).
	char*           AllocTextBuffer(size_t length);
	// Allocates text buffer and copies length characters into it. Appends 0.
	char*           AllocTextBuffer(const char* pbuffer, size_t length);
	void            FreeTextBuffer(char* pbuffer, size_t length);


	// Various functions for creating string nodes.
	// Returns a node copy/reference to text in question.
	// All lengths specified must be exact and not include trailing characters.

	GASStringNode*  CreateConstStringNode(const char* pstr, size_t length, UInt32 stringFlags);
	// Allocates node owning buffer.
	GASStringNode*  CreateStringNode(const char* pstr);
	GASStringNode*  CreateStringNode(const char* pstr, size_t length);
	// These functions also perform string concatenation.
	GASStringNode*  CreateStringNode(const char* pstr1, size_t l1,
		const char* pstr2, size_t l2);
	GASStringNode*  CreateStringNode(const char* pstr1, size_t l1,
		const char* pstr2, size_t l2,
		const char* pstr3, size_t l3);
	// Wide character; use special type of counting.
	GASStringNode*  CreateStringNode(const wchar_t* pwstr);  

	GASStringNode* GetEmptyStringNode() { return &EmptyStringNode;  }

public:

	GASStringManager();
	~GASStringManager();

	// Sets the log that will be used to report leaks during destructor.    
	void        SetLeakReportLog(GFxLog *plog, const char *pfilename);

	GASString   CreateEmptyString()
	{
		return GASString(GetEmptyStringNode());
	}

	// Create a string from a buffer.
	GASString   CreateString(const char *pstr)
	{
		return GASString(CreateStringNode(pstr));
	} 
	GASString   CreateString(const char *pstr, size_t length)
	{
		return GASString(CreateStringNode(pstr, length));
	}     
	GASString   CreateString(const GFxString& str)
	{
		return GASString(CreateStringNode(str.ToCStr(), str.GetSize()));
	}
	GASString   CreateConstString(const char *pstr, size_t length, UInt32 stringFlags = 0)
	{
		return GASString(CreateConstStringNode(pstr, length, stringFlags));
	}
	GASString   CreateConstString(const char *pstr)
	{
		return GASString(CreateConstStringNode(pstr, gfc_strlen(pstr), 0));
	}
	// Wide character; use special type of counting.
	GASString  CreateString(const wchar_t* pwstr)
	{
		return GASString(CreateStringNode(pwstr));
	}  
};


// ***** Hash table used for member Strings

struct GASStringHashFunctor
{
	inline size_t  operator() (const GASString& str) const
	{
		// No need to mask out hashFlags since flags are in high bits that
		// will be masked out automatically by the hash table.
		return str.GetHashFlags();
	}

	// Hash lookup for custom keys.
	inline size_t  operator() (const GASString::NoCaseKey& ikey) const
	{        
		return ikey.pStr->GetHashFlags();
	}
	inline size_t  operator() (const GASString::RawCompareKey& ikey) const
	{
		return ikey.Hash;
	}
};

// Case-insensitive string hash.
template<class U>
class GASStringHash : public GTL::ghash_uncached<GASString, U, GASStringHashFunctor>
{
	typedef GASStringHash<U>                                          self;
	typedef GTL::ghash_uncached<GASString, U, GASStringHashFunctor>   base_class;

public:
	// Delegated constructors.
	GASStringHash()                                        { }
	GASStringHash(int size_hint) : base_class(size_hint)   { }
	GASStringHash(const self& src) : base_class(src)       { }
	~GASStringHash()                                       { }

	void    operator = (const self& src)                   { base_class::operator = (src); }


	// *** Case-Insensitive / Case-Selectable 'get' lookups.

	bool    get_CaseInsensitive(const GASString& key, U* pvalue) const
	{
		GASString::NoCaseKey ikey(key);
		return base_class::get_alt(ikey, pvalue);
	}
	// Pointer-returning get variety.
	const U* get_CaseInsensitive(const GASString& key) const   
	{
		GASString::NoCaseKey ikey(key);
		return base_class::get_alt(ikey);
	}
	U*  get_CaseInsensitive(const GASString& key)
	{
		GASString::NoCaseKey ikey(key);
		return base_class::get_alt(ikey);
	}

	// Case-checking based on argument.
	bool    get_CaseCheck(const GASString& key, U* pvalue, bool caseSensitive) const
	{
		return caseSensitive ? base_class::get(key, pvalue) : get_CaseInsensitive(key, pvalue);
	}
	const U* get_CaseCheck(const GASString& key, bool caseSensitive) const   
	{
		return caseSensitive ? base_class::get(key) : get_CaseInsensitive(key);
	}
	U*      get_CaseCheck(const GASString& key, bool caseSensitive)
	{
		return caseSensitive ? base_class::get(key) : get_CaseInsensitive(key);
	}


	// *** Case-Insensitive / Case-Selectable find.

	typedef typename base_class::const_iterator  const_iterator;
	typedef typename base_class::iterator        iterator;

	iterator    find_CaseInsensitive(const GASString& key)
	{
		GASString::NoCaseKey ikey(key);
		return base_class::find_alt(ikey);
	}
	iterator    find_CaseCheck(const GASString& key, bool caseSensitive)
	{
		return caseSensitive ? base_class::find(key) : find_CaseInsensitive(key);
	}


	// *** Case-Selectable set.

	// Set just uses a find and assigns value if found.
	// The key is not modified - this behavior is identical to Flash string variable assignment.    
	void    set_CaseCheck(const GASString& key, const U& value, bool caseSensitive)
	{
		iterator it = find_CaseCheck(key, caseSensitive);
		if (it != base_class::end())
		{
			it->second = value;
		}
		else
		{
			base_class::add(key, value);
		}
	} 


	// *** Case-Insensitive / Case-Selectable remove.

	void     remove_CaseInsensitive(const GASString& key)
	{   
		GASString::NoCaseKey ikey(key);
		base_class::remove_alt(ikey);
	}

	void     remove_CaseCheck(const GASString& key, bool caseSensitive)
	{   
		if (caseSensitive)
			base_class::remove(key);
		else
			remove_CaseInsensitive(key);
	}
};


#endif
