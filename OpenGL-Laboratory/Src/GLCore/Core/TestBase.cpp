#include "pch.h"
#include "TestBase.h"
#include "GLCore/Events/MouseEvent.h"
#include "GLCore/Events/KeyEvent.h"
#include "GLCore/Events/LayerEvent.h"

namespace GLCore
{
	TestBase::TestBase (const std::string &name, const std::string &discription)
		: Layer (name), m_TestDiscription (discription), m_ViewPortSize(1, 1)
	{}

	glm::vec2 TestBase::s_MainViewportPosn = glm::vec2(0, 0);
	
	void TestBase::FlagSetter (Flags flag, bool val)
	{
		if (val == (bool)(m_Flags & flag))
			return;
		if (val) {
			m_Flags |= flag; // Set bit
			switch (flag)
			{
				case Flags::Viewport_Focused:
					LayerViewportFocusEvent event;
					OnEvent (event); break;
			}
		} else {
			m_Flags &= ~(flag); // Clear bit
			switch (flag) {
				case Flags::Viewport_Focused:
					LayerViewportLostFocusEvent event;
					OnEvent (event); break;
			}
		}
	}

	void TestBase::FilteredEvent (Event &event)
	{
		bool event_dispatched = false;
		EventDispatcher dispatcher (event);
		// Input
		event_dispatched |= dispatcher.CategoryDispatch<EventCategory::MOUSE_BUTTON> (
			[&](Event &e) {
				if (m_Flags & Flags::Viewport_Focused)
					OnEvent (e);
				return e.Handled;
			});
		event_dispatched |= dispatcher.CategoryDispatch<EventCategory::KEYBOARD> (
			[&](Event &e) {
				if (m_Flags & Flags::Viewport_Focused)
					OnEvent (e);
				return e.Handled;
			});
		event_dispatched |= dispatcher.Dispatch<MouseScrolledEvent> (
			[&](MouseScrolledEvent &e) {
				if (m_Flags & Flags::Viewport_Focused)
					OnEvent (e);
				return e.Handled;
			});
		event_dispatched |= dispatcher.Dispatch<MouseMovedEvent> (
			[&](MouseMovedEvent &e) {
				if (m_Flags & Flags::Viewport_Hovered) {
					MouseMovedEvent event (e.GetX () - m_ViewportPosnRelativeToMain.x, e.GetY () - m_ViewportPosnRelativeToMain.y);
					OnEvent (event);
					return event.Handled;
				}
				return false;
			});

		// Other Events passed raw
		if (!event_dispatched)
		{
			OnEvent (event);
		}
	}

	bool TestBase::ViewportSize (float x, float y)
	{
		if (m_ViewPortSize.x != x || m_ViewPortSize.y != y)
		{
			// Viewport size changed
			
			LayerViewportResizeEvent event (x, y);
			OnEvent (event);
			m_ViewPortSize.x = x, m_ViewPortSize.y = y;
			return true;
		}
		return false;
	}
}