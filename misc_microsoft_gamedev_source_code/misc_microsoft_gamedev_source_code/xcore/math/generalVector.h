//============================================================================
//
// File: generalVector.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

template<int size, typename scalarType = float>
struct BVecN : Utils::RelativeOperators< BVecN<size, scalarType> > 
{
   enum { numElements = size };
   enum { requiresAlignment = false };
         
   typedef BVecN<size, scalarType> VecType;
   typedef scalarType ScalarType;
               
   scalarType element[numElements];
   
   BVecN() { }
   
   explicit BVecN(scalarType s) 
   {
      set(s);
   }
   
#if 0   
   explicit BVecN(int s) 
   {
      set(s);
   }
#endif   
   
   explicit BVecN(const scalarType* pData)
   {
      setFromPtr(pData);
   }
               
   BVecN(scalarType sx, scalarType sy)
   {
      set(sx, sy);
   }
   
   BVecN(scalarType sx, scalarType sy, scalarType sz)
   {
      set(sx, sy, sz);
   }
   
   BVecN(scalarType sx, scalarType sy, scalarType sz, scalarType sw)
   {
      set(sx, sy, sz, sw);
   }

   BVecN(const BVecN& b)
   {
      *this = b;
   }

   template<int otherSize, typename otherScalarType>
   BVecN(const BVecN<otherSize, otherScalarType>& b)
   {
      *this = b;
   }
   
   template<int otherSize, typename otherScalarType>
   BVecN(const BVecN<otherSize, otherScalarType>& b, scalarType sw)
   {
      *this = b;
      //if (size >= 4)
      //   element[3] = sw;
      element[size - 1] = sw;
   }

   BVecN& operator= (const BVecN& b)
   {
      for (int i = 0; i < size; i++)
         element[i] = b.element[i];
      return *this;
   }
   
   BVecN& operator= (scalarType s)
   {
      for (int i = 0; i < size; i++)
         element[i] = s;
      return *this;
   }

   template<int otherSize, typename otherScalarType>
   BVecN& operator= (const BVecN<otherSize, otherScalarType>& b)
   {
      int n = Math::Min<int>(size, b.numElements);
      int i;
      for (i = 0; i < n; i++)
         element[i] = (scalarType)b.element[i];
      for ( ; i < size; i++)
         element[i] = 0.0f;
      return *this;
   }

   bool operator== (const BVecN& b) const
   {
      for (int i = 0; i < size; i++)
         if (element[i] != b.element[i])
            return false;
      return true;
   }

   // lexicographical less
   bool operator<(const BVecN& b) const
   {
      for (int i = 0; i < size; i++)
      {
         if (element[i] != b.element[i])
         {
            if (element[i] < b.element[i])
               return true;
            
            return false;
         }
      }
      return false;
   }

   BVecN& setW(scalarType sw)
   {
      //BCOMPILETIMEASSERT(size >= 4);
      //element[3] = sw;
      element[size - 1] = sw;
      return *this;
   }

   BVecN& clearW(void)
   {
      //BCOMPILETIMEASSERT(size >= 4);
      //element[3] = 0.0f;
      element[size - 1] = 0.0f;
      return *this;
   }

   BVecN& set(scalarType s)
   {
      for (int i = 0; i < size; i++)
         element[i] = s;
      return *this;
   }

#if 0   
   BVecN& set(int s)
   {
      for (int i = 0; i < size; i++)
         element[i] = static_cast<ScalarType>(s);
      return *this;
   }
#endif   
   
   BVecN& set(scalarType sx, scalarType sy)
   {
      element[0] = sx;
      if (size >= 2)
      {
         element[1] = sy;
         for (int i = 2; i < size; i++)
            element[i] = 0.0f;
      }
      return *this;
   }
   
   BVecN& set(scalarType sx, scalarType sy, scalarType sz)
   {
      element[0] = sx;
      if (size >= 2)
      {
         element[1] = sy;
         if (size >= 3)
         {
            element[2] = sz;
            for (int i = 3; i < size; i++)
               element[i] = 0.0f;
         }
      }
      return *this;
   }
   
