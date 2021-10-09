﻿#include "ExampleLayer.h"
#include <iomanip>
#include <limits>
#include <glm/gtx/norm.hpp>
#include <fstream>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <GLCore/Core/Input.h>
using namespace GLCore;
using namespace GLCore::Utils;

// Static data begin
constexpr uint32_t VertexBatchSize = 128*128*4;
const char *ViewProjectionIdentifierInShader = "u_ViewProjectionMat4";
const char *ModelMatrixIdentifierInShader = "u_ModelMat4";
// Static data end

bool MeanCurvatureCalculate (const char *debug_filename
							 , const std::vector<std::pair<glm::vec3, glm::vec3>> &posn_and_normals, const std::vector<GLuint> &indices, std::vector<glm::vec3> &curvature_diffuse_color
							 , std::vector<glm::vec3> &mean_curvature_normals, std::vector<float> &mean_curvature_values
							 , const std::vector<glm::vec3> &blend_betweencolors
							 , const bool trackOutput = true);

bool ExampleLayer::LoadModel (std::string filePath){
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
void ExampleLayer::OnAttach()
{
	EnableGLDebugging();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	ReloadSquareShader ();

	if (m_LoadedMeshPath.empty ()) {
		std::string filepath = "./assets/torus.obj";
		std::string temp = filepath;
		if (!LoadModel (std::move (filepath))) {
			LOG_ERROR ("Cannot Load Mesh");
		} else m_LoadedMeshPath = std::move(temp);
	}
	m_Camera.ReCalculateProjection (This_ViewportAspectRatio());
}
void ExampleLayer::OnDetach()
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
void ExampleLayer::OnUpdate(Timestep ts)
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
void ExampleLayer::OnImGuiRender()
{
	ImGui::Begin(ImGuiLayer::UniqueName("Controls"));

	if (ImGui::BeginTabBar (GLCore::ImGuiLayer::UniqueName ("Shaders Content"))) {
		if (ImGui::BeginTabItem (GLCore::ImGuiLayer::UniqueName ("Settings"))) {
			
			if(ImGui::DragFloat3 ("Cam Position", &m_Camera.Position[0]) ||
			ImGui::DragFloat3 ("Cam Rotation", &m_Camera.Rotation[0]))
				m_Camera.Dirtied ();
			if(ImGui::DragFloat ("Near plane", &m_Camera.Near, 1.0, 0.01f) ||
			ImGui::DragFloat ("Far  plane", &m_Camera.Far, 1.0, m_Camera.Near, 10000))
				m_Camera.ReCalculateProjection (1);
			
			if (ImGui::Button ("Calculate mean curvature")) {
				uint32_t i = m_LoadedMeshPath.size ();
				while (i > 0 && m_LoadedMeshPath[i-1] == '/' && m_LoadedMeshPath[i-1] == '\\')
					i--;
				if (m_MeshCVB && MeanCurvatureCalculate (&m_LoadedMeshPath[i], m_StaticMeshData, m_MeshIndicesData, m_MeshColorData
											, m_Result_MeanCurvatureNormal, m_Result_MeanCurvatureValue
											, m_BlendKhToColors
											, false)) {
					glBindBuffer (GL_ARRAY_BUFFER, m_MeshCVB);
					glBufferSubData (GL_ARRAY_BUFFER, 0, m_MeshColorData.size ()*sizeof (glm::vec3), m_MeshColorData.data ());
				}
			}
			if (ImGui::Button ("Load Another Model")) {
				std::string filePath = GLCore::Utils::FileDialogs::OpenFile ("Model\0*.obj\0");
				if (!filePath.empty ()) {
					std::string temp = filePath; // copy
					if (!LoadModel (std::move (filePath)))
						LOG_ERROR ("Cannot Load Mesh");
					else
						m_LoadedMeshPath = std::move(temp);
				}
			}
			if (ImGui::Button ("LookAt: {0.0.0}"))
				m_Camera.LookAt ({ 0,0,0 });

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

void ExampleLayer::OnEvent(Event& event)
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
				LOG_TRACE ("Delta mouse move: {0}, {1}", delta.x, delta.y);

				m_Camera.OrbitAround (delta*0.01f, { 0,0,0 });
				m_LastMousePosns = new_posn;
			}
			return true;
		});
	dispatcher.Dispatch<KeyPressedEvent> (
		[&](KeyPressedEvent &event) {
			switch(event.GetKeyCode ()){
				case Key::D:
					m_Camera.OrbitAround ({ 0.01f,0 }, { 0,0,0 }); break;
				case Key::A:
					m_Camera.OrbitAround ({ -0.01f,0 }, { 0,0,0 }); break;
			}
			return true;
		});
	dispatcher.Dispatch<MouseButtonReleasedEvent> (
		[&](MouseButtonReleasedEvent &event) {
			Application::Get ().GetWindow ().MouseCursor (true);
			return false;
		});
}
void ExampleLayer::ImGuiMenuOptions ()
{
	//
}
void ExampleLayer::OnSquareShaderReload ()
{
	m_Uniform.Mat4_ViewProjection = glGetUniformLocation (m_SquareShaderProgID, ViewProjectionIdentifierInShader);
	m_Uniform.Mat4_ModelMatrix    = glGetUniformLocation (m_SquareShaderProgID, ModelMatrixIdentifierInShader);

	glUseProgram (m_SquareShaderProgID); // You can find shader inside base.cpp as a static c_str
	glm::mat4 modelMatrix = glm::mat4 (1.0f);
	glUniformMatrix4fv (m_Uniform.Mat4_ModelMatrix, 1, GL_FALSE, glm::value_ptr (modelMatrix));
}


