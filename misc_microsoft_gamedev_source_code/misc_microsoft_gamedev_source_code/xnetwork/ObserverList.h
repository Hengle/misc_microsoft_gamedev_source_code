//==============================================================================
// BObserverList.h
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

#ifndef _ObserverList_H_
#define _ObserverList_H_

#if 1

//==============================================================================
// This is a helper class for handling collections of observer / event interfaces.
// To use this, derive a class from this class.  Specify the name of the event
// interface as the template argument.  Then, use the DECLARE_OBSERVER_METHOD
// macro to declare all of the methods in your observer interface.  This macro
// will generate a method in your class, that will call the method with the same
// name on each of the observer interfaces.  You also have to specify the argument
// list, both with and without types.  For example:
//
//		interface IFooObserver
//		{
//			void SpiffyEventHappened (
//				int Something,
//				int Else) = 0;
//		};
//
//		class IFooObserverList :
//			public ObserverList <IFooObserver>
//		{
//			DECLARE_OBSERVER_METHOD (SpiffyEventHappened,
//				(int Something, int Else),
//				(Something, Else))
//		}
//
// To use the observer list:
//
//		IFooObserver m_ObserverList;
//
//		// ... add some observers to the list ...
//
//		m_ObserverList.SpiffyEventHappened (42, 69);
//
//

template <class ObserverClass>
class BObserverList
{
	public:

      BObserverList() : 
         mListLockCount(0)
      {
      }

      virtual ~BObserverList()
      {
         if (mListLockCount != 0)
            BFATAL_FAIL("Attempting to delete an observer from within a notification.");
      }


	protected:

		struct ObserverEntry
		{
         ObserverEntry() : Key(-1), Observer(0), Removed(false)  {}

			LONG Key;
			ObserverClass* Observer;
         bool Removed;

         bool operator==( const ObserverEntry& rhs )  { return Observer == rhs.Observer; }
		};

	public:

      void Add (		
			IN ObserverClass * Observer,
         IN BOOL AllowDuplicates = FALSE)
		{
         Add(0, Observer, AllowDuplicates);
      }

		void Add (
			IN LONG Key,
			IN ObserverClass * Observer,
         IN BOOL AllowDuplicates = FALSE)
		{
         if (Observer == NULL)
         {
            BASSERT(0);
            return;
         }

         //
         // Search for duplicates, by observer pointer.
         //

         if (!AllowDuplicates)
         {
            long count = m_List.getNumber();
            for (long idx=0; idx<count; idx++)
            {
               if (m_List[idx].Observer == Observer)
               {
                  return;
               }
            }
         }

         long count = m_List.getNumber();
         m_List.setNumber(count+1);

         m_List[count].Key = Key;
         m_List[count].Observer = Observer;
         m_List[count].Removed = false;
		}

		//
		// This removes an observer instance, using the key supplied to the Add call.
		//

		void Remove (
			IN LONG Key)
		{
         long count = m_List.getNumber();
         for (long idx=0; idx<count; idx++)
         {
            if ( m_List[idx].Key == Key )
            {
               if ( mListLockCount > 0 )
                  m_List[idx].Removed = true;
               else
                  m_List.removeIndex(idx);
               break;
            }
         }
		}

		//
		// This removes an observer instance, using just the pointer to the observer
		// interface as the search key.
		//

		void Remove (
			IN ObserverClass * Observer)
		{
         if (Observer == NULL)
         {
            BASSERT(0);
            return;
         }

         long count = m_List.getNumber();
         for (long idx=0; idx<count; idx++)
         {
            if ( m_List[idx].Observer == Observer )
            {
               if ( mListLockCount > 0 )
                  m_List[idx].Removed = true;
               else
                  m_List.removeIndex(idx);

               break;
            }
         }
		}


	protected:

      // Protection against entry removal during list iteration.
      //
      long mListLockCount;

      void lockList(bool lock)
      {
         if ( lock )
            ++mListLockCount;
         else
         {
            --mListLockCount;
            BASSERT(mListLockCount >= 0);
         }
         
         // If we are unlocking the list
         // see if any entries were removed while we were locked.
         // If any, apply the removals now.
         //
         if (mListLockCount == 0)   
         {
            long count = m_List.getNumber();
            for (long idx=count-1; idx>=0; idx--)
            {
               if ( m_List[idx].Removed )
                  m_List.removeIndex(idx);
            }
         }
      }