   BVecN& set(scalarType sx, scalarType sy, scalarType sz, scalarType sw)
   {
      element[0] = sx;
      if (size >= 2)
      {
         element[1] = sy;
         if (size >= 3)
         {
            element[2] = sz;
            if (size >= 4)
            {
               element[3] = sw;
               for (int i = 4; i < size; i++)
                  element[i] = 0.0f;
            }
         }
      }
      return *this;
   }
   
   BVecN& setFromPtr(const scalarType* pData)
   {
      BASSERT(pData);
      memcpy(element, pData, size * sizeof(scalarType));
      return *this;
   }
   
   BVecN& setZero(void)
   {
      for (int i = 0; i < size; i++)
         element[i] = 0.0f;
      return *this;
   }
   
   BVecN& clear(void)
   {
      return setZero();
   }
   
   // symbolic component access
   scalarType  getX(void) const { return element[0]; }
   scalarType& getX(void)       { return element[0]; }
   
   scalarType  getY(void) const { BCOMPILETIMEASSERT(size >= 2); return element[1]; }
   scalarType& getY(void)       { BCOMPILETIMEASSERT(size >= 2); return element[1]; }
   
   scalarType  getZ(void) const { BCOMPILETIMEASSERT(size >= 3); return element[2]; }
   scalarType& getZ(void)       { BCOMPILETIMEASSERT(size >= 3); return element[2]; }
   
   scalarType  getW(void) const { BCOMPILETIMEASSERT(size >= 4); return element[3]; }
   scalarType& getW(void)       { BCOMPILETIMEASSERT(size >= 4); return element[3]; }
   
   // element access
   scalarType operator[] (int i) const { return element[debugRangeCheck(i, size)]; }
   scalarType& operator[] (int i) { return element[debugRangeCheck(i, size)]; }
   
   // in place vec add
   BVecN& operator+= (const BVecN& b)
   {
      for (int i = 0; i < size; i++)
         element[i] += b.element[i];
      return *this;
   }

   // in place vec sub
   BVecN& operator-= (const BVecN& b)
   {
      for (int i = 0; i < size; i++)
         element[i] -= b.element[i];
      return *this;
   }

   // in place vec mul
   BVecN& operator*= (const BVecN& b)
   {
      for (int i = 0; i < size; i++)
         element[i] *= b.element[i];
      return *this;
   }

   // in place scalar mul
   BVecN& operator*= (scalarType s)
   {
      for (int i = 0; i < size; i++)
         element[i] *= s;
      return *this;
   }

   // in place scalar div
   BVecN& operator/= (scalarType s)
   {
      for (int i = 0; i < size; i++)
         element[i] /= s;
      return *this;
   }
   
   // positive
   BVecN operator+ () const
   {
      return *this;
   }

   // negative
   BVecN operator- () const
   {
      BVecN ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = -element[i];
      return ret;
   }
   
   BVecN operator- (const BVecN& b) const
   {
      BVecN ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = element[i] - b.element[i];
      return ret;
   }

   // scale mul
   friend VecType operator* (scalarType s, const VecType& a)
   {
      VecType ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = s * a.element[i];
      return ret;
   }
   
   friend VecType operator* (const VecType& a, scalarType s)
   {
      VecType ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = s * a.element[i];
      return ret;
   }
   
   // scale div
   BVecN operator/ (scalarType s) const
   {
      BVecN ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = element[i] / s;
      return ret;
   }
   
   // vec add/sub
   friend VecType operator+ (const VecType& a, const VecType& b)
   {
      VecType ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = a.element[i] + b.element[i];
      return ret;
   }
               
   double dotPrecise(const VecType& b) const
   {
      double temp[size];
      
      for (int i = 0; i < size; i++)
         temp[i] = element[i] * b.element[i];
      
      Utils::BubbleSortElemFunc(temp, temp + size, Math::fAbs);
                                          
      // Add elements from lowest to largest magnitude.                                              
      double sum = 0;
      for (int i = 0; i < size; i++)
         sum += temp[i];
      
      return sum;
   }
   
