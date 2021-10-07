#pragma once
#include "GLCore/Core/Layer.h"
#include <glm/glm.hpp>
#include "GLCore/Events/LayerEvent.h"

namespace GLCore
{
	class TestsLayerManager;
	class TestBase
		: public Layer
	{
	public:
		TestBase(const std::string& name = "TestBase", const std::string& discription = "No_Discripton");
		virtual ~TestBase () = default;

		virtual void ImGuiMenuOptions() {}

		const std::string &GetDiscription () { return m_TestDiscription; }
	protected:
		enum Flags
		{
			None = 0,
			Viewport_Focused = BIT (0),
			Viewport_Hovered = BIT (1)
		};
		bool CheckFlags (int flags)
		{
			return m_Flags & flags;
		}
		glm::vec2 ViewportSize () { return m_ViewPortSize; }
		// Relative to window
		glm::vec2 ViewportPosition () { m_ViewportPosnRelativeToMain; };
	private:
		void FlagSetter (Flags, bool);
		void FilteredEvent (Event &event);
		bool ViewportSize (float x, float y);
		friend class TestsLayerManager;
	private:
		static glm::vec2 s_MainViewportPosn;
	private:
		//////
		// Frame-buffer and etc.
		/////
		int m_Flags = 0;
		glm::vec2 m_ViewPortSize = { 1,1 };

		glm::vec2 m_ViewportPosnRelativeToMain = { 0,0 };
		std::string m_TestDiscription;
	};
}