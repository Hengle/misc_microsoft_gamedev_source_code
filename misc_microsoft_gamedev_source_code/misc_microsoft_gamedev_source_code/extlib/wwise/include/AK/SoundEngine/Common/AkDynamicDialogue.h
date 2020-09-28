#ifndef _AK_SOUNDENGINE_AKDYNAMICDIALOGUE_H
#define _AK_SOUNDENGINE_AKDYNAMICDIALOGUE_H

#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace AK
{
	namespace SoundEngine
	{
		/// Dynamic Dialogue namespace
		/// \remarks The functions in this namespace are thread-safe, unless stated otherwise.
		namespace DynamicDialogue
		{
			/// Resolve dialogue event into an audio node ID based on specified argument path.
	        /// \return Unique ID of audio node, or AK_INVALID_UNIQUE_ID if no audio node is defined for specified argument path
			extern AKSOUNDENGINE_API AkUniqueID ResolveDialogueEvent(
					AkUniqueID			in_eventID,					///< Unique ID of dialogue event
					AkArgumentValueID*	in_aArgumentValues,			///< Argument path, as array of argument value IDs. AK_FALLBACK_ARGUMENTVALUE_ID indicates a fallback argument value
					AkUInt32			in_uNumArguments			///< Number of argument value IDs in in_aArgumentValues
				);

			/// Resolve a dialogue event into an audio node ID based on a specified argument path.
	        /// \return audio node, or AK_INVALID_UNIQUE_ID if no audio node is defined for specified argument path
			extern AKSOUNDENGINE_API AkUniqueID ResolveDialogueEvent(
					AkLpCtstr			in_pszEventName,			///< Name of dialogue event
					AkLpCtstr*			in_aArgumentValueNames,		///< Argument path, as array of argument value names. L"" indicates a fallback argument value
					AkUInt32			in_uNumArguments			///< Number of argument value names in in_aArgumentValueNames
				);
		}
	}
}

#endif // _AK_SOUNDENGINE_AKDYNAMICDIALOGUE_H
