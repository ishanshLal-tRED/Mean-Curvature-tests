#pragma once

#include <GLCore.h>
#include <GLCoreUtils.h>
#include "base.h"

class ExampleLayer : public SqrShader_Base
{
public:
	ExampleLayer (const char* name = "ExampleLayer")
		: SqrShader_Base (name, "just a base")
	{
		m_Camera.ReCalculateProjection (1.5f);
		m_Camera.ReCalculateTransform ();
	}
	virtual ~ExampleLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnEvent(GLCore::Event& event) override;
	virtual void OnUpdate(GLCore::Timestep ts) override;
	virtual void OnImGuiRender() override;
	virtual void ImGuiMenuOptions() override;
	virtual void OnSquareShaderReload () override;
private:
	bool LoadModel (std::string filePath);
public:
	struct Camera
	{
	public:

		// Transform
		glm::vec3 Position = {1,1,16};
		glm::vec3 Rotation = {0,0,0};

		// Properties
		float FOV_y = 45.0f; // in degrees
		float Near = 0.1f; // in degrees
		float Far = 1000.0f; // in degrees

		const glm::mat4 GetTransform ();
		const glm::mat4& GetView ();
		const glm::mat4& GetProjection ();
		void ReCalculateProjection (float aspectRatio);
		void ReCalculateTransform ();
		void LookAt (glm::vec3 positionInSpace);
		static glm::vec2 PitchYawFromDirn (glm::vec3 dirn);
	private:
		glm::mat4 m_Projection;
		glm::mat4 m_RotationMatrix; // ortho-graphic
	};
private:
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

	std::string m_LoadedMeshPath = "";
};