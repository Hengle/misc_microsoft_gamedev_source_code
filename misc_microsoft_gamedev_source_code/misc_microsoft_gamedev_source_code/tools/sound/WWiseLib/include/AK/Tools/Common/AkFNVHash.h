#ifndef _FNVHASH_H
#define _FNVHASH_H

// http://www.isthe.com/chongo/tech/comp/fnv/

//////////////////////////////////////////////////////////////////
//
// ***************************************************************
//
// IMPORTANT: The Migration Utility contains a C# version of this
// class, to assign Short IDs to objects created during migration.
// If you modify this class, be sure to update its C# counterpart,
// ShortIDGenerator, at the same time.
//
// ***************************************************************
//
//////////////////////////////////////////////////////////////////

template <unsigned char HashSize> 
class FNVHash
{
public:
	FNVHash();	///< Constructor
	~FNVHash();	///< Destructor

	/// Turn the provided data into a hash value.
	/// When Wwise use this hash with strings, it always provides lower case only string.
	unsigned int Compute( const unsigned char* in_pData, unsigned int in_dataSize ) const;

private:
	static const unsigned int s_prime32;
	static const unsigned int s_offsetBasis32;
};

#include <AK/Tools/Common/AkAssert.h>

template <unsigned char HashSize> 
const unsigned int FNVHash<HashSize>::s_prime32 = 16777619;

template <unsigned char HashSize> 
const unsigned int FNVHash<HashSize>::s_offsetBasis32 = 2166136261;

template <unsigned char HashSize> 
FNVHash<HashSize>::FNVHash()
{
	AKASSERT( HashSize <= 32 && "Doesn't support greater than 32 bits hash.  Feel free to add it." );
}

template <unsigned char HashSize> 
FNVHash<HashSize>::~FNVHash()
{
}

template <unsigned char HashSize> 
unsigned int FNVHash<HashSize>::Compute( const unsigned char* in_pData, unsigned int in_dataSize ) const
{
	const unsigned char* pEnd = in_pData + in_dataSize;		/* beyond end of buffer */

	unsigned int hval = s_offsetBasis32;

	// FNV-1 hash each octet in the buffer
	while( in_pData < pEnd ) 
	{
		// multiply by the 32 bit FNV magic prime mod 2^32
		hval *= s_prime32;

		// xor the bottom with the current octet
		hval ^= *in_pData++;
	}

	// XOR-Fold to the required number of bits
	if( HashSize == 32 )
		return hval;

	unsigned int mask = ((unsigned int)((AkInt64)1 << HashSize)-1);
	return (unsigned int)((AkInt64)hval >> HashSize) ^ (hval & mask);
}

#endif
