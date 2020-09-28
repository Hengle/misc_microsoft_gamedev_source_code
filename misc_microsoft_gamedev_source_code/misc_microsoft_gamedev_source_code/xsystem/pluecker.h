//==============================================================================
// Copyright (c) 1997-2000 Ensemble Studios
//
// Vector class
//==============================================================================

#ifndef _PLUECKER_H_
#define _PLUECKER_H_

/*
*	What are Pluecker coordinates? (from comp.graphics.algorithms)
*	
*	A common convention is to write umlauted u as "ue", so you'll also see "Pluecker". Lines in 3D can easily be given by 
* listing the coordinates of two distinct points, for a total of six numbers. Or, they can be given as the coordinates of 
* two distinct planes, eight numbers. What's wrong with these? Nothing; but we can do better. Pluecker coordinates are, 
* in a sense, halfway between these extremes, and can trivially generate either. Neither extreme is as efficient as 
* Pluecker coordinates for computations. 
*	
*	When Pluecker coordinates generalize to Grassmann coordinates, as laid out beautifully in [Hodge], Chapter VII,
* the determinant definition is clearly the one to use. But 3D lines can use a simpler definition. Take two distinct 
* points on a line, say 
*	--------------------------------------------------------------------------------
*      	 P = (Px,Py,Pz)
*      	 Q = (Qx,Qy,Qz)
*	--------------------------------------------------------------------------------
*	Think of these as vectors from the origin, if you like. The Pluecker coordinates for the line are essentially 
*	--------------------------------------------------------------------------------
*        U = P - Q
*        V = P x Q
* --------------------------------------------------------------------------------
* Except for a scale factor, which we ignore, U and V do not depend on the specific P and Q! Cross products are perpendicular 
* to their factors, so we always have U.V = 0. In [Stolfi] lines have orientation, so are the same only if their Pluecker 
* coordinates are related by a positive scale factor. 
* 
* As determinants of homogeneous coordinates, begin with the 4x2 matrix 
* --------------------------------------------------------------------------------
*         [ Px  Qx ] row x
*         [ Py  Qy ] row y
*         [ Pz  Qz ] row z
*         [  1   1 ] row w
* --------------------------------------------------------------------------------
* 
* Define Pluecker coordinate Gij as the determinant of rows i and j, in that order. Notice that Giw = Pi - Qi, which is Ui. 
* Now let (i,j,k) be a cyclic permutation of (x,y,z), namely (x,y,z) or (y,z,x) or (z,x,y), and notice that Gij = Vk. Determinants 
* are anti-symmetric in the rows, so Gij = -Gji. Thus all possible Pluecker line coordinates are either zero (if i=j) or components 
* of U or V, perhaps with a sign change. Taking the w component of a vector as 0, the determinant form will operate just as well 
* on a point P and vector U as on two points. We can also begin with a 2x4 matrix whose rows are the coefficients of homogeneous 
* plane equations, E.P=0, from which come dual coordinates G'ij. Now if (h,i,j,k) is an even permutation of (x,y,z,w), then Ghi = G'jk. 
* (Just swap U and V!) 
* 
* Got Pluecker, want points? No problem. At least one component of U is non-zero, say Ui, which is Giw. Create homogeneous points 
* Pj = Gjw + Gij, and Qj = Gij. (Don't expect the P and Q that we started with, and don't expect w=1.) 
* Want plane equations? Let (i,j,k,w) be an even permutation of (x,y,z,w), so G'jk = Giw. Then create Eh = G'hk, and Fh = G'jh. 
* 
* Example: Begin with P = (2,4,8) and Q = (2,3,5). Then U = (0,1,3) and V = (-4,6,-2). 
* The direct determinant forms are Gxw=0, Gyw=1, Gzw=3, Gyz=-4, Gzx=6, Gxy=-2, 
* and the dual forms are G'yz=0, G'zx=1, G'xy=3, G'xw=-4, G'yw=6, G'zw=-2. 
* Take Uz = Gzw = G'xy = 3 as a suitable non-zero element. Then two planes meeting in the line are 
* --------------------------------------------------------------------------------
*         (G'xy  G'yy  G'zy  G'wy).P = 0
*         (G'xx  G'xy  G'xz  G'xw).P = 0
* --------------------------------------------------------------------------------
* That is, a point P is on the line if it satisfies both these equations: 
* --------------------------------------------------------------------------------
*         3 Px + 0 Py + 0 Pz - 6 Pw = 0
*         0 Px + 3 Py - 1 Pz - 4 Pw = 0
* --------------------------------------------------------------------------------
* We can also easily determine if two lines meet, or if not, how they pass. If U1 and V1 are the coordinates of line 1, U2 and V2, 
* of line 2, we look at the sign of U1.V2 + V1.U2. If it's zero, they meet. The determinant form reveals even permutations 
* of (x,y,z,w): G1xw G2yz + G1yw G2zx + G1zw G2xy + G1yz G2xw + G1zx p2yw + G1xy p2zw 
* 
* Two oriented lines L1 and L2 can interact in three different ways: L1 might intersect L2, L1 might go clockwise around L2, 
* or L1 might go counterclockwise around L2. Here are some examples: 
* --------------------------------------------------------------------------------
* 
*              | L2            | L2            | L2
*         L1   |          L1   |          L1   |
*         -----+----->    ----------->    -----|----->
*              |               |               |
*              V               V               V
*          intersect    counterclockwise   clockwise
*              | L2            | L2            | L2
*         L1   |          L1   |          L1   |
*         <----+-----     <----|------    <-----------
*              |               |               |
*              V               V               V
* --------------------------------------------------------------------------------
* The first and second rows are just different views of the same lines, once from the "front" and once from the "back." 
* Here's what they might look like if you look straight down line L2 (shown here as a dot). 
* --------------------------------------------------------------------------------
*         L1                               ---------->
*         -----o---->     L1   o           L1   o
*                         ---------->
*          intersect    counterclockwise    clockwise
* --------------------------------------------------------------------------------
* The Pluecker coordinates of L1 and L2 give you a quick way to test which of the three it is. 
* --------------------------------------------------------------------------------
*         cw:   U1.V2 + V1.U2 < 0
*         ccw:  U1.V2 + V1.U2 > 0
*         thru: U1.V2 + V1.U2 = 0
* --------------------------------------------------------------------------------
* So why is this useful? Suppose you want to test if a ray intersects a triangle in 3-space. One way to do this is to represent 
* the ray and the edges of the triangle with Pluecker coordinates. The ray hits the triangle if and only if it hits one of 
* the triangle's edges, or it's "clockwise" from all three edges, or it's "counterclockwise" from all three edges. For example... 
* --------------------------------------------------------------------------------
*           o  _
*           | |\                  ...in this picture, the ray
*           |   \                 is oriented counterclockwise
*         ------ \ -->            from all three edges, so it
*           |     \               must intersect the triangle.
*           v      \
*           o-----> o
* --------------------------------------------------------------------------------
* Using Pluecker coordinates, ray shooting tests like this take only a few lines of code. 
*/

