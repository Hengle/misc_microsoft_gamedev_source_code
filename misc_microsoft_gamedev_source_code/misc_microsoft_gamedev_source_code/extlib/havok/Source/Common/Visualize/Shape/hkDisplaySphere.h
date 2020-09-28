/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */
#ifndef HK_VISUALIZE_SHAPE_SPHERE_H
#define HK_VISUALIZE_SHAPE_SPHERE_H

#include <Common/Visualize/Shape/hkDisplayGeometry.h>
#include <Common/Base/Types/Geometry/Sphere/hkSphere.h>

class hkDisplaySphere : public hkDisplayGeometry
{
	public:

		hkDisplaySphere(const hkSphere& sphere, int xRes, int yRes);

		virtual void buildGeometry();

		virtual void getWireframeGeometry(hkArray<hkVector4>& lines);
		
		inline void getSphere(hkSphere& s) { s = m_sphere; }
		
		inline int getXRes() { return m_xRes; }
		inline int getYRes() { return m_yRes; }

	protected:

		hkSphere m_sphere;

		int m_xRes;
		int m_yRes;
};


#endif // HK_VISUALIZE_DISPLAY_SPHERE_H

/*
* Havok SDK - PUBLIC RELEASE, BUILD(#20070919)
*
* Confidential Information of Havok.  (C) Copyright 1999-2007 
* Telekinesys Research Limited t/a Havok. All Rights Reserved. The Havok
* Logo, and the Havok buzzsaw logo are trademarks of Havok.  Title, ownership
* rights, and intellectual property rights in the Havok software remain in
* Havok and/or its suppliers.
*
* Use of this software for evaluation purposes is subject to and indicates 
* acceptance of the End User licence Agreement for this product. A copy of 
* the license is included with this software and is also available from salesteam@havok.com.
*
*/
