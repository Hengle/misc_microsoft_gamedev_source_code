//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file
/// Wwise source control plug-in interface, used to implement the source control plug-in.

#ifndef _AK_WWISE_ISOURCECONTROL_H
#define _AK_WWISE_ISOURCECONTROL_H

// Include the header file that defines the BSTR type.
#include <wtypes.h>

#include "ISourceControlUtilities.h"
#include "SourceControlContainers.h"

// Audiokinetic namespace
namespace AK
{
	// Audiokinetic Wwise namespace
	namespace Wwise
	{
		/// This class contains static constants that can be useful to the plug-in.
		class SourceControlConstant
		{
		public: 
			/// Maximum length that a work unit name can be
			static const unsigned int s_uiMaxWorkUnitName = 128;
			/// Invalid operation ID (MUST NOT BE USED as an operation ID in OperationListItem)
			static const DWORD s_dwInvalidOperationID = (DWORD)-1;
		};

		/// Wwise source control plug-in interface. This is the interface that the plug-in must implement. It contains
		/// all the necessary functions to perform source control operations and manage the Wwise source control UI.
		/// \warning The functions in this interface are not thread-safe, unless stated otherwise.
		/// \sa
		/// - \ref source_control_dll_creation_object_information
		class ISourceControl
		{
		public:	
			
			/// \name Enumeration types
			//@{

			/// Operation result. Some interface functions need to return the result of the operation. This is used
			/// by Wwise to manage various errors.
			enum OperationResult
			{
				OperationResult_Succeed = 0,	///< The operation succeeded
				OperationResult_Failed,			///< The operation failed
				OperationResult_NotImplemented	///< The operation is not implemented
			};

			/// Menu type. The operation list may vary depending on the location where a menu containing operations 
			/// needs to be displayed.
			enum OperationMenuType
			{
				OperationMenuType_WorkUnits = 0,///< The menu is displayed in the Workgroup Manager's 'Work Units' tab
				OperationMenuType_Sources,		///< The menu is displayed in the Workgroup Manager's 'Sources' tab
				OperationMenuType_Explorer		///< The menu is displayed in the Project Explorer
			};

			/// Pre/PostCreateOrModify Operation flags. These flags represent the operation(s) performed on files.
			enum CreateOrModifyOperation
			{
				CreateOrModifyOperation_Create = 1 << 0,	///< Files will be created during the operation
				CreateOrModifyOperation_Modify = 1 << 1,	///< Files will be modified during the operation
			};

			/// The operation's effect on the file(s) involved.
			enum OperationEffect
			{
				OperationEffect_LocalContentModification = 1 << 0,	///< The operation will modify the local content of the file
				OperationEffect_ServerContentModification = 1 << 1,	///< The operation will modify the remote content (on the server) of the file
			};

			//@}

			/// 'Filename to Status' map item. This is the type used in the AK::Wwise::ISourceControl::FilenameToStatusMap 
			/// SourceControlContainers::IAkMap template parameter structure.
			struct FilenameToStatusMapItem
			{
				BSTR m_bstrStatus;				///< Text displayed in the Workgroup Manager's 'Status' column
				BSTR m_bstrOwner;				///< Text displayed in the Workgroup Manager's 'Owners' column
			};

			/// Operation list item. This is the type used in the AK::Wwise::ISourceControl::OperationList SourceControlContainers::IAkList template class.
			struct OperationListItem
			{
				DWORD m_dwOperationID;			///< The operation ID
				LPCWSTR m_pszOperationName;		///< The name displayed in the menu
				bool m_bEnabled;				///< True: the operation is enabled in the menu, False: the operation is disabled (grayed out) in the menu
			};

			/// FilenameToIconMap item. This is the type used to display the file status icon and tool tip text 
			/// in the Project Explorer.
			struct FilenameToIconMapItem
			{
				HICON m_hIcon;					///< A handle to an icon that will be displayed in the Project Explorer
				BSTR m_bstrToolTip;				///< The tool tip text that will be displayed when the user mouses over the icon
			};

			/// \name List types
			//@{

			/// String List. When Wwise needs to pass a file name list, it gives this container to the plug-in.
			/// \sa
			/// - AK::Wwise::SourceControlContainers::IAkList
			typedef SourceControlContainers::IAkList<LPCWSTR, LPCWSTR> StringList;
			
