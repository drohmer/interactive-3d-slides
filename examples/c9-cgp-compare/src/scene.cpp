#include "scene.hpp"
#include "slide_bridge.hpp"

using namespace cgp;


void scene_structure::initialize()
{
	camera_control.initialize(inputs, window);
	camera_control.set_rotation_axis_z();
	camera_control.look_at({2.2f, 1.5f, 1.5f}, {0, 0, -0.1f}, {0, 0, 1});

	// A grid of 200 x 200 vertices, deformed in the vertex shader (CGP How-to: deform a mesh in a shader)
	surface.initialize_data_on_gpu(mesh_primitive_grid({-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}, 200, 200));
	surface.material.color = {0.8f, 0.95f, 1.0f};
	surface.material.texture_settings.two_sided = true;
	surface.shader.load(project::path + "shaders/mesh_deformation/mesh_deformation.vert.glsl",
	                    project::path + "shaders/mesh_deformation/mesh_deformation.frag.glsl");

	// The parameters of the demo. "normals" is the difference between the two copies of the slide
	// (?normals=plane in the address of one frame); the others are set on both at once by the slide.
	slide::param("normals", parameters.normals, {{"exact", "Exact normals"}, {"plane", "Normals of the plane"}});
	slide::param("ampl", parameters.amplitude, 0.0f, 0.15f, 0.005f).with_label("amplitude", "<i>A</i>");
	slide::param("freq", parameters.frequency, 5.0f, 40.0f, 0.5f).with_label("frequency", "<i>k</i>");
	slide::param("play", parameters.play).with_label("Play");
	slide::param("gui", parameters.gui).with_label("ImGui");
	// the two copies run their own clock: "restart" sets both to the same time
	slide::action("restart", [this] { t = 0; }).with_label("Restart");
}


void scene_structure::display_frame()
{
	environment.camera_view = camera_control.camera().matrix_view();
	environment.light = camera_control.camera_model.position();

	if (parameters.play)
		t += inputs.time_interval;

	// the uniforms of the shader, read at the next draw
	environment.uniform_generic.uniform_float["time"] = t;
	environment.uniform_generic.uniform_float["amplitude"] = parameters.amplitude;
	environment.uniform_generic.uniform_float["frequency"] = parameters.frequency;
	environment.uniform_generic.uniform_float["exact_normals"] = parameters.normals == "exact" ? 1.0f : 0.0f;
	draw(surface, environment);
}


void scene_structure::display_gui()
{
	// The ImGui window of the template, hidden in a slide unless gui=1 (see example c3)
	ImGui::GetStyle().Alpha = parameters.gui ? 1.0f : 0.0f;
	if (!parameters.gui) {
		ImGui::SetWindowCollapsed(true);
		ImGui::SetWindowPos(ImVec2(-10000, -10000));
		return;
	}
	ImGui::SetWindowCollapsed(false);
	ImGui::SetWindowPos(ImVec2(10, 10));
	ImGui::SliderFloat("amplitude", &parameters.amplitude, 0.0f, 0.15f);
	ImGui::SliderFloat("frequency", &parameters.frequency, 5.0f, 40.0f);
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
