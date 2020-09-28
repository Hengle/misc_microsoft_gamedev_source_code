//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file
/// Wwise audio plug-in interface, used to implement the Wwise side of a source or effect plug-in.

#ifndef _AK_WWISE_AUDIOPLUGIN_H
#define _AK_WWISE_AUDIOPLUGIN_H

#include <AK/Wwise/Utilities.h>

// Audiokinetic namespace
namespace AK
{
	class IXmlTextReader;
	class IXmlTextWriter;

	// Audiokinetic Wwise namespace
	namespace Wwise
	{
		/// Plug-in property set interface. An instance of this class is created and
		/// assigned to each plug-in, which in turn can use it to manage its properties.
		/// Whenever a property name is specified, it corresponds to the property
		/// name set in the plug-in's XML definition file.
		/// \warning The functions in this interface are not thread-safe, unless stated otherwise.
		/// \sa
		/// - \ref wwiseplugin_xml_properties_tag
		/// - AK::Wwise::IAudioPlugin::SetPluginPropertySet()
		/// - \ref wwiseplugin_propertyset
		class IPluginPropertySet
		{
		public:
			/// This function is called by Wwise to get the value of a property for a specified platform.
			/// \return True if successful, False otherwise
			/// \sa
			/// - \ref wwiseplugin_bank
			virtual bool GetValue( 
				const GUID & in_guidPlatform,	///< The unique ID of the queried platform
				LPCWSTR in_pszPropertyName,		///< The name of the property
				VARIANT & out_varProperty		///< The returned value of the property
				) = 0;
			
			/// This function is called by Wwise to set the value of a property for a specified platform.
			/// \return True if successful, False otherwise.
			virtual bool SetValue( 
				const GUID & in_guidPlatform,	///< The unique ID of the platform to modify
				LPCWSTR in_pszPropertyName,		///< The name of the property
				const VARIANT & in_varProperty	///< The value to set
				) = 0;
			
			/// This function is called by Wwise to get the current platform's identifier. 
			/// This can be passed to any function that has a parameter
			/// for a platform ID, such as GetValue() or SetValue(), when you want to make
			/// the call for the currently active platform.
			/// \return The unique ID of the current platform
			virtual GUID GetCurrentPlatform() = 0;

			/// Use this function to tell Wwise that something other than properties 
			/// has changed within the plugin.  This will set the plugin dirty (for save)
			/// and GetPluginData will be called when the plugin is about to play in Wwise, to
			/// transfer the internal data to the Sound Engine part of the plugin.
			/// Use ALL_PLUGIN_DATA_ID to tell that all the data has to be refreshed.
			virtual void NotifyInternalDataChanged(AkPluginParamID in_idData) = 0;

			/// Call this function when you are about to log an undo event to know if Wwise is 
			/// in a state where undos are enabled.  Undo logging can be disabled for a particular
			/// plugin object if it already lives in the undo stack or in the clipboard.
			virtual bool CanLogUndos() = 0;
		};

