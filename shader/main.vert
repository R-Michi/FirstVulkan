#version 460 core
#extension GL_ARB_separate_shader_objects : enable	// is requiered to use GLSL shaders in vulkan

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec2 a_uvCoords;

layout (location = 0) out vec4 frag_color;
layout (location = 1) out vec2 frag_uvCoords;

layout (binding = 0) uniform UBO 
{
	mat4 MVP;
} ubo;

void main()
{
	gl_Position = ubo.MVP * vec4(a_Pos, 1.0f);
	frag_color = a_Color;
	frag_uvCoords = a_uvCoords;
}