			/// Plug-in ID list. When Wwise needs to have the list of plug-ins that a DLL contains, it requests
			/// the list of plug-in IDs using a function exported by the DLL.
			typedef SourceControlContainers::IAkList<GUID> PluginIDList;

			/// When Wwise needs to have the list of operations that are available in a certain context, it requests
			/// the list of operations using this list type. The contexts are determined by the AK::Wwise::ISourceControl::OperationMenuType
			/// enumeration type.
			/// \sa
			/// - AK::Wwise::ISourceControl::OperationListItem
			/// - AK::Wwise::SourceControlContainers::IAkList
			typedef SourceControlContainers::IAkList<OperationListItem> OperationList;

			//@}

			/// \name Map types
			//@{

			/// The AK:Wwise::ISourceControl interface offers a way to display custom icons in the Project Explorer. This map
			/// type must be filled in by the plug-in when Wwise gives it a file name list. CString objects are used as keys, and are associated
			/// to FilenameToIconMapItem objects. The HICON m_hIcon member will be NULL when there is no icon associated with the file.
			/// \sa
			/// - AK::Wwise::SourceControlContainers::IAkMap
			typedef SourceControlContainers::IAkMap<LPCWSTR, LPCWSTR, FilenameToIconMapItem, const FilenameToIconMapItem&> FilenameToIconMap;
		
			/// When the Workgroup Manager needs to fill in the 'Status' and 'Owners' columns of work units or source lists,
			/// the plug-in needs to fill in this map with the corresponding text. File names are used as keys, and are associated
			/// to the text to be displayed in the 'Status' and 'Owners' columns.
			/// \sa
			/// - AK::Wwise::ISourceControl::FilenameToStatusMapItem
			/// - AK::Wwise::SourceControlContainers::IAkMap
			typedef SourceControlContainers::IAkMap<LPCWSTR, LPCWSTR, FilenameToStatusMapItem, const FilenameToStatusMapItem&> FilenameToStatusMap;

			//@}

			/// Plug-in information structure. This structure gives a simple overview of the plug-in's capabilities.
			class PluginInfo
			{
			public:
				BSTR m_bstrName;				///< The name of the plug-in displayed in the Project Settings plug-in list
				unsigned int m_uiVersion;		///< The current version of the plug-in

				bool m_bShowConfigDlgAvailable;	///< Used to enable/disable the 'Config...' button in the Project Settings
				bool m_bUpdateAllAvailable;		///< Indicates that the plug-in implements the AK::Wwise::ISourceControl::UpdateAll() function
				bool m_bStatusIconAvailable;	///< Indicates that the plug-in supports Project Explorer custom icons

				BSTR m_bstrUpdateAllName;		///< The text displayed for the user when referring to the 
												///< AK::Wwise::ISourceControl::UpdateAll() function. The 
												///< 'Update' operation has different names in different source control systems. 
												///< For example, Perforce uses 'Get Latest Version' while Subversion and CVS use 'Update'. 
			};

			/// This function is called when the plug-in is initialized after its creation.
			virtual void Init( 
				AK::Wwise::ISourceControlUtilities* in_pUtilities	///< A pointer to the utilities class. The interface is not 
																	///< destroyed while an instance of the plug-in exists.
				) = 0;

			/// This function is called when the plug-in is terminated before its destruction.
			virtual void Term() = 0;

			/// This function destroys the plug-in. The implementation is generally '{ delete this; }'.
			virtual void Destroy() = 0;

			/// This function is called when the user clicks the 'Config...' button in the Project Settings.
			/// \return True if the user accepts the configuration, False otherwise
			/// \sa
			/// - AK::Wwise::ISourceControl::PluginInfo::m_bShowConfigDlgAvailable
			virtual bool ShowConfigDlg() = 0;

			/// Gets the operation list to be displayed in a menu.
			virtual void GetOperationList( 
				OperationMenuType in_menuType,		///< The type of menu where the operation list will be displayed
				const StringList& in_rFilenameList,	///< The file name list for which Wwise needs to get the operation list
				OperationList& out_rOperationList	///< The returned operation list available in this context
				) = 0;

			/// Gets the operation effect on the file(s) involved in the operation.
			// \return A mask of all the applicable OperationEffect enum values
			virtual DWORD GetOperationEffect(
				DWORD in_dwOperationID	///< The ID of the operation, as specified in OperationListItem
				) = 0;