   // returns normalized vector
   BVecN normalizedPrecise(const BVecN& defaultVec = BVecN(0)) const
   {
      const double len = sqrt(dotPrecise(*this));
      if (0.0f == len)
         return defaultVec;
      
      return *this / len;
   }
   
   // dot product
   friend scalarType operator* (const VecType& a, const VecType& b)
   {
      scalarType sum = 0.0f;
      for (int i = 0; i < size; i++)
         sum += a.element[i] * b.element[i];
      return sum;
   }
               
   // vec division
   friend VecType operator/ (const VecType& a, const VecType& b)
   {
      VecType ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = a.element[i] / b.element[i];
      return ret;
   }

   friend BVecN<3, scalarType> operator% (const VecType& a, const VecType& b)
   {
      BCOMPILETIMEASSERT(size >= 2);
   
      if (2 == size)
         return BVecN<3, scalarType>(0.0f, 0.0f, a[0]*b[1] - a[1]*b[0]);
      else
         return BVecN<3, scalarType>(a[1]*b[2] - a[2]*b[1], a[2]*b[0] - a[0]*b[2], a[0]*b[1] - a[1]*b[0]);
   }
   
   // add elements
   scalarType horizontalAdd(void) const
   {
      scalarType sum = 0.0f;
      for (int i = 0; i < size; i++)
         sum += element[i];
      return sum;
   }

   // sum in first element
   BVecN horizontalAddSingle(void) const
   {
      BVecN ret(0);
      ret.element[0] = horizontalAdd();
      return ret;
   }

   // sum in all elements
   BVecN horizontalAddBroadcast(void) const
   {
      return BVecN(horizontalAdd());
   }
   
   // dot product 
   scalarType dot(const BVecN& b) const
   {
      return *this * b;
   }
   
   scalarType dot2(const BVecN& b) const
   {
      BCOMPILETIMEASSERT(size >= 2);
      return element[0] * b.element[0] + element[1] * b.element[1];
   }
   
   scalarType dot3(const BVecN& b) const
   {
      BCOMPILETIMEASSERT(size >= 3);
      return element[0] * b.element[0] + element[1] * b.element[1] + element[2] * b.element[2];
   }

   // squared length
   scalarType norm(void) const
   {
      scalarType sum = 0.0f;
      for (int i = 0; i < size; i++)
         sum += element[i] * element[i];
      return sum;
   }

   scalarType len2(void) const
   {
      return norm();
   }

   scalarType squaredLen(void) const 
   { 
      return norm(); 
   }
   
   // length
   scalarType len(void) const
   {
      return sqrt(norm());
   }
   
   // 1/length
   scalarType oneOverLen(void) const
   {
      const double l = sqrt(norm());
      if (l != 0.0f)
         return static_cast<scalarType>(1.0f / l);
      return 0.0f;
   }
   
   scalarType dist(const BVecN& p) const
   {
      return (p - *this).len();
   }
   
   scalarType dist2(const BVecN& p) const
   {
      return (p - *this).len2();
   }
   
   // In-place normalize.
   BVecN& normalize(void)
   {
      double l = element[0] * element[0];
      for (int i = 1; i < size; i++)
            l += element[i] * element[i];
      if (0.0f != l)
      {
         const double ool = 1.0f / sqrt(l);
         for (int i = 0; i < size; i++)
            element[i] = static_cast<scalarType>(element[i] * ool);
      }
      return *this;
   }
   
   // In-place normalize - whole vector is affected, but only xyz contribute to length.
   BVecN& normalize3(void)
   {
      BCOMPILETIMEASSERT(size >= 3);
      
      const double l = element[0] * element[0] + element[1] * element[1] + element[2] * element[2];
      if (0.0f != l)
      {
         const double ool = 1.0f / sqrt(l);
         for (int i = 0; i < size; i++)
            element[i] = static_cast<scalarType>(element[i] * ool);
      }
      
      return *this;
   }
   
