/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_COLLIDE2_EXTENDED_MESH2_SHAPE_H
#define HK_COLLIDE2_EXTENDED_MESH2_SHAPE_H

#include <Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h>
#include <Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h>

extern hkReal hkConvexShapeDefaultRadius;
extern const hkClass hkpExtendedMeshShapeClass;

class hkpMeshMaterial;
class hkpSimpleMeshShape;
class hkpMoppBvTreeShape;

/// A class for wrapping geometric collision detection information.  It can directly reference
/// sets of triangle strips with vertex striding, and either 16 or 32 bit indices to vertices.
/// It can also directly reference triangle soups, using three indices per triangle rather than one.
/// It also handles degenerate triangles internally, so no extra checking is required by the user.
/// The mesh shape creates hkTriangleShapes in the ShapeBuffer passed in to the getChildShape function.
/// It gives these shapes a radius (see hkpConvexShape::getRadius()) as specified by the hkpExtendedMeshShape::getRadius() function.
/// This class can also store lists of convex shapes.
class hkpExtendedMeshShape: public hkpShapeCollection
{
	public:

		HK_DECLARE_CLASS_ALLOCATOR(HK_MEMORY_CLASS_CDINFO);

		HK_DECLARE_REFLECTION();

	public:

			/// The striding of mesh indices
		enum IndexStridingType
		{
			INDICES_INVALID, // default, will raise assert.
				/// 16 bit "short" striding
			INDICES_INT16,
				/// 32 bit "int" striding
			INDICES_INT32,
			INDICES_MAX_ID
		};

		enum MaterialIndexStridingType
		{
			MATERIAL_INDICES_INVALID,
			MATERIAL_INDICES_INT8,
			MATERIAL_INDICES_INT16,
			MATERIAL_INDICES_MAX_ID
		};

		enum SubpartType
		{
			SUBPART_TRIANGLES,
			SUBPART_SHAPE
		};

		struct Subpart
		{
			public:
				HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_COLLIDE, hkpExtendedMeshShape::Subpart );
				HK_DECLARE_REFLECTION();

			public:
				inline Subpart(SubpartType type);

			public:
			
					/// Subpart type
				hkEnum<SubpartType,hkInt8> m_type;

					/// A type defining whether 8 or 16 bits are used to index material.
				hkEnum<MaterialIndexStridingType,hkInt8> m_materialIndexStridingType;

					/// The byte offset between two hkMeshMaterials
				hkInt16 m_materialStriding;

					/// a pointer, pointing to a strided array of material index (hkUint8), one index for each triangle.
					///  - You are limited to a maximum of 256 materials per subpart.
					///  - You are not forced to store those indices in a hkUint8 array, with the striding m_materialIndexStriding
					///    parameter you can extract this
					///  - If you do not want to use materials, simply set this element to HK_NULL
				const void* m_materialIndexBase; //+nosave

					/// The byte offset between two material indices (Usually sizeof(hkUint8) or...)
				hkUint16 m_materialIndexStriding;

					/// The number of materials
				hkUint16 m_numMaterials;

