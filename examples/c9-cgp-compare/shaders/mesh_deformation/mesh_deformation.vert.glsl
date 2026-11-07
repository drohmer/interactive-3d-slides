#version 330 core // Header for OpenGL 3.3 (CGP replaces it by the one of WebGL 2 on the web)

// Vertex shader: a plane deformed by a ripple, z = A cos(k r - 3 t), computed on the GPU.
// The normal is either the exact normal of the deformed surface (exact_normals = 1), or the normal of
// the plane before the deformation (exact_normals = 0): the shape is the same, but the lighting is wrong.

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec3 vertex_color;
layout (location = 3) in vec2 vertex_uv;

out struct fragment_data
{
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
} fragment;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Set by the program at each frame, from the parameters of the demo
uniform float time;
uniform float amplitude;      // A
uniform float frequency;      // k
uniform int exact_normals;    // 1: exact normals, 0: the normals of the plane

vec3 deformer(vec3 p0)
{
	float d = sqrt(p0.x*p0.x + p0.y*p0.y);
	return vec3(p0.x, p0.y, amplitude * cos(frequency*d - 3.0*time));
}

vec3 deformer_normal(vec3 p0)
{
	float d = max(sqrt(p0.x*p0.x + p0.y*p0.y), 1e-5);
	float omega = frequency*d - 3.0*time;
	// the partial derivatives of the deformed surface
	vec3 dpdx = vec3(1.0, 0.0, -frequency*p0.x/d*amplitude*sin(omega));
	vec3 dpdy = vec3(0.0, 1.0, -frequency*p0.y/d*amplitude*sin(omega));
	return normalize(cross(dpdx, dpdy));
}

void main()
{
	vec3 p_deformed = deformer(vertex_position);
	vec3 n_deformed = exact_normals == 1 ? deformer_normal(vertex_position) : vertex_normal;

	vec4 position = model * vec4(p_deformed, 1.0);
	mat4 modelNormal = transpose(inverse(model));
	vec4 normal = modelNormal * vec4(n_deformed, 0.0);

	fragment.position = position.xyz;
	fragment.normal   = normal.xyz;
	fragment.color = vertex_color;
	fragment.uv = vertex_uv;

	gl_Position = projection * view * position;
}
