//==============================================================================
// hash.h
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================
#pragma once

//-----------------------------------------------------------------------------------
// Low-level hash functions
//-----------------------------------------------------------------------------------
typedef DWORD (*HASH_FUNCTION)(const unsigned char *key, DWORD length, DWORD initialValue);

extern DWORD hash(const unsigned char *key, DWORD length, DWORD initialValue);
inline DWORD hash(const void *key, DWORD length, DWORD initialValue = 0) { return hash(reinterpret_cast<const uchar*>(key), length, initialValue); }

// Paul Hsieh's hash function, ~66% faster than hash()
extern DWORD hashFast(const unsigned char *key, DWORD length, DWORD initialValue = 0);
inline DWORD hashFast(const void *key, DWORD length, DWORD initialValue = 0) { return hashFast(reinterpret_cast<const uchar*>(key), length, initialValue); }

//-----------------------------------------------------------------------------------
// class BHash
//-----------------------------------------------------------------------------------
class BHash
{
public:
   BHash()
   {
      clear();
   }

   explicit BHash(uint prevHash)
   {
      clear(prevHash);
   }

   BHash(const void* pData, int dataLen, uint prevHash = 0)
   {
      clear(prevHash);
      update(pData, dataLen);
   }

   void clear(uint prevHash = 0)
   {
      v[0] = v[1] = 0x9E3779B9;
      v[2] = prevHash;
   }

   int size(void) const
   {
      return NumElements;
   }

   uint operator[] (int i) const 
   { 
      return v[debugRangeCheck(i, size())]; 
   }
   
   uint& operator[] (int i)    
   { 
      return v[debugRangeCheck(i, size())]; 
   }

   BHash& update(const void* pData, uint dataLen);

   operator uint() const
   {
      return v[2];
   }

protected:
   enum { NumElements = 3 };
   uint v[NumElements];

   void bitMix(void);
};

//-----------------------------------------------------------------------------------
// struct BBitHashable
// BEWARE, this evil struct accesses the parent's raw memory!!
//-----------------------------------------------------------------------------------
template<class Parent>
struct BBitHashable 
{
   operator size_t() const
   {
      return hashFast(reinterpret_cast<const uchar*>(this), sizeof(Parent));
   }

   operator BHash() const
   {
      return BHash(this, sizeof(Parent));
   }

   bool operator== (const BBitHashable& b) const
   {
      const uint8* p = reinterpret_cast<const uint8*>(this);
      const uint8* q = reinterpret_cast<const uint8*>(&b);
      for (int i = 0; i < sizeof(Parent); i++)
      {
         if (p[i] != q[i])
            return false;
      }
      return true;
   }

   bool operator!= (const BBitHashable& b) const
   {
      return !(*this == b);
   }

   bool operator< (const BBitHashable& b) const
   {
      const uint8* p = reinterpret_cast<const uint8*>(this);
      const uint8* q = reinterpret_cast<const uint8*>(&b);
      for (int i = 0; i < sizeof(Parent); i++)
      {
         if (p[i] < q[i])
            return true;
         else if (p[i] != q[i])
            return false;
      }
      return false;
   }
};

//-----------------------------------------------------------------------------------
// Intrusive generic hasher. Your type supplies a hash() method.
//-----------------------------------------------------------------------------------
template<class V> struct BIntrusiveHasher
{
   size_t operator() (const V& key) const { return key.hash(); }
};

//-----------------------------------------------------------------------------------
// Hash value by munging key's bits.
//-----------------------------------------------------------------------------------
template<class V> struct BBitHasher
{
   size_t operator () (const V& key) const { return hashFast(&key, sizeof(key)); }
};

//-----------------------------------------------------------------------------------
// operator size_t hasher. Your type supplies a operator size_t() method.
//-----------------------------------------------------------------------------------
template <class V> struct BHasher
{
   size_t operator() (const V& key) const { return static_cast<size_t>(key); }
};

//-----------------------------------------------------------------------------------
// Hash helpers
//-----------------------------------------------------------------------------------
template <> struct BHasher<char> { size_t operator() (const char& key) const { return BBitHasher<char>()(key); } };
template <> struct BHasher<uchar> { size_t operator() (const uchar& key) const { return BBitHasher<uchar>()(key); } };
template <> struct BHasher<short> { size_t operator() (const short& key) const { return BBitHasher<short>()(key); } };
template <> struct BHasher<ushort> { size_t operator() (const ushort& key) const { return BBitHasher<ushort>()(key); } };
template <> struct BHasher<int> { size_t operator() (const int& key) const { return BBitHasher<int>()(key); } };
template <> struct BHasher<uint> { size_t operator() (const uint& key) const { return BBitHasher<uint>()(key); } };
template <> struct BHasher<void*> { size_t operator() (void* const & key) const { return BBitHasher<void*>()(key); } };
template <> struct BHasher<int64> { size_t operator() (const int64& key) const { return BBitHasher<int64>()(key); } };
template <> struct BHasher<uint64> { size_t operator() (const uint64& key) const { return BBitHasher<uint64>()(key); } };

template <> struct BHasher<BString> { size_t operator() (const BString& key) const { return key.length() ? hashFast(key.getPtr(), key.length() * sizeof(BString::charType)) : 0; } };
template <> struct BHasher<const BString> { size_t operator() (const BString& key) const { return key.length() ? hashFast(key.getPtr(), key.length() * sizeof(BString::charType)) : 0; } };

template <> struct BHasher<BUString> { size_t operator() (const BUString& key) const { return key.length() ? hashFast(key.getPtr(), key.length() * sizeof(BUString::charType)) : 0; } };
template <> struct BHasher<const BUString> { size_t operator() (const BUString& key) const { return key.length() ? hashFast(key.getPtr(), key.length() * sizeof(BUString::charType)) : 0; } };

template <> struct BHasher<BSimString> { size_t operator() (const BSimString& key) const { return key.length() ? hashFast(key.getPtr(), key.length() * sizeof(BSimString::charType)) : 0; } };
template <> struct BHasher<const BSimString> { size_t operator() (const BSimString& key) const { return key.length() ? hashFast(key.getPtr(), key.length() * sizeof(BSimString::charType)) : 0; } };

template <> struct BHasher<BRenderString> { size_t operator() (const BRenderString& key) const { return key.length() ? hashFast(key.getPtr(), key.length() * sizeof(BRenderString::charType)) : 0; } };
template <> struct BHasher<const BRenderString> { size_t operator() (const BRenderString& key) const { return key.length() ? hashFast(key.getPtr(), key.length() * sizeof(BRenderString::charType)) : 0; } };

//-----------------------------------------------------------------------------------
// Generic string hasher
//-----------------------------------------------------------------------------------
template<class StringType> struct BStringHasher 
{ 
   size_t operator() (const StringType& key) const { return key.length() ? hashFast(key.getPtr(), key.length() * sizeof(StringType::charType)) : 0; } 
};

//-----------------------------------------------------------------------------------
// struct BHashable
// Only use with C-style (POD) structs!
//-----------------------------------------------------------------------------------
template<class DerivedType> struct BHashable
{
   size_t hash(void) const 
   { 
      return hashFast(this, sizeof(DerivedType));  
   }

   operator size_t() const
   {
      return hash();
   }

   bool operator == (const DerivedType& b) const
   {
      return memcmp(
         reinterpret_cast<const uchar*>(this), 		
         reinterpret_cast<const uchar*>(&b),
         sizeof(DerivedType)) == NULL;
   }
};
