#include "MainLayer.h"
#include "mean_curvature.h"
#include <iomanip>
#include <string>
#include <glm/gtx/norm.hpp>
#include <GLCore/Core/Input.h>
using namespace GLCore;
using namespace GLCore::Utils;

// Static data begin
constexpr uint32_t VertexBatchSize = 128*128*4;
const char *ViewProjectionIdentifierInShader = "u_ViewProjectionMat4";
const char *ModelMatrixIdentifierInShader = "u_ModelMat4";
// Static data end

bool MainLayer::load_model (std::string filePath){
	std::vector<std::pair<glm::vec3, glm::vec3>> meshVertices;
	std::vector<uint32_t> meshIndices;
	bool meshloaded = Helper::ASSET_LOADER::LoadOBJ_meshOnly(filePath.c_str (), meshVertices, meshIndices);
	if (meshloaded) {
		{ // load to memory
			std::vector<std::pair<glm::vec3, glm::vec3>> posn_and_normal;
			std::vector<GLuint> indices;
			posn_and_normal = std::move(meshVertices);
			indices = std::move(meshIndices);
			m_MeshColorData.clear ();
			// transfer mesh
			m_StaticMeshData = std::move (posn_and_normal);
			m_MeshColorData.resize (m_StaticMeshData.size ());
			for (auto &ele : m_MeshColorData)
				ele = glm::vec3 (0.7, 0.3, 0.05);
			m_MeshIndicesData = std::move (indices);
		}

		// Upload Mesh
		if(m_MeshVA)
			glDeleteVertexArrays (1, &m_MeshVA);
		if(m_MeshSVB)
			glDeleteBuffers (1, &m_MeshSVB);
		if(m_MeshCVB)
			glDeleteBuffers (1, &m_MeshCVB);
		if(m_MeshIB)
			glDeleteBuffers (1, &m_MeshIB);

		glGenVertexArrays (1, &m_MeshVA);
		glBindVertexArray (m_MeshVA);

		glGenBuffers (1, &m_MeshSVB);
		glBindBuffer (GL_ARRAY_BUFFER, m_MeshSVB);
		{
			size_t size = m_StaticMeshData.size ()*sizeof (glm::vec3[2]);
			glBufferData (GL_ARRAY_BUFFER, size, m_StaticMeshData.data (), GL_STATIC_DRAW);
		}
		
		glGenBuffers (1, &m_MeshCVB);
		glBindBuffer (GL_ARRAY_BUFFER, m_MeshCVB);
		glBufferData (GL_ARRAY_BUFFER, m_MeshColorData.size ()*sizeof (glm::vec3), m_MeshColorData.data(), GL_DYNAMIC_DRAW);
		
		{
			glEnableVertexAttribArray (0); // position
			glEnableVertexAttribArray (1); // normal
			glEnableVertexAttribArray (2); // color

			// glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer)
			uint32_t stride = sizeof (float) * 3 * 2;
			uint32_t offset1 = 0;
			uint32_t offset2 = sizeof (float) * 3;
			glBindBuffer (GL_ARRAY_BUFFER, m_MeshSVB);
			glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, stride, (void*)(offset1));
			glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(offset2));

			glBindBuffer (GL_ARRAY_BUFFER, m_MeshCVB);
			glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, sizeof (float) * 3, 0);
		}
		glGenBuffers (1, &m_MeshIB);
		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_MeshIB);
		glBufferData (GL_ELEMENT_ARRAY_BUFFER, m_MeshIndicesData.size ()*sizeof (GLuint), &m_MeshIndicesData[0], GL_STATIC_DRAW);
	}
	return meshloaded;
}
void MainLayer::calculate_my_curvature ()
{
	auto debugFile = std::string ();
#if MODE_DEBUG
	{
		uint32_t i = m_LoadedMeshPath.size ();
		while (i > 0 && m_LoadedMeshPath[i-1] != '/' && m_LoadedMeshPath[i-1] != '\\')
			i--;
		debugFile = std::string (&m_LoadedMeshPath[i]) + std::string (".txt");
	}
#endif
	if (m_MeshCVB && MeanCurvatureCalculate (debugFile.c_str (), m_StaticMeshData, m_MeshIndicesData, m_MeshColorData
											 , m_Result_MeanCurvatureNormal, m_Result_MeanCurvatureValue
											 , m_BlendKhToColors
											 , m_DebugOutput, &m_MinMaxMeanCurvature.x, &m_MinMaxMeanCurvature.y)) {
		glBindBuffer (GL_ARRAY_BUFFER, m_MeshCVB);
		glBufferSubData (GL_ARRAY_BUFFER, 0, m_MeshColorData.size ()*sizeof (glm::vec3), m_MeshColorData.data ());
	}
}
void MainLayer::OnAttach()
{
	EnableGLDebugging();
	//m_Camera.Position = { 0,0,16 };

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	ReloadSquareShader ();

	if (m_LoadedMeshPath.empty ()) {
		std::string filepath = "./assets/torus.obj";
		std::string temp = filepath;
		if (!load_model (std::move (filepath))) {
			LOG_ERROR ("Cannot Load Mesh");
		} else m_LoadedMeshPath = std::move(temp);
	}
	m_Camera.ReCalculateProjection (This_ViewportAspectRatio());
}
void MainLayer::OnDetach()
{
	if (m_MeshVA)
		glDeleteVertexArrays (1, &m_MeshVA);
	if (m_MeshSVB)
		glDeleteBuffers (1, &m_MeshSVB);
	if (m_MeshCVB)
		glDeleteBuffers (1, &m_MeshCVB);
	if (m_MeshIB)
		glDeleteBuffers (1, &m_MeshIB);

	DeleteSquareShader ();
}
void MainLayer::OnUpdate(Timestep ts)
{
	m_Camera.Update ();

	glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram (m_SquareShaderProgID); // You can find shader inside base.cpp as a static c_str
	glm::mat4 viewProjMat = m_Camera.GetProjection ()*m_Camera.GetView ();
	glUniformMatrix4fv (m_Uniform.Mat4_ViewProjection, 1, GL_FALSE, glm::value_ptr (viewProjMat));

	glBindVertexArray (m_MeshVA);
	glEnableVertexAttribArray (0);
	glEnableVertexAttribArray (1);
	glEnableVertexAttribArray (2);
	glDrawElements (GL_TRIANGLES, m_MeshIndicesData.size (), GL_UNSIGNED_INT, nullptr);
}
std::array<int, 10> just_an_arr = my_std::make_array<10> (23);
void MainLayer::OnImGuiRender()
{
	auto Tooltip = [](const char *display_me) {
		if (ImGui::IsItemHovered ()) {
			ImGui::BeginTooltip ();
			ImGui::PushTextWrapPos (ImGui::GetFontSize () * 50.0f);
			ImGui::TextUnformatted (display_me);
			ImGui::PopTextWrapPos ();
			ImGui::EndTooltip ();
		}
	};

	ImGui::Begin(ImGuiLayer::UniqueName("Controls"));

	if (ImGui::BeginTabBar (GLCore::ImGuiLayer::UniqueName ("Shaders Content"))) {
		if (ImGui::BeginTabItem (GLCore::ImGuiLayer::UniqueName ("Settings"))) {
			
			if(ImGui::DragFloat3 ("Cam Position", &m_Camera.Position[0]) ||
			   ImGui::DragFloat3 ("Cam Rotation", &m_Camera.Rotation[0]))
				m_Camera.Dirtied ();
			if(ImGui::DragFloat ("Near plane", &m_Camera.Near, 1.0, 0.01f) ||
			   ImGui::DragFloat ("Far  plane", &m_Camera.Far, 1.0, m_Camera.Near, 10000))
				m_Camera.ReCalculateProjection (1);
			
			ImGui::TextDisabled ("(?)");
			Tooltip ("Camera manipulator, Does what it says");
			ImGui::SameLine (); 
			if (ImGui::Checkbox ("Show Curvature Debug Output", &m_DebugOutput))
				if (m_DebugOutput)
					calculate_my_curvature ();

			if (ImGui::Button ("LookAt: {0.0.0}", ImVec2{ -1,ImGui::GetFontSize () + 5 }))
				m_Camera.LookAt ({ 0,0,0 });
			Tooltip ("Messed up your camera, just click me to face it towards object");
			
			if (ImGui::Button ("Calculate mean curvature", ImVec2{ -1,ImGui::GetFontSize () + 5 }))
				calculate_my_curvature ();
			Tooltip ("Calculates Mean curvature, Meat of the program (I'm a vegetarian though)\nVisualzer, maps data to min to max val\n");
			
			ImGui::Separator ();
			if (ImGui::Button ("Load Another Model", ImVec2{ -1,ImGui::GetFontSize () + 5 })) {
				std::string filePath = GLCore::Utils::FileDialogs::OpenFile ("Model\0*.obj\0");
				if (!filePath.empty ()) {
					std::string temp = filePath; // copy
					if (!load_model (std::move (filePath)))
						LOG_ERROR ("Cannot Load Mesh");
					else
						m_LoadedMeshPath = std::move (temp);
				}
			}; Tooltip ("Only Accepts .OBJ, as it was written in haste (to avoid vertex duplication),\nso the file should be triangulated mesh, with\n  v {vertex_position vec3}\n  vn{vertex_normal vec3}\n  vt{vertex_tex_co vec3}\n  f {triangle_face f/f/f f/f/f f/f/f}");

			ImGui::Text ("------------------\n| (?) Hover Over |\n------------------");
			Tooltip ("To use the visualizer, import a model, (there will be a default one).\nClick button \"Calculate mean curvature\",it will calculate mean curvature{Kh}\nand mean_curvature_normal_operaor{K(Xi)} for you and display it over screen.\nFor controls -\n  Up    Arrow | Mouse Drag Up = moves camera up, while looking at object\n  Down  Arrow | Mouse Drag Dn = moves camera Dn, while looking at object\n  D | Right Arrow | Mouse Drag Rt = moves camera right, while looking at object\n  A | Left  Arrow | Mouse Drag Lt = moves camera left,  while looking at object\n  W = moves camera closer ( forward (non-linearly))\n  D = moves camera farther(backward (non-linearly))");
			
			ImGui::Text ("Mean-Curvature: min{ %f }  max{ %f }", m_MinMaxMeanCurvature.x, m_MinMaxMeanCurvature.y);
			if (ImGui::CollapsingHeader ("Blend colors", NULL)) {
				ImGui::Indent ();
				ImGui::PushID (456586);

				int size = m_BlendKhToColors.size ();
				
				if (ImGui::InputInt (": Size", &size)) {
					m_BlendKhToColors.resize (MAX (2, size));
				}
				for (uint32_t i = 0; i < m_BlendKhToColors.size (); i++)
					ImGui::ColorPicker3 (std::to_string (i).c_str (), &m_BlendKhToColors[i][0]);

				ImGui::PopID ();
				ImGui::Unindent ();
			} else Tooltip ("Blend between colors, Min{top} : Max{bottom}");

			ImGui::EndTabItem ();
		}
		if (ImGui::BeginTabItem (GLCore::ImGuiLayer::UniqueName ("Square Shader Source"))) {

			OnImGuiSqureShaderSource ();

			ImGui::EndTabItem ();
		}
		ImGui::EndTabBar ();
	}

	ImGui::End();
}

void MainLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<LayerViewportResizeEvent> (
		[&](LayerViewportResizeEvent &event) {
			m_Camera.ReCalculateProjection (float (event.GetWidth ())/float (event.GetHeight ()));
			return false;
		});
	dispatcher.Dispatch<LayerViewportLostFocusEvent> (
		[&](LayerViewportLostFocusEvent &event) {
			Application::Get ().GetWindow ().MouseCursor (true);
			return false;
		});
	dispatcher.Dispatch<MouseButtonPressedEvent> (
		[&](MouseButtonPressedEvent &event) {
			Application::Get ().GetWindow ().MouseCursor (false);
			m_LastMousePosns = *(glm::vec2*)((void*)(&Input::GetMousePosn ()));
			return false;
		});
	dispatcher.Dispatch<MouseMovedEvent> (
		[&](MouseMovedEvent &event) {
			if (Input::IsMouseButtonPressed (Mouse::Button0)) {
				glm::vec2 new_posn (event.GetX (), event.GetY ());
				glm::vec2 delta = new_posn - m_LastMousePosns;
				//LOG_TRACE ("Delta mouse move: {0}, {1}", delta.x, delta.y);

				m_Camera.OrbitAround (delta*0.01f, { 0,0,0 });
				m_LastMousePosns = new_posn;
			}
			return true;
		});
	dispatcher.Dispatch<KeyPressedEvent> (
		[&](KeyPressedEvent &event) {
			switch(event.GetKeyCode ()){
				case Key::Up:
					m_Camera.OrbitAround ({ 0,0.01f }, { 0,0,0 }); break;
				case Key::Down:
					m_Camera.OrbitAround ({ 0,-0.01f }, { 0,0,0 }); break;
				case Key::Right:
					m_Camera.OrbitAround ({ 0.01f,0 }, { 0,0,0 }); break;
				case Key::Left:
					m_Camera.OrbitAround ({ -0.01f,0 }, { 0,0,0 }); break;
				case Key::D:
					m_Camera.OrbitAround ({ 0.01f,0 }, { 0,0,0 }); break;
				case Key::A:
					m_Camera.OrbitAround ({ -0.01f,0 }, { 0,0,0 }); break;
				case Key::W:
					m_Camera.Position += m_Camera.Front ()*std::min(1.0f, 0.005f*glm::distance2 (m_Camera.Position,glm::vec3(0))); break;
				case Key::S:
					m_Camera.Position -= m_Camera.Front ()*std::min (1.0f, 0.005f*glm::distance2 (m_Camera.Position, glm::vec3 (0))); break;
				case Key::RightShift:
					m_Camera.Position += m_Camera.Right (); //m_Camera.LookAt ({0,0,0}); break;
			}
			return true;
		});
	dispatcher.Dispatch<MouseButtonReleasedEvent> (
		[&](MouseButtonReleasedEvent &event) {
			Application::Get ().GetWindow ().MouseCursor (true);
			return false;
		});
}
void MainLayer::ImGuiMenuOptions ()
{
	//
}
void MainLayer::OnSquareShaderReload ()
{
	m_Uniform.Mat4_ViewProjection = glGetUniformLocation (m_SquareShaderProgID, ViewProjectionIdentifierInShader);
	m_Uniform.Mat4_ModelMatrix    = glGetUniformLocation (m_SquareShaderProgID, ModelMatrixIdentifierInShader);

	glUseProgram (m_SquareShaderProgID); // You can find shader inside base.cpp as a static c_str
	glm::mat4 modelMatrix = glm::mat4 (1.0f);
	glUniformMatrix4fv (m_Uniform.Mat4_ModelMatrix, 1, GL_FALSE, glm::value_ptr (modelMatrix));
}


