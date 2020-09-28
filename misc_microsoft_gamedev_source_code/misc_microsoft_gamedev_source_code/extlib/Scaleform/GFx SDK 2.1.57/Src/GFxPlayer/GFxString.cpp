/**********************************************************************

Filename    :   GFxString.cpp
Content     :   GFxString UTF8 string implementation with copy-on
                write semantics (thread-safe for assignment but not
                modification).
Created     :   April 27, 2007
Authors     :   Ankur

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxString.h"

#include "GFxLog.h"
#include "GUTF8Util.h"
#include "GDebug.h"

#include <stdlib.h>
#include <ctype.h>

#ifdef GFC_OS_QNX
# include <strings.h>
#endif

#define GFxString_LengthIsSize (UPInt(1) << GFxString::Flag_LengthIsSizeShift)

GFxString::DataDesc GFxString::NullData = {GFxString_LengthIsSize, 0, 1, {0} };

static UPInt RoundUp(UPInt len)
{
#ifdef ROUNDUP_NEAREST_MUL_16 
	// Round up to the nearest multiple of 16
	return (len + 15)& ~15;
#else
	return 2*len;
#endif
}


GFxString::GFxString()
{
	pData = (&NullData);
	pData->AddRef();
};

GFxString::GFxString(const char* data)
{
	if(data == NULL)
	{
		pData = (&NullData);
		pData->AddRef();
		return;
	}
	UPInt size = gfc_strlen(data); //length in bytes- doesn't matter if _data is UTF8

#ifdef ROUND_UP_ONINIT // Should we allocate more space than needed?
	UPInt new_size = RoundUp(size);
#else
	UPInt new_size = size;
#endif
	pData = (DataDesc*)GALLOC(sizeof(DataDesc)+ new_size);
	memcpy(pData->Data, data, size);
	pData->Data[size] = '\0'; // append null character
	pData->RefCount = 1;
	pData->Size		= size; 
	pData->BuffSize = new_size;
};

GFxString::GFxString(const char* _data, UPInt buflen)
{
	if(_data == NULL)
	{
		pData = (&NullData);
		pData->AddRef();
		return;
	}

	UPInt size = buflen;

#ifdef ROUND_UP_ONINIT // Should we allocate more space than needed?
	UPInt new_size = RoundUp(size);
#else
	UPInt new_size = size;
#endif
	pData = (DataDesc*)GALLOC(sizeof(DataDesc)+ new_size);
	memcpy(pData->Data, _data, size);
	pData->Data[size] = '\0'; // append null character
	pData->RefCount = 1;
	pData->Size		= size;
	pData->BuffSize = new_size;
};


GFxString::GFxString(const wchar_t* data)
{
    pData = (&NullData);
    pData->AddRef();
    // Simplified logic for wchar_t constructor.
    if (data)    
        *this = data;    
}

UPInt GFxString::GetLength() const 
{
	// Optimize length accesses for non-UTF8 character strings. 
	UPInt length, size = GetSize();    
	
	if (pData->Size & GFxString_LengthIsSize)
		return size;    
	
	length = (UPInt)GUTF8Util::GetLength(pData->Data, (UPInt)GetSize());
	
	if (length == GetSize())
		pData->Size |= GFxString_LengthIsSize;
	
	return length;
}

UInt32 GFxString::GetCharAt(UPInt index) const 
{  
	SPInt i = (SPInt) index;
	const char* buf = pData->Data;
	UInt32  c;

	do 
	{
		c = GUTF8Util::DecodeNextChar(&buf);
		i--;

		if (c == 0)
		{
			// We've hit the end of the string; don't go further.
			GASSERT(i == 0);
			return c;
		}
	} while (i >= 0);    

	return c;
}

UInt32 GFxString::GetFirstCharAt(UPInt index, const char** offset) const
{
    SPInt i = (SPInt) index;
    const char* buf = pData->Data;
    UInt32  c;

    do 
    {
        c = GUTF8Util::DecodeNextChar(&buf);
        i--;

        if (c == 0)
        {
            // We've hit the end of the string; don't go further.
            GASSERT(i == 0);
            return c;
        }
    } while (i >= 0);    

    *offset = buf;

    return c;
}

UInt32 GFxString::GetNextChar(const char** offset) const
{
    return GUTF8Util::DecodeNextChar(offset);
}

void GFxString::DataDesc::SetSize(UPInt size, bool flag)
{
	Size = size;
	if(flag)
	{
		Size |= GFxString_LengthIsSize;
	}
}

void GFxString::ResizeInternal(UPInt size, bool flag)
{
	if(size < pData->BuffSize)
	{
		//already have enough space- no need for reallocation
		pData->SetSize(size, flag);
		return;
	}

	UPInt new_size = RoundUp(size);
	pData = (DataDesc*)GREALLOC(pData, sizeof(DataDesc)+ new_size);
	pData->BuffSize = new_size;
	pData->SetSize(size, flag);
}

void GFxString::Resize(UPInt newSize)
{

	bool flag = false;
	if (pData->RefCount == 1)
	{
		if (newSize < GetSize())
		{
			pData->Data[newSize] = 0; 
			pData->SetSize(newSize, flag);
		}
		else
		{
			if (newSize > pData->BuffSize)
			{
				UPInt newBuffSize = RoundUp(newSize);
				pData = (DataDesc*)GREALLOC(pData, sizeof(DataDesc)+ newBuffSize);
				pData->BuffSize = newBuffSize;
				pData->SetSize(newSize, flag);
			}
		}
		GASSERT(pData->RefCount == 1);
	}
	else
	{
		UPInt newBuffSize = RoundUp(newSize);
		DataDesc* pNewNode = AllocNewNode(newBuffSize, 1, newSize, false);
		copy(pNewNode->Data, pData->Data, newSize);
		pData->Release();
		pData = pNewNode;
	}
}

void GFxString::AppendChar(UInt32 ch)
{
	char    buff[8];
	UPInt   index = 0;
	UPInt	index_now = index;
	UPInt   curr_size = GetSize();

	// Converts ch into UTF8 string and fills it into buff. Also increments index according to the number of bytes
	// in the UTF8 string.
    SPInt   encodeSize = 0;
	GUTF8Util::EncodeChar(buff, &encodeSize, ch);
    GASSERT(encodeSize >= 0);
    index = (UPInt)encodeSize;
	UPInt num_bytes_added = index - index_now; //these many bytes need to be appended.
	buff[num_bytes_added] = 0;
	UPInt new_size = curr_size + num_bytes_added;

	if (pData->RefCount == 1)
	{ 
		// No one else refers to this string, so its safe to append to existing string
		ResizeInternal(new_size, false); // Allocate more space
		copy(pData->Data + curr_size, buff, num_bytes_added);
		// Make sure no other thread tried to change the reference count
		GASSERT(pData->RefCount == 1);
	}
	else
	{
		//allocate new node
		UPInt new_size_rounded = RoundUp(new_size);
		DataDesc* pNewNode = AllocNewNode(new_size_rounded, 1, new_size, false);
		copy(pNewNode->Data, pData->Data, GetSize());
		copy(pNewNode->Data + GetSize(), buff, num_bytes_added);
		pData->Release();
		pData = pNewNode;
	}
}

void GFxString::AppendString(const wchar_t* pstr, SPInt len)
{
    if (!pstr)
        return;

    SPInt   srcSize		= GUTF8Util::GetEncodeStringSize(pstr, len);
    UPInt   origSize	= GetSize();
    UPInt	size		= srcSize + origSize;

    if(pData->RefCount == 1)
    {
        ResizeInternal(size, false);
        GUTF8Util::EncodeString(pData->Data + origSize,  pstr, len);
        GASSERT(pData->RefCount == 1);
    }
    else
    {
        UPInt new_size_rounded = RoundUp(size);
        DataDesc* pNewNode = AllocNewNode(new_size_rounded, 1, size, false);
        copy(pNewNode->Data, pData->Data, origSize);
        
        GUTF8Util::EncodeString(pNewNode->Data + origSize,  pstr, len);
        pData->Release();
        pData = pNewNode;
    }
}

void GFxString::AppendString(const char* putf8str, SPInt utf8StrSz)
{
    if (!putf8str || !utf8StrSz)
        return;
    if (utf8StrSz == -1)
        utf8StrSz = (SPInt)gfc_strlen(putf8str);

    UPInt   origSize	= GetSize();
    UPInt   size		= (UPInt)utf8StrSz + origSize;

    if(pData->RefCount == 1)
    {
        ResizeInternal(size, false);
        copy(pData->Data + origSize,  putf8str, utf8StrSz);
        GASSERT(pData->RefCount == 1);
    }
    else
    {
        UPInt new_size_rounded = RoundUp(size);
        DataDesc* pNewNode = AllocNewNode(new_size_rounded, 1, size, false);
        copy(pNewNode->Data, pData->Data, origSize);
        copy(pNewNode->Data + origSize,  putf8str, utf8StrSz);
        pData->Release();
        pData = pNewNode;
    }
}

GFxString::DataDesc* GFxString::AllocNewNode(UPInt newSizeRounded, int refCount, UPInt newSize, bool flag)
{
	DataDesc* pNewData = (DataDesc*)GALLOC(sizeof(DataDesc)+ newSizeRounded);
	pNewData->RefCount = refCount;
	pNewData->BuffSize = newSizeRounded;
	pNewData->SetSize(newSize, flag);
	return pNewData;
}

void    GFxString::operator = (const char* str)
{
	str = str ? str : "";
	UPInt size = gfc_strlen(str);

	if(pData->RefCount == 1)
	{
		ResizeInternal(size, false);
		memcpy(pData->Data, str, gfc_strlen(str)+1);
	}
	else
	{
		UPInt new_size = RoundUp(size);
		DataDesc* pNewNode = AllocNewNode(new_size, 1, size, false);
		copy(pNewNode->Data, str, size);
		pData->Release();
		pData = pNewNode;
	}
}

void    GFxString::operator = (const wchar_t* wstr)
{
	wstr = wstr ? wstr : L"";
	UPInt size = (UPInt)GUTF8Util::GetEncodeStringSize(wstr);
		
	if(pData->RefCount == 1)
	{
		ResizeInternal(size, false);
		GUTF8Util::EncodeString(pData->Data, wstr);
	}
	else
	{
		//allocate new node
		UPInt new_size = RoundUp(size);
		DataDesc* pNewNode = AllocNewNode(new_size, 1, size, false);
		GUTF8Util::EncodeString(pNewNode->Data, wstr);
		pData->Release();
		pData = pNewNode;
	}
}

void    GFxString::operator = (const GFxString& src)
{ 
	src.pData->AddRef();
	pData->Release();
	pData = src.pData;
}

void    GFxString::operator += (const GFxString& src)
{
	UPInt   srcSize	= src.GetSize();
	UPInt   origSize = GetSize();
	UPInt   size = srcSize + origSize;
	bool	flag = (((src.pData->Size & GFxString_LengthIsSize) & (pData->Size & GFxString_LengthIsSize)) == GFxString_LengthIsSize);

	if(pData->RefCount == 1)
	{
		ResizeInternal(size, flag);
		copy(pData->Data + origSize, src.pData->Data, srcSize);
		GASSERT(pData->RefCount == 1);
	}
	else
	{
		UPInt new_size_rounded = RoundUp(size);
		DataDesc* pNewNode = AllocNewNode(new_size_rounded, 1, size, flag);
		copy(pNewNode->Data, pData->Data, origSize);
		copy(pNewNode->Data + origSize, src.pData->Data, src.GetSize());
		pData->Release();
		pData = pNewNode;
	}
}

void   GFxString::operator += (const char* str)
{ 
	str = str ? str : "";

	UPInt	srcSize = gfc_strlen(str);
	UPInt	origSize = GetSize();
	UPInt	size = srcSize + origSize;

	if(pData->RefCount == 1)
	{
		ResizeInternal(size, false);
		copy(pData->Data + origSize, str, srcSize); 
		GASSERT(pData->RefCount == 1);
	}
	else
	{
		UPInt new_size_rounded = RoundUp(size);
		DataDesc* pNewNode = AllocNewNode(new_size_rounded, 1, size, false);
		copy(pNewNode->Data, pData->Data, origSize);
		copy(pNewNode->Data + origSize, str, srcSize);
		pData->Release();
		pData = pNewNode;
	}
}

void	GFxString::operator += (const wchar_t* wstr)
{
	wstr = wstr ? wstr : L"";

	UPInt		srcSize		= GUTF8Util::GetEncodeStringSize(wstr);
	UPInt		origSize	= GetSize();
	UPInt		size		= srcSize + origSize;
	
	if(pData->RefCount == 1)
	{
		ResizeInternal(size, false);
		GUTF8Util::EncodeString(pData->Data + origSize,  wstr);
		GASSERT(pData->RefCount == 1);
	}
	else
	{
		UPInt new_size_rounded = RoundUp(size);
		DataDesc* pNewNode = AllocNewNode(new_size_rounded, 1, size, false);
		copy(pNewNode->Data, pData->Data, origSize);
		GUTF8Util::EncodeString(pNewNode->Data + origSize,  wstr);
		pData->Release();
		pData = pNewNode;
	}
}

void	GFxString::operator += (char str)
{ 
	UPInt   srcSize		= 1;
	UPInt   origSize	= GetSize();
	UPInt   size		= srcSize + origSize;
	bool	flag		= ((pData->Size & GFxString_LengthIsSize)==  GFxString_LengthIsSize);
	
	if(pData->RefCount == 1)
	{
		ResizeInternal(size, flag);
		copy(pData->Data + origSize, &str, srcSize);
		GASSERT(pData->RefCount == 1);
	}
	else
	{
		//allocate new node
		UPInt new_size_rounded = RoundUp(size);
		DataDesc* pNewNode = AllocNewNode(new_size_rounded, 1, size, flag);
		copy(pNewNode->Data, pData->Data, origSize);
		copy(pNewNode->Data + origSize, &str, srcSize);
		pData->Release();
		pData = pNewNode;
	}
}

GFxString   GFxString::operator + (const char* str) const
{ 
	str = str ? str : "";
	GFxString tmp1 = *this;
	tmp1 += str;
	return tmp1;
}

GFxString   GFxString::operator + (const GFxString& src) const
{ 
	GFxString tmp1 = *this;
	tmp1 += src;
	return tmp1;
}

void    GFxString::Remove (UPInt posAt, SPInt len)
{
	bool	flag = ((pData->Size & GFxString_LengthIsSize)==  GFxString_LengthIsSize);
	
	if(pData->RefCount == 1)
	{
		InternalRemove(posAt, len);
		// This check is necessary because when the last character is removed, the string can become NULL 
		// and the refCount will be equal to that of the NULL string. 
		if(pData != &NullData)
		{
			GASSERT((pData->RefCount == 1));
		}	
	}
	else
	{
		//allocate new node
		UPInt size	= GetSize(); 
		UPInt bufSize = pData->BuffSize;
		DataDesc* pNewNode	= AllocNewNode(bufSize, 1, size, flag);
		copy(pNewNode->Data, pData->Data, GetSize());
		pData->Release();
		pData = pNewNode;
		InternalRemove(posAt, len);
	}
}
void      GFxString::InternalRemove(UPInt posAt, SPInt len)
{
	// len indicates the number of characters to remove. 
	UPInt length = GetLength();

	//	Make sure there are enough characters in the string 
	if ((SPInt)(length - posAt) < len) 
	{
		return; 
	}
	
	if (length == 1)// we have only one character, deleting it will result in an empty string.
	{
		GFREE(pData);
		pData = (&NullData);
		pData->AddRef();
		return;
	}
	
	if((pData->Size & GFxString_LengthIsSize))
	{ 
		// For character strings
		memcpy(pData->Data + posAt, pData->Data + posAt + len, GetSize() - posAt - len + 1);
		pData->Size -= len; // Decrement Size appropriately
		return;
	}
		
	// Get the byte position of the UTF8 char at position posAt
	SPInt bytePos = GUTF8Util::GetByteIndex(posAt, pData->Data);
	// Get position of next character
	SPInt bytePosNext = GUTF8Util::GetByteIndex(posAt+len, pData->Data);

	// Note that in case we are trying to get the byte index of the EOS (end of string) 
	// character (or any character afterwards)- i.e., the character after 
	// the last character- GetByteIndex will return the number of bytes in the string. 
	// This is not what the	documentation of  GetByteIndex says, so be careful if the 
	// implementation of GetByteIndex changes in the future.

	SPInt charSize;

	if (bytePosNext == bytePos)
	{
		// Since we are trying to remove a non existent character. 
		// For every valid character, bytePosNext > bytePos.
		return; 
	}
	else
	{
		charSize =  bytePosNext - bytePos;
	}
	
	// +1 for the null character that is not counted in pData->Size
	memcpy(pData->Data + bytePos, pData->Data + bytePos + charSize, GetSize() - bytePosNext+1);
	pData->Size -= charSize; 
	//decrement Size appropriately
}

GFxString   GFxString::Substring(UPInt start, UPInt end)
{
	UPInt len = GetLength();
	if (end > len)
	{ 
		return GFxString((const char*)NULL);
	}
	
	if (pData->Size & GFxString_LengthIsSize)
	{ 
		// For character strings
		GFxString tmp(pData->Data + start, end - start);
		return tmp;
	}

	// Get position of starting character
	SPInt bytePosStart = GUTF8Util::GetByteIndex(start, pData->Data);
	// Get position of end character
	SPInt bytePosEnd = GUTF8Util::GetByteIndex(end, pData->Data);
	GFxString tmp(pData->Data + bytePosStart, (UPInt)(bytePosEnd - bytePosStart));
	return tmp;
}

void GFxString::Clear()
{	
    NullData.AddRef();
    pData->Release();
    pData = (&NullData);
}

GFxString   GFxString::ToUpper() const 
{
	const char* buf = pData->Data;
	GFxString str;
	for (;;)
	{
		UInt32 c = GUTF8Util::DecodeNextChar(&buf);

		if (c == 0)
		{
			// We've hit the end of the string; don't go further.
			return str;
		}
		str.AppendChar (gfc_towupper(wchar_t(c)));
	}

	//return str;
}

GFxString   GFxString::ToLower() const 
{
	const char* buf = pData->Data;
	GFxString str;
	for (;;)
	{
		UInt32 c = GUTF8Util::DecodeNextChar(&buf);

		if (c == 0) {
			// We've hit the end of the string; don't go further.
			return str;
		}
		str.AppendChar (gfc_towlower(wchar_t(c)));
	}

	//return str;
}

GFxString& GFxString::Insert (const char* substr, UPInt posAt, SPInt len)
{
	//len indicates number of bytes that should
	//be inserted- if it is -1, the whole substr should be inserted. 
	UPInt subLen    = (len < 0) ? gfc_strlen(substr) : (UPInt)len; 
	UPInt curr_size	= GetSize();
	UPInt new_size	= curr_size + subLen;
	UPInt byteIndex;

	if (pData->Size & GFxString_LengthIsSize)
	{
		byteIndex = posAt;
	}
	else
	{
		byteIndex = (UPInt)GUTF8Util::GetByteIndex(posAt, pData->Data); 
	}
	if (pData->RefCount == 1)
	{ 
        //no one else refers to this string, so its safe to append to existing string
		ResizeInternal(new_size, false); //allocate more space
		char* pbuf = pData->Data;
		memmove(pbuf + byteIndex + subLen, pbuf + byteIndex, curr_size - byteIndex + 1);
		memcpy (pbuf + byteIndex, substr, subLen);
		// Make sure no other thread tried to change the reference count
		GASSERT(pData->RefCount == 1);
	}
	else
	{
		//allocate new node
		UPInt new_size_rounded	= RoundUp(new_size);
		DataDesc* pNewNode		= AllocNewNode(new_size_rounded, 1, new_size, false);

		copy(pNewNode->Data, pData->Data, GetSize());
		char* pbuf = pNewNode->Data;
		memmove(pbuf + byteIndex + subLen, pbuf + byteIndex, curr_size - byteIndex + 1);
		memcpy (pbuf + byteIndex, substr, subLen);
		pData->Release();
		pData = pNewNode;
	}

	return *this;
}

GFxString& GFxString::Insert (const UInt32* substr, UPInt posAt, SPInt len)
{
	for (SPInt i = 0; i < len; ++i)
	{
		UPInt charw = InsertCharAt(substr[i], posAt);
		posAt += charw;
	}
	return *this;
}

UPInt GFxString::InsertCharAt(UInt32 c, UPInt posAt)
{
	char buf[8];
	SPInt index = 0;
	GUTF8Util::EncodeChar(buf, &index, c);
    GASSERT(index >= 0);
	buf[(UPInt)index] = 0;

	Insert(buf, posAt, index);
	return (UPInt)index;
}


int  GFxString::CompareNoCase(const char* a, const char* b)
{
    return gfc_stricmp(a, b);
}

int  GFxString::CompareNoCase(const char* a, const char* b, SPInt len)
{
    if (len)
    {
        SPInt f,l;
        SPInt slen = len;
        const char *s = b;
        do {
            f = (SPInt)tolower((int)(*(a++)));
            l = (SPInt)tolower((int)(*(b++)));
        } while (--len && f && (f == l) && *b != 0);

        if (f == l && (len != 0 || *b != 0))
        {
            f = (SPInt)slen;
            l = (SPInt)gfc_strlen(s);
            return int(f - l);
        }

        return int(f - l);
    }
    else
        return (0-(int)gfc_strlen(b));
}

// ***** Implement hash static functions

// Hash function
size_t GFxString::BernsteinHashFunction(const void* pdataIn, size_t size, size_t seed)
{
	const UByte*    pdata   = (const UByte*) pdataIn;
	size_t          h       = seed;
	while (size > 0)
	{
		size--;
		h = ((h << 5) + h) ^ (UInt) pdata[size];
	}

	return h;
}

// Hash function, case-insensitive
size_t GFxString::BernsteinHashFunctionCIS(const void* pdataIn, size_t size, size_t seed)
{
	const UByte*    data = (const UByte*) pdataIn;
	size_t          h = seed;
	while (size > 0)
	{
		size--;
		h = ((h << 5) + h) ^ (UInt) tolower(data[size]);
	}

	// Alternative: "sdbm" hash function, suggested at same web page above.
	// h = 0;
	// for bytes { h = (h << 16) + (h << 6) - hash + *p; }
	return h;
}


void GFxString::EscapeSpecialHTML(const char* psrc, UPInt length, GFxString* pescapedStr)
{
    GUNUSED(length);

    UInt32 ch;

    while((ch = GUTF8Util::DecodeNextChar(&psrc)) != 0)
    {
        if (ch == '<')
        {
            pescapedStr->AppendString("&lt;", 4);
        }
        else if (ch == '>')
        {
            pescapedStr->AppendString("&gt;", 4);
        }
        else if (ch == '\"')
        {
            pescapedStr->AppendString("&quot;", 6);
        }
        else if (ch == '\'')
        {
            pescapedStr->AppendString("&apos;", 6);
        }
        else if (ch == '&')
        {
            pescapedStr->AppendString("&amp;", 5);
        }
        else
        {
            pescapedStr->AppendChar(ch);
        }
    }
}

void GFxString::UnescapeSpecialHTML(const char* psrc, UPInt length, GFxString* punescapedStr)
{
    GUNUSED(length);

    UInt32 ch;

    while ((ch = GUTF8Util::DecodeNextChar(&psrc)) != 0)
    {
        if (ch == '&')
        {
            if (strncmp(psrc, "quot;", 5) == 0)
            {
                punescapedStr->AppendChar('\"');
                psrc += 5;
                break;
            }
            else if (strncmp(psrc, "apos;", 5) == 0)
            {
                punescapedStr->AppendChar('\'');
                psrc += 5;
                break;
            }
            else if (strncmp(psrc, "amp;", 4) == 0)
            {
                punescapedStr->AppendChar('&');
                psrc += 4;
                break;
            }
            else if (strncmp(psrc, "lt;", 3) == 0)
            {
                punescapedStr->AppendChar('<');
                psrc += 3;
                break;
            }
            else if (strncmp(psrc, "gt;", 3) == 0)
            {
                punescapedStr->AppendChar('>');
                psrc += 3;
                break;
            }
            else
                punescapedStr->AppendChar(ch);
        }
        else
            punescapedStr->AppendChar(ch);
    }
}


// ***** GFxWStringBuffer class


GFxWStringBuffer::GFxWStringBuffer(const GFxWStringBuffer& other)
    : pText(0), Length(0), Reserved(0,0)
{
    // Our reserve is 0. It's ok, although not effient. Should we add
    // a different constructor?
    if (other.pText && Resize(other.Length+1))    
        memcpy(pText, other.pText, (other.Length+1)*sizeof(wchar_t));
}

GFxWStringBuffer::~GFxWStringBuffer()
{
    if ((pText != Reserved.pBuffer) && pText)
        GFREE(pText);
}

bool     GFxWStringBuffer::Resize(UPInt size)
{    
    if ((size > Length) && (size >= Reserved.Size))
    {
        wchar_t* palloc = (wchar_t*) GALLOC(sizeof(wchar_t)*(size+1));
        if (palloc)
        {
            if (pText)
                memcpy(palloc, pText, (Length+1)*sizeof(wchar_t));
            palloc[size] = 0;

            if ((pText != Reserved.pBuffer) && pText)
                GFREE(pText);
            pText  = palloc;
            Length = size;
            return true;
        }
        return false;
    }

    if (pText)
        pText[size] = 0;
    Length = size;
    return true;
}


// Assign buffer data.
GFxWStringBuffer& GFxWStringBuffer::operator = (const GFxWStringBuffer& buff)
{
    SetString(buff.pText, buff.Length);
    return *this;
}

GFxWStringBuffer& GFxWStringBuffer::operator = (const GFxString& str)
{
    UPInt size = str.GetLength();
    if (Resize(size) && size)    
        GUTF8Util::DecodeString(pText, str.ToCStr(), size);
    return *this;
}

GFxWStringBuffer& GFxWStringBuffer::operator = (const char* putf8str)
{
    UPInt size = GUTF8Util::GetLength(putf8str);
    if (Resize(size) && size)    
        GUTF8Util::DecodeString(pText, putf8str, size);
    return *this;
}

GFxWStringBuffer& GFxWStringBuffer::operator = (const wchar_t *pstr)
{
    UPInt length = 0;
    for (const wchar_t *pstr2 = pstr; *pstr2 != 0; pstr2++)
        length++;     
    SetString(pstr, length);
    return *this;
}

void GFxWStringBuffer::SetString(const wchar_t* pstr, UPInt length)
{
    if (length == GFC_MAX_UPINT)
        length = gfc_wcslen(pstr);
    if (Resize(length) && length)
        memcpy(pText, pstr, (length+1)*sizeof(wchar_t));    
}

void GFxWStringBuffer::StripTrailingNewLines()
{
    SPInt len = SPInt(Length);
    // check, is the content already null terminated
    if (len > 0 && pText[len -1] == 0)
        --len; //if yes, skip the '\0'
    for (SPInt i = len - 1; i >= 0 && (pText[i] == '\n' || pText[i] == '\r'); --i)
    {
        --Length;
        pText[i] = 0;
    }
}
