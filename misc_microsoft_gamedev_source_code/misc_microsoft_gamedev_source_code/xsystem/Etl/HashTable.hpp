//==============================================================================
// HashTable.hpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

#ifndef __HashTable__
#define __HashTable__

#include <Etl\Support.hpp>
#include <Etl\Hash.hpp>
#include <Etl\Pair.hpp>
#include <Etl\Array.hpp>

#pragma warning(disable : 4786)

namespace Etl
{
template <class Key, class T, class HashFcn, class EqualKey> class HashTableIterator;

template <class Key, class T, class HashFcn = Hash<Key>, class EqualKey = EqualKeyDefault<Key> >
class HashTable
{
public:

   HashTable();
   HashTable(long lDefaultTableSize);
   ~HashTable();
   void insert(const Key& key, const T t);
   void erase(const Key& key);
   T& operator [] (const Key& key);
   unsigned long count(void) const;

   typedef HashTableIterator<Key, T, HashFcn, EqualKey> Iterator;

   inline Iterator begin(void)
      { return Iterator(&m_Buckets, m_pBegin, m_lFirstFilledBucket, Iterator::ItBegin); }
   inline Iterator end(void)
      { return Iterator(&m_Buckets, m_pEnd, m_lLastFilledBucket, Iterator::ItEnd); }

private:

   enum { TableGrowthSize = 1024 };

   long generateHashKey(const Key& key) const;

   long m_lFirstFilledBucket;
   long m_lLastFilledBucket;
   long m_lTableSize;
   //long m_lCollisionMaxCount;
   //long m_lCurrentCollisionMaxCount;
   unsigned long m_ulCount;

   friend class HashTableIterator<Key, T, HashFcn, EqualKey>;
   struct Bucket : public Pair<Key, T>
   {
      Bucket() : pNext(0){}
      Bucket(const Key& k, const T& d) : Pair<Key,T>(k, d), pNext(0){}
      Bucket *pNext;
   };