/////////////////
// Camera
void ExampleLayer::Camera::Update ()
{
	if (m_Dirty) {
		re_calculate_transform ();
	}
	m_Dirty = false;
}
const glm::mat4 ExampleLayer::Camera::GetTransform ()
{
	glm::mat4 matrix = m_RotationMatrix;
	for (uint16_t i = 0; i < 3; i++) {
		matrix[3][i] = Position[i];
	}
	return matrix;
}
const glm::mat4 &ExampleLayer::Camera::GetView ()
{
	glm::mat4 matrix = glm::transpose (m_RotationMatrix); // orthogonal
	matrix = glm::translate (matrix, -Position);
	//matrix = glm::inverse (GetTransform ());
	return matrix;
}
const glm::mat4 &ExampleLayer::Camera::GetProjection ()
{
	return m_Projection;
}
void ExampleLayer::Camera::ReCalculateProjection (float aspectRatio)
{
	m_Projection = glm::perspective (glm::radians (FOV_y), aspectRatio, Near, Far);
}
void ExampleLayer::Camera::re_calculate_transform ()
{
	m_RotationMatrix =
		glm::rotate (
			glm::rotate (
				glm::rotate (
					glm::mat4 (1.0f)
				,glm::radians (Rotation.y), { 0,1,0 })
			,glm::radians (Rotation.x), { 1,0,0 })
		,glm::radians (Rotation.z), { 0,0,1 });
}
void ExampleLayer::Camera::LookAt (glm::vec3 positionInSpace)
{
	LOG_TRACE ("LookAt");
	glm::vec3 dirn (glm::normalize (positionInSpace - Position));
	glm::vec2 angl = PitchYawFromDirn (-dirn); //Inverting The Model Matrix Flipps the dirn of forward, so flipping the dirn to the trouble of flipping pitch yaw
	glm::vec3 eular (angl.x, angl.y, 0);
	Rotation = glm::degrees(eular);
	m_Dirty = true;
}
void ExampleLayer::Camera::OrbitAround (glm::vec2 _2d_dirn, glm::vec3 origin) {

	glm::vec3 cam_dirn (glm::normalize (Position - origin));
	float radius = glm::length (origin - Position); // of imaginary sphere;
	constexpr glm::vec3 world_up = { 0,1,0 };
	glm::vec3 right = glm::cross (cam_dirn, world_up); // X
	glm::vec3 up = glm::cross (right, cam_dirn); // Y

	right *= _2d_dirn.x*radius; // circumpherence is 2pi*r
	up *= _2d_dirn.y*radius;

	Position += (right + up);

	Position = (glm::normalize (Position-origin))*radius + origin;

	LookAt (origin);// {-68.5 -47.7 -63.4}
}
glm::vec2 ExampleLayer::Camera::PitchYawFromDirn (glm::vec3 dirn)
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

	std::cout << "Dirn: " << dirn << '\n';

	//negating pitch --\/ as pitch is clockwise rotation while eular rotation is counter-Clockwise
	return { pitch,yaw };
}

