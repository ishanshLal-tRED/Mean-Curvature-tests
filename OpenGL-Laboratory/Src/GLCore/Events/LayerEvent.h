#pragma once

#include "GLCore/Events/Event.h"

namespace GLCore
{
	class LayerCloseEvent: public Event
	{
	public:
		LayerCloseEvent () {}
		EVENT_CLASS_TYPE (LayerClose)
		EVENT_CLASS_CATEGORY (EventCategory::LAYER)
	};

	class LayerViewportResizeEvent: public Event
	{
	public:
		LayerViewportResizeEvent (uint32_t width, uint32_t height)
			: m_Width (width), m_Height (height)
		{}

		inline uint32_t GetWidth () const { return m_Width; }
		inline uint32_t GetHeight () const { return m_Height; }

		std::string ToString () const override
		{
			std::stringstream ss;
			ss << "ViewportResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str ();
		}

		EVENT_CLASS_TYPE (LayerViewportResize)
		EVENT_CLASS_CATEGORY (EventCategory::LAYER)
	private:
		uint32_t m_Width, m_Height;
	};
	
	class LayerViewportFocusEvent: public Event
	{
	public:
		LayerViewportFocusEvent () {}

		EVENT_CLASS_TYPE (LayerViewportFocus)
		EVENT_CLASS_CATEGORY (EventCategory::LAYER)
	};
	class LayerViewportLostFocusEvent: public Event
	{
	public:
		LayerViewportLostFocusEvent () {}

		EVENT_CLASS_TYPE (LayerViewportLostFocus)
		EVENT_CLASS_CATEGORY (EventCategory::LAYER)
	};
}