   // In-place normalize, use default vector if zero vector.
   BVecN& normalize(const BVecN& defaultVec)
   {
      double l = element[0] * element[0];
      for (int i = 1; i < size; i++)
            l += element[i] * element[i];
      if (0.0f == l)
         *this = defaultVec;
      else
      {
         const double ool = 1.0f / sqrt(l);
         for (int i = 0; i < size; i++)
            element[i] *= ool;
      }
      return *this;
   }
   
   bool normalize(scalarType* pLen)
   {
      double l = element[0] * element[0];
      for (int i = 1; i < size; i++)
         l += element[i] * element[i];
      
      if (0.0f == l)
         return false;
      
      const double len = sqrt(l);
      if (pLen)
         *pLen = static_cast<scalarType>(len);
      
      const double ool = 1.0f / len;
      for (int i = 0; i < size; i++)
      {
         element[i] = static_cast<scalarType>(element[i] * ool);
      }
      
      return true;
   }
   
   bool normalize3(scalarType* pLen)
   {
      BCOMPILETIMEASSERT(size >= 3);
      
      const double l = element[0] * element[0] + element[1] * element[1] + element[2] * element[2];
      
      if (0.0f == l)
         return false;

      const double len = sqrt(l);
      if (pLen)
         *pLen = static_cast<scalarType>(len);

      const double ool = 1.0f / len;
      for (int i = 0; i < size; i++)
         element[i] *= ool;

      return true;
   }

   // In-place normalize - returns 0 or 1/sqrt(norm())
   scalarType tryNormalize(void)
   {
      double l = element[0] * element[0];
      for (int i = 1; i < size; i++)
            l += element[i] * element[i];
      if (0.0f != l)
      {
         l = 1.0f / sqrt(l);
         for (int i = 0; i < size; i++)
            element[i] = static_cast<scalarType>(element[i] * l);
      }
      return static_cast<scalarType>(l);
   }
   
   // in place normalize
   // returns 0 or 1/sqrt(norm())
   scalarType tryNormalize(const BVecN& defaultVec)
   {
      double l = element[0] * element[0];
      for (int i = 1; i < size; i++)
            l += element[i] * element[i];
      if (0.0f == l)
         *this = defaultVec;
      else
      {
         l = 1.0f / sqrt(l);
         for (int i = 0; i < size; i++)
            element[i] = static_cast<scalarType>(element[i] * l);
      }
      return static_cast<scalarType>(l);
   }
   
   // Returns normalized vector.
   BVecN normalized(void) const
   {
      return BVecN(*this).normalize();
   }
   
   // Returns normalized vector.
   BVecN normalized(const BVecN& defaultVec) const
   {
      return BVecN(*this).normalize(defaultVec);
   }
         
   // component replication
   BVecN broadcast(const int e) const
   {
      debugRangeCheck(e, size);

      BVecN ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = element[e];
      
      return ret;
   }
   
   // 2D component swizzle
   BVecN<2, scalarType> swizzle(int x, int y) const
   {
      debugRangeCheck(x, size);
      debugRangeCheck(y, size);
      return BVecN<2, scalarType>(element[x], element[y]);
   }
   
   // 3D component swizzle
   BVecN<3, scalarType> swizzle(int x, int y, int z) const
   {
      debugRangeCheck(x, size);
      debugRangeCheck(y, size);
      debugRangeCheck(z, size);
      return BVecN<3, scalarType>(element[x], element[y], element[z]);
   }
   
   // 4D component swizzle
   BVecN<4, scalarType> swizzle(int x, int y, int z, int w) const
   {
      debugRangeCheck(x, size);
      debugRangeCheck(y, size);
      debugRangeCheck(z, size);
      debugRangeCheck(w, size);
      return BVecN<4, scalarType>(element[x], element[y], element[z], element[w]);
   }
   
   bool isVector(void) const
   {
      return element[size - 1] == 0.0f;
   }

   bool isVector(scalarType tol) const
   {
      return Math::EqualTol<scalarType>(element[size - 1], 0.0f, tol);
   }
   
