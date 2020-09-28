/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */
#ifndef HKSCENEDATA_MESH_HKXINDEXBUFFER_HKCLASS_H
#define HKSCENEDATA_MESH_HKXINDEXBUFFER_HKCLASS_H

/// hkxIndexBuffer meta information
extern const class hkClass hkxIndexBufferClass;

/// A structure that binds vertices to triangles. For the moment, as the
/// serialization 	 does not support unions, we have two explicit buffers, one of
/// 16bit storage and 	 one of 32bit storage so that the endianness can be swapped
/// on write
class hkxIndexBuffer
{
	public:

		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_SCENE_DATA, hkxIndexBuffer );
		HK_DECLARE_REFLECTION();
	
			/// The index buffer represents
		enum IndexType
		{
				/// 
			INDEX_TYPE_INVALID,
				/// 
			INDEX_TYPE_TRI_LIST,
				/// 
			INDEX_TYPE_TRI_STRIP,
				/// 
			INDEX_TYPE_TRI_FAN,
				/// 
			INDEX_TYPE_MAX_ID
		};
		
		
		//
		// Members
		//
	public:
		
			/// The index buffer represents a triangle list, strip or fan
		hkEnum<IndexType, hkInt8> m_indexType;
		
			/// The pointer to the generic index buffer of index size 16
		hkUint16* m_indices16;
		hkInt32 m_numIndices16;
		
			/// The pointer to the generic index buffer of index size 32
		hkUint32* m_indices32;
		hkInt32 m_numIndices32;
		
			/// The vertex index is usually the index given by index buffer + vertexBaseOffset.
			/// If the indexBase is null then the format is assumed to be a triangle list
			/// starting at vertexBaseOffset.
		hkUint32 m_vertexBaseOffset;
		
			/// Due to serialization, the index buffers above are 'simple arrays' and thus the
			/// 	num indices will be equal to the length, unless those arrays are empty (a
			/// trivial 	vertex buffer based, no indices, just len in a vb).
		hkUint32 m_length;

	public:

			/// Returns the number of triangles
		int getNumTriangles () const;

		void getTriangleIndices (hkUint32 triIndex, hkUint32& indexAOut, hkUint32& indexBOut, hkUint32& indexCOut);

};

#endif // HKSCENEDATA_MESH_HKXINDEXBUFFER_HKCLASS_H

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
