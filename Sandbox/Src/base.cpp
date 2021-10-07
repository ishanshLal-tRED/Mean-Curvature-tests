#include "base.h"

using namespace GLCore;
using namespace GLCore::Utils;

const char *SqrShader_Base::s_default_sqr_shader_vert = R"(
#version 440 core
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec3 in_Color;

layout (location = 0) out vec3 p_Color;

uniform mat4 u_ViewProjectionMat4;
uniform mat4 u_ModelMat4;

void main()
{
	gl_Position = u_ViewProjectionMat4 * u_ModelMat4 * vec4(in_Position, 1.0f);
	p_Color = in_Color;
})";
const char *SqrShader_Base::s_default_sqr_shader_frag = R"(
#version 440 core
layout (location = 0) in vec3 p_Color;

layout (location = 0) out vec4 o_Color;

void main()
{
	o_Color = vec4(p_Color,1.0);
})";

SqrShader_Base::SqrShader_Base (const char *name, const char *discription
								,const char *default_sqr_shader_vert_src, const char *default_sqr_shader_frag_src
	): TestBase (std::string (name), std::string (discription))
	, m_SquareShaderTXT_vert (Buffer::Create (default_sqr_shader_vert_src, 512))
	, m_SquareShaderTXT_frag (Buffer::Create (default_sqr_shader_frag_src, 512))
{
};