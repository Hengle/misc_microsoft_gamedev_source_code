//==============================================================================
// UTManager
//==============================================================================

#pragma once


template <class T>
class UTSimpleAllocator
{
};


template <class T, class A=UTSimpleAllocator<T>>     
class UTManager
{
public:
   UTManager() : mbInitialized(false) {};
   ~UTManager() {};

   inline bool init( void )
   {
      mpAllocator = new A();

      if (!mpAllocator)
         return (false);

      mbInitialized = true;
   }


protected:
   A*          mpAllocator;
   bool        mbInitialized;    
};