			/// Gets the text to be displayed in the 'Status' and 'Owners' columns of the Workgroup Manager.
			/// \return The result of the operation
			virtual AK::Wwise::ISourceControl::OperationResult GetFileStatus( 
				const StringList& in_rFilenameList,		///< A list of the file names for which Wwise needs to get the status
				FilenameToStatusMap& out_rFileStatusMap ///< The returned 'Filename To Status' map
				) = 0;

			/// In a similar way to AK::Wwise::ISourceControl::GetFileStatus(), this function gets the icons to be displayed in the
			/// Project Explorer.
			/// \return The result of the operation
			virtual AK::Wwise::ISourceControl::OperationResult GetFileStatusIcons( 
				const StringList& in_rFilenameList,		///< A list of the file names for which Wwise needs to get the icons
				FilenameToIconMap& out_rFileIconsMap	///< The returned 'Filename To Icons' map
				) = 0;

			/// Gets the files that should be displayed in the Workgroup Manager file list, but that are not on the local disk.
			/// Deleted files that need to be submitted to the server are an example of implementation.
			/// \return The result of the operation
			virtual AK::Wwise::ISourceControl::OperationResult GetMissingFilesInDirectories( 
				const StringList& in_rDirectoryList,	///< A list of directories in which Wwise needs to get missing files
				StringList& out_rFilenameList			///< The returned missing files
				) = 0;

			/// Performs an operation on files. This function is called when the user clicks on a source control operation
			/// in a menu.
			virtual void DoOperation( 
				DWORD in_dwOperationID,				///< The ID of the operation that the user selected from the menu
				const StringList& in_rFilenameList	///< A list of the names of the files that the user selected in the Workgroup Manager or in the
													///< Project Explorer.
				) = 0;

			/// Performs the 'Update All' operation. This function is called when the user clicks on menu items from the 'Update All'
			/// menu in the Workgroup Manager. It is also called when the user updates sources.
			virtual void UpdateAll( 
				const StringList& in_rPathList		///< The path list to update. Can be directories or file names.
				) = 0;

			/// This method is called when the user adds files to the project (Work Units or Sources), saves the project, 
			/// or triggers any call to a Wwise operation that could alter source control files. It is called before Wwise performs
			/// the operation and is always followed by a call to PostCreateOrModify.
			/// \return The result of the operation	
			virtual AK::Wwise::ISourceControl::OperationResult PreCreateOrModify(
				const StringList& in_rFilenameList,		///< A list of the names of the files that are to be added (some files may already exist)
				CreateOrModifyOperation in_eOperation,	///< The operation(s) that will be performed on these files
				bool& out_rContinue						///< A returned flag that indicates if Wwise is continuing the current operation
				) = 0;

			/// This method is called when the user adds files to the project (Work Units or Sources), saves the project, 
			/// or triggers any call to a Wwise operation that could alter source control files. It is called after Wwise performs 
			/// the operation and is always preceded by a call to PreCreateOrModify.
			/// \return The result of the operation
			virtual AK::Wwise::ISourceControl::OperationResult PostCreateOrModify(
				const StringList& in_rFilenameList,		///< A list of the names of the files that are to be added (Some files may already exist)
				CreateOrModifyOperation in_eOperation,	///< The operation(s) that will be performed on these files
				bool& out_rContinue						///< A returned flag that indicates if Wwise is continuing the current operation
				) = 0;

			//@}

			/// \name Exported functions prototypes
			//@{

			/// Gets the plug-in ID list contained by the DLL file.
			typedef void (__stdcall* GetSourceControlIDListFuncPtr)( 
				PluginIDList& out_rPluginIDList		///< The List of plug-in IDs
				);

			/// Gets the AK::Wwise::ISourceControl::PluginInfo class associated with a given plug-in ID.
			typedef void (__stdcall* GetSourceControlPluginInfoFuncPtr)( 
				const GUID& in_rguidPluginID,		///< The ID of the plug-in
				PluginInfo& out_rPluginInfo			///< The returned plug-in info
				);

			/// Gets an instance of a plug-in.
			/// \return A pointer to an AK::Wwise::ISourceControl instance
			typedef ISourceControl* (__stdcall* GetSourceControlInstanceFuncPtr)( 
				const GUID& in_guidPluginID			///< The requested plug-in ID
				);
		};
	}
}

#endif // _AK_WWISE_ISOURCECONTROL_H