bool MeanCurvatureCalculate (const char* debug_filename
							 ,const std::vector<std::pair<glm::vec3, glm::vec3>> &posn_and_normals, const std::vector<GLuint> &indices, std::vector<glm::vec3> &curvature_diffuse_color
							 ,std::vector<glm::vec3> &mean_curvature_normals, std::vector<float> &mean_curvature_values
							 ,const std::vector<glm::vec3> &blend_betweencolors
							 ,const bool trackOutput)
{
	float max_curvature = -std::numeric_limits<float>::max ();
	float min_curvature =  std::numeric_limits<float>::max ();

	std::vector<glm::vec3> array_K_Xi;
	array_K_Xi.resize (posn_and_normals.size ());
	std::vector<float> array_K_h; // mean curvature value
	array_K_h.resize (posn_and_normals.size ());

#if MODE_DEBUG
	const char *fn = debug_filename != nullptr ? debug_filename : "curvatureResults.txt";
	std::ofstream ofs (fn);
	if (ofs.bad ()) {
		std::cout << "File creation error - " << fn << '\n';
		__debugbreak ();
	}
	std::mutex mutex_ofs;
#endif

	std::mutex mutex_curvature_push, mutex_cout, sync_lock, another_sync_lock;
	std::condition_variable barrier; std::unique_lock synced_lock (sync_lock);
	bool first_to_reach_access_it = true;

	std::atomic<size_t> vertices_processed = 0;
	if (trackOutput)
		std::cout << "index | A_mixed    |    curvature Kh    |    K(Xi)\n";
	auto mean_curvature_func = [&](const uint32_t start, const uint32_t stride) {

	std::ostringstream cout_stream;
	#if MODE_DEBUG
		std::ostringstream out_stream;
		out_stream << std::fixed << std::setprecision (8);
	#endif

		for (size_t curr_indice = start; curr_indice < posn_and_normals.size (); curr_indice += stride, vertices_processed++) // repeat for every vertex
		{
			if (!trackOutput && curr_indice % (stride+1) == 0) {
				mutex_cout.lock ();
				std::cout << "\r  vertices_processed: " << vertices_processed << " out_of: " << posn_and_normals.size ();
				mutex_cout.unlock ();
			}
			
		#if MODE_DEBUG
			out_stream << "\nIDX: " << curr_indice << '\n';
		#endif

			glm::vec3 K_Xi = glm::vec3 (0);
			{
				std::vector<uint32_t> ring;
				{
					std::vector<uint32_t> indices_of_neighbouring_vertives; // bucket of neighboring vertices

					for (size_t i = 0; i < indices.size (); i++) { // Scan for associated primitives
						if (indices[i] == curr_indice) {
							uint32_t i_mod_3 = (i % 3),
								grp = i - i_mod_3;// note: we are assuming model is made up of triangles, so grps of 3
							indices_of_neighbouring_vertives.push_back (indices[grp + ((i_mod_3 + 1)%3)]); // for anticlockwise iteration
							indices_of_neighbouring_vertives.push_back (indices[grp + ((i_mod_3 + 2)%3)]); // for anticlockwise iteration
						}
					}
					//LOG_ASSERT (indices_of_neighbouring_vertives.empty ());
					if (indices_of_neighbouring_vertives.empty ()) {
						glm::vec3 vec = posn_and_normals[curr_indice].first;
						LOG_WARN ("vertice with empty ring: {3}, [{0}, {1}, {2}]", vec.x, vec.y, vec.z, curr_indice);
						continue;
					}

					ring.push_back (indices_of_neighbouring_vertives[0]);
					ring.push_back (indices_of_neighbouring_vertives[1]);
					bool keep_adding = true;
					while (keep_adding && !indices_of_neighbouring_vertives.empty ()) {
						keep_adding = false;

						// forward chain extension
						// 0-----0 - - 0 <-to_find_this
						//       ^-is_common_vertex
						for (auto itr = indices_of_neighbouring_vertives.begin (); itr != indices_of_neighbouring_vertives.end (); itr += 2) { // every odd position
							if (ring.back () == *itr) { // found shared vertex
								ring.push_back (*(itr+1)); // extend ring
								indices_of_neighbouring_vertives.erase (itr+1);
								indices_of_neighbouring_vertives.erase (itr);
								keep_adding = true;
								break;
							}
						}
						// backward chain extension
						// to_find_this->0 - - 0-----0
						// is_common_vertex ---^
						for (size_t i = 1; i < indices_of_neighbouring_vertives.size (); i += 2) { // every odd position
							if (ring.front () == indices_of_neighbouring_vertives[i]) { // found shared vertex
								ring.insert (ring.begin (), indices_of_neighbouring_vertives[i-1]); // extend ring
								auto itr = indices_of_neighbouring_vertives.begin () + (i-1);
								indices_of_neighbouring_vertives.erase (itr+1);
								indices_of_neighbouring_vertives.erase (itr);
								keep_adding = true;
								break;
							}
						}

					}
				}


			#if MODE_DEBUG
				out_stream << "RING: ";
				for (auto val : ring)
					out_stream << std::setw (5) << val << ' ';
				out_stream << '\n';
			#endif

				auto angle_between_edges = [&](uint32_t curr_vertex_index, uint32_t other1_vertex_index, uint32_t other2_vertex_index) -> float {
					glm::vec3 edge1 = glm::normalize (posn_and_normals[other1_vertex_index].first - posn_and_normals[curr_vertex_index].first);
					glm::vec3 edge2 = glm::normalize (posn_and_normals[other2_vertex_index].first - posn_and_normals[curr_vertex_index].first);
					float e1e2sin_theta = glm::length (glm::cross (edge1, edge2));
					return asinf (MIN (1.0, e1e2sin_theta));
				};
				auto cotf = [](float angle_in_rad) -> float { return (angle_in_rad < glm::half_pi<float> ()) ? tanf (glm::half_pi<float> () - angle_in_rad) : -tanf (angle_in_rad - glm::half_pi<float> ()); };
				auto sqr = [](float val) -> float { return val*val; };
				//      /\X
				//     /  \
				//   Q/____\R
				float A_mixed = 0;
				glm::vec3 sigma_mean_curvature_normal_operator = glm::vec3 (0);
				for (uint32_t i = 1; i < ring.size (); i++) {
					float Q = angle_between_edges (ring[i-1], ring[i], curr_indice)
						, R = angle_between_edges (ring[i], curr_indice, ring[i-1])
						, X = angle_between_edges (curr_indice, ring[i], ring[i-1]);
					float pXR2, pXQ2, cot_Q, cot_R;
					glm::vec3 XR_diff, XQ_diff;
					{
						XR_diff = posn_and_normals[curr_indice].first - posn_and_normals[ring[i-1]].first;
						XQ_diff = posn_and_normals[curr_indice].first - posn_and_normals[ring[i]].first;
						cot_Q = cotf (Q), cot_R = cotf (R);
						pXR2 = glm::length2 (XR_diff), pXQ2 = glm::length2 (XQ_diff), cot_Q, cot_R;
					}

					if (Q < glm::half_pi<float> () && R < glm::half_pi<float> () && X < glm::half_pi<float> ()) { // Acute △
						float A_voronoi = (pXR2*cot_Q + pXQ2*cot_R)*0.125; // (1/8)*((X-R)^2 * cotf(∠Q)  +  (X-Q)^2 * cotf(∠R))
						A_mixed += A_voronoi;
					} else {
						float Area_T = (pXR2*cot_Q/sqr (sinf (R)) + pXQ2*cot_R/sqr (sinf (Q)))*0.5;
						if (X >= glm::half_pi<float> ())
							A_mixed += Area_T*0.5;
						else
							A_mixed += Area_T*0.25;
					}

					glm::vec3 mean_curvature_normal_operator_for_curr_tri
						= cot_Q*XR_diff +  cot_R*XQ_diff;
					sigma_mean_curvature_normal_operator += mean_curvature_normal_operator_for_curr_tri;
				}
				K_Xi = (sigma_mean_curvature_normal_operator)*float (1.0/(2.0*A_mixed));

				if (trackOutput) {
					cout_stream << std::setw (4) << curr_indice << ' ' << A_mixed << ' ';
				}
			#if MODE_DEBUG
				out_stream << "A_mixed: " << A_mixed;
			#endif
			}
			float K_h = glm::length (K_Xi)*0.5;

			mutex_curvature_push.lock ();
			array_K_Xi[curr_indice] = K_Xi;
			array_K_h[curr_indice] = K_h;
			max_curvature = MAX (K_h, max_curvature);
			min_curvature = MIN (K_h, min_curvature);
			mutex_curvature_push.unlock ();
			if (trackOutput)
				cout_stream << K_h << ' ' << K_Xi << '\n';

		#if MODE_DEBUG
			out_stream << " mean_curvature: " << K_h << " K(Xi) " << K_Xi << "\ncurrvertex: " << posn_and_normals[curr_indice].first << " normal: " << posn_and_normals[curr_indice].second << '\n';
		#endif
		}
		{ // brain blown away
			barrier.notify_all ();
			another_sync_lock.lock ();
			if (vertices_processed < posn_and_normals.size ()) { // not changing the owner thread, other threads skips it
				barrier.wait (synced_lock, [&] { return vertices_processed >= posn_and_normals.size (); });
				if(!trackOutput)
					std::cout << "\r  vertices_processed: " << vertices_processed << " out_of: " << posn_and_normals.size () << '\n';
				else
					std::cout << "\nresulting mean_curvatures{max: " << max_curvature << ", min: " << min_curvature << "}\n\n";
				synced_lock.unlock ();
			}
			std::cout << cout_stream.str ();
			another_sync_lock.unlock ();
		}

		float min_max_curvature_diff = (max_curvature - min_curvature);
	#if MODE_DEBUG

		mutex_ofs.lock ();
		if (first_to_reach_access_it) {
			ofs << "mean_curvatures{max: " << max_curvature << ", min: " << min_curvature << "}\n\n";
			first_to_reach_access_it = false;
		}
		ofs << out_stream.str ();
		mutex_ofs.unlock ();
	#endif
		
		//for (size_t i = start; i < array_K_Xi.size (); i += stride) {
		//	curvature_diffuse_color[i] = -glm::normalize(array_K_Xi[i]);
		//}
		auto blend = [](float ratio, const std::vector<glm::vec3> &blend_between) -> glm::vec3 {
			ratio *= (blend_between.size () - 1);
			float low_contri = std::floor (ratio);
			float high_contri = std::ceil (ratio);
			int low = low_contri;
			int high = high_contri;
			return low_contri*blend_between[low] + high_contri*blend_between[high];
		};
		for (size_t i = start; i < array_K_Xi.size (); i += stride) {
			float ratio = array_K_h[i];
			ratio -= min_curvature;
			ratio /= min_max_curvature_diff;

			curvature_diffuse_color[i] = blend (ratio, blend_betweencolors);
		};
	};
#define USE_MT 1
#if USE_MT
	std::vector<std::thread> threads;
	threads.reserve (std::thread::hardware_concurrency () - 1);
	for (uint32_t i = 1; i < std::thread::hardware_concurrency (); i++)
		threads.push_back (std::thread (mean_curvature_func, i, std::thread::hardware_concurrency ()));
	mean_curvature_func (0, std::thread::hardware_concurrency ());
	for (std::thread &ref : threads)
		ref.join ();
#else
	mean_curvature_func (0, 1);
#endif
#if MODE_DEBUG
	ofs.close ();
#endif
	mean_curvature_normals = std::move (array_K_Xi), mean_curvature_values = std::move (array_K_h);

	return true;
}