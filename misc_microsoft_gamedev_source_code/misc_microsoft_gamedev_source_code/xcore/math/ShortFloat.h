//==============================================================================
// File: ShortFloat.h
//
// Copyright (c) 2002 Ensemble Studios
//==============================================================================
#ifndef __SHORTFLOAT_H__
#define __SHORTFLOAT_H__

//==============================================================================
// 
//==============================================================================
float shortFloatToFloat(WORD sf);

//==============================================================================
// BShortFloat
//==============================================================================
class BShortFloat
{
   public:
      BShortFloat(void) {}

      explicit BShortFloat(float f);

      float asFloat(void) const;

      BShortFloat &operator=(BShortFloat sf);

      BShortFloat &operator=(float f);

      BShortFloat operator*(float a) const;
      BShortFloat operator*(BShortFloat a) const;

      BShortFloat operator/(float a) const;
      BShortFloat operator/(BShortFloat a) const;

      BShortFloat operator-(float a) const;
      BShortFloat operator-(BShortFloat a) const;

      BShortFloat operator+(float a) const;
      BShortFloat operator+(BShortFloat a) const;


      BShortFloat &operator*=(float a);
      BShortFloat &operator*=(BShortFloat a);

      BShortFloat &operator/=(float a);
      BShortFloat &operator/=(BShortFloat a);

      BShortFloat &operator-=(float a);
      BShortFloat &operator-=(BShortFloat a);

      BShortFloat &operator+=(float a);
      BShortFloat &operator+=(BShortFloat a);

      bool operator==(const BShortFloat &a);
      bool operator==(float a);

      bool operator>=(const BShortFloat &a);
      bool operator>=(float a);

      bool operator>(const BShortFloat &a);
      bool operator>(float a);

      bool operator<=(const BShortFloat &a);
      bool operator<=(float a);

      bool operator<(const BShortFloat &a);
      bool operator<(float a);

      WORD mValue;
   
}; // BShortFloat

#endif