					/// The base for the material table, the byte offset between two hkMeshMaterials is defined by
					/// m_materialStriding
					/// Note: On PS3 this must be aligned to a m_materialStriding boundary if you wish to access materials in your own code on SPU.
				const hkpMeshMaterial* m_materialBase; //+nosave

		};

			/// A vertices subpart defines a triangle, a triangle list or a triangle strip.
		struct TrianglesSubpart : public hkpExtendedMeshShape::Subpart
		{
			public:

				HK_DECLARE_REFLECTION();
				HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_COLLIDE, hkpExtendedMeshShape::TrianglesSubpart );

			public:

					/// A partial initializing constructor. It will only set values in Debug
					/// apart from a default material that gets set in Release too.
				inline TrianglesSubpart(); 

					/// the number of triangles
				int	m_numTriangleShapes;

					//
					//	Vertex information
					//
					/// A pointer to the first vertex, defined by three floats.
					/// Must be aligned on a 16 byte boundary
				const float*  m_vertexBase; //+nosave

					/// The byte offset between two consecutive vertices (usually 12, 16 or more).
				int		m_vertexStriding;

					/// The number of vertices.
				int		m_numVertices;

					/// Triangle extrusion
				hkVector4 m_extrusion;

					//
					// Triangle Index Information
					//

					/// A pointer to triples of vertex indices.
					/// Used to be a union type, but to make
					/// auto serialization possible, we leave it as 
					/// a void* here.
				const void*	m_indexBase; //+nosave
					

					/// The byte offset between two indices triples.
					///  - Eg. (Usually sizeof(hkUint16) if you use triangle strips
					///  - or 3 * sizeof(hkUint16) if you use independent triangles
				int m_indexStriding;

					/// A type defining whether 16 or 32 bits are used to index vertices.
				hkEnum<IndexStridingType,hkInt8> m_stridingType;

					/// A flag used to specify whether triangles should be returned wound the same way
					/// or alternate ways.  It must be set to 0 or 1, and defaults to 0.
					/// If Triangle strips are used, each subsequent triangle in a list
					/// will be wound the alternate way. However for one sided welding, we require that
					/// all triangles have a consistent winding. This flag should be set to 1 in this case.  This
					/// means the first triangle will be returned with vertices set to (0, 1, 2) and the second
					/// triangle will be returned with vertices set to (1, 3, 2). If this flag is set to 0 the
					/// second triangle will be returned with vertices set to (1, 2, 3). If independent triangles
					/// are used this flag should be set to 0 to maintain winding.
				hkInt8 m_flipAlternateTriangles;

					/// This information is set automatically when a subpart is added to a mesh and should be left at -1.
					/// It is the offset in a global array for all triangles in the mesh of the first triangle of this sub piece
					/// This info is used for "welding" collisions between triangles.
				int m_triangleOffset; //+default(-1)

		};

			/// A shapes subpart defines a list of one or more convex shapes of type hkpConvexShape.
		struct ShapesSubpart : public hkpExtendedMeshShape::Subpart
		{
			public:

				HK_DECLARE_REFLECTION();
				HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_COLLIDE, hkpExtendedMeshShape::ShapesSubpart );

			public:

				 ShapesSubpart( const hkpConvexShape*const* childShapes, int numChildShapes, const hkVector4& offset = hkVector4::getZero() );
				 ShapesSubpart( const hkpConvexShape*const* childShapes, int numChildShapes, const hkTransform& transform );
				~ShapesSubpart();

			public:

				const hkpConvexShape*const*	m_childShapes;
				int 						m_numChildShapes;

				hkBool						m_offsetSet;
				hkBool						m_rotationSet;

				hkTransform					m_transform;

				int							m_pad[8];
		};

	public:

			/// Constructs a new hkpExtendedMeshShape.
			/// This mesh supports triangle soups as well as shape soups.
			///    - The triangles are grouped in subparts and can be scaled and get a radius applied
			///    - The shapes can be grouped in subparts and can be translated on a per subpart basis.
			/// "numBitsForSubpart" is the number of bits used (in the 32 bit shape key) for the subpart index.
			/// Note that the highest bit is used as the type identifier, discerning whether this subpart consists
			/// of triangles or convex shapes.
			/// The remaining bits from the 32 bit shape key are used for the terminal index. By
			/// default numBitsForSubpartIndex is 12, which means the mesh shape can have 2^11 - 1
			/// subparts (0xffffffff is the "invalid" shape key) = 2047, and each subpart can have 2^20 triangles = 1048576.
			/// The subpart is stored in the high bits, so you can extract subpart/terminal indices like this:<br>
			/// int subpartIndex = key >> ( 32 - mymesh->getNumBitsForSubpartIndex() );<br>
			/// int terminalIndex = key & ( ~0U >> mymesh->getNumBitsForSubpartIndex() );
		hkpExtendedMeshShape( hkReal radius = hkConvexShapeDefaultRadius, int numBitsForSubpartIndex = 12 );


			/// A utility constructor that creates and extended mesh shape from a mesh shape.
		hkpExtendedMeshShape( const class hkpMeshShape* meshShape );

		
			/// Destructor. Simply removes the references to all childShapes
		~hkpExtendedMeshShape();

			/// Compute welding info. In order to weld collisions between triangles in this mesh, welding info must be created.
			/// Note that this function only creates welding information between triangles, not between convex pieces.
			/// You must call this after all subparts have been added to the mesh.
			/// The hkpMoppBvTreeShape you pass in must be built referencing this hkpMeshShape.
			/// This adds an additional 2 bytes per triangle storage overhead.
			/// This is an expensive call, and should be done off line, and the resultant hkpMeshShape
			/// serialized, to save the runtime overhead of computing the welding info.
		void computeWeldingInfo( const hkpMoppBvTreeShape* mopp, hkpWeldingUtility::WeldingType weldingType );

		//
		// Subpart access
		//

			/// Adds a triangle subpart. To modify member xxxx of this triangle subpart later on, call getTrianglesSubpartAt(int ).xxxx = yyyy.
		virtual void addTrianglesSubpart( const TrianglesSubpart& part );

			/// Adds a shape subpart. To modify member xxxx of this shape subpart later on, call getShapesSubpartAt(int ).xxxx = yyyy.
		virtual void addShapesSubpart( const ShapesSubpart& part );

			/// Returns the number of all triangle subparts.
		inline int getNumTrianglesSubparts() const;

			/// Returns the number of all shape subparts.
		inline int getNumShapesSubparts() const;

			/// Gets read/write access to a triangle subpart.
		inline TrianglesSubpart& getTrianglesSubpartAt( int i );

			/// Gets const access to a triangle subpart.
		inline const TrianglesSubpart& getTrianglesSubpartAt( int i ) const;

			/// Gets read/write access to a shape subpart.
		inline ShapesSubpart& getShapesSubpartAt( int i );

			/// Gets const access to a shape subpart.
		inline const ShapesSubpart& getShapesSubpartAt( int i ) const;

			/// Gets the subpart that a shape key belongs to.
		inline const Subpart& getSubPart( hkpShapeKey shapeKey ) const;

			/// Gets the index of a subpart that a shape key belongs to.
			/// Note that this will only return the index within the subpart's specific type list, i.e. either the index in the triangles list or in the shapes list.
			/// You can get the subpart's type by calling getSubpartType().
		HK_FORCE_INLINE hkInt32 getSubPartIndex( hkpShapeKey shapeKey ) const;

			/// Get the terminal shape's index within the subpart.
		HK_FORCE_INLINE hkInt32 getTerminalIndexInSubPart( hkpShapeKey key ) const;

			/// Gets the number of bits of a shape key used to encode the subpart.
		HK_FORCE_INLINE hkInt32 getNumBitsForSubpartIndex() const;

			/// Returns the first shape key of a subpart
		HK_FORCE_INLINE hkInt32 getSubpartShapeKeyBase( int subpartIndex) const;

			/// Get the type of the shape referenced by the supplied shape key.
		HK_FORCE_INLINE SubpartType getSubpartType( hkpShapeKey key ) const;

		//
		// Scaling and radius access
		//

			/// Set the scaling of all triangle subparts of the shape
		inline const hkVector4&	getScaling() const;

		void setScaling( const hkVector4& scaling ) ;


			/// Gets the extra radius for every triangle.
		inline hkReal getRadius() const;

			/// Sets the extra radius for every triangle.
		inline void setRadius(hkReal r );

		//
		// hkpShape Collection interface 
		//


			/// Get the first child shape key.
		virtual hkpShapeKey getFirstKey() const;

			/// This function implements hkpShapeCollection::getNextKey
			/// NOTE: This function calls hkpTriangleUtil::isDegenerate to make sure no keys for degenerate triangles are returned
			/// If you are implementing your own mesh shape, your getNextKey function must make sure that it similarly does
			/// not return keys for degenerate triangles. You can use the hkpTriangleUtil::isDegenerate utility function to check whether
			/// triangles are valid.
		virtual hkpShapeKey getNextKey( hkpShapeKey oldKey ) const;

		// Gets the mesh material by shape key, or returns HK_NULL if m_materialIndexBase isn't defined
		const hkpMeshMaterial* getMeshMaterial( hkpShapeKey key ) const;

		HK_SPU_VIRTUAL_DECLSPEC(const hkpShape*)	HK_GET_CHILD_SHAPE_FUNCTION;

		HK_SPU_VIRTUAL_DECLSPEC(hkUint32) HK_GET_COLLISION_FILTER_INFO_FUNCTION;

		//
		// hkpShape interface
		//
		// hkpShape interface implementation.
		HK_SPU_VIRTUAL_DECLSPEC(void) HK_GET_AABB_FUNCTION;
		
		void recalcAabbExtents();

		virtual void calcStatistics( hkStatisticsCollector* collector) const;

		virtual int calcSizeForSpu(const CalcSizeForSpuInput& input, int spuBufferSizeLeft) const;

