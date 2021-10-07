#pragma once

#include "pch.h"
#include "../Core/Core.h"

namespace GLCore {

	// Events in Hazel are currently blocking, meaning when an event occurs it
	// immediately gets dispatched and must be dealt with right then an there.
	// For the future, a better strategy might be to buffer events in an event
	// bus and process them during the "event" part of the update stage.

	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,

		// Handled Per-Layer basis through filtering
		LayerClose,
		LayerViewportResize, LayerViewportFocus, LayerViewportLostFocus
	};

	enum EventCategory
	{
		None = 0,
		APPLICATION    = BIT(0),
		INPUT          = BIT(1), // Input Events will be filtered per layer basis
		KEYBOARD       = BIT(2),
		MOUSE          = BIT(3),
		MOUSE_BUTTON   = BIT(4),
		LAYER          = BIT(5)
	};

#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::##type; }\
							  virtual EventType GetEventType() const override { return GetStaticType(); }\
							  virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

	class Event
	{
	public:
		bool Handled = false;

		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return GetName(); }

		inline bool IsInCategory(EventCategory category)
		{
			return GetCategoryFlags() & category;
		}
	};

	class EventDispatcher
	{
	public:
		EventDispatcher(Event &event)
			: m_Event(event)
		{}
		
		// F will be deduced by the compiler,  F -> bool (*func) (T&)
		template<typename T, typename F>
		bool Dispatch(const F& func)
		{
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				m_Event.Handled = func(static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}
		// F will be deduced by the compiler, F -> bool (*func) (Event&)
		template<EventCategory Flag, typename F>
		bool CategoryDispatch(const F& func)
		{
			if (m_Event.IsInCategory(Flag))
			{
				m_Event.Handled = func(m_Event);
				return true;
			}
			return false;
		}
	private:
		Event &m_Event;
	};

	inline std::ostream& operator<<(std::ostream& os, const Event &e)
	{
		return os << e.ToString();
	}

}

