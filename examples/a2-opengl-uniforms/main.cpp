// A2 - Raw OpenGL 3.3: the cube of A1, driven live from the slide.
//
// The same structure as A1, with uniforms that the panel of the page (or the slide,
// through postMessage) changes while the demo runs:
//   speed (turns/s), twist (deformation in the vertex shader), hue (color), light (on/off),
//   and a button reset.
// The panel of the page is built from these declarations: no HTML to write.
//
// Example of address: index.html?twist=1.2&controls=twist,hue,speed,light,reset

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#else
#include "glad.hpp"
#endif
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "slide_bridge.hpp"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

// The parameters of the demo, changed live
float speed = 0.15f;      // turns per second
float twist = 0.0f;       // radians per unit of height
float hue = 0.58f;        // in [0, 1]
bool light = true;
std::string background = "light";

GLFWwindow* window = nullptr;
GLuint program = 0, vao = 0;
GLsizei vertex_count = 0;
GLint uniform_matrix = -1, uniform_twist = -1, uniform_hue = -1, uniform_light = -1;
float angle = 0.0f;       // accumulated, so that a change of speed does not make the cube jump
double last_time = 0.0;

#ifdef __EMSCRIPTEN__
char const* const shader_header = "#version 300 es\nprecision highp float;\n";
#else
char const* const shader_header = "#version 330 core\n";
#endif

// The twist turns each point around the vertical axis by an angle proportional to its height
char const* const vertex_shader = R"(
layout (location = 0) in vec3 position;
layout (location = 1) in float face;
uniform mat4 matrix;
uniform float twist;
out vec3 world_position;
out float face_shade;
void main() {
    float a = twist * position.y;
    vec3 p = vec3(cos(a) * position.x + sin(a) * position.z, position.y, -sin(a) * position.x + cos(a) * position.z);
    world_position = p;
    face_shade = face;
    gl_Position = matrix * vec4(p, 1.0);
}
)";

// Lighting without normals: the normal of the surface comes from the derivatives of the position
char const* const fragment_shader = R"(
in vec3 world_position;
in float face_shade;
uniform float hue;
uniform int light;
out vec4 frag_color;
vec3 hsv_to_rgb(vec3 c) {
    vec3 p = abs(fract(c.xxx + vec3(1.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);
    return c.z * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), c.y);
}
void main() {
    vec3 base = hsv_to_rgb(vec3(fract(hue + 0.04 * face_shade), 0.55, 0.92));
    if (light == 0) { frag_color = vec4(base, 1.0); return; }
    vec3 n = normalize(cross(dFdx(world_position), dFdy(world_position)));
    vec3 to_light = normalize(vec3(0.4, 0.8, 0.6));
    float diffuse = abs(dot(n, to_light));
    frag_color = vec4(base * (0.35 + 0.65 * diffuse), 1.0);
}
)";

GLuint compile(GLenum type, char const* body)
{
	GLuint shader = glCreateShader(type);
	char const* sources[] = {shader_header, body};
	glShaderSource(shader, 2, sources, nullptr);
	glCompileShader(shader);
	GLint ok = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[1024];
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		std::fprintf(stderr, "Shader error: %s\n", log);
	}
	return shader;
}

struct mat4 { float m[16]; };   // column-major: m[column * 4 + row]

mat4 multiply(mat4 const& a, mat4 const& b)
{
	mat4 r{};
	for (int c = 0; c < 4; ++c)
		for (int l = 0; l < 4; ++l)
			for (int k = 0; k < 4; ++k)
				r.m[c * 4 + l] += a.m[k * 4 + l] * b.m[c * 4 + k];
	return r;
}

mat4 rotation_x(float a) { float c = std::cos(a), s = std::sin(a); return {{1,0,0,0, 0,c,s,0, 0,-s,c,0, 0,0,0,1}}; }
mat4 rotation_y(float a) { float c = std::cos(a), s = std::sin(a); return {{c,0,-s,0, 0,1,0,0, s,0,c,0, 0,0,0,1}}; }
mat4 translation_z(float z) { return {{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,z,1}}; }
mat4 perspective(float fov_y, float aspect, float near, float far)
{
	float const f = 1.0f / std::tan(fov_y / 2);
	return {{f / aspect,0,0,0, 0,f,0,0, 0,0,(far + near) / (near - far),-1, 0,0,2 * far * near / (near - far),0}};
}

