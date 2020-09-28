/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */
#ifndef HKSCENEDATA_GRAPH_HKXNODE_HKCLASS_H
#define HKSCENEDATA_GRAPH_HKXNODE_HKCLASS_H

#include <Common/SceneData/Attributes/hkxAttributeHolder.h>

/// hkxNode meta information
extern const class hkClass hkxNodeClass;

/// A node in a scene graph
class hkxNode : public hkxAttributeHolder
{
	public:

		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_SCENE_DATA, hkxNode );
		HK_DECLARE_REFLECTION();

			/// This structure describes a note track annotation ( text and time ).
			/// AnnotationData is DEPRICATED. Use hkxAttribute data, and look for animated strings types
		struct AnnotationData
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_SCENE_DATA, hkxNode::AnnotationData );
			HK_DECLARE_REFLECTION();

				/// annotation time stamp.
			hkReal m_time;
			
				/// A string description of the event
			char* m_description;
		};
				
		//
		// Members
		//
	public:
		
			/// Human readable name this object.
		char* m_name;
		
			/// The object at this node, if one. (mesh, skin, light, camera, etc.) Check class
			/// (m_class) of reflected to find out if not null.
		hkVariant m_object;
		
			/// Raw keyframe data
		hkMatrix4* m_keyFrames;
		hkInt32 m_numKeyFrames;
		
			/// The children of this node. This link forms the scene graph
		hkxNode** m_children;
		hkInt32 m_numChildren;
		
			/// Annotation Data for this node
		struct AnnotationData* m_annotations;
		hkInt32 m_numAnnotations;
		
			/// User data
		char* m_userProperties;

			/// Selection flag
		hkBool m_selected;

			/// Looks for the first child that matches the given name (case insensitive)
		hkxNode* findChildByName (const char* childName) const;

			/// Recursively looks for the first descendant with each name (case insensitive). This is done depth-first.
		hkxNode* findDescendantByName (const char* name) const;

			/// Constructs a path (parent-first list of nodes from this one to the node we search, both included)
			/// Returns HK_FAILURE if the node is not a descendant of this
		hkResult getPathToNode (const hkxNode* theNode, hkArray<const hkxNode*>& pathOut) const;

			/// Recursively counts the number of descendants
		int getNumDescendants () const;

			/// Search the node and its children for a hkVariant containing the specified object.
			/// This is done depth-first. Will return HK_NULL if not found.
		hkVariant* findVariantByObject( void* object );
};

#endif // HKSCENEDATA_GRAPH_HKXNODE_HKCLASS_H

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
