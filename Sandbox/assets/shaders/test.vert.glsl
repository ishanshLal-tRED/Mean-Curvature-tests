#version 440 core

layout (location = 0) in vec3 a_Position;
layout (location = 0) out vec3 pass_Posn;

uniform mat4 u_ViewProjectionMat4;

void main()
{
	gl_Position = u_ViewProjectionMat4 * vec4(a_Position, 1.0f);
}