		/// Wwise plug-in interface. This must be implemented for each source or
		/// effect plug-in that is exposed in Wwise.
		/// \warning The functions in this interface are not thread-safe, unless stated otherwise.
		/// \sa
		/// - \ref wwiseplugin_object
		class IAudioPlugin
			: public IPluginBase
		{
		public:
			/// Dialog type. Source plug-ins can be edited in the Property Editor or
			/// the Contents Editor, while effect plug-ins can only be edited in the
			/// Effect Editor.
			/// \sa
			/// - \ref wwiseplugin_dialogcode
			enum eDialog
			{
				SettingsDialog,			///< Main plug-in dialog. This is the dialog used in the Property
										///< Editor for source plug-ins, and in the Effect Editor for
										///< effect plug-ins.
				ContentsEditorDialog	///< Contents Editor dialog. This is the small dialog used in the
										///< Contents Editor for source plug-ins.
			};

			/// The property set interface is given to the plug-in through this method. It is called by Wwise during
			/// initialization of the plug-in, before most other calls.
			/// \warning This function is guaranteed to be called by a single thread at a time.
			/// \sa
			/// - \ref wwiseplugin_propertyset
			virtual void SetPluginPropertySet( 
				IPluginPropertySet * in_pPSet	///< A pointer to the property set interface
				) = 0;

			/// This function is called by Wwise to determine if the plug-in is in a playable state.
			/// \warning This function is guaranteed to be called by a single thread at a time.
			/// \return True if the plug-in is in a playable state, False otherwise
			virtual bool IsPlayable() const = 0;

			/// Initialize custom data to default values. This is called by Wwise after SetPluginPropertySet() 
			/// when creating a new instance of the plug-in (i.e. not during a load). The properties on the
			/// PropertySet do not need to be initialized in this method.
			/// \warning This function is guaranteed to be called by a single thread at a time.
			virtual void InitToDefault() = 0;

			/// Delete function called when the user press "delete" button on a plugin. This entry point must 
			/// set the undo/redo action properly. 
			/// \warning This function is guaranteed to be called by a single thread at a time.
			virtual void Delete() = 0;

			/// Load file 
			/// \return \b true if load succeeded.
			virtual bool Load( IXmlTextReader* in_pReader ) = 0;

			/// Save file
			/// \return \b true if save succeeded.
			virtual bool Save( IXmlTextWriter* in_pWriter ) = 0;

			/// Copy the plugin's custom data into another instance of the same plugin. This is used
			/// during copy/paste and delete. The properties on the PropertySet do not need to
			/// be copied in this method.
			/// \warning This function is guaranteed to be called by a single thread at a time.
			virtual bool CopyInto(
				IAudioPlugin* io_pWObject		 // The object that will receive the custom data of this object.
				) const = 0;

			/// This function is called by Wwise when the current platform changes.
			/// \warning This function is guaranteed to be called by a single thread at a time.
			/// \sa
			/// - \ref wwiseplugin_platformchange
			virtual void NotifyCurrentPlatformChanged( 
				const GUID & in_guidCurrentPlatform		///< The unique ID of the new platform
				) = 0;

			/// This function is called by Wwise when a plug-in property changes (for example, 
			/// through interaction with a UI control bound to a property, or through undo/redo operations).
			/// \warning This function is guaranteed to be called by a single thread at a time.
			/// \sa
			/// - \ref wwiseplugin_propertychange
			virtual void NotifyPropertyChanged( 
				const GUID & in_guidPlatform,	///< The unique ID of the queried platform
				LPCWSTR in_pszPropertyName		///< The name of the property
				) = 0;

            /// This function is called by Wwise to obtain parameters that will be written to a bank. 
			/// Because these can be changed at run-time, the parameter block should stay relatively small. 
			/// Larger data should be put in the Data Block.
			/// \warning This function is guaranteed to be called by a single thread at a time.
            /// \return True if the plug-in put some parameters in the bank, False otherwise
			/// \sa
			/// - \ref wwiseplugin_bank
			/// - \ref wwiseplugin_propertyset
            virtual bool GetBankParameters( 
				const GUID & in_guidPlatform,	///< The unique ID of the queried platform
				IWriteData* in_pDataWriter		///< A pointer to the data writer interface
				) const = 0;

			/// This function is called by Wwise to obtain parameters that will be sent to the 
			/// sound engine when Wwise is connected.  This block should contain only data
			/// that is NOT a property defined in the plugin xml file.  The parameter ID
			/// should be something different than the ones used in the plugin xml.
			/// \warning This function is guaranteed to be called by a single thread at a time.
			/// \return True if the plug-in has some plugin-defined data.  False otherwise.
			/// \sa
			/// - AK::Wwise::IPluginPropertySet::NotifyInternalDataChanged
			/// - AK::IAkPluginParam::ALL_PLUGIN_DATA_ID
			/// - AK::IAkPluginParam::SetParam
			virtual bool GetPluginData(
				const GUID & in_guidPlatform,		///< The unique ID of the queried platform
				AkPluginParamID in_idParam,	///< The plugin-defined parameter ID
				IWriteData* in_pDataWriter			///< A pointer to the data writer interface
				) const = 0;

			/// This function is called by Wwise to get the plug-in's HINSTANCE used for loading resources.
			/// \warning This function is guaranteed to be called by a single thread at a time.
			/// \return A handle to the instance of the plug-in DLL
			/// \sa
			/// - \ref wwiseplugin_dialogcode
			virtual HINSTANCE GetResourceHandle() const = 0;

			/// This function is called by Wwise to get the plug-in dialog parameters.
			/// \warning This function is guaranteed to be called by a single thread at a time.
			/// \return True if a dialog was returned, False otherwise
			/// \sa
			/// - \ref wwiseplugin_dialogcode
			/// - \ref wwiseplugin_dialog_guide
			virtual bool GetDialog( 
				eDialog in_eDialog,				///< The dialog type
				UINT & out_uiDialogID,			///< The returned resource ID of the dialog
				PopulateTableItem *& out_pTable	///< The returned table of property-control bindings (can be NULL)
				) const = 0;

			/// Window message handler for dialogs. This is very similar to a standard WIN32 window procedure.
			/// \warning This function is guaranteed to be called by a single thread at a time.
			/// \return True if the message has been processed by the plug-in, False otherwise
			/// \sa
			/// - \ref wwiseplugin_dialogcode
			virtual bool WindowProc( 
				eDialog in_eDialog,		///< The dialog type
				HWND in_hWnd,			///< The window handle of the dialog
				UINT in_message,		///< The incoming message. This is a standard Windows message ID (ex. WM_PAINT).
				WPARAM in_wParam,		///< The WPARAM of the message (see MSDN)
				LPARAM in_lParam,		///< The LPARAM of the message (see MSDN)
				LRESULT & out_lResult 	///< The returned value if the message has been processed (it is only considered if the method also returns True)
				) = 0;

			/// This function is called by Wwise to get the user-friendly name of the specified property.
			/// This function should write the user-friendly name of
			/// the specified property to the WCHAR buffer out_pszDisplayName,
			/// which is of length in_unCharCount.
			/// \warning This function is guaranteed to be called by a single thread at a time.
			/// \return True if the property has a user-friendly name, False otherwise
			/// \sa
			/// - \ref wwiseplugin_displaynames
			virtual bool DisplayNameForProp( 
				LPCWSTR in_pszPropertyName,		///< The internal name of the property
				LPWSTR out_pszDisplayName,		///< The returned user-friendly name
				UINT in_unCharCount				///< The number of WCHAR in the buffer, including the terminating NULL
				) const = 0;

			/// This function is called by Wwise to get the user-friendly names of possible values for the 
			/// specified property.
			/// This function should write pairs of value and text for the specified property to
			/// the WCHAR buffer out_pszDisplayName, which is of length in_unCharCount.
			/// Pairs are separated by commas, and each pair contains the value and the
			/// text, separated by a colon. Here are a few examples:
			/// - Numeric property: "-100:Left,0:Center,100:Right"
			/// - Boolean property: "0:Off,1:On"
			/// - Numeric property seen as an enumeration: "0:Low Pass,1:High Pass,2:Band Pass,3:Notch,4:Low Shelf,5:High Shelf,6:Peaking"
			///
			/// \warning This function is guaranteed to be called by a single thread at a time.
			/// \return True if the property has user-friendly names for some values, False otherwise
			/// \sa
			/// - \ref wwiseplugin_displaynames
			virtual bool DisplayNamesForPropValues( 
				LPCWSTR in_pszPropertyName,		///< The internal name of the property
				LPWSTR out_pszValuesName,		///< The returned property value names
				UINT in_unCharCount				///< The number of WCHAR in the buffer, including the terminating NULL character
				) const = 0;

			/// Called when the user clicks on the '?' icon.
			/// \warning This function is guaranteed to be called by a single thread at a time.
			/// \return True if the plug-in handled the help request, false otherwise
			/// \sa
			/// - \ref wwiseplugin_help
			virtual bool Help( 
				HWND in_hWnd,					///< The handle of the dialog
				eDialog in_eDialog				///< The dialog type
				) const = 0;
		};
	}
}

#endif // _AK_WWISE_AUDIOPLUGIN_H
