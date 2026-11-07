#include "scene.hpp"
#include "slide_bridge.hpp"

using namespace cgp;


void scene_structure::initialize()
{
	camera_control.initialize(inputs, window);
	camera_control.set_rotation_axis_z();

	// The files are read from assets/, as on the desktop. For the web, the CMakeLists.txt of CGP packs
	// this folder into project.data (--preload-file), downloaded with the demo: keep it small.

	// A single OBJ file, without texture (CGP How-to: load an OBJ file)
	mesh camel_mesh = mesh_load_file_obj(project::path + "assets/camel.obj");
	camel.initialize_data_on_gpu(camel_mesh);
	camel.material.color = {0.85f, 0.7f, 0.5f};
	camel_triangles = float(camel_mesh.connectivity.size());

	// The parameters of the demo: the buttons of the slide choose the model, and the slide shows the
	// number of triangles of the model shown
	slide::param("model", parameters.model, {{"camel", "Camel"}, {"sponza", "Sponza"}})
		.on_change([this] { load_sponza(); place_camera(); slide::notify(); });   // notify: for the ImGui buttons (slide::set from C++)
	slide::param("turn", parameters.turn).with_label("Turn");
	slide::param("gui", parameters.gui).with_label("ImGui");
	slide::readout("triangles", [this] { return parameters.model == "sponza" ? sponza_triangles : camel_triangles; })
		.with_label("triangles");
	load_sponza();   // only if the address asks for it at once (?model=sponza)
	place_camera();
}


// Sponza is read the first time it is shown: parsing its 2.6 MB OBJ file takes a few seconds, the camel
// is shown without waiting for it
void scene_structure::load_sponza()
{
	if (parameters.model != "sponza" || !sponza.empty()) return;

	// An OBJ file with its materials (sponza.mtl) and its textures (*.JPG): one mesh per material
	// (CGP How-to: load an OBJ with materials). On the web, the names of the files are case-sensitive.
	std::vector<mesh_obj_advanced_loader::shape_element_node> parts =
		mesh_load_file_obj_advanced(project::path + "assets/sponza/", "sponza.obj");
	for (auto const& part : parts)
		sponza_triangles += float(part.mesh_element.connectivity.size());
	sponza = mesh_obj_advanced_loader::convert_to_mesh_drawable(parts);
	for (mesh_drawable& part : sponza) {          // this file is y-up and 35 units wide
		part.model.rotation = rotation_transform::from_axis_angle({1, 0, 0}, Pi / 2);
		part.model.scaling = 0.1f;
	}
}


// A view adapted to each model
void scene_structure::place_camera()
{
	if (parameters.model == "sponza")
		camera_control.look_at({1.5f, -0.35f, 0.25f}, {-0.6f, 0.1f, 0.55f}, {0, 0, 1});
	else
		camera_control.look_at({5.2f, -3.5f, 2.6f}, {0, 0, 0}, {0, 0, 1});   // the camel is about 4 units long
}


void scene_structure::display_frame()
{
	environment.camera_view = camera_control.camera().matrix_view();
	environment.light = camera_control.camera_model.position();

	if (parameters.model == "sponza") {
		for (mesh_drawable const& part : sponza)
			draw(part, environment);
	}
	else {
		if (parameters.turn)
			angle += 0.5f * inputs.time_interval;
		camel.model.rotation = rotation_transform::from_axis_angle({0, 0, 1}, angle);
		draw(camel, environment);
	}
}


void scene_structure::display_gui()
{
	// The template always draws its ImGui window: in a slide, the buttons of the slide replace it,
	// so the window is hidden unless gui=1 (see example c3)
	ImGui::GetStyle().Alpha = parameters.gui ? 1.0f : 0.0f;
	if (!parameters.gui) {
		ImGui::SetWindowCollapsed(true);
		ImGui::SetWindowPos(ImVec2(-10000, -10000));
		return;
	}
	ImGui::SetWindowCollapsed(false);
	ImGui::SetWindowPos(ImVec2(10, 10));
	if (ImGui::Button("Camel")) slide::set("model", "camel");
	ImGui::SameLine();
	if (ImGui::Button("Sponza")) slide::set("model", "sponza");
	ImGui::Checkbox("turn", &parameters.turn);
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