   Array<Bucket*> m_Buckets;
   Bucket* m_pBegin;
   Bucket* m_pEnd;

};

template <class Key, class T, class HashFcn, class EqualKey>
HashTable<Key, T, HashFcn, EqualKey>::HashTable(long lDefaultTableSize) :
   m_lFirstFilledBucket(0x7fffffff),
   m_lLastFilledBucket(0),
   m_lTableSize(lDefaultTableSize),
   m_ulCount(0),
   m_Buckets(lDefaultTableSize)
{
   m_Buckets.setAll(0);
}

template <class Key, class T, class HashFcn, class EqualKey>
HashTable<Key, T, HashFcn, EqualKey>::HashTable() :
   m_lFirstFilledBucket(0x7fffffff),
   m_lLastFilledBucket(0),
   m_lTableSize(TableGrowthSize),
   m_ulCount(0),
   m_Buckets(TableGrowthSize)
{
   m_Buckets.setAll(0);
}

template <class Key, class T, class HashFcn, class EqualKey>
void HashTable<Key, T, HashFcn, EqualKey>::erase(const Key& k)
{
   long lBucket = generateHashKey(k);
   EqualKey ekEqualKey;

   Bucket* pBucket = m_Buckets.get(lBucket);

   if (ekEqualKey(pBucket->first, k))
   {
      m_ulCount--;
      m_Buckets.set(lBucket, pBucket->pNext);
      if (lBucket == m_lLastFilledBucket)
      {
         for (long lIndex = lBucket; lIndex >= m_lFirstFilledBucket; lIndex--)
         {
            if (m_Buckets.get(lIndex))
            {
               Bucket* pLast = m_Buckets.get(lIndex);
               //Bucket* pPrev = m_Buckets.get(lIndex);
               while (pLast->pNext)
                  pLast = pLast->pNext;

               m_pEnd = pLast;
            }
         }
         m_lLastFilledBucket = 0;
         m_lFirstFilledBucket = 0x7fffffff;
         m_pBegin = 0;
         m_pEnd = 0;
      }
      else if (lBucket == m_lFirstFilledBucket)
      {
         for (long lIndex = lBucket; lIndex < m_lLastFilledBucket; lIndex++)
         {
            if (m_Buckets.get(lIndex))
            {
               m_pBegin = m_Buckets.get(lIndex);
               break;
            }
         }
         m_lFirstFilledBucket = lIndex;
         m_pBegin = m_Buckets.get(m_lFirstFilledBucket);
      }
   }
   else
   {
      Bucket* pPrev = pBucket;
      while (pBucket)
      {
         if (ekEqualKey(pBucket->first, k))
         {
            m_ulCount--;
            pPrev->pNext = pBucket->pNext;
            if (lBucket >= m_lLastFilledBucket)
            {
               m_pEnd = pPrev;
            }
            break;
         }
         pBucket = pBucket->pNext;
         pPrev = pBucket;
      }
   }

   delete pBucket;
}

template <class Key, class T, class HashFcn, class EqualKey>
HashTable<Key, T, HashFcn, EqualKey>::~HashTable()
{
   for (long lIndex = 0; lIndex < m_Buckets.length(); lIndex++)
   {
      Bucket* pRoot = m_Buckets.get(lIndex);

      while (pRoot)
      {
         Bucket* pCurrent = pRoot;
         pRoot = pRoot->pNext;
         delete pCurrent;
      }
      m_Buckets.set(lIndex, 0);
   }
}

template <class Key, class T, class HashFcn, class EqualKey>
inline unsigned long HashTable<Key, T, HashFcn, EqualKey>::count(void) const
{
   return m_ulCount;
}

template <class Key, class T, class HashFcn, class EqualKey>
T& HashTable<Key, T, HashFcn, EqualKey>::operator [] (const Key& k)
{
   long lBucket = generateHashKey(k);
   EqualKey ekEqualKey;

   m_lFirstFilledBucket = Min<long>(m_lFirstFilledBucket, lBucket);
   m_lLastFilledBucket = Max<long>(m_lLastFilledBucket, lBucket);

   if (!m_Buckets.get(lBucket))
   {
      Bucket *pNewBucket = new Bucket;
      pNewBucket->first = k;
      m_Buckets.set(lBucket, pNewBucket);

      if (m_lFirstFilledBucket == lBucket)
         m_pBegin = pNewBucket;
      if (m_lLastFilledBucket == lBucket)
         m_pEnd = pNewBucket;

      m_ulCount++;
      return pNewBucket->second;
   }

   Bucket* pBucket = m_Buckets.get(lBucket);
   Bucket* pPrevBucket = 0;

   while (pBucket)
   {
      if (ekEqualKey(pBucket->first, k))
         return pBucket->second;

      pPrevBucket = pBucket;
      pBucket = pBucket->pNext;
   }

   pPrevBucket->pNext = new Bucket;
   pPrevBucket->pNext->first = k;
   if (m_lLastFilledBucket == lBucket)
      m_pEnd = pPrevBucket->pNext;

   return pPrevBucket->pNext->second;
}

template <class Key, class T, class HashFcn, class EqualKey>
void HashTable<Key, T, HashFcn, EqualKey>::insert(const Key& k, const T t)
{
   EqualKey ekEqualKey;
   long lBucket = generateHashKey(k);

   m_lFirstFilledBucket = Min<long>(m_lFirstFilledBucket, lBucket);
   m_lLastFilledBucket = Max<long>(m_lLastFilledBucket, lBucket);

   if (!m_Buckets.get(lBucket))
   {
      m_Buckets.set(lBucket, new Bucket(k, t));
      if (m_lFirstFilledBucket == lBucket)
         m_pBegin = m_Buckets.get(lBucket);
      if (m_lLastFilledBucket == lBucket)
         m_pEnd = m_Buckets.get(lBucket);
   }
   else
   {
      Bucket* pBucket = m_Buckets.get(lBucket);
      Bucket* pPrevBucket = 0;

      while (pBucket)
      {
         if (ekEqualKey(pBucket->first, k))
         {
            pBucket->second = t;
            return;
         }
         pPrevBucket = pBucket;
         pBucket = pBucket->pNext;
      }
      pPrevBucket->pNext = new Bucket(k, t);
      if (m_lLastFilledBucket == lBucket)
         m_pEnd = pPrevBucket->pNext;
   }
   m_ulCount++;
}

template <class Key, class T, class HashFcn, class EqualKey>
inline long HashTable<Key, T, HashFcn, EqualKey>::generateHashKey(const Key& key) const
{
   HashFcn h;
   return h(key) % m_lTableSize;
}


template <class Key, class T, class HashFcn, class EqualKey>
class HashTableIterator
{
public:

