#pragma once

#include "Core.h"

#include "Window.h"
#include "../Events/Event.h"
#include "../Events/ApplicationEvent.h"

#include "Timestep.h"

#include "GLCore/Core/Layer.h"
#include "GLCore/Core/TestsLayerManager.h"
#include "../ImGui/ImGuiLayer.h"

namespace GLCore {
	class Application
	{
	public:
		Application(const std::string& name = "OpenGL Sandbox", uint32_t width = 1280, uint32_t height = 720);
		virtual ~Application() = default;

		static float GetTimeInSeconds ();
		void Run();
		bool ApplicationClose ();

		void OnEvent(Event &e);

		template<class Typ, class... Types>
		void PushLayer (Types&&... Args) { m_TestsManager.PushTest (new Typ(std::forward<Types> (Args)...)); }

		void ActivateLayer   (uint16_t posn) { m_TestsManager.ActivateTest   (posn); }
		void DeActivateLayer (uint16_t posn) { m_TestsManager.DeActivateTest (posn); }

		inline Window& GetWindow() { return *m_Window; }

		inline static Application& Get() { return *s_Instance; }
	private:
		bool OnWindowClose(WindowCloseEvent &e);
		friend class ImGuiLayer;
	private:
		std::unique_ptr<Window> m_Window;

		// Layer's
		ImGuiLayer m_ImGuiLayer;
		TestsLayerManager m_TestsManager;

		bool m_Running = true;
		float m_LastFrameTime = 0.0f;
	private:
		static Application* s_Instance;
	};
}