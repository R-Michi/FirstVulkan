#version 460 core
#extension GL_ARB_separate_shader_objects : enable // is requiered to use GLSL shaders in vulkan

layout (location = 0) in vec4 frag_color;
layout (location = 1) in vec2 frag_uvCoords;

layout (location = 0) out vec4 out_color;

layout (binding = 1) uniform sampler2D tex;

void main()
{
	out_color = texture(tex, frag_uvCoords) * frag_color;
}