/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_MESHWELDINGUTILITY_H
#define HK_MESHWELDINGUTILITY_H


class hkpBvTreeShape;
class hkpTriangleShape;


	/// Utility functions for building runtime welding information for triangular meshes
class hkpMeshWeldingUtility
{

	public:
		
			/// A function to generate welding information for a shape in a mesh. This welding should be
			/// stored by the mesh and set as the welding information in the triangle returned by getChildShape() with that shape key.
		static hkUint16 HK_CALL calcWeldingInfoForTriangle( hkpShapeKey shapeKey, const hkpBvTreeShape* moppShape, bool testConsistantWinding = false );

	protected:

			// Helper for getWeldingInfoForTriangle
		static void HK_CALL calcBitcodeForTriangleEdge ( const hkpBvTreeShape* moppShape, const hkpTriangleShape* triangleShape, hkpShapeKey triangleShapeKey, int edgeIndex, hkInt16& combinedBitcodesOut,  bool testConsistantWinding );
			// Helper for getWeldingInfoForTriangle
		static int HK_CALL createSingularVertexArray(const hkVector4 *vertices0, const hkVector4 *vertices1, int edgeIndex, hkVector4* vertexArrayOut, int orderedEdgeVerticesOnTriangle1[2] ) ;

			// Helper for getWeldingInfoForTriangle
		static hkUint16 HK_CALL modifyCombinedEdgesBitcode(hkUint16 combinedBitcode, int edgeIndex, int bitcode);

			// Helper for getWeldingInfoForTriangle
		static int HK_CALL calcEdgeAngleBitcode(const hkVector4* vertices);

			// Helper for getWeldingInfoForTriangle
		static void HK_CALL calcAntiClockwiseTriangleNormal(const hkVector4& vertex0, const hkVector4& vertex1, const hkVector4& vertex2, hkVector4& normal);

			// Helper for getWeldingInfoForTriangle
		static hkReal HK_CALL calcAngleFromVertices(const hkVector4* vertices, hkReal& sinAngleOut, hkReal& cosAngleOut);

};

#endif // HK_MESHWELDINGUTILITY_H

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