/////////////////
// Camera
void MainLayer::Camera::Update ()
{
	if (m_Dirty) {
		re_calculate_transform ();
	}
	m_Dirty = false;
}
const glm::mat4 MainLayer::Camera::GetTransform ()
{
	glm::mat4 matrix = m_RotationMatrix;
	for (uint16_t i = 0; i < 3; i++) {
		matrix[3][i] = Position[i];
	}
	return matrix;
}
const glm::mat4 MainLayer::Camera::GetView ()
{
	glm::mat4 matrix = glm::transpose (m_RotationMatrix); // orthogonal
	matrix = glm::translate (matrix, -Position);
	//matrix = glm::inverse (GetTransform ());
	return matrix;
}
const glm::mat4 &MainLayer::Camera::GetProjection ()
{
	return m_Projection;
}
void MainLayer::Camera::ReCalculateProjection (float aspectRatio)
{
	m_Projection = glm::perspective (glm::radians (FOV_y), aspectRatio, Near, Far);
}
void MainLayer::Camera::re_calculate_transform ()
{
	m_RotationMatrix =
		glm::rotate (
			glm::rotate (
				glm::rotate (
					glm::mat4 (1.0f) // Uff the gimbl lock
				,glm::radians (Rotation.y), { 0,1,0 })
			,glm::radians (Rotation.x), { 1,0,0 })
		,glm::radians (Rotation.z), { 0,0,1 });
}
void MainLayer::Camera::LookAt (glm::vec3 positionInSpace)
{
	glm::vec3 dirn (glm::normalize (positionInSpace - Position));
	glm::vec2 angl = PitchYawFromDirn (-dirn); //Inverting The Model Matrix Flipps the dirn of forward, so flipping the dirn to the trouble of flipping pitch yaw
	glm::vec3 eular (angl.x, angl.y, 0);
	Rotation = glm::degrees(eular);
	m_Dirty = true;
}
void MainLayer::Camera::OrbitAround (glm::vec2 _2d_dirn, glm::vec3 origin) {


	glm::vec3 cam_dirn (glm::normalize (Position - origin));
	float radius = glm::length (origin - Position); // of imaginary sphere;
	constexpr glm::vec3 world_up = { 0,1,0 };
	glm::vec3 right = glm::cross (cam_dirn, world_up); // X
	glm::vec3 up = glm::cross (right, cam_dirn); // Y

	right *= _2d_dirn.x*radius; // circumpherence is 2pi*r
	up *= _2d_dirn.y*radius;

	Position += (right + up);

	Position = (glm::normalize (Position-origin))*radius + origin;

	LookAt (origin);
}
glm::vec2 MainLayer::Camera::PitchYawFromDirn (glm::vec3 dirn)
{
	float x = dirn.x;
	float y = dirn.y;
	float z = dirn.z;
	float xz = sqrt (x*x + z*z);//length of projection in xz plane
	float xyz = sqrt(y*y + xz*xz);//length of dirn

	float pitch = -asin (y / xyz);
	float yaw = acos (z / xz);
	//front is  0, 0, 1;
	if (dirn.x < 0)
		yaw = -yaw;

	//negating pitch --\/ as pitch is clockwise rotation while eular rotation is counter-Clockwise
	return { pitch,yaw };
}