   bool isPoint(void) const
   {
      return element[size - 1] == 1.0f;
   }
         
   bool isPoint(scalarType tol) const
   {
      return Math::EqualTol<scalarType>(element[size - 1], 1.0f, tol);
   }
   
   // true if unit length or nearly so
   bool isUnit(scalarType tol = Math::fSmallEpsilon) const
   {
      return Math::EqualTol<scalarType>(len(), 1.0f, tol);
   }
         
   // clamp components between [low, high]
   BVecN& clampComponents(scalarType low, scalarType high)
   {
      for (int i = 0; i < size; i++)
         if (element[i] < low)
            element[i] = low;
         else if (element[i] > high)
            element[i] = high;
      return *this;
   }
   
   scalarType minComponent(void) const
   {
      scalarType m = element[0];
      for (int i = 1; i < size; i++)
         if (element[i] < m)
            m = element[i];
      return m;
   }

   scalarType maxComponent(void) const
   {
      scalarType m = element[0];
      for (int i = 1; i < size; i++)
         if (element[i] > m)
            m = element[i];
      return m;
   }

   scalarType minAbsComponent(void) const
   {
      scalarType m = fabs(element[0]);
      for (int i = 1; i < size; i++)
         if (fabs(element[i]) < m)
            m = fabs(element[i]);
      return m;
   }
         
   scalarType maxAbsComponent(void) const
   {
      scalarType m = fabs(element[0]);
      for (int i = 1; i < size; i++)
         if (fabs(element[i]) > m)
            m = fabs(element[i]);
      return m;
   }

   int minorAxis(void) const
   {
      scalarType m = fabs(element[0]);
      int a = 0;
      for (int i = 1; i < size; i++)
         if (fabs(element[i]) < m)
         {
            m = fabs(element[i]);
            a = i;
         }
      return a;
   }
   
   int majorAxis(void) const
   {
      scalarType m = fabs(element[0]);
      int a = 0;
      for (int i = 1; i < size; i++)
         if (fabs(element[i]) > m)
         {
            m = fabs(element[i]);
            a = i;
         }
      return a;
   }
   void projectionAxesFast(int& uAxis, int& vAxis) const
   {
      BCOMPILETIMEASSERT(size == 3);
      const int axis = majorAxis();
      uAxis = Math::NextWrap(axis, size);
      vAxis = Math::NextWrap(uAxis, size);
   }

   void projectionAxes(int& uAxis, int& vAxis) const
   {
      BCOMPILETIMEASSERT(size == 3);
      const int axis = majorAxis();
      if (element[axis] < 0.0f)
      {
         vAxis = Math::NextWrap(axis, size);
         uAxis = Math::NextWrap(vAxis, size);
      }
      else
      {
         uAxis = Math::NextWrap(axis, size);
         vAxis = Math::NextWrap(uAxis, size);
      }
   }

   // [-1,1] -> [0,1]
   BVecN rangeCompressed(void) const
   {
      return *this * .5f + BVecN(.5f);
   }
   
   // [0,1] -> [-1,1]
   BVecN rangeExpanded(void) const
   {
      return (*this - BVecN(.5f)) * 2.0f;
   }

   // Returns vector with w = 0
   BVecN toVector(void) const
   {
      BVecN ret(*this);
      ret[size - 1] = 0.0f;
      return ret;
   }

   // Returns vector with w = 1
   BVecN toPoint(void) const
   {
      BVecN ret(*this);
      ret[size - 1] = 1.0f;
      return ret;
   }
   
   BVecN project(void) const
   {
      BVecN ret;
      const scalarType s = element[size - 1];
      if (s == 0.0f)
         ret = *this;
      else
      {
         const scalarType oos = 1.0f / s;
         for (int i = 0; i < size - 1; i++)
            ret[i] = element[i] * oos;
         ret[size - 1] = 1.0f;
      }
      return ret;
   }

   BVecN floor(void) const
   {
      BVecN ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = ::floor(element[i]);
      return ret;
   }

