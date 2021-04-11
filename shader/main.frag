#version 460 core
#extension GL_ARB_separate_shader_objects : enable // is requiered to use GLSL shaders in vulkan

layout (location = 0) in vec4 frag_color;

layout (location = 0) out vec4 out_color;

void main()
{
	out_color = frag_color;
}