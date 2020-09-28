// system_maxplugin.cpp
// Plugin specific System class.
#include "system_maxplugin.h"
#include "x86/win32/core/win_hdr.h"

namespace gr 
{
	PluginSystem::PluginSystem() : Win32System()
	{
	}
	
	PluginSystem::~PluginSystem()
	{
	}
    			
	void PluginSystem::abort(void)
	{
		throw "Shared library terminated!";
	}
    
  void PluginSystem::message(MessageLevel level, const char* pMessage)
  {
		const size_t len = strlen(pMessage);
		
		bool crlf = true;
		
		if (len)
		{
			if (pMessage[len - 1] == '\n')
				crlf = false;
		}

		char buf[1024];
		_snprintf(buf, sizeof(buf), "[%i]%s", level, pMessage);
		
		if (crlf)
			strcat(buf, "\n");

		OutputDebugString(buf);
  };

	// true to ignore
	bool PluginSystem::assertEvent(const char* Pfile, int line, const char* Pexpr)
	{
		if (debuggerPresent())
			return false;
		
		char buf[1024];
		_snprintf(buf, sizeof(buf), 
			"%s(%i): Assert() failed! Expression: %s",
			Pfile, line, Pexpr);

		message(System::ASSERT, buf);
		
		abort();

		return false;
	}

	// true to ignore
	bool PluginSystem::verifyEvent(const char* Pfile, int line, const char* Pexpr)
	{
		if (debuggerPresent())
			return false;

		char buf[1024];
		
		_snprintf(buf, sizeof(buf), 
			"%s(%i): Verify() failed! Expression: %s",
			Pfile, line, Pexpr);

		message(System::VERIFY, buf);
		
		abort();

		return false;
	}
  
  PluginSystem gPluginSystemObject;

	System& gSystem = gPluginSystemObject;
	Win32System& gWin32System = gPluginSystemObject;
	PluginSystem& gPluginSystem = gPluginSystemObject;

} // namespace gr

