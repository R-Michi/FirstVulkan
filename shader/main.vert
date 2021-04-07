#version 460 core
#extension GL_ARB_separate_shader_objects : enable	// is requiered to use GLSL shaders in vulkan
#extension GL_KHR_vulkan_glsl : enable				// needed to trick the code completion, prints a warning (not supported)

vec2 positions[3] = vec2[](
	vec2(0.0f, -0.5f),
	vec2(0.5f, 0.5f),
	vec2(-0.5f, 0.5f)
);

void main()
{
	gl_Position = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
}