#ifdef HK_PLATFORM_SPU

		static void HK_CALL registerSimulationFunctions( hkpShape::ShapeFuncs& sf );

		static void HK_CALL registerCollideQueryFunctions( hkpShape::ShapeFuncs& sf );

#endif

	protected:

		void assertTrianglesSubpartValidity( const TrianglesSubpart& part );
		void assertShapesSubpartValidity   ( const ShapesSubpart&    part );

	public:

		hkInt32								m_numBitsForSubpartIndex;

		hkVector4							m_scaling;
		
		// Cached AABB
		hkVector4							m_aabbHalfExtents;
		hkVector4							m_aabbCenter;

			/// This member variable to determine the maximum triangle size allowed.
			/// This defaults to 1e-7 (and is used to check against the triangle area squared). If you have algorithms
			/// that fail with triangles passed by this value, you can increase it to make the culling more aggressive.
		static hkReal m_triangleDengeneracyTolerance;

	protected:
		
			// On PS3 these must be aligned according to their size.
		struct TrianglesSubpart*			m_trianglesSubparts;	
		int									m_numTrianglesSubparts;

		struct ShapesSubpart*				m_shapesSubparts;		
		int									m_numShapesSubparts;

	public:

		hkArray<hkUint16> m_weldingInfo;
				
		hkEnum<hkpWeldingUtility::WeldingType, hkUint8> m_weldingType; // +default(hkpWeldingUtility::WELDING_TYPE_NONE)

	public:

		hkpExtendedMeshShape( hkFinishLoadedObjectFlag flag );

	protected:

			/// If this shape only contains a single triangle subpart and/or single shapes subpart then these
			/// Can be embedded directly into the shape. This saves significantly on DMA transfers
		struct TrianglesSubpart m_embeddedTrianglesSubpart;  

			/// Internal methods to help simplify the memory mgt involved when the subparts are embedded
		struct TrianglesSubpart* expandOneTriangleSubparts();
		struct ShapesSubpart*	 expandOneShapesSubparts();
		void freeTriangleSubparts();
		void freeShapesSubparts();

		/// The radius can only be set on construction.
		hkReal m_triangleRadius;
		int m_pad16[3]; //+serialized(false)
};


#include <Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.inl>


#endif // HK_COLLIDE2_EXTENDED_MESH2_SHAPE_H

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