//==============================================================================
// BPluecker Class
//
//==============================================================================
class BPluecker
{
public:
      BVector                    u,v;

      BPluecker()
      {
         // no assignment
         //u.zero();
         //v.zero();
      }

      // The direct determinant forms are    Gxw,  Gyw,  Gzw,  Gyz,  Gzx,  Gxy 
      // The dual forms are                  G'yz, G'zx, G'xy, G'xw, G'yw, G'zw

      BPluecker(const BVector& p, const BVector& q)
      {
         u.assignDifference  ( p, q );
         v.assignCrossProduct( u, q );
      }

      void set(const BVector& p, const BVector& q)
      {
         u.assignDifference  ( p, q );
         v.assignCrossProduct( u, q ); 
      }

      void setFromDirection(const BVector& dir, const BVector& origin)
      {
         u = dir;
         v.assignCrossProduct( u, origin );
      }

      const BVector pointNearOrigin() const
      {
         BVector temp;
         temp.assignCrossProduct(u, v);
         const float scale = v.dot(v);
         BASSERT( scale != 0.0f );
         return BVector( temp.x/scale, temp.y/scale, temp.z/scale );
      }

      const float distanceToOriginSquared() const
      {
         return ( (v.dot(v) ) / (u.dot(u))  );
      }

      // debug
      const bool isValid() const 
      {
         return ( ( ( u.x*v.z ) - ( u.y*v.y ) + ( u.z*v.x ) ) == 0.0f );
      }

      // 5 adds, 6 muls  -- todo optimize for 2d, make 4d Pluecker class
      // returns an unnormlized barycentric coordinate
      const float dotProduct(const BPluecker& p) const
      {
         return ( u.dot(p.v) + v.dot(p.u) );
         //return ( (u.x*p.v.y) + (u.y*p.v.z) + (u.z*p.v.x) + (v.x*p.u.z) + (v.y*p.u.x) + (v.z*p.u.y) );
      }
};


#endif         // _PLUECKER_H_
