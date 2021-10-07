#include "pch.h"
#include "Window.h"

#ifdef GLCORE_PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsWindow.h"
#endif

namespace GLCore
{
	Window *Window::Create (const WindowProps &props)
	{
	#ifdef GLCORE_PLATFORM_WINDOWS
		return new WindowsWindow (props);
	#else
		LOG_ASSERT (false, "Unknown platform");
		return nullptr;
	#endif
	}
}