      BDynamicSimArray<ObserverEntry> m_List;
};
// BObserverList

//==============================================================================
//
#define DECLARE_OBSERVER_METHOD(MethodName, ArgumentTypedList, ArgumentNameList) \
	public: void MethodName ArgumentTypedList { \
      lockList(true); \
      for (long idx=0; idx<m_List.getNumber(); idx++) \
		{ \
         if(m_List[idx].Observer) \
         {\
            if (m_List[idx].Removed == false)\
               (m_List[idx].Observer) -> MethodName ArgumentNameList;\
         }\
         else\
         {\
            BASSERT(0);\
         }\
      } \
      lockList(false); \
	}
#endif 

#if 0 //#else // #ifndef _BANG

#include "CopyList.h"

template <class ObserverClass>
class BObserverList
{
	public:


	protected:

		class ObserverEntry
		{
         public:
            bool operator==(const ObserverEntry &other)
            {
               if ((other.Observer == Observer) && (other.Key == Key))
                  return true;
               else
                  return false;
            }
         
			   LONG				Key;
			   ObserverClass *		Observer;
		};


	public:

      void Add (		
			IN ObserverClass * Observer,
         IN BOOL AllowDuplicates = FALSE)
		{
         Add(0, Observer, AllowDuplicates);
      }

		void Add (
			IN LONG Key,
			IN ObserverClass * Observer,
         IN BOOL AllowDuplicates = FALSE)
		{
         if (Observer == NULL)
         {
            BASSERT(0);
            return;
         }
			ObserverEntry Entry;

			Entry.Key = Key;
			Entry.Observer = Observer;

         //
         // Search for duplicates, by observer pointer.
         //

         if (!AllowDuplicates)
         {
            BHandle i;
            ObserverEntry *p = m_List.getHead(i);
            while (p)
            {
               if (p->Observer == Observer)
               {
                  return;
               }

               p = m_List.getNext(i);
            }
         }

			m_List.addToTail (Entry);
		}

		//
		// This removes an observer instance, using the key supplied to the Add call.
		//

		void Remove (
			IN LONG Key)
		{
			BHandle i;
         ObserverEntry *p = m_List.getHead(i);
         while (p)
         {
            if (p->Key == Key)            
            {
               if (i == mCurrent)               
                  m_List.getNext(mCurrent);
               
               p = m_List.removeAndGetNext(i);
            }
            else
               p = m_List.getNext(i);
         }              
		}

		//
		// This removes an observer instance, using just the pointer to the observer
		// interface as the search key.
		//

		void Remove (
			IN ObserverClass * Observer)
		{
         if (Observer == NULL)
         {
            BASSERT(0);
            return;
         }

			BHandle i;
         ObserverEntry *p = m_List.getHead(i);
         while (p)
         {
            if (p->Observer == Observer)   
            {
               if (i == mCurrent)               
                  m_List.getNext(mCurrent);
                           
               p = m_List.removeAndGetNext(i);
            }
            else
               p = m_List.getNext(i);
         }              
		}


	protected:

      BCopyList<ObserverEntry> m_List;      
      BHandle                  mCurrent;
};
// BObserverList

#define DECLARE_OBSERVER_METHOD(MethodName, ArgumentTypedList, ArgumentNameList) \
	public: void MethodName ArgumentTypedList { \
      ObserverEntry *p = m_List.getHead(mCurrent); \
      while (p) \
      { \
         BHandle temp = mCurrent; \
         if(p->Observer)\
         {\
            p->Observer-> MethodName ArgumentNameList; \
         }\
         else\
         {\
            BASSERT(0);\
         }\
         if (temp == mCurrent) \
            p = m_List.getNext(mCurrent); \
         else if (mCurrent) \
            p = m_List.getItem(mCurrent); \
         else \
            p = 0; \
      } \
      mCurrent = NULL; \
	}


#endif

//==============================================================================
//
#endif // _ObserverList_H_

//==============================================================================
// eof: ObserverList.h
