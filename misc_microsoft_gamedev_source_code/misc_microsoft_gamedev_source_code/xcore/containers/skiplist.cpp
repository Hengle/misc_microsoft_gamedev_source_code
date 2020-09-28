//==============================================================================
//
// File: skiplist.cpp
//
// Copyright (c) 2008 Ensemble Studios
//
// Paged skip list container class
//
//==============================================================================
#include "xcore.h"
#include "containers\skiplist.h"

#include "math\random.h"
#include "timer.h"

struct BTestObject
{
   DWORD mValue;
   DWORD mMagic;
   static DWORD mNum;
   
   BTestObject(DWORD value = 0) : mValue(value), mMagic(0xAABB1234)
   {
      mNum++;
   }
   
   BTestObject(const BTestObject& other)
   {
      mValue = other.mValue;
      mMagic = other.mMagic;
      mNum++;
   }
   
   ~BTestObject()
   {
      BVERIFY(mMagic == 0xAABB1234);
      mMagic = 0xFEEEFEEE;
            
      mNum--;
   }
   
   bool operator< (const BTestObject& other) const
   {
      return mValue < other.mValue;
   }
   
   bool operator== (const BTestObject& other) const
   {
      return mValue == other.mValue;
   }
};

//DEFINE_BUILT_IN_TYPE(BTestObject);
#define CHECK_NUM

DWORD BTestObject::mNum;

template<uint N>
void skipListTestFunc(uint seed)
{
   printf("Testing N: %u, Seed %u\n", N, seed);
   
   {
      typedef BSkipList<DWORD, BEmptyStruct, N> BSetType;
      BSetType set;
      set.insert(20);
      set.insert(60);
      set.insert(1);
      set.insert(99);
      set.erase(60);
      for (BSetType::const_iterator it = set.begin(); it != set.end(); ++it)
      {
         printf("%u\n", it->first);
      }
   }
   
   {
      typedef BSkipList<BString, BString, N> BMapType;
      BMapType map;
      map.insert("A", "1");
      map.insert("D", "4");
      map.insert("B", "2");
      map.insert("C", "3");
      map.erase("A");
      for (BMapType::const_iterator it = map.begin(); it != map.end(); ++it)
      {
         printf("%s %s\n", it->first.getPtr(), it->second.getPtr());
      }
   }
   
   typedef BSkipList<DWORD, BTestObject, N> BSkipListType;
   
   BSkipListType skipList;
   skipList.setMaxListLevel(14);
   
   BDynamicArray<DWORD> objects;
   
   Random rand;
   rand.setSeed(seed);
   
   uint round = 0;
   while (round < 500)
   {
      round++;
      
      if (rand.iRand(0, 2))
      {
         uint n = rand.iRand(0, 1000);
         for (uint i = 0; i < n; i++)
         {
            for ( ; ; )
            {
               //DWORD d = rand.uRand();
               DWORD d = rand.iRand(0, 100);

               BSkipListType::InsertResult res = skipList.insert(d, BTestObject(d), true);
               if (res.second)
               {
                  //bool s0 = skipList.check();
                  //BVERIFY(s0);
                  
                  objects.pushBack(d);
#ifdef CHECK_NUM                  
                  BVERIFY(BTestObject::mNum == objects.getSize());
#endif                  
                  
                  BSkipListType::const_iterator it(skipList.find(d));
                  BSkipListType::const_iterator it1(it);
                  BVERIFY(it == it1);
                  BVERIFY(it != skipList.end());
                  BVERIFY(it->first == d);
                  BVERIFY(it->second.mValue == d);
                  
                  if (it != skipList.begin())
                  {
                     BSkipListType::const_iterator prevIt(it);
                     prevIt--;
                     BVERIFY(prevIt->first < d);
                  }
                  
                  BVERIFY(skipList.getSize() == objects.getSize());
                  
                  break;
               }
            }               
         }
      }
      
      bool s0 = skipList.check();
      BVERIFY(s0);
      
      if (!skipList.getEmpty())
      {
         uint i1 = rand.iRand(0, skipList.getSize());
         uint i2 = rand.iRand(0, skipList.getSize());
         BSkipListType::const_iterator it1 = skipList.findByIndex(i1);
         BVERIFY(it1 != skipList.end());
         
         BSkipListType::iterator it2 = skipList.findByIndex(i2);
         BVERIFY(it2 != skipList.end());
      }

      uint index = 0; 
      for (BSkipListType::const_iterator it = skipList.begin(); it != skipList.end(); ++it, ++index)
      {
         BVERIFY(skipList.findByIndex(index) == it);
      }
      
      for (uint i = 0; i < 20; i++)
      {
         if (objects.isEmpty())
            break;
            
         DWORD d = objects[0];
         
         {
            BSkipListType::iterator lower = skipList.lowerBound(d);
            BSkipListType::iterator upper = skipList.upperBound(d);
            
            {
               BSkipListType::iterator test(lower);
               test - 1;
               test -= 1;
               test + 1;
               test += 1;
               
               BSkipListType::const_iterator test1(lower);
               
               test1 - 1;
               test1 -= 1;
               test1 + 1;
               test1 += 1;
            }
            
            BSkipListType::iterator lowerPrev(lower - 1);
                        
            if (lowerPrev != skipList.begin())
            {
               BVERIFY(lowerPrev->first < d);
            }
                                    
            while (lower != upper)
            {
               BVERIFY(lower->first == d);
               lower++;
            }
            
            if (upper != skipList.end())
            {
               BVERIFY(upper->first > d);
            }
         }
         
         BSkipListType::iterator it = skipList.begin();
         while (it != skipList.end())
         {
            if (it->first == d)
               break;
            ++it;
         }
         BVERIFY(it != skipList.end());
         
         for ( ; ; )
         {
            BSkipListType::iterator nextIt(it);
            ++nextIt;
            
            if (nextIt == skipList.end())
               break;
            if (nextIt->first != d)
               break;
               
            it = nextIt;
         }
         
         objects.eraseUnordered(0);
         skipList.erase(it);
      }  
      
      BVERIFY(skipList.check());       
      
      for (uint i = 0; i < objects.getSize(); i++)
      {
         DWORD d = objects[i];
         BSkipListType::iterator it(skipList.find(d));
         BSkipListType::iterator it1(it);
         BVERIFY(it == it1);
         
         BVERIFY(it != skipList.end());
         BVERIFY(it->first == d);
         BVERIFY(it->second.mValue == d);
         
         if (rand.iRand(0, 20) == 0)
         {
            BVERIFY(skipList.erase(it));
            objects.eraseUnordered(i);
         }
      }

#ifdef CHECK_NUM                        
      BVERIFY(BTestObject::mNum == objects.getSize());
#endif      
      
      if (rand.iRand(0, 2))
      {
         uint n = rand.iRand(0, 1000);
         for (uint i = 0; i < n; i++)
         {  
            if (!objects.getSize())
               break;
            DWORD index = rand.iRand(0, objects.getSize());
            DWORD d = objects[index];
            
            bool success = skipList.erase(d);
            BVERIFY(success);
            
            //bool s0 = skipList.check();
            //BVERIFY(s0);
            
            objects.eraseUnordered(index);
            BVERIFY(skipList.getSize() == objects.getSize());

#ifdef CHECK_NUM                              
            BVERIFY(BTestObject::mNum == objects.getSize());
#endif            
         }
      }
      
      bool s1 = skipList.check();
      BVERIFY(s1);
      
      {
         BSkipListType clone(skipList);
         BVERIFY(clone.check());
         
         clone.clear();
         
         clone = skipList;
         
         BVERIFY(clone.check());
         
         BVERIFY(clone == skipList);
         BVERIFY(!(clone < skipList));
         BVERIFY(!(skipList < clone));
      }

#ifdef CHECK_NUM                        
      BVERIFY(BTestObject::mNum == objects.getSize());
#endif      
      
      if (rand.iRand(0, 10) == 0)
      {
         BSkipListType other(skipList);
         skipList.swap(other);
      }
                  
      printf("N: %u Entries: %u Nodes: %u\n", N, objects.getSize(), skipList.getNumNodes());
   }
}

