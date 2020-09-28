// system_maxplugin.h
// Plugin specific System class.
#pragma once
#ifndef SYSTEM_MAXPLUGIN
#define SYSTEM_MAXPLUGIN

#include "x86/win32/core/system_win32.h"

namespace gr 
{
	class PluginSystem : public Win32System
	{
	public:
		PluginSystem();
		virtual ~PluginSystem();
    			
		virtual void abort(void);
    
    virtual void message(MessageLevel level, const char* pMessage);

		// true to ignore
		virtual bool assertEvent(const char* Pfile, int line, const char* Pexpr);

		// true to ignore
		virtual bool verifyEvent(const char* Pfile, int line, const char* Pexpr);
	};

	extern PluginSystem& gPluginSystem;

} // namespace gr

#endif // SYSTEM_PLUGIN
