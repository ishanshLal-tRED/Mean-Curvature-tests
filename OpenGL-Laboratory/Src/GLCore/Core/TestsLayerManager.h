#pragma once

namespace GLCore
{
	class TestBase;
	namespace Utils
	{
		class Framebuffer;
	}
	class TestsLayerManager
	{
	public:
		TestsLayerManager () = default;
		~TestsLayerManager ();

		void PushTest (TestBase* test);

		void UpdateActiveLayers (Timestep deltatime);
		void ImGuiRender ();
		void OnImGuiRenderAll ();
		void ProcessEvent (Event &event);

		void ActivateTest (uint16_t posn);
		void DeActivateTest (uint16_t posn);
	private:
		void ShowTestMenu ();
		bool m_ShowTestMenu = false;
	private:
		uint32_t m_DockspaceID;
		static const uint8_t g_MaxNumOfAllowedTests = 2;

		TestBase* m_ActiveTests[g_MaxNumOfAllowedTests] = { 0 };
		Utils::Framebuffer *m_ActiveTestFramebuffers[g_MaxNumOfAllowedTests] = { 0 }; // Heap allocated array, beacause openGL is not initailized till now
		std::vector <TestBase*>   m_AllTests;
	};
}