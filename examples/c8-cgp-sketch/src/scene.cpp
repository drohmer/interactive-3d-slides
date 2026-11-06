#include "scene.hpp"
#include "slide_bridge.hpp"

#include <cmath>

using namespace cgp;


void scene_structure::initialize()
{
	camera_control.initialize(inputs, window);
	camera_control.set_rotation_axis_z();
	camera_control.look_at({0, -0.01f, 3.0f}, {0, 0, 0}, {0, 1, 0});   // facing the sheet

	sheet.initialize_data_on_gpu(mesh_primitive_quadrangle({-1.6f, -1, -0.01f}, {1.6f, -1, -0.01f}, {1.6f, 1, -0.01f}, {-1.6f, 1, -0.01f}));
	sheet.material.color = {1, 1, 1};
	sheet.material.phong = {1, 0, 0, 1};   // no shading: a flat white sheet

	// The parameters of the demo. The keys of the demo (keyboard_event below) and the buttons of the
	// slide go through the same parameters and actions.
	slide::param("color", parameters.color, {{"black", "Black"}, {"red", "Red"}, {"blue", "Blue"}});
	slide::param("width", parameters.width, 0.005f, 0.1f, 0.005f).with_label("width");
	slide::param("draw", parameters.draw).with_label("Draw (off: turn the camera)");
	slide::param("gui", parameters.gui).with_label("ImGui");
	slide::action("undo", [this] { undo(); }).with_label("Undo");
	slide::action("clear", [this] { clear(); }).with_label("Clear");
	slide::readout("strokes", [this] { return float(strokes.size()); }).with_label("strokes");

	// ?sample=1 starts with a stroke already drawn (a spiral): the image of the demo in a PDF
	if (slide::has_arg("sample")) {
		stroke_structure stroke;
		stroke.color = {0.05f, 0.05f, 0.05f};
		for (int k = 0; k <= 200; ++k) {
			float const a = 0.1f * k, r = 0.004f * k;
			stroke.points.push_back({r * std::cos(a), r * std::sin(a), 0});
		}
		parameters.width = 0.04f;
		update_ribbon(stroke);
		strokes.push_back(stroke);
	}
}


void scene_structure::display_frame()
{
	environment.camera_view = camera_control.camera().matrix_view();
	environment.light = camera_control.camera_model.position();

	draw(sheet, environment);
	for (stroke_structure const& stroke : strokes)
		if (stroke.points.size() > 1)
			draw(stroke.ribbon, environment);
}


// The point of the plane through the origin, facing the camera, under the mouse
vec3 scene_structure::mouse_position_3d() const
{
	intersection_structure const I = picking_plane_orthogonal_to_camera(
		inputs.mouse.position.current, {0, 0, 0}, camera_control.camera_model, camera_projection);
	return I.position;
}


// The ribbon of a stroke: two vertices per point, on each side of the stroke, in the plane facing the camera
void scene_structure::update_ribbon(stroke_structure& stroke)
{
	size_t const N = stroke.points.size();
	if (N < 2) return;
	vec3 const front = camera_control.camera_model.front();
	mesh m;
	for (size_t k = 0; k < N; ++k) {
		vec3 const a = stroke.points[k > 0 ? k - 1 : 0];
		vec3 const b = stroke.points[k + 1 < N ? k + 1 : N - 1];
		vec3 side = cross(b - a, front);
		side = norm(side) > 1e-6f ? normalize(side) : vec3{0, 1, 0};
		m.position.push_back(stroke.points[k] - 0.5f * parameters.width * side);
		m.position.push_back(stroke.points[k] + 0.5f * parameters.width * side);
		if (k + 1 < N) {
			unsigned int const i = static_cast<unsigned int>(2 * k);
			m.connectivity.push_back({i, i + 1, i + 2});
			m.connectivity.push_back({i + 1, i + 3, i + 2});
		}
	}
	m.fill_empty_field();
	stroke.ribbon.clear();
	stroke.ribbon.initialize_data_on_gpu(m);
	stroke.ribbon.material.color = stroke.color;
	stroke.ribbon.material.phong = {1, 0, 0, 1};
	stroke.ribbon.material.texture_settings.two_sided = true;
}


void scene_structure::undo()
{
	if (strokes.empty()) return;
	strokes.back().ribbon.clear();   // free the GPU buffers
	strokes.pop_back();
	slide::notify();                 // the number of strokes changed: the page and the slide show it
}


void scene_structure::clear()
{
	for (stroke_structure& stroke : strokes)
		stroke.ribbon.clear();
	strokes.clear();
	slide::notify();
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
	ImGui::Checkbox("draw", &parameters.draw);
	ImGui::SliderFloat("width", &parameters.width, 0.005f, 0.1f);
	if (ImGui::Button("undo")) undo();
	if (ImGui::Button("clear")) clear();
}


void scene_structure::mouse_move_event()
{
	if (parameters.draw) {
		// extend the current stroke while the left button is down
		if (inputs.mouse.click.left && !inputs.mouse.on_gui && !strokes.empty()) {
			stroke_structure& stroke = strokes.back();
			vec3 const p = mouse_position_3d();
			if (stroke.points.empty() || norm(p - stroke.points.back()) > 0.005f) {
				stroke.points.push_back(p);
				update_ribbon(stroke);
			}
		}
	}
	else if (!inputs.keyboard.shift)
		camera_control.action_mouse_move();
}


void scene_structure::mouse_click_event()
{
	if (parameters.draw && !inputs.mouse.on_gui
		&& inputs.mouse.click.last_action == last_mouse_cursor_action::click_left) {
		// a new stroke, with the current color
		vec3 const colors[] = {{0.05f, 0.05f, 0.05f}, {0.85f, 0.1f, 0.1f}, {0.1f, 0.3f, 0.9f}};
		stroke_structure stroke;
		stroke.color = parameters.color == "red" ? colors[1] : parameters.color == "blue" ? colors[2] : colors[0];
		stroke.points.push_back(mouse_position_3d());
		strokes.push_back(stroke);
		slide::notify();
	}
	else if (parameters.draw && inputs.mouse.click.last_action == last_mouse_cursor_action::release_left) {
		// a click without a move leaves a stroke of a single point: remove it
		if (!strokes.empty() && strokes.back().points.size() < 2) {
			strokes.pop_back();
			slide::notify();
		}
	}
	else
		camera_control.action_mouse_click();
}


// The keys of the demo: letters and digits only. The arrow keys and Page Down / Page Up are sent to the
// slides by the page of the demo (slide-frame.html): keep them for the presentation.
void scene_structure::keyboard_event()
{
	// The GLFW codes of the keys (GLFW_KEY_Z...): the names of the keys are not known on the web
	auto const& key = inputs.keyboard.last_action;
	if (key.action != GLFW_PRESS) return;
	if (key.key == GLFW_KEY_Z) slide::set("undo", "");
	else if (key.key == GLFW_KEY_C) slide::set("clear", "");
	else if (key.key == GLFW_KEY_D) slide::set("draw", parameters.draw ? "0" : "1");
	else if (key.key == GLFW_KEY_1) slide::set("color", "black");
	else if (key.key == GLFW_KEY_2) slide::set("color", "red");
	else if (key.key == GLFW_KEY_3) slide::set("color", "blue");
	else return;
	slide::notify();   // a value changed from the program: the panel and the slide show it
}


void scene_structure::idle_frame()
{
	camera_control.idle_frame();
}
