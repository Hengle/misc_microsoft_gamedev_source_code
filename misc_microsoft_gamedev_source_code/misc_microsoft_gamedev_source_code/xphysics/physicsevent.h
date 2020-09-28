//==============================================================================
// PhysicsEvent.h
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#ifndef _PhysicsEvent_H_
#define _PhysicsEvent_H_


// Base PhysicsEvent index values for various systems
enum
{
   cPhysicsEventBaseAnim=0,
   cPhysicsEventBaseSim=1000,
   cPhysicsEventBaseMP=2000,
   cPhysicsEventBasePhysics=3000,
};

//==============================================================================
// class BPhysicsEvent (Struct)
//==============================================================================
class BPhysicsEvent
{
   public:

      BPhysicsEvent(void) : mType(0), mData1(0), mData2(0), mData3(0)  {}
      ~BPhysicsEvent()                               {}

      long  mType;

      union
      {      
         long  mData1;
         float mFloatData1;
         void* mPointer1;
      };

      union
      {
         long  mData2;
         float mFloatData2;
         void* mPointer2;
      };

      union
      {
         long  mData3;
         float mFloatData3;
         //void* mSender;
         void* mPointer3;
      };

};

//==============================================================================
// class IPhysicsEventObserver (pure abstract interface)
//==============================================================================
class IPhysicsEventObserver
{
   public:
      virtual bool notification( const BPhysicsEvent &PhysicsEvent ) = 0;
};

//==============================================================================
// class IPhysicsEventSubject (pure abstract interface)
//==============================================================================
class IPhysicsEventSubject
{
   public:
      virtual bool  attach(IPhysicsEventObserver& observer) = 0;
      virtual bool  detach(IPhysicsEventObserver& observer) = 0;
      virtual void  notify( const BPhysicsEvent &PhysicsEvent ) const = 0;
};

//==============================================================================
// class BPhysicsEventSubject
// basic implementation of a subject
//==============================================================================
class BPhysicsEventSubject : public IPhysicsEventSubject
{
   public:

      BPhysicsEventSubject();
      virtual ~BPhysicsEventSubject();

      virtual bool  attach(IPhysicsEventObserver& observer);
      virtual bool  detach(IPhysicsEventObserver& observer);
      virtual void  notify(const BPhysicsEvent &PhysicsEvent ) const;

   protected:
      BPointerList<IPhysicsEventObserver> mObservers;
};


//==============================================================================
// Implementation Macros for PhysicsEvent Subjects
//==============================================================================
#define DECLARE_SUBJECT_INTERFACE \
   protected: \
      BPhysicsEventSubject mSubjectImpl; \
   public: \
      virtual bool  attach(IPhysicsEventObserver& observer) { return mSubjectImpl.attach(observer); } \
      virtual bool  detach(IPhysicsEventObserver& observer) { return mSubjectImpl.detach(observer); } \
      virtual void  notify(const BPhysicsEvent &PhysicsEvent ) const {mSubjectImpl.notify(PhysicsEvent); } 


#endif