

#ifndef __Sequence__
#define __Sequence__

template <class T>
class Sequence
{
public:
   inline virtual T* getFirst(void) = 0;
   inline virtual T* getLast(void)  = 0;
   inline virtual T* getNext(void)  = 0;
   inline virtual T* getPrev(void)  = 0;
};



#endif
