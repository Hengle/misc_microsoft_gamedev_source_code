//==============================================================================
// utparray.h
//
// Copyright (c) 1998, 1999 Ensemble Studios
//==============================================================================

#ifndef _UTPARRAY_
#define _UTPARRAY_

//==============================================================================
// CLASS definition for the BPointerArray template.
//
// NOTE: Any templated classes must have a default constructor (one that
//       takes no arguments), a default copy constructor, an assignment operator,
//       and an equality operator.
//==============================================================================
template <class Type> class BPointerArray
{
   public:
      BPointerArray( int initialSize = 128, float rF = 2.0f, bool dE = TRUE) :
         mValue(NULL),
         mNumber(0),
         mMaximumSize(0),
         mCompact(FALSE),
         mResizeFactor(rF),
         mDeleteElements(dE)
      {
         //If we have an initial size, allocate the array.
         if (initialSize > 0)
            resize(initialSize);
         if (mResizeFactor < 1.0f)
         {
            BASSERT(0);
         }
      }

// -e1927
      BPointerArray( const BPointerArray &pa) { *this = pa; }
// +e1927

      ~BPointerArray( void )
      {
         //Delete the value array.
         if (mValue != NULL) 
         {
            //Only delete the elements in the array if we're supposed to.
            if (mDeleteElements == TRUE)
            {
               for (int i=0; i < mMaximumSize; i++)
                  if (mValue[i] != NULL)
                     delete mValue[i];
               delete mValue;
            }
            else
               delete mValue;
            mValue=NULL;
         }
         mNumber=0;
         mMaximumSize=0;
      }

      // Don't copy me.
      //lint -e715 -e1539 -e1529
      BPointerArray& operator=(const BPointerArray &v) { v; BASSERT(0); return *this; }
      //lint +e715 +e1539 +e1529

      Type& operator[]( int v )
      {
         // Sanity.
         if(v<0)
         {
            BFAIL("Negative index, not able to give a valid item back.");
            
            // Force to 0 to get something like the normal behavior.
            v = 0;
         }
         
         if (v > (mMaximumSize-1))
         {
            int newSize=(int)((float)v*mResizeFactor)+1;
            resize(newSize);
         }

         return(mValue[v]);
      }

      Type& getAt( int v )
      {
      #ifdef _DEBUG
         if (v > (mMaximumSize-1))
         {
            OutputDebugString(B("Array access out of range"));
            BASSERT(0);
            return mValue[0];
         }
      #endif // _DEBUG

         return mValue[v];

      } // getAt

      bool contains(Type r)
      {
         //Returns TRUE if the argument value is in the array, FALSE if not.
         if (!mValue)
            return false;

         for (int i=0; (i < mNumber) && (i < mMaximumSize); i++)
            if (*(mValue[i]) == *r)
               return true;

         return false;
      }

      int position(const Type* r)
      {
         if (mValue == NULL)
            return(-1);
         //Returns the position of the argument value if it's in the array, -1 if it's not.
         for (int i=0; i < mMaximumSize; i++)
            if (*(mValue[i]) == *r)
               return(i);
         return(-1);
      }

      int add(Type r)
      {
         //If we already have this one in the array, return -1.
         if (contains(r) == TRUE)
            return(-1);

         //Adds the argument value as the next value in the array (based off of
         //the mNumber) and returns that index.
         if (mNumber > (mMaximumSize-1))
         {
            int newSize=(int)((float)mMaximumSize*mResizeFactor)+1;
            resize(newSize);
         }
         mValue[mNumber]=r;
         mNumber++;
         return(mNumber-1);
      }

      int append(Type r)
      {
         //Adds the argument value as the next value in the array (based off of
         //the mNumber) and returns that index.
         if (mNumber > (mMaximumSize-1))
         {
            int newSize=(int)((float)mMaximumSize*mResizeFactor)+1;
            resize(newSize);
         }
         mValue[mNumber]=r;
         mNumber++;
         return(mNumber-1);
      }

      int unconditionalAdd(const Type r)
      {
         //Adds the argument value as the next value in the array (based off of
         //the mNumber) and returns that index.
         if (mNumber > (mMaximumSize-1))
         {
            int newSize=(int)((float)mMaximumSize*mResizeFactor)+1;
            resize(newSize);
         }
         mValue[mNumber]=r;
         mNumber++;
         return(mNumber-1);
      }

      bool remove(Type r)
      {
         //NOTE: This doesn't do any deletion of the stuff in the pointer.

         //Removes the first instance of the argument value from the
         //array and shifts the remaining values up to fill the gap.
         for (int i=0; (i < mMaximumSize) && (mValue[i] != r); i++)
            ;
         if (i >= mMaximumSize)
            return(FALSE);

         //Compact the array if we're supposed to.
         if (mCompact == TRUE)
         {
            for (; (i < (mMaximumSize-1)); i++)
               mValue[i]=mValue[i+1];
            mNumber--;
            mValue[mMaximumSize-1]=NULL;
            if (mNumber < 0)
               mNumber=0;
         }
         else
            mValue[i]=NULL;

         //Return TRUE to indicate success.
         return(TRUE);
      }

      bool removeIndex(int v)
      {
         //Does a simple overwrite (with the zero value) of the given index.
         if (v < mMaximumSize)
         {
            mValue[v]=NULL;
            return(TRUE);
         }
         return(FALSE);
      }

      bool resize(int s)
      {
         if(s <= mMaximumSize)
            return true;

         //Allocate the new array.  Bomb out if we cannot.

         Type* temp=new Type[s];
         if (temp == NULL)
         {
            BASSERT(0);
            return false;
         }

         //If we have an old value, copy over as much of it as will fit.
         long i=0;
         if (mValue)
         {
            for (; (i < mMaximumSize) && (i < s); i++)
            {
               temp[i]=mValue[i];
               mValue[i] = 0;
            }
            delete [] mValue;
         }

         //If we still have room left in the array, zero out the remaining fields.
         for (; i < s; i++)
            temp[i]=0;

         //Save the new array and counts.
         mValue=temp;
         mMaximumSize=s;
         if (mNumber > mMaximumSize)
            mNumber=mMaximumSize;

         //Return TRUE to indicate success.
         return true;
      }

      const int      getNumber(void) const { return(mNumber); }
      const void     incrementNumber(void) { mNumber++; }
      const void     decrementNumber(void) { mNumber--; }
      void           setNumber(int n) { mNumber = n; }
      const int      getCompact(void) const { return(mCompact); }
      void           setCompact(bool v) { mCompact=v; }
      void           zeroNumber(void) { mNumber=0; }
      const int      maximumSize(void) const { return(mMaximumSize); }
      Type           *getValue(void) const {return(mValue);}

   protected:
      Type*                         mValue;
      int                           mNumber;
      int                           mMaximumSize;
      bool                          mCompact;
      float                         mResizeFactor;
      bool                          mDeleteElements;
};


#endif
