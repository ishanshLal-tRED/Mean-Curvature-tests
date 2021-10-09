#pragma once

#include <GLCore.h>
#include <GLCoreUtils.h>
#include "base.h"

class ExampleLayer : public SqrShader_Base
{
public:
	ExampleLayer (const char* name = "ExampleLayer")
		: SqrShader_Base (name, "just a base")
	{}
	virtual ~ExampleLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnEvent(GLCore::Event& event) override;
	virtual void OnUpdate(GLCore::Timestep ts) override;
	virtual void OnImGuiRender() override;
	virtual void ImGuiMenuOptions() override;
	virtual void OnSquareShaderReload () override;
private:
	bool load_model (std::string filePath);
	void calculate_my_curvature ();
public:
	struct Camera
	{
	public:

		void Dirtied () { m_Dirty = true; };
		void Update ();

		const glm::mat4  GetTransform          ();
		const glm::mat4  GetView               ();
		const glm::mat4& GetProjection         ();
		void             ReCalculateProjection (float aspectRatio);
		void             LookAt                (glm::vec3 positionInSpace);
		void             OrbitAround           (glm::vec2 _2d_dirn_of_movement_over_imaginary_Sphere, glm::vec3 origin);
		static glm::vec2 PitchYawFromDirn      (glm::vec3 dirn);

		const glm::vec3 Front () { return - *(glm::vec3 *)(&m_RotationMatrix[2]); } // Z-axis, negate as camera
		const glm::vec3 Up    () { return   *(glm::vec3 *)(&m_RotationMatrix[1]); }
		const glm::vec3 Right () { return   *(glm::vec3 *)(&m_RotationMatrix[0]); }
	private:
		void re_calculate_transform ();
	private: 
		bool m_Dirty = true;
	public:
		// Transform
		glm::vec3 Position = { 0, 0, 16 };
		glm::vec3 Rotation = {0,0,0};
		// Properties
		float FOV_y = 45.0f; // in degrees
		float Near  = 0.1f;
		float Far   = 1000.0f;
	private:
		glm::mat4 m_Projection;
		glm::mat4 m_RotationMatrix; // ortho-graphic
	};
private:
	bool m_DebugOutput = false;
	Camera m_Camera;
	
	GLuint m_MeshVA = 0, m_MeshSVB = 0, m_MeshCVB = 0, m_MeshIB = 0;
	// vertexArray, vertexBuff(posn & nrml), vertexColorsBuff, indices
	struct
	{
		GLuint Mat4_ViewProjection;
		GLuint Mat4_ModelMatrix;

	}m_Uniform;

	std::vector<std::pair<glm::vec3, glm::vec3>> m_StaticMeshData; // {vertex_position, vertex_normal}, static VBO
	std::vector<glm::vec3> m_MeshColorData; // Dynamic VBO
	std::vector<GLuint> m_MeshIndicesData;

	std::vector<glm::vec3> m_Result_MeanCurvatureNormal;
	std::vector<float> m_Result_MeanCurvatureValue;

	std::vector<glm::vec3> m_BlendKhToColors{
												glm::vec3{0,0,85},
												glm::vec3{0.0f,0.15f,0.65f},
												glm::vec3{0.15f,0.25f,0.55f},
												glm::vec3{0.15f,0.55f,0.05f},
												glm::vec3{0.65f,0.35f,0},
												glm::vec3{0.85f,0.15f,0},
												glm::vec3{1.0f,0,0}
	};
	glm::vec2 m_LastMousePosns = { 0,0 };
	glm::vec2 m_MinMaxMeanCurvature = { 0,0 };

	std::string m_LoadedMeshPath = "";
};