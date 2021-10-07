#pragma once

#include "Core.h"

#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace GLCore {

	class Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetLogger() { return s_Logger; }
	private:
		static std::shared_ptr<spdlog::logger> s_Logger;
	};

}

// Client log macros
#define LOG_TRACE(...)         ::GLCore::Log::GetLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...)          ::GLCore::Log::GetLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)          ::GLCore::Log::GetLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)         ::GLCore::Log::GetLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...)      ::GLCore::Log::GetLogger()->critical(__VA_ARGS__)


#include <filesystem>
#ifdef GLCORE_ENABLE_ASSERTS

	// Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
	// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
	#define LOG_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { LOG##type##ERROR(msg, __VA_ARGS__); GLCORE_DEBUGBREAK(); } }
	#define LOG_INTERNAL_ASSERT_WITH_MSG(type, check, ...) LOG_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define LOG_INTERNAL_ASSERT_NO_MSG(type, check) LOG_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", GLCORE_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)
	
	#define LOG_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define LOG_INTERNAL_ASSERT_GET_MACRO(...) GLCORE_EXPAND_MACRO( LOG_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, LOG_INTERNAL_ASSERT_WITH_MSG, LOG_INTERNAL_ASSERT_NO_MSG) )
	
	// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define LOG_ASSERT(...) GLCORE_EXPAND_MACRO( LOG_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
#else
	#define LOG_ASSERT(...)
#endif