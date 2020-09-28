/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */
#ifndef HK_SERIALIZE_XML_OBJECT_READER_H
#define HK_SERIALIZE_XML_OBJECT_READER_H

#include <Common/Serialize/Serialize/hkObjectReader.h>

class hkStreamReader;
class hkXmlParser;

/// Reads a single object from an XML stream.
class hkXmlObjectReader : public hkObjectReader
{
	public:

			/// Create an hkXmlObject.
		hkXmlObjectReader(hkXmlParser* parser);

			///
		~hkXmlObjectReader();

			/// Method inherited from hkObjectReader.
			/// See also an alternative convenience interface
			/// to readObject which takes an expandable buffer.
		virtual int readObject( hkStreamReader* reader, void* buf, int bufLen, const hkClass& klass, hkRelocationInfo& reloc );

			/// Read an object into buf, expanding buffer as necessary.
		hkResult readObject( hkStreamReader* reader, hkArray<char>& buf, const hkClass& klass, hkRelocationInfo& reloc );

			/// Read a raw binary chunk. (base64 encoded in a CDATA section)
		virtual hkResult readRaw( hkStreamReader* reader, void* buf, int bufLen );

	public:

			/// Decode 'sr' into 'buf'.
		static hkResult HK_CALL base64read( hkStreamReader* sr, void* buf, int decodedLength );

	protected:

		hkXmlParser* m_parser;
};

#endif // HK_SERIALIZE_XML_OBJECT_READER_H

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