void skipListTest()
{  
   uint seed = 1;
   for ( ; ; )
   {
      skipListTestFunc<2>(seed+1);
      skipListTestFunc<1>(seed);
      
      skipListTestFunc<3>(seed+2);
      skipListTestFunc<4>(seed+3);
      skipListTestFunc<8>(seed+4);
      skipListTestFunc<16>(seed+5);
      skipListTestFunc<256>(seed+6);
      seed += 10;
   }
}

#ifndef XBOX
#include <set>
void skipListPerfTest()
{
   Random rand;
   const uint N = 2000;
   BDynamicArray<DWORD> values(N);
   for (uint i = 0; i < N; i++)
      values[i] = rand.uRand();

   BTimer skipListTimer;
   skipListTimer.start();
   {
      typedef BSkipList<DWORD, BEmptyStruct, 16> BSkipListType;   
      BSkipListType skipList(17);
      for (uint i = 0; i < N; i++)
         skipList.insert(values[i], BEmptyStruct(), true);

      for (uint i = 0; i < N; i++)
      {
         BSkipListType::const_iterator it(skipList.find(values[i]));
         BVERIFY(it != skipList.end());
      }
      
      //skipList.check();

      for (uint i = 0; i < N; i++)
      {
         bool success = skipList.erase(values[i]);
         BVERIFY(success);
      }
   }      

   double skipListTime = skipListTimer.getElapsedSeconds();

   printf("Skip List Time: %f\n", skipListTime);

   BTimer mapTimer;
   mapTimer.start();
   {
      typedef std::multiset<DWORD> BSetType;
      BSetType set;

      for (uint i = 0; i < N; i++)
         set.insert(values[i]);

      for (uint i = 0; i < N; i++)
      {
         BSetType::const_iterator it(set.find(values[i]));
         BVERIFY(it != set.end());
      }

      for (uint i = 0; i < N; i++)
      {
         set.erase(values[i]);
      }  
   }

   double mapTime = mapTimer.getElapsedSeconds();
   printf("Multiset Time: %f\n", mapTime);
}
#endif
