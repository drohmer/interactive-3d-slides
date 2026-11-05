// A1 - Raw OpenGL 3.3: a turning cube, set by the address of the slide.
//
// One source for the native program (OpenGL 3.3 core, GLFW, glad) and for the web
// (WebGL 2 = OpenGL ES 3.0, compiled with Emscripten). The differences are marked
// with #ifdef __EMSCRIPTEN__: the headers, the first line of the shaders, the size
// of the drawing surface and the main loop.
//
// Settings (command line, or address of the iframe: index.html?speed=0.3&tilt=30&background=dark)
//   --speed <turns per second>   --tilt <degrees>   --background <light|dark>   --size WxH (native)

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

// The settings of the demo
float speed = 0.15f;              // turns per second
float tilt = 25.0f;               // degrees, rotation around the horizontal axis
std::string background = "light";

GLFWwindow* window = nullptr;
GLuint program = 0, vao = 0;
GLint uniform_matrix = -1;

// The first line of the shaders: OpenGL 3.3 core on the desktop, OpenGL ES 3.0 on the web
#ifdef __EMSCRIPTEN__
char const* const shader_header = "#version 300 es\nprecision highp float;\n";
#else
char const* const shader_header = "#version 330 core\n";
#endif

char const* const vertex_shader = R"(
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
uniform mat4 matrix;              // projection * view * model
out vec3 vertex_color;
void main() {
    vertex_color = color;
    gl_Position = matrix * vec4(position, 1.0);
}
)";

char const* const fragment_shader = R"(
in vec3 vertex_color;
out vec4 frag_color;
void main() {
    frag_color = vec4(vertex_color, 1.0);
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

// 4x4 matrices, column-major as OpenGL reads them: m[column * 4 + row]
struct mat4 { float m[16]; };

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

// A cube of side 1 centered at the origin: 6 faces of 2 triangles, one color per face
void create_cube()
{
	float const p[8][3] = {{-.5f,-.5f,-.5f}, {.5f,-.5f,-.5f}, {.5f,.5f,-.5f}, {-.5f,.5f,-.5f},
	                       {-.5f,-.5f,.5f}, {.5f,-.5f,.5f}, {.5f,.5f,.5f}, {-.5f,.5f,.5f}};
	int const faces[6][4] = {{4,5,6,7}, {1,0,3,2}, {5,1,2,6}, {0,4,7,3}, {7,6,2,3}, {0,1,5,4}};
	float const colors[6][3] = {{.90f,.30f,.25f}, {.25f,.55f,.85f}, {.30f,.70f,.40f},
	                            {.95f,.70f,.20f}, {.60f,.40f,.80f}, {.35f,.75f,.80f}};
	float data[36 * 6];
	int n = 0;
	for (int f = 0; f < 6; ++f)
		for (int corner : {0, 1, 2, 0, 2, 3}) {
			for (int k = 0; k < 3; ++k) data[n++] = p[faces[f][corner]][k];
			for (int k = 0; k < 3; ++k) data[n++] = colors[f][k];
		}

	GLuint vbo;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
}

// Size of the drawing surface, in pixels
void surface_size(int& width, int& height)
{
#ifdef __EMSCRIPTEN__
	// the page sizes the canvas to the iframe (slide-frame.html)
	emscripten_get_canvas_element_size("#canvas", &width, &height);
#else
	glfwGetFramebufferSize(window, &width, &height);   // pixels, also on a Retina screen
#endif
}

void frame()
{
	int width = 1, height = 1;
	surface_size(width, height);
	glViewport(0, 0, width, height);
	float const shade = (background == "dark") ? 0.12f : 1.0f;
	glClearColor(shade, shade, shade, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float const pi = 3.14159265f;
	float const angle = 2 * pi * speed * float(glfwGetTime());
	mat4 const model = multiply(rotation_x(tilt * pi / 180), rotation_y(angle));
	mat4 const view = translation_z(-3.0f);
	mat4 const projection = perspective(40 * pi / 180, float(width) / float(height > 0 ? height : 1), 0.1f, 10.0f);
	mat4 const matrix = multiply(projection, multiply(view, model));

	glUseProgram(program);
	glUniformMatrix4fv(uniform_matrix, 1, GL_FALSE, matrix.m);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glfwSwapBuffers(window);
	glfwPollEvents();
}

int main(int argc, char* argv[])
{
	// The settings: from the command line, or from the address of the iframe
	slide::parse(argc, argv);
	slide::param("speed", speed, 0.0f, 1.0f, 0.01f).with_label("speed (turns/s)");
	slide::param("tilt", tilt, -90.0f, 90.0f, 1.0f).with_label("tilt (degrees)");
	slide::param("background", background, {"light", "dark"});
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
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);   // needed on macOS
#endif
	window = glfwCreateWindow(width, height, "A1 - OpenGL cube", nullptr, nullptr);
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
	create_cube();
	glEnable(GL_DEPTH_TEST);

#ifdef __EMSCRIPTEN__
	// The browser calls frame() at each display refresh; main() returns to it
	emscripten_set_main_loop(frame, 0, 1);
#else
	while (!glfwWindowShouldClose(window))
		frame();
	glfwTerminate();
#endif
	return 0;
}
