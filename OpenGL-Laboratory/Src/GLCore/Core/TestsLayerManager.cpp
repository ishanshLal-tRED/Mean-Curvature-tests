#include "pch.h"
#include "GLCore/Core/TestBase.h"
#include "GLCore/Util/Core/Framebuffer.h"
#include "TestsLayerManager.h"

#include "GLCore/Core/Application.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace GLCore
{
	TestsLayerManager::~TestsLayerManager ()
	{
		for (uint16_t i = 0; i < g_MaxNumOfAllowedTests; i++) {
			if (m_ActiveTests[i]) {
				m_ActiveTests[i]->OnDetach ();
				m_ActiveTests[i] = nullptr;
			}
			if (m_ActiveTestFramebuffers[i])
			{
				delete m_ActiveTestFramebuffers[i];
				m_ActiveTestFramebuffers[i] = nullptr;
			}
		}
		{
			for (uint16_t i = 0; i < m_AllTests.size (); i++)
				delete m_AllTests[i];
			m_AllTests.clear ();
		}
	}
	void TestsLayerManager::PushTest (TestBase *test)
	{
		for (TestBase *_test: m_AllTests) {
			if (test->GetName () == _test->GetName ()) {
				LOG_ERROR ( "Similar Named Test: {0} Already Exists, Please resolve name conflict EXITING APPLICATION", test->GetName ());
				Application::Get ().ApplicationClose ();
				return;
			}
		}
		m_AllTests.emplace_back (test);
	}

	void TestsLayerManager::UpdateActiveLayers (Timestep deltatime)
	{
		uint8_t testIndex = 0;
		for (TestBase *test : m_ActiveTests) 	{
			if (test)
			{
				////
				// Here Will be code for frame buffer
				////
				m_ActiveTestFramebuffers[testIndex]->Bind ();
				test->OnUpdate (deltatime);
				m_ActiveTestFramebuffers[testIndex]->Unbind ();
			} else break;
			testIndex++;
		}
	}
	void TestsLayerManager::ImGuiRender ()
	{
		{// DockSpace

			static bool dockspaceOpen = true;
			static bool showImGuiDemoWindow = false;
			static constexpr bool optFullscreenPersistant = true;
			bool optFullscreen = optFullscreenPersistant;

			static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

			// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
			// because it would be confusing to have two docking targets within each others.
			ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
			if (optFullscreen) {
				ImGuiViewport *viewPort = ImGui::GetMainViewport ();
				ImGui::SetNextWindowPos (viewPort->Pos);
				ImGui::SetNextWindowSize (viewPort->Size);
				ImGui::SetNextWindowViewport (viewPort->ID);
				ImGui::PushStyleVar (ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::PushStyleVar (ImGuiStyleVar_WindowBorderSize, 0.0f);

				windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
			}

			// when using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render background.
			if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
				windowFlags |= ImGuiWindowFlags_NoBackground;

			// Important: note that we proceed even if Begin() return false (i.e window is collapsed).
			// This is because we want to keep our DockSpace() active. If a DockSpace is inactive,
			// all active windows docked into it will lose their parent and become undocked.
			// any change of dockspce/settings would lead towindows being stuck in limbo and never being visible.
			ImGui::PushStyleVar (ImGuiStyleVar_WindowPadding, ImVec2 (0.0f, 0.0f));
			ImGui::Begin ("Main DockSpace", &dockspaceOpen, windowFlags);
			ImGui::PopStyleVar ();

			if (optFullscreen)
				ImGui::PopStyleVar (2);

			// DockSpace
			ImGuiIO &io = ImGui::GetIO ();
			ImGuiStyle &style = ImGui::GetStyle ();
			float defaultMinWinSize = style.WindowMinSize.x;
			style.WindowMinSize.x = 280;

			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
				m_DockspaceID = ImGui::GetID ("Main DockSpace");
				ImGui::DockSpace (m_DockspaceID, ImVec2 (0.0f, 0.0f), dockspaceFlags);
			}

			style.WindowMinSize.x = defaultMinWinSize;

			// DockSpace's MenuBar
			if (ImGui::BeginMenuBar ()) {
				if (ImGui::BeginMenu ("Main")) {

					if (ImGui::MenuItem ("Show Demo Window")) showImGuiDemoWindow = true;
					ImGui::Separator ();
					if (ImGui::MenuItem ("Show Tests Menu")) m_ShowTestMenu = true;
					ImGui::Separator ();
					if (ImGui::MenuItem ("Exit")) Application::Get ().ApplicationClose ();

					ImGui::EndMenu ();
				}
				{
					for (TestBase *test : m_ActiveTests) {
						if (test) {
							ImGui::PushID (ImGuiLayer::UniqueName ("--Menu"));
							ImGui::Bullet ();
							test->ImGuiMenuOptions ();
							ImGui::PopID ();
						} else break;
					}
				}
				ImGui::EndMenuBar ();
			}

			if (showImGuiDemoWindow)
			{
				ImGui::ShowDemoWindow (&showImGuiDemoWindow);
			}

			ShowTestMenu ();

			// Here goes Stuff that will be put inside DockSpace
			OnImGuiRenderAll ();

			ImGui::End ();
		}
	}
	void TestsLayerManager::OnImGuiRenderAll ()
	{
		int8_t closeTestID = -1;
		{
			ImVec2 mainViewportPosn = ImGui::GetMainViewport ()->Pos;
			TestBase::s_MainViewportPosn.x = mainViewportPosn.x;
			TestBase::s_MainViewportPosn.y = mainViewportPosn.y;
		}

		// name_conflict-fix
		uint16_t test_index = 0;
		for (TestBase *test : m_ActiveTests) {
			if (test)
			{
				
				////
				// Here Will be code for framebuffer-out -> view-port_Window, new-ImGuiWindow(a persistant one that will force your viewport-window to-be attached to itself), pop-on-close-buttonpress etc.
				////
				// TODO: fixing multiple tests problem
				
				ImGui::SetNextWindowDockID (m_DockspaceID, ImGuiCond_Once);
				bool closeTest = true;
				
				ImGui::Begin (ImGuiLayer::UniqueName(test->GetName ()), &closeTest);
				{
					ImGui::PushID (ImGuiLayer::UniqueName ("--Test"));
					if (!closeTest) {
						LayerCloseEvent event;
						test->OnEvent (event);
						closeTestID = test_index;
					}

					ImGuiID dockspace_id;
					dockspace_id = ImGui::GetID (ImGuiLayer::UniqueName("MyDockspace"));
					ImGui::DockSpace (dockspace_id);

					ImGui::SetNextWindowDockID (dockspace_id, ImGuiCond_Always);
					ImGui::PushStyleVar (ImGuiStyleVar_WindowPadding, ImVec2 (0, 0));
					ImGui::Begin (ImGuiLayer::UniqueName("Viewport"), NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
					{
						ImVec2 ContentRegionAvail = ImGui::GetContentRegionAvail ();
						ImVec2 ContentDrawStartPos = ImGui::GetCursorScreenPos ();
						ImGui::Image (reinterpret_cast<void *>(m_ActiveTestFramebuffers[test_index]->GetColorAttachmentRendererID ()), ContentRegionAvail, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
						test->FlagSetter (TestBase::Viewport_Focused, ImGui::IsWindowFocused ());
						test->FlagSetter (TestBase::Viewport_Hovered, ImGui::IsWindowHovered ());

						if (test->ViewportSize (ContentRegionAvail.x, ContentRegionAvail.y)) {
							m_ActiveTestFramebuffers[test_index]->Resize ((uint32_t)test->m_ViewPortSize.x, (uint32_t)test->m_ViewPortSize.y);
						}
						test->m_ViewportPosnRelativeToMain.x = ContentDrawStartPos.x - TestBase::s_MainViewportPosn.x;
						test->m_ViewportPosnRelativeToMain.y = ContentDrawStartPos.y - TestBase::s_MainViewportPosn.y;
					}
					ImGui::End ();
					ImGui::PopStyleVar ();
		
					ImGui::SetNextWindowDockID (dockspace_id, ImGuiCond_FirstUseEver);
					test->OnImGuiRender ();
					ImGui::PopID ();
				}
				ImGui::End ();
				
			}else break;
			test_index++;
		}
		if (closeTestID > -1)
		{
			DeActivateTest (closeTestID);
		}
	}
	void TestsLayerManager::ProcessEvent (Event &event)
	{
		for (TestBase *test : m_ActiveTests) {
			if (test && !event.Handled) {
				test->FilteredEvent (event);
			} else break;
		}
	}
	
	void TestsLayerManager::ActivateTest (uint16_t posn)
	{
		for (uint16_t i = 0; i < g_MaxNumOfAllowedTests; i++)
		{
			if (m_ActiveTests[i] == m_AllTests[posn]) {
				LOG_WARN ("test Already running");
				return;
			}
			
			if (m_ActiveTests[i] != nullptr) {
				continue;
			}
			m_ActiveTests[i] = m_AllTests[posn];
			m_ActiveTests[i]->OnAttach ();
			if (m_ActiveTestFramebuffers[i])
				m_ActiveTestFramebuffers[i]->Resize ((uint32_t)m_ActiveTests[i]->m_ViewPortSize.x, (uint32_t)m_ActiveTests[i]->m_ViewPortSize.y);
			else	
				m_ActiveTestFramebuffers[i] = new Utils::Framebuffer (Utils::FramebufferSpecification{ (uint32_t)m_ActiveTests[i]->m_ViewPortSize.x, (uint32_t)m_ActiveTests[i]->m_ViewPortSize.y, { Utils::FramebufferTextureFormat::RGBA8, Utils::FramebufferTextureFormat::Depth } });
			return;
		}
		LOG_WARN ("!! No more Active tests allowed !!");
	}
	void TestsLayerManager::DeActivateTest (uint16_t posn)
	{
		if (posn < g_MaxNumOfAllowedTests) {
			if (m_ActiveTests[posn] != nullptr) {
				for (uint16_t i = posn; i < (g_MaxNumOfAllowedTests - 1); i++) {
					m_ActiveTests[i] = m_ActiveTests[i + 1];
				}
				m_ActiveTests[g_MaxNumOfAllowedTests - 1] = nullptr;
				return;
			} else {
				LOG_WARN ("!! nullptr found at: posn {0}", posn);
				return;
			}
		}
		LOG_WARN ("!! Out of bounds: posn {0}, Active_Tests_stack_size {1} !!", posn, g_MaxNumOfAllowedTests);
	}

	void TestsLayerManager::ShowTestMenu ()
	{
		if (m_ActiveTests[0] != nullptr && !m_ShowTestMenu)
			return;
		ImGuiWindowFlags flags;
		if (m_ActiveTests[0] == nullptr)
			ImGui::SetNextWindowDockID (m_DockspaceID, ImGuiCond_Always), flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;
		else
			ImGui::SetNextWindowSizeConstraints (ImVec2(300, 200), ImVec2(ImGui::GetWindowWidth () * 0.7f, ImGui::GetWindowHeight () * 0.7f)), flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize;

		ImGui::Begin ("Tests Menu", &m_ShowTestMenu, flags);
		const ImVec2 RegionSize = ImGui::GetContentRegionAvail ();
		uint16_t i = 0;
		for (TestBase *test: m_AllTests) {
			ImGui::PushID (i);
			ImGui::SetNextItemWidth (RegionSize.x/2.0f);
			bool un_collapsed = ImGui::CollapsingHeader (test->GetName ().c_str (), NULL, ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_DefaultOpen);
			ImGui::SameLine ();
			{
				float x = RegionSize.x/2.0f;
				float _x = ImGui::GetCursorPosX ();
				ImGui::SameLine (x > _x ? x : _x);
			}
			if (ImGui::Button ("Start Test")) {
				ActivateTest (i);
				m_ShowTestMenu = false;
			}
			if (un_collapsed) {
				ImGui::Indent ();
				ImGui::Text (test->GetDiscription ().c_str ());
				ImGui::Unindent ();
			}
			ImGui::PopID ();
			i++;
		}
		ImGui::End ();
	}
}