   BVecN ceil(void) const
   {
      BVecN ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = ::ceil(element[i]);
      return ret;
   }

   // true if all elements are 0
   bool isZero(void) const
   {
      for (int i = 0; i < size; i++)
         if (element[i] != 0.0f)
            return false;
      return true;
   }
   
   // true if any element is nonzero
   bool isNonZero(void) const
   {
      return !isZero();
   }
   
   BVecN negateXYZ(void) const
   {
      BVecN ret;
      
      ret[0] = -element[0];
      if (size >= 2)
      {
         ret[1] = -element[1];
         
         if (size >= 3)
            ret[2] = -element[2];
      }
      
      for (uint i = 3; i < size; i++)
         ret[i] = element[i];
      
      return ret;
   }
   
   BVecN negateW(void) const
   {
      BVecN ret(this);

      ret[size - 1] = -ret[size - 1];
      
      return ret;
   }
   
   BVecN<2, scalarType> compSelect2(int c0, int c1) const
   {
      return BVecN<2>(element[debugRangeCheck(c0, size)], element[debugRangeCheck(c1, size)]);
   }
   
   BVecN<3, scalarType> compSelect3(int c0, int c1, int c2) const
   {
      return BVecN<3, scalarType>(element[debugRangeCheck(c0, size)], element[debugRangeCheck(c1, size)], element[debugRangeCheck(c2, size)]);
   }
   
   // remove projection of v on dir from v, dir must be unit length
   static VecType removeCompUnit(const VecType& v, const VecType& dir)
   {
      BASSERT(dir.isUnit());
      const VecType rhs(dir * (v * dir));
      return v - rhs;
   }
   
   // returns vector with a 1 in the selected component
   static VecType makeAxisVector(int axis, scalarType s = 1.0f)
   {
      VecType ret;
      ret.setZero();
      ret.element[debugRangeCheck(axis, size)] = s;
      return ret;
   }
   
   // -pi   <= yaw (long)  < pi    
   // -pi/2 <= pitch (lat) < pi/2   
   // x=yaw (XZ) -180 to 180
   // y=pitch (Y) -90 to 90
   // z=mag/radius
   static BVecN<3, scalarType> makeCartesian(const BVecN<3>& v)
   {
      const scalarType yaw = v[0];
      const scalarType pitch = v[1];
      const scalarType mag = v[2];
      return BVecN<3, scalarType>(
         sin(yaw)*cos(pitch)*mag,
         sin(pitch)*mag,
         cos(yaw)*cos(pitch)*mag);
   }
   
   // x=yaw,y=pitch,z=mag
   static BVecN<3, scalarType> makeSpherical(const BVecN<3, scalarType>& v)
   {
      //asin(z)=atan2(z,sqrt(x*x+y*y))
      return BVecN<3, scalarType>(atan2(v[0], v[2]), asin(v[1]), v.len());
   }
   
   // Returns random normalized vector
   static BVecN<3, scalarType> makeRandomSpherical(void)
   {
      scalarType z = Math::fRand(-1.0f, 1.0f);
      scalarType phi = Math::fRand(0.0f, Math::fTwoPi);
      scalarType theta = asin(z);
      scalarType cosTheta = cos(theta);
      return BVecN<3, scalarType>(cosTheta * cos(phi), cosTheta * sin(phi), z);
   }
   
   static VecType makeRandomUniform(scalarType l = 0.0f, scalarType h = 1.0f)
   {
      VecType ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = Math::fRand(l, h);
      return ret;
   }

   // vec multiply
   static VecType multiply(const VecType& a, const VecType& b)
   {
      VecType ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = a.element[i] * b.element[i];
      return ret;
   }
   
   static VecType abs(const VecType& a)
   {
      VecType ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = fabs(a.element[i]);
      return ret;
   }

   // elementwise min
   static VecType elementMin(const VecType& a, const VecType& b)
   {
      VecType ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = Math::Min(a.element[i], b.element[i]);
      return ret;
   }