// A cube of side 1 (taller than wide, so that the twist shows), each face a grid of n x n
// squares: the twist bends the faces, which needs vertices inside them
void create_cube(int n)
{
	float const height = 1.6f;
	// each face: a corner and two edges
	float const faces[6][9] = {
		{-.5f,-.5f,.5f,  1,0,0,  0,1,0}, {.5f,-.5f,-.5f, -1,0,0,  0,1,0},
		{.5f,-.5f,.5f,   0,0,-1, 0,1,0}, {-.5f,-.5f,-.5f, 0,0,1,  0,1,0},
		{-.5f,.5f,.5f,   1,0,0,  0,0,-1}, {-.5f,-.5f,-.5f, 1,0,0,  0,0,1}};
	std::vector<float> data;
	for (int f = 0; f < 6; ++f) {
		auto point = [&](int i, int j, int k) {
			float const u = float(i) / n, v = float(j) / n;
			float c = faces[f][k] + u * faces[f][3 + k] + v * faces[f][6 + k];
			return k == 1 ? c * height : c;
		};
		for (int i = 0; i < n; ++i)
			for (int j = 0; j < n; ++j)
				for (auto ij : {std::pair<int, int>{0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1}}) {
					for (int k = 0; k < 3; ++k) data.push_back(point(i + ij.first, j + ij.second, k));
					data.push_back(float(f));
				}
	}
	vertex_count = GLsizei(data.size() / 4);

	GLuint vbo;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(data.size() * sizeof(float)), data.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
}

void surface_size(int& width, int& height)
{
#ifdef __EMSCRIPTEN__
	emscripten_get_canvas_element_size("#canvas", &width, &height);
#else
	glfwGetFramebufferSize(window, &width, &height);
#endif
}

void frame()
{
	double const now = glfwGetTime();
	float const pi = 3.14159265f;
	angle += 2 * pi * speed * float(now - last_time);
	last_time = now;

	int width = 1, height = 1;
	surface_size(width, height);
	glViewport(0, 0, width, height);
	float const shade = (background == "dark") ? 0.12f : 1.0f;
	glClearColor(shade, shade, shade, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 const model = multiply(rotation_x(20 * pi / 180), rotation_y(angle));
	mat4 const view = translation_z(-3.4f);
	mat4 const projection = perspective(40 * pi / 180, float(width) / float(height > 0 ? height : 1), 0.1f, 10.0f);
	mat4 const matrix = multiply(projection, multiply(view, model));

	glUseProgram(program);
	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, matrix.m);
	glUniform1f(uniform_twist, twist);
	glUniform1f(uniform_hue, hue);
	glUniform1i(uniform_light, light ? 1 : 0);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, vertex_count);

	glfwSwapBuffers(window);
	glfwPollEvents();
}

int main(int argc, char* argv[])
{
	slide::parse(argc, argv);
	// Each parameter once: its range gives the slider of the panel
	slide::param("speed", speed, 0.0f, 1.0f, 0.01f).with_label("speed (turns/s)", "speed");
	slide::param("twist", twist, -3.0f, 3.0f, 0.05f).with_label("twist (rad/unit)", "twist");
	slide::param("hue", hue, 0.0f, 1.0f, 0.01f).with_label("hue");
	slide::param("light", light).with_label("light");
	slide::param("background", background, {"light", "dark"});
	slide::action("reset", [] { speed = 0.15f; twist = 0.0f; hue = 0.58f; light = true; angle = 0.0f; })
		.with_label("reset");
	int width = 800, height = 600;
	std::sscanf(slide::arg("size", "800x600").c_str(), "%dx%d", &width, &height);

	if (!glfwInit()) return 1;
#ifdef __EMSCRIPTEN__
	// The window is the canvas, sized by the page to the iframe: glfwCreateWindow would resize it
	emscripten_get_canvas_element_size("#canvas", &width, &height);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	window = glfwCreateWindow(width, height, "A2 - OpenGL uniforms", nullptr, nullptr);
	if (!window) { std::fprintf(stderr, "Cannot create an OpenGL 3.3 window\n"); return 1; }
	glfwMakeContextCurrent(window);
#ifndef __EMSCRIPTEN__
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::fprintf(stderr, "Cannot load OpenGL\n"); return 1; }
	glfwSwapInterval(1);
#endif

	program = glCreateProgram();
	glAttachShader(program, compile(GL_VERTEX_SHADER, vertex_shader));
	glAttachShader(program, compile(GL_FRAGMENT_SHADER, fragment_shader));
	glLinkProgram(program);
	uniform_matrix = glGetUniformLocation(program, "matrix");
	uniform_twist = glGetUniformLocation(program, "twist");
	uniform_hue = glGetUniformLocation(program, "hue");
	uniform_light = glGetUniformLocation(program, "light");
	create_cube(24);
	glEnable(GL_DEPTH_TEST);
	last_time = glfwGetTime();

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(frame, 0, 1);
#else
	while (!glfwWindowShouldClose(window))
		frame();
	glfwTerminate();
#endif
	return 0;
}
