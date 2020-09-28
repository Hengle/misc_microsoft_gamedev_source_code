//============================================================================
// UIImageViewerControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"

class BUIImageViewerControl: public BUIControl
{
public:
   enum Events
   {
      eImageChanged = UIImageViewerControlID,
   };
   BEGIN_EVENT_MAP( UIImageViewerControl )
      MAP_CONTROL_EVENT( ImageChanged )
   END_EVENT_MAP()

   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIImageViewerControl );

public:
   BUIImageViewerControl( void );
   virtual ~BUIImageViewerControl( void );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   virtual void setAutoTransition(bool bAutoTransition);
   virtual void showNextPicture();
   virtual void addImage(const char* url);
   virtual void setImageSize(int width, int height);
   virtual void reset();
   virtual void start();
   virtual void clearImages();
   virtual void setViewDuration(float duration);

protected:
   float mViewDuration;
   int   mImageWidth;
   int   mImageHeight;
   bool  mbAutoTransition;
};