//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file
/// Wwise SDK utilities.

#ifndef _AK_WWISE_UTILITIES_H
#define _AK_WWISE_UTILITIES_H

#include <AK/SoundEngine/Common/AkTypes.h>

//////////////////////////////////////////////////////////////////////////
// Populate Table macros
//////////////////////////////////////////////////////////////////////////

/// Starts the declaration of a "populate table" which is used
/// to bind controls such as checkboxes and radio buttons to
/// properties of your plug-in.
///
/// \param theName The name of the populate table. It must be unique within the current scope.
///
/// \sa
/// - \ref wwiseplugin_dialog_guide_poptable
/// - AK_POP_ITEM()
/// - AK_END_POPULATE_TABLE()
#define AK_BEGIN_POPULATE_TABLE(theName) AK::Wwise::PopulateTableItem theName[] = {

/// Declares an association between a dialog control and a plug-in
/// property within a "populate table".
///
/// \param theID The resource ID of the control (checkbox or radio button)
/// \param theProp The name of the property, as defined in your plug-in's
///        XML definition file (refer to \ref wwiseplugin_xml_properties_tag)
/// \param theParams Options for the control (depend on the control type, see \ref wwiseplugin_dialog_guide_poptable)
///
/// \sa
/// - \ref wwiseplugin_dialog_guide_poptable
/// - \ref wwiseplugin_xml_properties_tag
/// - AK_BEGIN_POPULATE_TABLE()
/// - AK_END_POPULATE_TABLE()
#define AK_POP_ITEM(theID, theProp, theParams) {theID, theProp, theParams},

/// Ends the declaration of a "populate table".
///
/// \sa
///	- \ref wwiseplugin_dialog_guide_poptable
/// - AK_BEGIN_POPULATE_TABLE()
/// - AK_POP_ITEM()
#define AK_END_POPULATE_TABLE() AK_POP_ITEM(0, NULL, NULL) };

//////////////////////////////////////////////////////////////////////////
// Utilities
//////////////////////////////////////////////////////////////////////////

// Audiokinetic namespace
namespace AK
{
	// Audiokinetic Wwise namespace
	namespace Wwise
	{
		/// Represents the association between a dialog control (such as
		/// a checkbox or radio button) and a plug-in property.
		/// \aknote
		/// You should not need to use this structure directly. Instead, use the 
		/// AK_BEGIN_POPULATE_TABLE(), AK_POP_ITEM(), and AK_END_POPULATE_TABLE() macros.
		/// \endaknote
		/// \sa
		/// - \ref wwiseplugin_dialog_guide_poptable
		struct PopulateTableItem
		{
			UINT uiID;				///< The dialog control resource ID
			LPCWSTR pszProp;		///< The property name
			LPCWSTR pszParams;		///< Options for the control (depends on the control type)
		};

		/// Base interface for all Wwise plug-ins.
		/// \warning The functions in this interface are not thread-safe, unless stated otherwise.
		/// \sa
		/// - \ref AK::Wwise::IAudioPlugin
		class IPluginBase
		{
		public:
			/// This will be called to delete the plug-in. The object
			/// is responsible for deleting itself when this method
			/// is called.
			/// \sa
			/// - \ref wwiseplugin_destroy
			virtual void Destroy() = 0;
		};

        /// Interface used to write data that can be converted, if needed, for the target
		/// platform.
		/// \aknote
		/// All functions perform the appropriate platform-specific byte reordering
		/// except where noted otherwise.
		/// \endaknote
		/// \warning The functions in this interface are not thread-safe, unless stated otherwise.
		/// \sa
		/// - \ref wwiseplugin_bank
		/// - AK::Wwise::IAudioPlugin::GetBankParameters()
        class IWriteData
        {
        public:
            /// Writes a block of data.
			/// \akcaution This data will always be written as-is, with no
			///            platform-specific conversion. \endakcaution
			/// \return True if all the data could be written, False otherwise
            virtual bool WriteData(
				LPCVOID in_pData,			///< A pointer to the buffer containing the data to be written
				UINT32 in_cBytes,			///< The number of bytes to write
				UINT32 & out_cWritten		///< The number of bytes actually written
			) = 0;

			/// Writes a string. If wide characters (such as Unicode) are supported on the target
			/// platform, the string is left intact. Otherwise, it is converted to Ansi.
			/// \return True if successful, False otherwise
            virtual bool WriteString(
				LPCWSTR in_szString,		///< The string to be written
				UINT32 in_uiStringLength	///< The string length, in number of characters
			) = 0;

			/// Writes a boolean value.
			/// \return True if successful, False otherwise
            virtual bool WriteBool(
				bool in_bBool				///< Value to be written
			) = 0;

			/// Writes a byte value.
			/// \return True if successful, False otherwise
            virtual bool WriteByte(
				BYTE in_bByte				///< Value to be written
			) = 0;

			/// Writes a 16-bit integer.
			/// \return True if successful, False otherwise
            virtual bool WriteInt16(
				UINT16 in_uiInt16			///< Value to be written
			) = 0;

			/// Writes a 32-bit integer.
			/// \return True if successful, False otherwise
            virtual bool WriteInt32(
				UINT32 in_uiInt32			///< Value to be written
			) = 0;

			/// Writes a 64-bit integer.
			/// \return True if successful, False otherwise
            virtual bool WriteInt64(
				UINT64 in_uiInt64			///< Value to be written
			) = 0;

			/// Writes a 32-bit, single-precision floating point value.
			/// \return True if successful, False otherwise
            virtual bool WriteReal32(
				float in_fReal32			///< Value to be written
			) = 0;

			/// Writes a 64-bit, double-precision floating point value.
			/// \return True if successful, False otherwise
            virtual bool WriteReal64(
				double in_dblReal64			///< Value to be written
			) = 0;
        };
	}
}

#endif // _WWISE_UTILITIES_H
