#include "scene.hpp"
#include "slide_bridge.hpp"

#include <cmath>

using namespace cgp;


void scene_structure::initialize()
{
	camera_control.initialize(inputs, window);
	camera_control.set_rotation_axis_z();
	camera_control.look_at({2.1f, -2.1f, 1.5f}, {0, 0, -0.15f}, {0, 0, 1});

	// A grid of 120 x 120 vertices, moved at each frame (CGP How-to: update a mesh every frame)
	int const N = 120;
	surface_mesh = mesh_primitive_grid({-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}, N, N);
	surface.initialize_data_on_gpu(surface_mesh);
	surface.material.color = {0.35f, 0.58f, 0.85f};
	surface.material.phong.specular = 0.25f;

	// The wireframe: one grid line out of 6 in each direction (all the edges of 120 x 120 vertices
	// would cover the surface), each line following the surface vertex by vertex.
	// mesh_primitive_grid stores the vertex (ku, kv) at the index ku * N + kv.
	int const step = 6;
	auto index = [N](int ku, int kv) { return static_cast<unsigned int>(ku * N + kv); };
	for (int ku = 0; ku < N; ++ku)
		for (int kv = 0; kv < N; ++kv) {
			bool const line_u = (ku % step == 0) || ku == N - 1;   // ku fixed, along kv
			bool const line_v = (kv % step == 0) || kv == N - 1;   // kv fixed, along ku
			if (line_u && kv + 1 < N) wire_edges.push_back(uint2{index(ku, kv), index(ku, kv + 1)});
			if (line_v && ku + 1 < N) wire_edges.push_back(uint2{index(ku, kv), index(ku + 1, kv)});
		}
	wire_position.resize(2 * wire_edges.size());
	update_wire();
	wire.display_type = curve_drawable_display_type::Segments;
	wire.initialize_data_on_gpu(wire_position);
	wire.color = {0.1f, 0.15f, 0.3f};

	// The parameters of the demo, declared once: the panel of the page, the buttons of the
	// slide and postMessage go through them. Their values given at start (?ampl=0.2) are applied here.
	slide::param("ampl", wave.amplitude, 0.0f, 0.3f, 0.01f).with_label("amplitude", "<i>A</i>");
	slide::param("k", wave.wave_number, 2.0f, 20.0f, 0.5f).with_label("wave number", "<i>k</i>");
	slide::param("speed", wave.speed, 0.0f, 10.0f, 0.1f).with_label("speed", "<i>&omega;</i>");
	slide::param("damping", wave.damping, 0.0f, 4.0f, 0.1f).with_label("damping", "<i>d</i>");
	slide::param("shape", wave.shape, {{"ripple", "Ripple"}, {"waves", "Waves"}})
		.on_change([this] { t = 0; });
	slide::param("wireframe", wave.wireframe).with_label("Wireframe");
	slide::param("play", wave.play).with_label("Play");
	slide::param("gui", wave.gui).with_label("ImGui");
	slide::action("drop", [this] { t = 0; wave.shape = "ripple"; }).with_label("Drop");
}


// Height of the surface at (x, y), time t
static float height(wave_parameters const& w, float x, float y, float t)
{
	if (w.shape == "waves")
		return w.amplitude * std::sin(w.wave_number * x - w.speed * t);

	// a ripple from the center: its front moves at the speed omega / k
	float const r = std::sqrt(x * x + y * y);
	float const front = (w.wave_number > 0 ? w.speed / w.wave_number : 0) * t;
	float const reached = std::fmin(std::fmax((front - r) * 8.0f, 0.0f), 1.0f);   // smooth front
	return reached * w.amplitude * std::exp(-w.damping * r) * std::sin(w.wave_number * r - w.speed * t);
}


void scene_structure::display_frame()
{
	environment.camera_view = camera_control.camera().matrix_view();
	environment.light = camera_control.camera_model.position();

	if (wave.play)
		t += inputs.time_interval;

	// CPU: move the vertices, then their normals
	for (vec3& p : surface_mesh.position)
		p.z = height(wave, p.x, p.y, t);
	surface_mesh.normal_update();

	// GPU: overwrite the buffers (same size, no reallocation)
	surface.vbo_position.update(surface_mesh.position);
	surface.vbo_normal.update(surface_mesh.normal);

	draw(surface, environment);
	if (wave.wireframe) {
		update_wire();
		wire.vbo_position.update(wire_position);
		draw(wire, environment);
	}
}


// The two ends of each edge, slightly above the surface (no polygon offset for lines in WebGL)
void scene_structure::update_wire()
{
	for (size_t k = 0; k < wire_edges.size(); ++k) {
		wire_position[2 * k] = surface_mesh.position[wire_edges[k].x] + vec3{0, 0, 0.002f};
		wire_position[2 * k + 1] = surface_mesh.position[wire_edges[k].y] + vec3{0, 0, 0.002f};
	}
}


void scene_structure::display_gui()
{
	// The template always draws its ImGui window: in a slide, the page has its own panel,
	// so the window is hidden unless gui=1. ImGui keeps a corner of a window on screen:
	// it is also collapsed and made transparent.
	ImGui::GetStyle().Alpha = wave.gui ? 1.0f : 0.0f;
	if (!wave.gui) {
		ImGui::SetWindowCollapsed(true);
		ImGui::SetWindowPos(ImVec2(-10000, -10000));
		return;
	}
	ImGui::SetWindowCollapsed(false);
	ImGui::SetWindowPos(ImVec2(10, 10));
	// the same variables as the panel of the page (which shows the new values at its next change)
	ImGui::SliderFloat("amplitude", &wave.amplitude, 0.0f, 0.3f);
	ImGui::SliderFloat("wave number", &wave.wave_number, 2.0f, 20.0f);
	ImGui::SliderFloat("speed", &wave.speed, 0.0f, 10.0f);
	ImGui::SliderFloat("damping", &wave.damping, 0.0f, 4.0f);
	ImGui::Checkbox("wireframe", &wave.wireframe);
	if (ImGui::Button("drop"))
		slide::set("drop", "");
}


void scene_structure::mouse_move_event()
{
	if (!inputs.keyboard.shift)
		camera_control.action_mouse_move();
}
void scene_structure::mouse_click_event()
{
	camera_control.action_mouse_click();
}
void scene_structure::keyboard_event()
{
	camera_control.action_keyboard();
}
void scene_structure::idle_frame()
{
	camera_control.idle_frame();
}
