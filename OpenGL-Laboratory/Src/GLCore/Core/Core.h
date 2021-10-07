// Mostly from Hazel
#pragma once

#include <memory>

//////////
// PLAT-FORM DETECTION
// Platform detection using predefined macros
#ifdef _WIN32
	/* Windows x64/x86 */
	#ifdef _WIN64
		/* Windows x64  */
		#define GLCORE_PLATFORM_WINDOWS
	#else
		/* Windows x86 */
		#error "x86 Builds are not supported!"
	#endif
#elif defined(__APPLE__) || defined(__MACH__)
	#include <TargetConditionals.h>
	/* TARGET_OS_MAC exists on all the platforms
	 * so we must check all of them (in this order)
	 * to ensure that we're running on MAC
	 * and not some other Apple platform */
	#if TARGET_IPHONE_SIMULATOR == 1
		#error "IOS simulator is not supported!"
	#elif TARGET_OS_IPHONE == 1
		#define GLCORE_PLATFORM_IOS
		#error "IOS is not supported!"
	#elif TARGET_OS_MAC == 1
		#define GLCORE_PLATFORM_MACOS
		#error "MacOS is not supported!"
	#else
		#error "Unknown Apple platform!"
	#endif
/* We also have to check __ANDROID__ before __linux__
 * since android is based on the linux kernel
 * it has __linux__ defined */
#elif defined(__ANDROID__)
	#define GLCORE_PLATFORM_ANDROID
	#error "Android is not supported!"
#elif defined(__linux__)
	#define GLCORE_PLATFORM_LINUX
	#error "Linux is not supported!"
#else
	/* Unknown compiler/platform */
	#error "Unknown platform!"
#endif // End of platform detection



#ifdef MODE_DEBUG
	#if defined(GLCORE_PLATFORM_WINDOWS)
		#define GLCORE_DEBUGBREAK() __debugbreak()
	#elif defined(GLCORE_PLATFORM_LINUX)
		#include <signal.h>
		#define GLCORE_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define GLCORE_ENABLE_ASSERTS
#else
#define GLCORE_DEBUGBREAK()
#endif

#define GLCORE_EXPAND_MACRO(x) x
#define GLCORE_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define GLCORE_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

#include <array>
#include <type_traits>
#include <utility>
namespace my_std
{
	namespace Internal
	{
		template<std::size_t size, typename T, std::size_t... indexes>
		constexpr auto make_array_n_impl (T &&value, std::index_sequence<indexes...>)
		{
			return std::array<std::decay_t<T>, size>{(static_cast<void>(indexes), value)..., std::forward<T> (value)};
		}

		template<std::size_t size, typename T>
		constexpr auto make_array_n (std::integral_constant<std::size_t, 0>, T &&)
		{
			return std::array<std::decay_t<T>, 0>{};
		}
		template<std::size_t size, typename T>
		constexpr auto make_array_n (std::integral_constant<std::size_t, size>, T &&value)
		{
			return make_array_n_impl<size> (std::forward<T> (value), std::make_index_sequence<size - 1>{});
		}
	};
	template<std::size_t size, typename T>
	constexpr auto make_array (T &&value)
	{
		return Internal::make_array_n (std::integral_constant<std::size_t, size>{}, std::forward<T> (value));
	}
};