   HashTableIterator();

   HashTableIterator operator = (const HashTableIterator&);
   void operator ++ (void);
   void operator ++ (int);
   bool operator == (const HashTableIterator&) const;
   bool operator != (const HashTableIterator&) const;
   inline Pair<Key,T>* operator -> (void);

   inline operator * () const;

   enum { ItBegin = -1, ItNode = 0, ItEnd = 1 };

private:
   friend class HashTable<Key, T, HashFcn, EqualKey>;

   typedef HashTable<Key, T, HashFcn, EqualKey>::Bucket HashTableBucket;

   HashTableIterator(Array<HashTableBucket*>* pBuckets, HashTableBucket* pBucket, long lBucket, char chBounce);

   char m_chBounce;
   long m_lBucket;
   HashTableBucket* m_pBucket;
   Array<HashTableBucket*>* m_pBuckets;
};

template <class Key, class T, class HashFcn, class EqualKey>
HashTableIterator<Key, T, HashFcn, EqualKey>::HashTableIterator() :
   m_pBuckets(0),
   m_pBucket(0),
   m_chBounce(0),
   m_lBucket(-1)
{
}

template <class Key, class T, class HashFcn, class EqualKey>
HashTableIterator<Key, T, HashFcn, EqualKey>::HashTableIterator(
   Array<HashTableBucket*>* pBuckets, HashTableBucket* pBucket, long lBucket, char chBounce) :
   m_pBuckets(pBuckets),
   m_pBucket(pBucket),
   m_chBounce(chBounce),
   m_lBucket(lBucket)
{
}

template <class Key, class T, class HashFcn, class EqualKey>
bool HashTableIterator<Key, T, HashFcn, EqualKey>::operator !=
   (const HashTableIterator<Key, T, HashFcn, EqualKey>& it) const
{
   if (m_pBucket != it.m_pBucket || m_chBounce != it.m_chBounce)
      return true;

   return false;
}

template <class Key, class T, class HashFcn, class EqualKey>
HashTableIterator<Key, T, HashFcn, EqualKey>::operator * () const
{
   return reinterpret_cast<Pair<Key,T>*>*m_pBucket;
}

template <class Key, class T, class HashFcn, class EqualKey>
Pair<Key,T>* HashTableIterator<Key, T, HashFcn, EqualKey>::operator -> (void)
{
   return m_pBucket;
}

template <class Key, class T, class HashFcn, class EqualKey>
HashTableIterator<Key, T, HashFcn, EqualKey> HashTableIterator<Key, T, HashFcn, EqualKey>::operator =
  (const HashTableIterator& it)
{
   m_pBuckets = it.m_pBuckets;
   m_pBucket = it.m_pBucket;
   m_chBounce = it.m_chBounce;
   m_lBucket = it.m_lBucket;

   return *this;
}

template <class Key, class T, class HashFcn, class EqualKey>
void HashTableIterator<Key, T, HashFcn, EqualKey>::operator ++ (void)
{
   if (m_pBucket->pNext)
   {
      m_pBucket = m_pBucket->pNext;
      m_chBounce = ItNode;
   }
   else
   {
      long lLastKnownFilledBucket = m_lBucket;
      while (++m_lBucket < m_pBuckets->length())
      {
         m_chBounce = ItNode;
         if (m_pBuckets->get(m_lBucket))
         {
            m_pBucket = m_pBuckets->get(m_lBucket);
            return;
         }
      }
      m_lBucket = lLastKnownFilledBucket;
      m_chBounce = ItEnd;
   }
}

template <class Key, class T, class HashFcn, class EqualKey>
void HashTableIterator<Key, T, HashFcn, EqualKey>::operator ++ (int)
{
   if (m_pBucket->pNext)
   {
      m_pBucket = m_pBucket->pNext;
      m_chBounce = ItNode;
   }
   else
   {
      long lLastKnownFilledBucket = m_lBucket;
      while (++m_lBucket < m_pBuckets->length())
      {
         m_chBounce = ItNode;
         if (m_pBuckets->get(m_lBucket))
         {
            m_pBucket = m_pBuckets->get(m_lBucket);
            return;
         }
      }
      m_lBucket = lLastKnownFilledBucket;
      m_chBounce = ItEnd;
   }
}

} // namespace

#endif
