// Select and drag the control points of a curve: direct manipulation in a slide.
// The picking comes from the How-to "Select and drag with the mouse" of the CGP documentation.
#include "scene.hpp"
#include "slide_bridge.hpp"

#include <cmath>

using namespace cgp;

static int const sample_count = 200;
static float const point_radius = 0.07f;


void scene_structure::initialize()
{
	camera_control.initialize(inputs, window);
	camera_control.set_rotation_axis_z();
	camera_control.look_at({0.0f, -1.6f, 2.4f}, {0, 0.2f, 0}, {0, 0, 1});

	ball.initialize_data_on_gpu(mesh_primitive_sphere(point_radius));

	// The settings of the curve, and a value computed by the program (read only):
	// the slide shows the length while the points are dragged
	slide::param("curve", settings.type, {{"bezier", "B&eacute;zier"}, {"spline", "Spline"}});
	slide::param("points", settings.points, 3, 7).with_label("points").on_change([this] { reset_points(); });
	slide::param("polygon", settings.polygon).with_label("Polygon");
	slide::param("gui", settings.gui).with_label("ImGui");
	slide::action("reset", [this] { reset_points(); }).with_label("Reset");
	slide::readout("length", [this] { return length; }).with_label("length");

	reset_points();
	samples.resize(sample_count);
	update_curve();
	curve.initialize_data_on_gpu(samples);
	curve.color = {0.85f, 0.25f, 0.15f};
}


// Control points along a zigzag
void scene_structure::reset_points()
{
	int const n = settings.points;
	control.resize(n);
	for (int k = 0; k < n; ++k)
		control[k] = {-1.0f + 2.0f * k / (n - 1.0f), (k % 2 == 0) ? -0.2f : 0.7f, 0.0f};
	picked = -1;
	polygon.clear();
	polygon.initialize_data_on_gpu(control);
	polygon.color = {0.6f, 0.6f, 0.6f};
}


// Bezier curve of any degree: de Casteljau
static vec3 bezier(numarray<vec3> const& p, float t)
{
	numarray<vec3> q = p;
	for (size_t level = 1; level < q.size(); ++level)
		for (size_t k = 0; k + level < q.size(); ++k)
			q[k] = (1 - t) * q[k] + t * q[k + 1];
	return q[0];
}

// Catmull-Rom spline through all the points (end points repeated)
static vec3 spline(numarray<vec3> const& p, float t)
{
	int const n = int(p.size());
	float const u = t * (n - 1);
	int const i = std::min(int(u), n - 2);
	float const s = u - i;
	vec3 const p0 = p[std::max(i - 1, 0)], p1 = p[i], p2 = p[i + 1], p3 = p[std::min(i + 2, n - 1)];
	return 0.5f * (2.0f * p1 + (-p0 + p2) * s + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * s * s
	               + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * s * s * s);
}

void scene_structure::update_curve()
{
	length = 0;
	for (int k = 0; k < sample_count; ++k) {
		float const t = k / (sample_count - 1.0f);
		samples[k] = (settings.type == "spline") ? spline(control, t) : bezier(control, t);
		if (k > 0)
			length += norm(samples[k] - samples[k - 1]);
	}
}


void scene_structure::display_frame()
{
	environment.camera_view = camera_control.camera().matrix_view();
	environment.light = camera_control.camera_model.position();

	update_curve();
	curve.vbo_position.update(samples);
	draw(curve, environment);

	if (settings.polygon) {
		polygon.vbo_position.update(control);
		draw(polygon, environment);
	}
	for (int k = 0; k < int(control.size()); ++k) {
		ball.model.translation = control[k];
		ball.material.color = (k == picked) ? vec3{1.0f, 0.55f, 0.0f} : vec3{0.25f, 0.45f, 0.85f};
		draw(ball, environment);
	}

	// The program changed a value by itself: tell the page and the slide
	if (std::abs(length - notified_length) > 1e-4f) {
		notified_length = length;
		slide::notify();
	}
}


void scene_structure::display_gui()
{
	// The template always draws its ImGui window: hidden in a slide unless gui=1
	// (collapsed, out of sight and transparent: ImGui keeps a corner of a window on screen)
	ImGui::GetStyle().Alpha = settings.gui ? 1.0f : 0.0f;
	if (!settings.gui) {
		ImGui::SetWindowCollapsed(true);
		ImGui::SetWindowPos(ImVec2(-10000, -10000));
		return;
	}
	ImGui::SetWindowCollapsed(false);
	ImGui::SetWindowPos(ImVec2(10, 10));
	ImGui::Text("Dragged point: %d", picked);
	ImGui::Text("Length: %.3f", length);
}


void scene_structure::mouse_move_event()
{
	// While a point is dragged, it follows the mouse in the plane facing the camera
	if (picked != -1) {
		intersection_structure const I = picking_plane_orthogonal_to_camera(
			inputs.mouse.position.current, control[picked], camera_control.camera_model, camera_projection);
		if (I.valid)
			control[picked] = I.position;
	}
	if (!inputs.keyboard.shift)
		camera_control.action_mouse_move();   // does nothing while is_active is false
}

void scene_structure::mouse_click_event()
{
	if (inputs.mouse.click.last_action == last_mouse_cursor_action::click_left) {
		picking_structure const picking = picking_spheres(inputs.mouse.position.current,
			control, point_radius, camera_control.camera_model, camera_projection);
		if (picking.active)
			picked = picking.index;
	}
	if (inputs.mouse.click.last_action == last_mouse_cursor_action::release_left)
		picked = -1;
	camera_control.is_active = (picked == -1);   // no camera motion while a point is dragged
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