   // elementwise max
   static VecType elementMax(const VecType& a, const VecType& b)
   {
      VecType ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = Math::Max(a.element[i], b.element[i]);
      return ret;
   }

   // See "The Pleasures of Perp Dot Products", Graphics Gems IV, page 138.
   BVecN perp(void) const
   {
      BCOMPILETIMEASSERT(size == 2);
      return BVecN(-element[1], element[0]);
   }

   // this->perp() dot b
   scalarType perpDot(const BVecN& b) const
   {
      BCOMPILETIMEASSERT(size == 2);
      return element[0] * b.element[1] - element[1] * b.element[0];
   }

   static bool equalTol(const VecType& a, const VecType& b, scalarType tol = Math::fSmallEpsilon)
   {
      for (int i = 0; i < size; i++)
         if (!Math::EqualTol<scalarType>(a[i], b[i], tol))
            return false;
      return true;
   }
   
   static bool equalTol3(const VecType& a, const VecType& b, scalarType tol = Math::fSmallEpsilon)
   {
      for (int i = 0; i < Math::Min(3, size); i++)
         if (!Math::EqualTol<scalarType>(a[i], b[i], tol))
            return false;
      return true;
   }

   static VecType lerp(const VecType& a, const VecType& b, scalarType t)
   {
      VecType ret;
      for (int i = 0; i < size; i++)
         ret.element[i] = a.element[i] + (b.element[i] - a.element[i]) * t;
      return ret;
   }

   scalarType* setFromPtr(scalarType* pDest) const
   {
      for (int i = 0; i < size; i++)
         *pDest++ = element[i];
      return pDest;
   }

   static VecType makeFromPtr(const scalarType* pFloats, int n)
   {
      VecType ret;
      n = Math::Min(size, n);
      for (int i = 0; i < n; i++)
         ret.element[i] = pFloats[i];
      for ( ; i < size; i++)
         ret.element[i] = 0.0f;
      return ret;            
   }

   // false if any elements are invalid floats
   bool isValid(void) const
   {
      for (int i = 0; i < size; i++)
         if (!Math::IsValidFloat(element[i]))
            return false;
      return true;
   }

   bool allLess(const BVecN& b) const
   {
      for (int i = 0; i < size; i++)
         if (element[i] >= b.element[i])
            return false;
      return true;
   }

   bool allLessEqual(const BVecN& b) const
   {
      for (int i = 0; i < size; i++)
         if (element[i] > b.element[i])
            return false;
      return true;
   }

   bool allGreater(const BVecN& b) const
   {
      for (int i = 0; i < size; i++)
         if (element[i] <= b.element[i])
            return false;
      return true;
   }

   bool allGreaterEqual(const BVecN& b) const
   {
      for (int i = 0; i < size; i++)
         if (element[i] < b.element[i])
            return false;
      return true;
   }

   const scalarType* getPtr(void) const { return &element[0]; }
         scalarType* getPtr(void)       { return &element[0]; }

#ifdef DEBUG
   void debugCheck(void) const
   {
      for (int i = 0; i < size; i++)
         BASSERT(Math::IsValidFloat(element[i]));
   }
#else
   void debugCheck(void) const
   {
   }
#endif

   void log(BTextDispatcher& l) const
   {
      for (int i = 0; i < size; i++)
      {
         l.printf("%f", (double)element[i]);
         if (i < size - 1)
            l.printf(" ");
      }
      l.printf("\n");
   }

   friend BStream& operator<< (BStream& dst, const VecType& src)
   {
      for (int i = 0; i < size; i++)
         dst << src.element[i];
      return dst;
   }

   friend BStream& operator>> (BStream& src, VecType& dst)
   {
      for (int i = 0; i < size; i++)
         src >> dst.element[i];
      return src;
   }
};

typedef BVecN<2, float>    BVec2;
typedef BVecN<3, float>    BVec3;
typedef BVecN<4, float>    BVec4;

typedef BVecN<2, double>   BVec2D;
typedef BVecN<3, double>   BVec3D;
typedef BVecN<4, double>   BVec4D;

